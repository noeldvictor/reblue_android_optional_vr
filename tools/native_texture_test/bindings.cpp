/**
 * @copyright Copyright (c) 2026 reblue contributors
 * @license BSD 3-Clause License
 */
#include "gpu/scene/native_texture_binding.h"
#include "gpu/scene/native_texture_table.h"
#include "gpu/scene/native_texture_table_source.h"
#include "gpu/scene/fenced_asset_cache.h"
#include "gpu/scene/scene_recipe_residency.h"
#include "gpu/sampler_key.h"
#include <cstdlib>
#include <iostream>
#include <latch>
#include <thread>

using namespace bd::gpu;
using namespace bd::gpu::scene;
using D = plume::RenderTextureViewDimension;

static void Check(bool ok, const char *why) {
  if (!ok) {
    std::cerr << why << '\n';
    std::exit(1);
  }
}
static NativeTextureGpuHandle Image(uint64_t id, uint32_t slot, D dim) {
  auto asset = std::make_shared<NativeTextureAsset>();
  asset->id = id;
  auto gpu = std::make_shared<NativeTextureGpu>();
  gpu->asset = asset;
  gpu->descriptor = slot;
  gpu->dimension = dim;
  return gpu;
}

static void TestTextureTables() {
  // Literal source addresses/layout are independent of the production decoder.
  std::unordered_map<uint64_t, uint32_t> source{{0x1000, 3}, {0x1004, 0x2000},
      {0x2018, 0x3000}, {0x2034, 0}, {0x2050, 0x3000}};
  auto read = [&](uint64_t address) -> std::optional<uint32_t> {
    const auto it = source.find(address);
    return it == source.end() ? std::nullopt : std::optional(it->second);
  };
  auto keys = ReadTextureTableSources(0x1000, read);
  Check(keys && *keys == std::vector<uint32_t>({0x3000, 0, 0x3000}), "complete loaded table layout");
  source[0x40B8] = 0x1000;
  Check(!CompletedTextureTable(0x4000, 0, read) && !CompletedTextureTable(0x4000, 2, read),
        "allocated async table cannot publish before all requests finish");
  Check(CompletedTextureTable(0x4000, 1, read) == 0x1000,
        "completed async poll publishes the asset's table, not the boolean return");
  source[0x40B8] = 0;
  Check(!CompletedTextureTable(0x4000, 1, read) &&
        !CompletedTextureTable(UINT32_MAX - 3, 1, read), "absent/overflow async table refused");
  Check(!ReadTextureTableSources(0, read) && !ReadTextureTableSources(0x1001, read) &&
        !ReadTextureTableSources(UINT32_MAX - 3, read), "table header bounds/alignment");
  source[0x1000] = 4097;
  Check(!ReadTextureTableSources(0x1000, read), "table count checked before allocation");
  source[0x1000] = 3; source[0x1004] = UINT32_MAX - 15;
  Check(!ReadTextureTableSources(0x1000, read), "record address cannot wrap");
  source[0x1004] = 0x2000; source.erase(0x2050);
  Check(!ReadTextureTableSources(0x1000, read), "partial table never publishes a prefix");
  source[0x1000] = 0; source[0x1004] = 0;
  Check(ReadTextureTableSources(0x1000, read)->empty(), "empty table is valid without entries");
  Check(TextureTableSourceIndex(UINT32_MAX, 2) == 1 && TextureTableSourceIndex(7, 3) == 10,
        "source offset addition preserves unsigned 32-bit behavior");

  NativeTextureTableLibrary library;
  NativeTextureTableSlot initial{{Image(70, 700, D::TEXTURE_2D_ARRAY), {}, {}}, true};
  std::vector<NativeTextureTableSlot> slots{initial, {{}, true}, initial};
  auto table = library.Create(std::move(slots), keys->capacity() * sizeof(uint32_t));
  const auto first_id = table->id;
  const auto first_bytes = library.Bytes();
  Check(first_id && first_bytes && library.Live() == 1, "bounded native identity and accounting");
  auto unchanged = RebindTextureTable(library, table, *keys, keys->capacity(), 0x9999, initial);
  Check(unchanged == table && library.Bytes() == first_bytes, "unrelated image creates no table");
  unchanged.reset();
  unchanged = RebindTextureTable(library, table, *keys, keys->capacity(), 0x3000, initial);
  Check(unchanged == table && library.Bytes() == first_bytes, "identical image reuses the existing table");
  unchanged.reset();
  const NativeTextureTableSlot changed{{Image(80, 800, D::TEXTURE_CUBE), {}, {}}, true};
  std::mutex snapshot_mutex;
  std::latch captured(1), update_attempted(1);
  auto current_image = initial.image;
  bool update_blocked = false;
  std::thread updater([&] {
    captured.wait();
    update_blocked = !snapshot_mutex.try_lock();
    if (!update_blocked) snapshot_mutex.unlock();
    update_attempted.count_down();
    std::lock_guard lock(snapshot_mutex);
    current_image = changed.image;
  });
  NativeTextureTableHandle snapshot;
  const uint32_t snapshot_sources[] = {0x3000, 0};
  PublishTextureTableSnapshot(snapshot_mutex, snapshot_sources, [&](uint32_t source) {
    if (source) { captured.count_down(); update_attempted.wait(); }
    return source ? current_image : NativeTextureBinding{};
  }, [&](std::vector<NativeTextureTableSlot> snapshot_slots) {
    Check(update_blocked && snapshot_slots[0].available &&
          snapshot_slots[0].image == initial.image && snapshot_slots[1].available &&
          !snapshot_slots[1].image.primary, "mirror lock spans snapshot collection and publication");
    snapshot = library.Create(std::move(snapshot_slots));
  });
  updater.join();
  Check(current_image == changed.image && snapshot->slots[0].image == initial.image,
        "later replacement cannot mutate a published immutable generation");
  snapshot.reset();
  try {
    PublishTextureTableSnapshot(snapshot_mutex, {}, [](uint32_t) { return NativeTextureBinding{}; },
        [](auto) { throw 1; });
    Check(false, "publication exception must escape");
  } catch (int) {}
  Check(snapshot_mutex.try_lock(), "publication failure must release the mirror lock");
  snapshot_mutex.unlock();
  auto replacement = RebindTextureTable(library, table, *keys, keys->capacity(), 0x3000, changed);
  Check(replacement && replacement->id > first_id && replacement->slots[0].image == changed.image &&
        replacement->slots[2].image == changed.image && replacement->slots[1].available &&
        !replacement->slots[1].image.primary, "replace every alias, preserve known null selection");
  Check(table->slots[0].image == initial.image && library.Live() == 2,
        "old consumers retain the exact immutable image generation");
  auto invalidated = RebindTextureTable(library, replacement, *keys, keys->capacity(),
                                        0x3000, NativeTextureTableSlot{});
  Check(invalidated && !invalidated->slots[0].available && !invalidated->slots[2].available &&
        invalidated->slots[1].available, "eviction marks unavailable without inventing a null binding");
  auto repaired = RebindTextureTable(library, invalidated, *keys, keys->capacity(), 0x3000, changed);
  Check(repaired && repaired->slots[0].image == changed.image && repaired->slots[0].available,
        "subsequent image publication repairs its live source associations");
  Check(!RebindTextureTable(library, table, std::span<const uint32_t>(*keys).first(1),
                           keys->capacity(), 0x3000, changed), "association mismatch refused");
  source.clear(); keys->clear(); initial = {};
  Check(table->slots[0].image.primary->asset->id == 70, "native assets survive source destruction");
  table.reset(); replacement.reset(); invalidated.reset(); repaired.reset();
  Check(library.Live() == 0 && library.Bytes() == 0, "all table generations release their accounting");
  auto empty = library.Create({});
  Check(empty && empty->id > first_id, "retirement cannot reuse a native table ID");
  Check(!library.Create(std::vector<NativeTextureTableSlot>(4097)), "native table slot cap");
  Check(!library.Create({}, SIZE_MAX), "adapter byte accounting cannot overflow");
  NativeTextureTableLibrary tiny(1);
  Check(!tiny.Create({}), "even an empty owner needs budget");
  NativeTextureTableLibrary one(NativeTextureTableLibrary::kMaxBytes, 1);
  auto pinned = one.Create({changed});
  Check(pinned && !one.Create({changed}), "pinned tables count against the live-owner cap");
  pinned.reset();
  Check(bool(one.Create({changed})), "releasing the lease restores capacity");
  NativeTextureTableLibrary inline_lists;
  std::vector<NativeTextureTableHandle> references;
  for (size_t i = 0; i < 8192; ++i) {
    auto reference = inline_lists.Create({changed}, sizeof(uint32_t));
    Check(bool(reference), "many inline single-slot lists must not be capped by GPU image count");
    references.push_back(std::move(reference));
  }
  Check(inline_lists.Live() == references.size() &&
        inline_lists.Bytes() <= NativeTextureTableLibrary::kMaxBytes,
        "inline list aliases retain one image within the unchanged aggregate byte budget");
  references.clear();
  Check(inline_lists.Live() == 0 && inline_lists.Bytes() == 0,
        "retiring all inline lists releases the complete table accounting");
  NativeTextureTableHandle survivor;
  { NativeTextureTableLibrary temporary; survivor = temporary.Create({changed}); }
  Check(survivor->slots[0].image.primary->asset->id == 80, "native lease may outlive the library");
}

