/**
 * @file    deferred.cpp
 * @brief   Host deferred-work contracts, without game memory or a GPU.
 * @copyright Copyright (c) 2026 reblue contributors
 * @license BSD 3-Clause, see LICENSE
 */
#include "gpu/scene/deferred_depth.h"
#include "gpu/scene/deferred_entry_bridge.h"
#include "gpu/scene/deferred_work.h"
#include "gpu/scene/native_deferred_contract.h"
#include <array>
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <limits>
#include <random>
#include <unordered_map>

using namespace bd::gpu::scene;

void TestCallbackContract() {
  constexpr uint32_t registry = (uint32_t(-32030) << 16) - 31132;
  constexpr uint32_t lights = 0x82E246F4, shader = 0x82783A58, visual = 0x9000;
  std::unordered_map<uint64_t,uint32_t> words{
      {registry,0x1000},{registry+8,2},{0x1000,lights},{0x1004,shader},
      {lights,0x2000},{0x2000,0x8218B310},{0x2004,0x820DFA50},
      {shader,0x2100},{0x2100,0x82174270},{0x2104,0x820DFA50},
      {0x2108,0x82174648},{0x210C,0x82174C60},
      {registry+12,0x3000},{registry+20,11},
      {visual,0x4000},{0x4020,0x820DFA50},{0x4024,0x820DFA50},
      {visual+3000,0},{visual+1864,0}};
  for (uint32_t i = 0; i < 11; ++i) {
    const uint32_t object = i == 10 ? shader : i == 0 ? 0x82DD6100 :
        i == 9 ? 0x82DD6FC0 : 0x82DD62A0 + (i-1)*420;
    words[0x3000+i*4] = object;
    if (i == 10) continue;
    const uint32_t table = 0x5000+i*32;
    words[object] = table;
    words[table+8] = i == 0 ? 0x82176708 : i == 9 ? 0x820D1998 : 0x82177650;
    words[table+12] = 0x820DFA50;
  }
  const auto read = [&](uint64_t address) -> std::optional<uint32_t> {
    const auto it = words.find(address);
    return it == words.end() ? std::nullopt : std::optional(it->second);
  };
  for (uint32_t mode = 0; mode < 6; ++mode) {
    words[visual+1864] = mode;
    assert(CheckNativeDeferredContract(visual,read) == mode);
  }
  const auto valid = words;
  for (const auto &[address,value] : std::array<std::pair<uint64_t,uint32_t>,12>{{
      {registry+8,1}, {0x1000,shader}, {0x2000,0x82174270},
      {0x2104,0x82174C60}, {registry+20,12}, {0x3004,0x82DD6100},
      {0x3028,0x82DD6FC0}, {0x5008,0xDEADBEEF}, {0x500C,0},
      {0x4024,0x82174C60}, {visual+3000,14}, {visual+1864,6}}}) {
    words = valid; words[address] = value;
    assert(!CheckNativeDeferredContract(visual,read));
  }
  words = valid; words.erase(0x4020);
  assert(!CheckNativeDeferredContract(visual,read));
  words = valid; words[registry+20] = 1; words[0x3000] = shader;
  assert(CheckNativeDeferredContract(visual,read) == 5); // optional features removed
  assert(!CheckNativeDeferredContract(0,read));
}

