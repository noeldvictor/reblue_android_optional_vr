/**
 * @brief Load-owned texture associations and native table lookup.
 * @copyright Copyright (c) 2026 reblue contributors
 * @license BSD 3-Clause License
 */
#include "gpu/scene/native_texture_table_bridge.h"
#include "gpu/scene/native_texture_table_source.h"
#include "gpu/scene/native_texture_binding_bridge.h"
#include "gpu/native_texture_mirror.h"
#include "gpu/frame_stats.h"
#include "core/logging.h"
#include "core/memory_helpers.h"
#include "core/settings.h"
#include <algorithm>
#include <unordered_map>
#include <rex/cvar.h>
#include <rex/hook.h>
#include <rex/ppc/context.h>

REXCVAR_DEFINE_BOOL(bd_native_texture_tables, true, kCvarGroup,
    "Load-owned native texture tables with explicit source selection adapters.");
REXCVAR_DEFINE_BOOL(bd_native_texture_tables_verify, false, kCvarGroup,
    "Compare load-owned texture tables with the original read-only lookup.");
REX_EXTERN(__imp__hcgLoadTextureArray);
REX_EXTERN(__imp__hcgTextureListRelease);
REX_EXTERN(__imp__bdLookupCurrentTableTexture);
REX_EXTERN(__imp__sub_8217B3C0);
REX_EXTERN(__imp__sub_8217B518);

namespace bd::gpu::scene {
namespace {
constexpr uint32_t kSelection = (uint32_t(-32036) << 16) - 7864;
struct Import {
  NativeTextureTableHandle native;
  std::vector<uint32_t> sources; // temporary ABI return mapping, never native IDs
};
struct Store {
  std::mutex mutex;
  NativeTextureTableLibrary library;
  std::unordered_map<uint32_t, Import> tables;
  uint64_t published = 0, retired = 0, refused = 0, replaced = 0;
  uint64_t reads = 0, fallback = 0, checks = 0, wrong = 0, image_checks = 0, image_wrong = 0;
  uint64_t native_reads = 0, native_missing = 0;
  uint32_t frame = 0;
};
Store &Tables() { static Store store; return store; }
std::optional<uint32_t> Word(uint64_t address) {
  if (!address || (address & 3) || address > UINT32_MAX - 3) return {};
  const auto *word = bd::mem::try_at<const be_u32>(uint32_t(address));
  return word ? std::optional(uint32_t(*word)) : std::nullopt;
}
void Report(Store &store) {
  const auto frame = FrameStatFrameCount();
  if (frame - store.frame < 300) return;
  BD_INFO("[native-texture-tables] {} published {} retired {} indexed / {} bytes; "
          "{} replacements {} refused; {} lookups {} fallback; {} checks wrong {}; "
          "{} image checks wrong {}; {} native image reads {} unavailable; "
          "source selection/return ABI and dynamic overrides remain",
      store.published, store.retired, store.tables.size(), store.library.Bytes(),
      store.replaced, store.refused, store.reads, store.fallback, store.checks,
      store.wrong, store.image_checks, store.image_wrong, store.native_reads, store.native_missing);
  store.frame = frame;
}
void Retire(uint32_t table) {
  auto &store = Tables(); std::lock_guard lock(store.mutex);
  if (store.tables.erase(table)) ++store.retired;
}
void Publish(uint32_t table, uint32_t asset = 0) {
  Retire(table);
  auto &store = Tables();
  auto sources = ReadTextureTableSources(table, Word);
  if (REXCVAR_GET(bd_native_texture_tables_verify)) {
    static std::atomic<uint32_t> examples{0};
    const auto example = ++examples;
    if (example <= 16 || (example <= 4096 && (example & (example - 1)) == 0))
      BD_INFO("[native-table-source] publication {} asset 0x{:08X} vtable 0x{:08X} "
              "table 0x{:08X} count {} entries 0x{:08X} sources {}",
          example, asset, asset ? Word(asset).value_or(0) : 0, table,
          Word(table).value_or(UINT32_MAX), Word(uint64_t(table) + 4).value_or(0),
          sources ? sources->size() : SIZE_MAX);
  }
  if (!sources) {
    std::lock_guard lock(store.mutex);
    ++store.refused; Report(store); return;
  }
  WithNativeTextureTableSnapshot(*sources, [&](std::vector<NativeTextureTableSlot> slots) {
    std::lock_guard lock(store.mutex);
    if (store.tables.size() >= NativeTextureTableLibrary::kMaxTables) {
      ++store.refused; Report(store); return;
    }
    auto native = store.library.Create(std::move(slots), sources->capacity() * sizeof(uint32_t));
    if (!native) { ++store.refused; Report(store); return; }
    store.tables.insert_or_assign(table, Import{std::move(native), std::move(*sources)});
    ++store.published; Report(store);
  });
}
void PublishCompleted(uint32_t asset, uint32_t result) {
  const auto table = CompletedTextureTable(asset, result, Word);
  if (!table) return;
  auto &store = Tables();
  { std::lock_guard lock(store.mutex); if (store.tables.contains(*table)) return; }
  Publish(*table, asset);
}
struct Selection {
  uint32_t source = 0;
  std::optional<NativeTextureBinding> native;
};
std::optional<Selection> Lookup(uint32_t selector) {
  const auto source_table = Word(kSelection + 4);
  if (!source_table) return {};
  if (!*source_table) return Selection{}; // absent table returns null, not fallback
  const auto offset = Word(kSelection);
  if (!offset) return {};
  auto &store = Tables(); std::lock_guard lock(store.mutex);
  const auto it = store.tables.find(*source_table);
  if (it == store.tables.end()) return {};
  const auto index = TextureTableSourceIndex(*offset, selector);
  if (index >= it->second.sources.size()) {
    const auto fallback = Word(kSelection + 32);
    return fallback ? std::optional(Selection{*fallback, {}}) : std::nullopt;
  }
  const auto &slot = it->second.native->slots[index];
  return Selection{it->second.sources[index], slot.available
      ? std::optional(slot.image) : std::nullopt};
}
} // namespace

std::optional<NativeTextureBinding> FindLoadedNativeTableTexture(uint32_t source_table, uint32_t slot) {
  if (!REXCVAR_GET(bd_native_texture_tables)) return {};
  auto &store = Tables(); std::lock_guard lock(store.mutex);
  const auto it = store.tables.find(source_table);
  if (it != store.tables.end() && slot < it->second.native->slots.size() &&
      it->second.native->slots[slot].available) {
    ++store.native_reads;
    return it->second.native->slots[slot].image;
  }
  ++store.native_missing;
  return {};
}

NativeTextureTableHandle FindLoadedNativeTextureTable(uint32_t source_table) {
  if (!REXCVAR_GET(bd_native_texture_tables)) return {};
  auto &store = Tables(); std::lock_guard lock(store.mutex);
  const auto it = store.tables.find(source_table);
  return it == store.tables.end() ? nullptr : it->second.native;
}

void NativeTextureTableImageChanged(uint32_t source_image, NativeTextureTableSlot slot) noexcept {
  if (!source_image) return;
  auto &store = Tables(); std::lock_guard lock(store.mutex);
  for (auto it = store.tables.begin(); it != store.tables.end();) {
    auto &entry = it->second;
    if (std::find(entry.sources.begin(), entry.sources.end(), source_image) == entry.sources.end()) {
      ++it; continue;
    }
    NativeTextureTableHandle replacement;
    try {
      replacement = RebindTextureTable(store.library, entry.native, entry.sources,
                                       entry.sources.capacity(), source_image, slot);
    } catch (...) { /* A refused update must invalidate the old lookup. */ }
    if (!replacement) {
      it = store.tables.erase(it); ++store.retired; ++store.refused;
    } else {
      store.replaced += entry.native != replacement;
      entry.native = std::move(replacement); ++it;
    }
  }
}
} // namespace bd::gpu::scene

