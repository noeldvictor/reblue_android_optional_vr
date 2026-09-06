/**
 * @file    native_material_library.h
 * @brief   Shared material ownership, desktop cooking and source-free loading.
 * @copyright Copyright (c) 2026 reblue contributors
 * @license   BSD 3-Clause License
 */
#pragma once

#include "gpu/scene/native_material_asset.h"
#include <filesystem>
#include <list>
#include <memory>
#include <mutex>
#include <unordered_map>

namespace bd::gpu::scene {
struct NativeMaterial {
  NativeMaterialId id;
  NativeMaterialAsset asset;
};
using NativeMaterialHandle = std::shared_ptr<const NativeMaterial>;

struct NativeMaterialLibraryStats {
  uint64_t cooked = 0, loaded = 0, memory_hits = 0;
  uint64_t invalid = 0, write_failures = 0, budget_refusals = 0;
  uint64_t disk_budget_refusals = 0, disk_bytes = 0;
  size_t disk_files = 0;
  // Last write preflight, not a live volume inventory. A refused bounded scan
  // may stop early; in that case these are only the observed lower bounds.
  bool disk_inventory_complete = false;
  size_t resident = 0;
};

struct NativeMaterialDiskBudget {
  uint64_t max_bytes = 1ull << 20; // logical payload, including invalid/foreign files
  size_t max_files = 4096;         // also bounds small-file allocation overhead
  uint64_t min_free_bytes = 20ull << 30;
};

// Independent of the game, GPU, runtime and guest memory. Draw references pin
// immutable assets. At capacity, only unreferenced least-recently-used assets
// are evicted; a full pinned library refuses further imports, never grows.
class NativeMaterialLibrary {
public:
  explicit NativeMaterialLibrary(std::filesystem::path directory,
                                 size_t capacity = 16384,
                                 NativeMaterialDiskBudget disk_budget = {});
  NativeMaterialHandle Resolve(const NativeMaterialAsset &asset);
  NativeMaterialHandle Load(NativeMaterialId id);
  NativeMaterialLibraryStats Stats() const;
  static std::filesystem::path FileName(NativeMaterialId id);

private:
  struct Entry {
    NativeMaterialHandle material;
    std::list<NativeMaterialId>::iterator recent;
  };
  NativeMaterialHandle Find(NativeMaterialId id);
  bool Read(NativeMaterialId id, NativeMaterialAsset &asset);
  bool MakeRoom();
  NativeMaterialHandle Insert(NativeMaterialId id, const NativeMaterialAsset &asset);
  bool Write(NativeMaterialId id, std::span<const uint8_t> file);
  std::filesystem::path directory_;
  size_t capacity_;
  NativeMaterialDiskBudget disk_budget_;
  mutable std::mutex mutex_;
  std::list<NativeMaterialId> recent_;
  std::unordered_map<NativeMaterialId, Entry> entries_;
  NativeMaterialLibraryStats stats_;
};
} // namespace bd::gpu::scene