void TestMixedOrder() {
  const std::array<float,3> compatibility{10,30,20};
  const std::array<DeferredInsertion,4> native{{{30,0},{20,1},{30,1},{20,3}}};
  std::vector<DeferredSelection> merged;
  assert(MergeDeferredWork(compatibility,native,merged));
  const std::array<uint32_t,7> submitted{100,0,101,102,1,2,103};
  for (size_t i=0;i<merged.size();++i)
    assert(merged[i].index + (merged[i].native ? 100 : 0) == submitted[i]);
  std::vector<DeferredSortItem> order;
  for (uint32_t i=0;i<merged.size();++i) {
    const auto &entry=merged[i];
    order.push_back({entry.native ? native[entry.index].depth : compatibility[entry.index],i});
  }
  assert(OrderDeferredWork(order));
  const std::array<uint32_t,7> sorted{100,102,1,101,2,103,0};
  for (size_t i=0;i<order.size();++i) {
    const auto entry=merged[order[i].payload];
    assert(entry.index + (entry.native ? 100 : 0) == sorted[i]);
  }
  const auto unchanged = merged;
  const auto refuses = [&](std::span<const DeferredInsertion> values,size_t limit=5140) {
    assert(!MergeDeferredWork(compatibility,values,merged,limit));
    assert(merged.size()==unchanged.size());
    for (size_t i=0;i<merged.size();++i)
      assert(merged[i].native==unchanged[i].native && merged[i].index==unchanged[i].index);
  };
  auto bad=native; bad[1].preceding=4; refuses(bad);
  bad=native; bad[2].preceding=0; refuses(bad);
  bad=native; bad[0].depth=std::numeric_limits<float>::quiet_NaN(); refuses(bad);
  refuses(native,6);
  assert(MergeDeferredWork({},std::array<DeferredInsertion,2>{{{1,0},{2,0}}},merged));
  assert(merged.size()==2 && merged[0].native && merged[1].native);
  assert(MergeDeferredWork(compatibility,{},merged) && merged.size()==3 && !merged[0].native);
  assert(MergeDeferredWork({},{},merged) && merged.empty());
}

