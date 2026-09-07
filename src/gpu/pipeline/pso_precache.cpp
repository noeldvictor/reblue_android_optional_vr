/**
 * @file    gpu/pipeline/pso_precache.cpp
 * @copyright Copyright (c) 2026 Tom Clay <tomc@tctechstuff.com>
 *            All rights reserved.
 * @license   BSD 3-Clause License
 *            See LICENSE file in the project root for full license text.
 */
#include "gpu/pipeline/pso_precache.h"

#include <condition_variable>
#include <deque>
#include <exception>
#include <mutex>
#include <thread>
#include <unordered_set>
#include <utility>

#include "core/logging.h"
#include "core/threading.h"
#include "gpu/device.h"
#include "gpu/pipeline/pipeline_cache.h"
#include "gpu/pipeline/native_pipeline_program.h"
#include "gpu/settings.h"

namespace bd::gpu {

namespace {

struct WorkItem {
  PipelineState state; // value copy, carries live GuestShader*/decl*
  TokenPtr token;      // null for ungated work
  NativePipelineHandle program; // pins layout/shaders/input before async dispatch
};

std::mutex g_queueMutex;
std::condition_variable g_queueCv;
// Two lanes share one mutex/cv. Priority holds a load's own predicted PSOs so
// they compile ahead of the background backlog (boot replay residual + global
// coverage), the set a draw in the first frames after a load needs warm.
std::deque<WorkItem> g_priorityQueue;
std::deque<WorkItem> g_queue;

std::mutex g_dedupMutex;
std::unordered_set<u64> g_queuedOrDone;
constexpr size_t kMaxNativePending = 256;
size_t g_native_pending = 0; // queued plus currently compiling; under dedup mutex

std::atomic<size_t> g_buildFailures{0};

std::once_flag g_startOnce;

// Thread-local so a loader thread brackets its own loads without disturbing
// other threads or the boot replay path.
thread_local TokenPtr t_currentLoadToken;

void ProcessItem(WorkItem &item) {
  bool succeeded = false;
  try {
    succeeded = GetOrCreatePipeline(item.state) != nullptr;
    if (!succeeded) {
      const size_t n =
          g_buildFailures.fetch_add(1, std::memory_order_relaxed) + 1;
      if (n == 1 || (n & 0x3FF) == 0)
        BD_WARN("pso_precache: GetOrCreatePipeline returned null ({} total, "
                "possible device-loss/transient-alloc failure)",
                n);
    }
  } catch (const std::exception &e) {
    BD_ERROR("pso_precache GetOrCreatePipeline exception: {}", e.what());
  } catch (...) {
    BD_ERROR("pso_precache GetOrCreatePipeline unknown exception");
  }

  // Native failures must be retryable and cannot leave a retired pointer's key
  // in permanent dedup storage. A successful cache entry pins that program.
  if (item.program) {
    std::lock_guard lock(g_dedupMutex);
    --g_native_pending;
    if (!succeeded) g_queuedOrDone.erase(HashPipelineState(item.state));
  }
  // Release even on a nullptr build or thrown compile so the gate cannot hang.
  if (item.token)
    item.token->ReleasePending();
}

void WorkerLoop() {
  // A load hands every worker an 8ms link at once, and at normal priority that
  // convoy preempts the render thread and the frame stretches to 300ms.
  DemoteThreadToBackground();
  for (;;) {
    WorkItem item;
    {
      std::unique_lock<std::mutex> lock(g_queueMutex);
      g_queueCv.wait(
          lock, [] { return !g_priorityQueue.empty() || !g_queue.empty(); });
      if (!g_priorityQueue.empty()) {
        item = std::move(g_priorityQueue.front());
        g_priorityQueue.pop_front();
      } else if (!g_queue.empty()) {
        item = std::move(g_queue.front());
        g_queue.pop_front();
      } else {
        continue;
      }
    }
    try {
      ProcessItem(item);
    } catch (const std::exception &e) {
      BD_ERROR("pso_precache worker exception: {}", e.what());
    } catch (...) {
      BD_ERROR("pso_precache worker unknown exception");
    }
  }
}

void StartWorkerPool() {
  std::call_once(g_startOnce, [] {
    const unsigned hw = std::thread::hardware_concurrency();
    unsigned count = hw > 3 ? (hw * 2u) / 3u : 2u;
    if (hw > 5u && count > hw - 3u)
      count = hw - 3u; // leave 3 for the game
    if (count < 2u)
      count = 2u;
    // Never joinable: a static vector of these std::terminates if the message
    // loop unwinds to main instead of exiting via TerminateProcessNow.
    for (unsigned i = 0; i < count; ++i)
      std::thread(WorkerLoop).detach();
    BD_DEBUG("pso_precache: started {} compiler thread(s)", count);
  });
}

} // namespace

bool PrecacheEnabled() { return Settings::Get().PSOPrecache(); }

namespace {

// Callers gate on master switch/device readiness so a disabled precache never
// starts the pool or poisons the dedup set.
void EnqueueResolved(const PipelineState &state, TokenPtr token,
                     bool priority) {
  if (!NativePipelineStateValid(state)) return;
  auto program = state.native_program ? state.native_program->Lease() : nullptr;
  const u64 key = HashPipelineState(state);
  {
    std::lock_guard<std::mutex> lock(g_dedupMutex);
    if (program && g_native_pending >= kMaxNativePending) return;
    if (!g_queuedOrDone.insert(key).second)
      return;
    if (program) ++g_native_pending;
  }

  // Retain these local references for rollback if starting/allocating work
  // throws after dedup insertion. No token or native quota can remain stuck.
  const auto rollback_token = token;
  const bool native = bool(program);
  bool pending_added = false;
  try {
    StartWorkerPool();
    if (token) {
      token->AddPending();
      pending_added = true;
    }
    std::lock_guard<std::mutex> lock(g_queueMutex);
    if (priority)
      g_priorityQueue.push_back(WorkItem{state, std::move(token), std::move(program)});
    else
      g_queue.push_back(WorkItem{state, std::move(token), std::move(program)});
  } catch (...) {
    if (pending_added) rollback_token->ReleasePending();
    std::lock_guard lock(g_dedupMutex);
    g_queuedOrDone.erase(key);
    if (native) --g_native_pending;
    throw;
  }
  g_queueCv.notify_one();
}

} // namespace

void EnqueuePipeline(const PipelineState &state) {
  if (!PrecacheEnabled() || !Video::HostDevice())
    return;
  TokenPtr token = t_currentLoadToken; // auto-attach to active load capture
  // Capture before the move: argument evaluation order is unspecified, and
  // std::move(token) below would otherwise race static_cast<bool>(token).
  const bool priority = static_cast<bool>(token);
  EnqueueResolved(state, std::move(token), priority);
}

void EnqueuePipelinePriority(const PipelineState &state) {
  if (!PrecacheEnabled() || !Video::HostDevice())
    return;
  EnqueueResolved(state, nullptr, true); // ignore t_currentLoadToken
}

void BeginLoadCapture() {
  t_currentLoadToken = std::make_shared<CompileToken>();
}

void EndLoadCapture() {
  TokenPtr token = std::move(t_currentLoadToken);
  t_currentLoadToken.reset();
  if (!token)
    return;
  const u32 enqueued = token->Total();
  if (enqueued == 0)
    return; // load reused only warm pipelines, nothing to do

  // Pushed to the priority lane and warm ahead of background work, so a heavy
  // load spawn still can't freeze the load thread for hundreds of ms.
  BD_DEBUG("pso_precache: load queued {} priority pipeline(s) (cache total {})",
           enqueued, PipelineCacheSize());
}

} // namespace bd::gpu
