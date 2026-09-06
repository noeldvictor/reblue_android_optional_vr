/**
 * @brief Native material-pass dispatch and shader-recipe binding order.
 * @copyright Copyright (c) 2026 reblue contributors
 * @license BSD 3-Clause, see LICENSE
 */
#pragma once
#include <cstdint>
#include <stdexcept>

namespace bd::gpu::scene {
inline constexpr uint32_t kMaterialParticipantLimit = 256;

template <class Port, class Handle>
void SelectMaterialDeclaration(Port &port, Handle declaration) {
  if (!declaration || declaration == port.CachedDeclaration()) return;
  port.BindDeclaration(declaration);
  port.CacheDeclaration(declaration);
}

template <class Port>
void BindMaterialShaders(Port &port, bool force) {
  for (uint32_t stage = 0; stage < 2; ++stage) {
    const auto recipe = port.ShaderRecipe(stage);
    if (recipe) {
      if (!force && port.Shader(recipe) == port.CachedShader(stage)) continue;
      port.BindShader(stage, port.Shader(recipe));
      // A binding callback may update the recipe. Cache after the bind, then
      // read the next stage's recipe; neither can be snapshotted early.
      port.CacheShader(stage, port.Shader(recipe));
    } else if (port.CachedShader(stage)) {
      port.BindShader(stage, 0);
      port.CacheShader(stage, 0);
    }
  }
}

template <class Port>
void SelectMaterialRecipe(Port &port, uint32_t mode) {
  if (!mode) return;
  SelectMaterialDeclaration(port, port.RecipeDeclaration(mode));
  port.SetShaderRecipe(0, port.RecipeShader(mode, 0));
  port.SetShaderRecipe(1, port.RecipeShader(mode, 1));
  port.ZeroResult();
  BindMaterialShaders(port, false);
}

template <class Port>
void InvokeMaterialParticipant(Port &port, bool ending) {
  auto participant = port.FirstParticipant();
  if (!participant) { port.ZeroResult(); return; }
  const auto mode = port.ActiveMode(); // after shader setup, not the requested mode
  for (uint32_t visited = 0; participant; ++visited) {
    if (visited == kMaterialParticipantLimit)
      throw std::runtime_error("Native material participant chain exceeded its bound");
    if (port.ParticipantMode(participant) == mode) {
      port.Invoke(participant, ending); // first match only, method read here
      return;
    }
    participant = port.NextParticipant(participant);
  }
  port.ZeroResult();
}

template <class Port>
void BeginMaterialPass(Port &port, uint32_t mode) {
  port.SaveShaderRecipe(0, port.ShaderRecipe(0));
  port.SaveShaderRecipe(1, port.ShaderRecipe(1));
  port.SetActiveMode(mode);
  SelectMaterialRecipe(port, mode);
  InvokeMaterialParticipant(port, false);
}

template <class Port>
void EndMaterialPass(Port &port) {
  InvokeMaterialParticipant(port, true);
  // The participant may mutate saved recipes, including sequential aliases.
  port.SetShaderRecipe(0, port.SavedShaderRecipe(0));
  port.SetShaderRecipe(1, port.SavedShaderRecipe(1));
  port.ZeroResult();
  BindMaterialShaders(port, false);
  // Active mode intentionally remains the last published mode, not a stack pop.
}
} // namespace bd::gpu::scene