int main() {
  TestTextureTables();
  const NativeTextureIndices nulls{1, 2, 3};
  auto two = Image(10, 100, D::TEXTURE_2D_ARRAY);
  auto cube = Image(20, 200, D::TEXTURE_CUBE);
  auto volume = Image(30, 300, D::TEXTURE_3D);
  NativeTextureBinding atlas{two, {}, cube};
  auto slots = TextureIndices(atlas, nulls);
  Check(slots.image_2d == 100 && slots.image_3d == 2 && slots.image_cube == 200,
        "native atlas and explicit cube companion");
  slots = TextureIndices({volume, two, {}}, nulls);
  Check(slots.image_2d == 100 && slots.image_3d == 300 && slots.image_cube == 3,
        "native volume and explicit slice companion");
  slots = TextureIndices({cube, {}, {}}, nulls);
  Check(slots.image_2d == 1 && slots.image_3d == 2 && slots.image_cube == 200,
        "cube must not populate other dimensions");
  slots = TextureIndices({}, nulls);
  Check(slots.image_2d == 1 && slots.image_3d == 2 && slots.image_cube == 3,
        "unbound slot resets every dimension");
  auto copied = atlas;
  two.reset();
  cube.reset();
  Check(copied == atlas && copied.primary->asset->id == 10 &&
            copied.cube->asset->id == 20,
        "material owns both assets after importer owners disappear");
  Check(copied != NativeTextureBinding{atlas.primary, {}, {}},
        "companion changes invalidate binding identity");

  FencedAssetCache<NativeTextureGpu> residency(64, 2);
  auto imported = residency.Acquire(42, 32, [] { return Image(42, 400, D::TEXTURE_2D_ARRAY); });
  NativeTextureBinding material{imported, {}, {}};
  imported.reset();
  unsigned retired = 0;
  auto retire = [&](const NativeTextureGpu &gpu) {
    Check(gpu.descriptor == 400, "retire the material's exact descriptor");
    ++retired;
  };
  residency.MarkUnused(0);
  residency.AfterFence(0, retire);
  Check(!retired && material.primary->asset->id == 42,
        "material pins image after the importer is gone");
  material = {}; // template invalidation/pruning drops the native owner
  residency.AfterFence(0, retire);
  Check(!retired, "dropping a material does not retire an unmarked image");
  residency.MarkUnused(1);
  residency.AfterFence(0, retire);
  Check(!retired, "material release cannot use an unrelated fence");
  residency.AfterFence(1, retire);
  Check(retired == 1 && residency.Stats().resident == 0,
        "material release retires exactly once after its marked fence");

  struct Recipe { uint32_t used_frame; NativeTextureBinding binding; };
  Check(HasDirectRecipe(false, true) && HasDirectRecipe(true, false) &&
            !HasDirectRecipe(false, false),
        "a volatile direct recipe cannot be classified as list-only");
  Check(SceneImportEpoch{1, 2} == SceneImportEpoch{1, 2} &&
            SceneImportEpoch{1, 2} != SceneImportEpoch{2, 2} &&
            SceneImportEpoch{1, 2} != SceneImportEpoch{1, 3},
        "texture or geometry changes expire the imported recipe");
  std::unordered_map<uint64_t, Recipe> draws;
  std::unordered_map<uint64_t, int> lists;
  draws.emplace(1, Recipe{100, atlas});
  draws.emplace(2, Recipe{400, atlas});
  lists.emplace(1, 1);
  lists.emplace(2, 1);
  lists.emplace(3, 1); // genuinely list-only node
  uint64_t retired_key = 0;
  auto forget = [&](uint64_t id) { retired_key = id; };
  Check(PruneNodeRecipes(draws, lists, 400, 300, forget) == 0,
        "recipe is retained at the age boundary");
  Check(PruneNodeRecipes(draws, lists, 401, 300, forget) == 1 && retired_key == 1 &&
            !draws.contains(1) && !lists.contains(1) && draws.contains(2) &&
            lists.contains(2) && lists.contains(3),
        "retiring draws must also retire their deferred list, not unrelated lists");
  draws.at(2).used_frame = 700; // lookup of even an empty/volatile recipe touches it
  Check(PruneNodeRecipes(draws, lists, 701, 300, forget) == 0,
        "a visited recipe does not lose its native owners");
  draws.at(2).used_frame = UINT32_MAX - 10;
  Check(PruneNodeRecipes(draws, lists, 10, 30, forget) == 0 &&
            PruneNodeRecipes(draws, lists, 30, 30, forget) == 1,
        "frame-age pruning handles unsigned frame wrap");

  const plume::RenderSamplerDesc base;
  const SamplerKey key(base);
  auto distinct = [&](const plume::RenderSamplerDesc &d) {
    Check(SamplerKey(d) != key, "sampler state omitted from identity");
  };
  auto changed = base; changed.mipLODBias = 1; distinct(changed);
  changed = base; changed.minLOD = 1; distinct(changed);
  changed = base; changed.maxLOD = 1; distinct(changed);
  changed = base; changed.anisotropyEnabled = true; distinct(changed);
  changed = base; changed.maxAnisotropy = 1; distinct(changed);
  changed = base; changed.comparisonEnabled = true; distinct(changed);
  changed = base; changed.comparisonFunc = plume::RenderComparisonFunction::LESS; distinct(changed);
  changed = base; changed.addressU = plume::RenderTextureAddressMode::CLAMP; distinct(changed);
  changed = base; changed.addressV = plume::RenderTextureAddressMode::CLAMP; distinct(changed);
  changed = base; changed.addressW = plume::RenderTextureAddressMode::CLAMP; distinct(changed);
  changed = base; changed.minFilter = plume::RenderFilter::NEAREST; distinct(changed);
  changed = base; changed.magFilter = plume::RenderFilter::NEAREST; distinct(changed);
  changed = base; changed.mipmapMode = plume::RenderMipmapMode::NEAREST; distinct(changed);
  changed = base; changed.borderColor = plume::RenderBorderColor::OPAQUE_WHITE; distinct(changed);
  changed = base; changed.shaderVisibility = plume::RenderShaderVisibility::PIXEL; distinct(changed);
  changed = base; changed.mipLODBias = -0.0f;
  Check(SamplerKey(changed) == key && SamplerKeyHash{}(SamplerKey(changed)) == SamplerKeyHash{}(key),
        "signed zero uses the same sampler");
  std::cout << "native texture bindings and complete sampler identity passed\n";
}
