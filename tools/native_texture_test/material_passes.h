/**
 * @brief Callback-sensitive native material lifecycle and binding fixtures.
 * @copyright Copyright (c) 2026 reblue contributors
 * @license BSD 3-Clause, see LICENSE
 */
#pragma once
#include "gpu/scene/native_material_pass.h"
#include <array>
#include <cassert>
#include <functional>
#include <vector>

namespace material_pass_test {
struct Port {
  struct Recipe { uint32_t vs = 0, ps = 0, declaration = 0; };
  struct Participant { uint32_t mode = 0, next = 0; };
  std::array<Recipe, 16> recipes{};
  std::array<Participant, 258> participants{};
  std::array<uint32_t, 2> shader_recipes{}, saved{}, cached{}, bound{};
  std::array<uint32_t, 32> shaders{};
  std::vector<uint32_t> events;
  uint32_t active = 0, first = 0, declaration = 0, bound_declaration = 0, result = 99;
  std::function<void(uint32_t)> changed;
  void Event(uint32_t event) { events.push_back(event); if (changed) changed(event); }
  void ZeroResult() { result = 0; Event(10); }
  uint32_t ShaderRecipe(uint32_t stage) { return shader_recipes.at(stage); }
  void SetShaderRecipe(uint32_t stage, uint32_t value) { shader_recipes.at(stage) = value; Event(20 + stage); }
  uint32_t CachedShader(uint32_t stage) { return cached.at(stage); }
  void CacheShader(uint32_t stage, uint32_t value) { cached.at(stage) = value; Event(30 + stage); }
  uint32_t Shader(uint32_t recipe) { return shaders.at(recipe); }
  void BindShader(uint32_t stage, uint32_t value) { bound.at(stage) = value; Event(40 + stage); }
  uint32_t CachedDeclaration() { return declaration; }
  void CacheDeclaration(uint32_t value) { declaration = value; Event(50); }
  void BindDeclaration(uint32_t value) { bound_declaration = value; Event(51); }
  uint32_t RecipeDeclaration(uint32_t mode) { return recipes.at(mode).declaration; }
  uint32_t RecipeShader(uint32_t mode, uint32_t stage) { return stage ? recipes.at(mode).ps : recipes.at(mode).vs; }
  void SaveShaderRecipe(uint32_t stage, uint32_t value) { saved.at(stage) = value; Event(60 + stage); }
  uint32_t SavedShaderRecipe(uint32_t stage) { return saved.at(stage); }
  void SetActiveMode(uint32_t value) { active = value; Event(70); }
  uint32_t ActiveMode() { return active; }
  uint32_t FirstParticipant() { return first; }
  uint32_t ParticipantMode(uint32_t participant) { return participants.at(participant).mode; }
  uint32_t NextParticipant(uint32_t participant) { return participants.at(participant).next; }
  void Invoke(uint32_t participant, bool ending) { result = participant; Event(1000 + participant * 2 + ending); }
};
inline void Bindings() {
  using namespace bd::gpu::scene;
  for (bool force : {false, true}) for (uint32_t vs_recipe : {0u, 1u, 2u})
    for (uint32_t ps_recipe : {0u, 3u, 4u}) for (uint32_t vs_cache : {0u, 101u, 202u})
      for (uint32_t ps_cache : {0u, 303u, 404u}) {
        Port port;
        port.shaders[1] = 101; port.shaders[2] = 202; port.shaders[3] = 303; port.shaders[4] = 0;
        port.shader_recipes = {vs_recipe, ps_recipe};
        port.bound = port.cached = {vs_cache, ps_cache};
        std::vector<uint32_t> expected;
        auto expected_bound = port.bound;
        for (uint32_t stage = 0; stage < 2; ++stage) {
          const auto recipe = port.shader_recipes[stage];
          const auto value = recipe ? port.shaders[recipe] : 0;
          const bool bind = recipe ? force || value != port.cached[stage] : port.cached[stage] != 0;
          if (bind) { expected.push_back(40 + stage); expected.push_back(30 + stage); expected_bound[stage] = value; }
        }
        BindMaterialShaders(port, force);
        assert(port.events == expected && port.bound == expected_bound && port.cached == expected_bound);
      }
  Port changed;
  changed.shader_recipes = {1, 2}; changed.shaders[1] = 101; changed.shaders[2] = 202; changed.shaders[3] = 303;
  changed.changed = [&](uint32_t event) {
    if (event == 40) { changed.shaders[1] = 404; changed.shader_recipes[1] = 3; }
  };
  BindMaterialShaders(changed, false);
  assert(changed.bound == (std::array<uint32_t, 2>{101, 303}));
  assert(changed.cached == (std::array<uint32_t, 2>{404, 303}));
  for (uint32_t cached : {0u, 8u, 9u}) for (uint32_t requested : {0u, 8u, 9u}) {
    Port port; port.declaration = port.bound_declaration = cached;
    SelectMaterialDeclaration(port, requested);
    const bool bind = requested && requested != cached;
    assert(port.declaration == (bind ? requested : cached));
    assert(port.bound_declaration == port.declaration);
    assert(port.events == (bind ? std::vector<uint32_t>{51, 50} : std::vector<uint32_t>{}));
  }
}
inline void Lifecycle() {
  using namespace bd::gpu::scene;
  Port idle;
  SelectMaterialRecipe(idle, 0);
  assert(idle.events.empty() && idle.result == 99);
  idle.shader_recipes = {1, 2};
  BeginMaterialPass(idle, 0);
  assert(idle.saved == idle.shader_recipes && idle.active == 0 && idle.result == 0);
  assert(idle.events == (std::vector<uint32_t>{60, 61, 70, 10}));

  Port live;
  live.shader_recipes = {1, 2}; live.recipes[3] = {3, 4, 9}; live.first = 1;
  live.shaders[3] = 303; live.shaders[4] = 404; live.shaders[5] = 505;
  live.participants[1] = {3, 2}; live.participants[2] = {4, 3}; live.participants[3] = {4, 0};
  live.changed = [&](uint32_t event) {
    if (event == 60) live.shader_recipes[1] = 7; // sequential save alias
    if (event == 61) assert(live.saved[1] == 7);
    if (event == 51) live.recipes[3].ps = 5; // declaration bind changes later recipe read
    if (event == 40) live.active = 4; // choose callback using live published mode
    if (event == 1004) live.saved = {0, 0};
  };
  BeginMaterialPass(live, 3);
  assert(live.saved == (std::array<uint32_t, 2>{0, 0}));
  assert(live.shader_recipes == (std::array<uint32_t, 2>{3, 5}));
  assert(live.bound == (std::array<uint32_t, 2>{303, 505}));
  assert(live.events == (std::vector<uint32_t>{60, 61, 70, 51, 50, 20, 21, 10, 40, 30, 41, 31, 1004}));
  live.changed = [&](uint32_t event) {
    if (event == 1005) { live.saved = {1, 2}; live.active = 8; }
    if (event == 20) live.saved[1] = 5; // restore next word only after first publication
  };
  live.events.clear();
  EndMaterialPass(live);
  assert(live.shader_recipes == (std::array<uint32_t, 2>{1, 5}));
  assert(live.bound == (std::array<uint32_t, 2>{0, 505}) && live.active == 8);
  assert(live.events == (std::vector<uint32_t>{1005, 20, 21, 10, 40, 30}));

  for (uint32_t count : {0u, 1u, 256u, 257u}) {
    Port chain; chain.first = count ? 1 : 0; chain.active = 0xffffffff;
    for (uint32_t i = 1; i <= count; ++i) chain.participants[i] = {0, i == count ? 0 : i + 1};
    if (count && count <= 256) chain.participants[count].mode = 0xffffffff;
    if (count == 257) {
      try { InvokeMaterialParticipant(chain, false); assert(false); } catch (const std::runtime_error &) {}
      assert(chain.events.empty());
    } else {
      InvokeMaterialParticipant(chain, false);
      assert(chain.events == (std::vector<uint32_t>{count ? 1000 + count * 2 : 10}));
    }
  }
  Port cycle; cycle.first = 1; cycle.active = 4; cycle.participants[1] = {3, 1};
  try { InvokeMaterialParticipant(cycle, true); assert(false); } catch (const std::runtime_error &) {}
  assert(cycle.events.empty());
  Port absent; absent.first = 1; absent.active = 9; absent.participants[1] = {3, 0};
  InvokeMaterialParticipant(absent, false);
  assert(absent.result == 0 && absent.events == std::vector<uint32_t>{10});
  Port failed; failed.first = 1; failed.participants[1] = {0, 0};
  failed.changed = [](uint32_t event) { if (event == 1003) throw 1; };
  try { EndMaterialPass(failed); assert(false); } catch (int) {}
  assert(failed.events == std::vector<uint32_t>{1003}); // no restoration/replay after failed callback
}
inline void Run() { Bindings(); Lifecycle(); }
} // namespace material_pass_test