REX_HOOK_RAW(hcgLoadTextureArray) {
  __imp__hcgLoadTextureArray(ctx, base);
  if (!REXCVAR_GET(bd_native_texture_tables) || !ctx.r3.u32) return;
  try { bd::gpu::scene::Publish(ctx.r3.u32); }
  catch (const std::exception &error) {
    bd::gpu::scene::Retire(ctx.r3.u32);
    BD_WARN("[native-texture-tables] load publication failed: {}", error.what());
  }
}
REX_HOOK_RAW(hcgTextureListRelease) {
  bd::gpu::scene::Retire(ctx.r3.u32);
  __imp__hcgTextureListRelease(ctx, base);
}
REX_HOOK_RAW(sub_8217B3C0) {
  const uint32_t asset = ctx.r3.u32;
  __imp__sub_8217B3C0(ctx, base);
  if (!REXCVAR_GET(bd_native_texture_tables)) return;
  try { bd::gpu::scene::PublishCompleted(asset, ctx.r3.u32); }
  catch (const std::exception &error) {
    if (const auto table = bd::gpu::scene::CompletedTextureTable(asset, 1, bd::gpu::scene::Word))
      bd::gpu::scene::Retire(*table);
    BD_WARN("[native-texture-tables] async publication failed: {}", error.what());
  }
}
REX_HOOK_RAW(sub_8217B518) {
  // Async table teardown releases requests, then frees the table directly;
  // unlike the synchronous path, it never calls hcgTextureListRelease.
  if (const auto table = bd::gpu::scene::CompletedTextureTable(ctx.r3.u32, 1, bd::gpu::scene::Word))
    bd::gpu::scene::Retire(*table);
  __imp__sub_8217B518(ctx, base);
}
REX_HOOK_RAW(bdLookupCurrentTableTexture) {
  using namespace bd::gpu::scene;
  const auto selected = REXCVAR_GET(bd_native_texture_tables) ? Lookup(ctx.r3.u32) : std::nullopt;
  if (!selected) {
    __imp__bdLookupCurrentTableTexture(ctx, base);
    auto &store = Tables(); std::lock_guard lock(store.mutex);
    ++store.fallback; Report(store); return;
  }
  bool same = true, image_same = true;
  const bool verify = REXCVAR_GET(bd_native_texture_tables_verify);
  const bool image_verifiable = selected->native && selected->native->primary;
  if (verify) {
    __imp__bdLookupCurrentTableTexture(ctx, base); // read-only independent reference
    same = ctx.r3.u32 == selected->source;
    if (image_verifiable)
      image_same = *selected->native == CaptureNativeTexture(bd::gpu::ResolveGuestTexture(ctx.r3.u32));
  }
  if (same) ctx.r3.u64 = selected->source;
  auto &store = Tables(); std::lock_guard lock(store.mutex);
  ++store.reads;
  if (verify) {
    ++store.checks; store.wrong += !same;
    store.image_checks += image_verifiable;
    store.image_wrong += !image_same;
  }
  Report(store);
}