void TestDepth() {
  const DeferredMatrix identity{1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
  DeferredDepthRecipe bounds;
  bounds.centre = {2, 3, -10};
  bounds.radius = 2;
  assert(EvaluateDeferredDepth(bounds, identity, identity) == 8);
  auto world = identity;
  world[14] = -5;
  assert(EvaluateDeferredDepth(bounds, world, identity) == 13);
  auto view = identity;
  view[14] = 7;
  assert(EvaluateDeferredDepth(bounds, world, view) == 6);
  // Camera rotation uses X, not the previously captured Z key.
  view = {0, 0, -1, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1};
  assert(EvaluateDeferredDepth(bounds, identity, view) == 0);
  // Preserve the producer's unscaled far extent; no invented max-axis scale.
  world = identity;
  world[0] = 2;
  world[5] = 3;
  world[10] = 4;
  assert(EvaluateDeferredDepth(bounds, world, identity) == 38);
  bounds.radius = -2;
  assert(EvaluateDeferredDepth(bounds, identity, identity) == 12);
  bounds.radius = 2;

  // Compare arbitrary affine transforms against independent double-precision
  // point->world->view math, not a copy of the optimized Z-column expression.
  std::mt19937 random(123);
  auto value = [&] { return (int(random() % 2001) - 1000) / 100.0f; };
  for (int sample = 0; sample < 1000; ++sample) {
    world = identity;
    view = identity;
    for (size_t row = 0; row < 4; ++row)
      for (size_t col = 0; col < 3; ++col) {
        world[row * 4 + col] = value();
        view[row * 4 + col] = value();
      }
    for (auto &axis : bounds.centre)
      axis = value();
    bounds.radius = value();
    std::array<double, 4> point{bounds.centre[0], bounds.centre[1],
                                bounds.centre[2], 1};
    std::array<double, 4> transformed{};
    for (size_t col = 0; col < 4; ++col)
      for (size_t row = 0; row < 4; ++row)
        transformed[col] += point[row] * world[row * 4 + col];
    double reference = -bounds.radius;
    for (size_t row = 0; row < 4; ++row)
      reference -= transformed[row] * view[row * 4 + 2];
    const auto actual = EvaluateDeferredDepth(bounds, world, view);
    assert(actual && std::fabs(*actual - reference) <
                         0.001 + std::fabs(reference) * 1e-5);
  }

  bounds = {};
  for (float bad : {std::numeric_limits<float>::quiet_NaN(),
                    std::numeric_limits<float>::infinity(),
                    -std::numeric_limits<float>::infinity()}) {
    bounds.radius = bad;
    assert(!EvaluateDeferredDepth(bounds, identity, identity));
    bounds.radius = 0;
    for (size_t i = 0; i < 3; ++i) {
      bounds.centre[i] = bad;
      assert(!EvaluateDeferredDepth(bounds, identity, identity));
      bounds.centre[i] = 0;
    }
    for (size_t i = 0; i < 16; ++i) {
      world = identity;
      world[i] = bad;
      assert(!EvaluateDeferredDepth(bounds, world, identity));
      assert(!EvaluateDeferredDepth(bounds, identity, world));
    }
  }
  world = identity;
  world[10] = std::numeric_limits<float>::max();
  bounds.centre[2] = 2;
  assert(!EvaluateDeferredDepth(bounds, world,
                                identity)); // finite inputs overflow
  bounds.kind = static_cast<DeferredDepthRecipe::Kind>(99);
  assert(!EvaluateDeferredDepth(bounds, identity, identity));
  bounds.kind = DeferredDepthRecipe::Kind::Fixed;
  bounds.fixed_depth = -17;
  world.fill(std::numeric_limits<float>::quiet_NaN());
  assert(EvaluateDeferredDepth(bounds, world, world) ==
         -17); // no matrix dependency
  bounds.fixed_depth = std::numeric_limits<float>::infinity();
  assert(!EvaluateDeferredDepth(bounds, identity, identity));

  // Live movement must actually change back-to-front submission order.
  bounds = {};
  bounds.centre = {0, 0, -10};
  std::array work{
      DeferredSortItem{*EvaluateDeferredDepth(bounds, identity, identity), 0},
      DeferredSortItem{15, 1}};
  assert(OrderDeferredWork(work) && work[0].payload == 1);
  world = identity;
  world[14] = -20;
  work[1].depth = *EvaluateDeferredDepth(bounds, world, identity);
  assert(OrderDeferredWork(work) && work[0].payload == 0);

  std::array<DeferredEntryRecipe, 2> entries;
  entries[0].depth = bounds;
  bounds.kind = DeferredDepthRecipe::Kind::Fixed;
  bounds.fixed_depth = -100;
  entries[1].depth = bounds;
  std::vector<float> depths{99};
  assert(EvaluateDeferredEntryDepths(entries, world, identity, depths));
  assert((depths == std::vector<float>{30, -100}));
  const auto saved = depths;
  entries[1].depth.reset(); // no inferred policy from old image bytes
  assert(!EvaluateDeferredEntryDepths(entries, world, identity, depths));
  assert(depths == saved); // no partial update from entry 0
  bounds.fixed_depth = std::numeric_limits<float>::quiet_NaN();
  entries[1].depth = bounds;
  assert(!EvaluateDeferredEntryDepths(entries, world, identity, depths));
  assert(depths == saved);
  assert(EvaluateDeferredEntryDepths({}, world, identity, depths));
  assert(depths.empty());
}

int main() {
  TestCallbackContract();
  TestMixedOrder();
  TestDepth();
  std::array items{DeferredSortItem{2, 0}, DeferredSortItem{-1, 1},
                   DeferredSortItem{2, 2}, DeferredSortItem{7, 3}};
  assert(OrderDeferredWork(items));
  assert(items[0].payload == 3 && items[1].payload == 0 &&
         items[2].payload == 2 && items[3].payload == 1);
  assert(OrderDeferredWork({}));
  for (float bad : {std::numeric_limits<float>::quiet_NaN(),
                    std::numeric_limits<float>::infinity(),
                    -std::numeric_limits<float>::infinity()}) {
    auto invalid = items;
    invalid[1].depth = bad;
    assert(!OrderDeferredWork(invalid));
    for (size_t i = 0; i < items.size(); ++i)
      assert(invalid[i].payload == items[i].payload);
  }
  std::vector<DeferredSortItem> many;
  std::mt19937 random(17);
  for (uint32_t i = 0; i < 5140; ++i)
    many.push_back({float(int(random() % 200) - 100), i});
  assert(OrderDeferredWork(many));
  std::vector<bool> seen(many.size());
  for (size_t i = 0; i < many.size(); ++i) {
    assert(!seen[many[i].payload]);
    seen[many[i].payload] = true;
    if (i) {
      assert(many[i - 1].depth >= many[i].depth);
      if (many[i - 1].depth == many[i].depth)
        assert(many[i - 1].payload < many[i].payload);
    }
  }
  DeferredBatchPlan plan;
  const std::array<uint32_t, 3> sizes{816, 832, 912};
  assert(PlanDeferredBatch({3000, 100, 8, 4}, sizes, plan));
  assert((plan.offsets == std::vector<uint32_t>{100, 916, 1748}));
  assert(plan.bytes_used == 2660 && plan.items_used == 7);
  const auto saved = plan;
  for (DeferredArenaState invalid : {DeferredArenaState{2659, 100, 8, 4},
                                     {3000, 100, 6, 4},
                                     {99, 100, 8, 4},
                                     {3000, 100, 3, 4},
                                     {3000, 101, 8, 4}}) {
    assert(!PlanDeferredBatch(invalid, sizes, plan));
    assert(plan.offsets == saved.offsets &&
           plan.bytes_used == saved.bytes_used &&
           plan.items_used == saved.items_used);
  }
  assert(PlanDeferredBatch({2660, 100, 7, 4}, sizes, plan)); // exact fit
  for (uint32_t bad : {0u, 3u, 0xfffffffcu}) {
    assert(!PlanDeferredBatch({3000, 100, 8, 4}, std::span(&bad, 1), plan));
    assert(plan.offsets == saved.offsets &&
           plan.bytes_used == saved.bytes_used);
  }
  uint32_t four = 4;
  assert(!PlanDeferredBatch({UINT32_MAX, UINT32_MAX - 3, 8, 4},
                            std::span(&four, 1), plan));
  assert(PlanDeferredBatch({0, 0, 0, 0}, {}, plan));
  assert(plan.offsets.empty() && !plan.bytes_used && !plan.items_used);

  std::vector<uint8_t> image(816 + 9 * 4, 0x35);
  image[289] = 9;
  std::array<uint8_t, 64> matrix;
  for (size_t i = 0; i < matrix.size(); ++i)
    matrix[i] = uint8_t(i);
  auto relocated = image;
  assert(ValidDeferredEntryImage(image));
  assert(RelocateDeferredEntry(image, matrix, 0x50000, 0x12345678, relocated));
  assert(std::equal(matrix.begin(), matrix.end(), relocated.begin() + 16));
  assert(relocated[264] == 0 && relocated[265] == 5 && relocated[266] == 1 &&
         relocated[267] == 0x84);
  assert(relocated[268] == 0x12 && relocated[269] == 0x34 &&
         relocated[270] == 0x56 && relocated[271] == 0x78);
  for (size_t i = 0; i < image.size(); ++i)
    if (!(i >= 16 && i < 80) && !(i >= 264 && i < 272))
      assert(relocated[i] == image[i]);
  const auto before = relocated;
  assert(RelocateDeferredEntry(image, matrix, 0x50000, 0x12345678, relocated,
                               12.5f));
  assert(relocated[276] == 0x41 && relocated[277] == 0x48 &&
         relocated[278] == 0 && relocated[279] == 0);
  for (size_t i = 0; i < before.size(); ++i)
    if (i < 276 || i >= 280)
      assert(relocated[i] == before[i]);
  const auto fresh = relocated;
  assert(!RelocateDeferredEntry(image, matrix, 0x50000, 0, relocated,
                                std::numeric_limits<float>::quiet_NaN()));
  assert(relocated == fresh);
  relocated = before;
  for (uint32_t bad : {0u, 1u, 0xffffff00u}) {
    assert(!RelocateDeferredEntry(image, matrix, bad, 0, relocated));
    assert(relocated == before);
  }
  image[289] = 10; // declared bones exceed payload
  assert(!ValidDeferredEntryImage(image));
  assert(!RelocateDeferredEntry(image, matrix, 0x50000, 0, relocated));
  assert(relocated == before);
  image.resize(816);
  image[289] = 0xff; // negative bone count has no palette payload
  assert(ValidDeferredEntryImage(image));
  image.resize(815);
  assert(!ValidDeferredEntryImage(image));
}
