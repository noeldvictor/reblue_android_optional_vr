/**
 * @brief Immutable native shader/layout/input ownership for the shared PSO cache.
 * @copyright Copyright (c) 2026 reblue contributors
 * @license BSD 3-Clause, see LICENSE
 */
#pragma once
#include "gpu/scene/native_vertex_input.h"
#include <algorithm>

namespace bd::gpu {
class NativePipelineProgram;
using NativePipelineHandle = std::shared_ptr<const NativePipelineProgram>;

// Native producers supply compiled shaders, never shader hashes or register
// masks. This fixed-size owner pins the complete interface through async builds
// and cached pipeline lifetime. GPU shader storage belongs to the producer's
// resource budget; this class performs no compilation, source import or disk IO.
class NativePipelineProgram final : public std::enable_shared_from_this<NativePipelineProgram> {
public:
  static constexpr size_t kMaxSpecializations = 8;
  static NativePipelineHandle Create(
      std::shared_ptr<plume::RenderPipelineLayout> layout,
      std::shared_ptr<plume::RenderShader> vertex,
      std::shared_ptr<plume::RenderShader> pixel,
      scene::NativeVertexInputHandle input,
      std::span<const plume::RenderSpecConstant> specializations = {}) {
    if (!layout || !vertex || !input || specializations.size() > kMaxSpecializations) return {};
    for (size_t i = 0; i < specializations.size(); ++i)
      for (size_t j = 0; j < i; ++j)
        if (specializations[i].index == specializations[j].index) return {};
    auto program = std::shared_ptr<NativePipelineProgram>(new NativePipelineProgram);
    program->layout_ = std::move(layout);
    program->vertex_ = std::move(vertex);
    program->pixel_ = std::move(pixel); // null is an explicit depth-only recipe
    program->input_ = std::move(input);
    program->count_ = uint32_t(specializations.size());
    std::copy(specializations.begin(), specializations.end(), program->specializations_.begin());
    return program;
  }
  NativePipelineHandle Lease() const { return shared_from_this(); }
  const plume::RenderPipelineLayout *Layout() const { return layout_.get(); }
  const plume::RenderShader *Vertex() const { return vertex_.get(); }
  const plume::RenderShader *Pixel() const { return pixel_.get(); }
  const scene::NativeVertexInput *Input() const { return input_.get(); }
  std::span<const plume::RenderSpecConstant> Specializations() const {
    return {specializations_.data(), count_};
  }
  NativePipelineProgram(const NativePipelineProgram &) = delete;
  NativePipelineProgram &operator=(const NativePipelineProgram &) = delete;
private:
  NativePipelineProgram() = default;
  std::shared_ptr<plume::RenderPipelineLayout> layout_;
  std::shared_ptr<plume::RenderShader> vertex_, pixel_;
  scene::NativeVertexInputHandle input_;
  std::array<plume::RenderSpecConstant, kMaxSpecializations> specializations_{};
  uint32_t count_ = 0;
};

// A native program is authoritative, including a deliberately absent fragment
// shader. Reject mixed contracts instead of inheriting translated masks, shader
// replacements or input declarations. State remains POD for the existing key.
template <class State> bool NativePipelineStateValid(const State &state) {
  if (!state.native_program) return true;
  return !state.vertexShader && !state.pixelShader && !state.vertexDeclaration &&
      !state.native_vertex_input && !state.specConstants && !state.instancing &&
      !state.occlusionCounting &&
      (state.native_program->Pixel() || !state.colorWriteEnable ||
       state.renderTargetFormat == plume::RenderFormat::UNKNOWN);
}

inline void ApplyNativePipelineProgram(const NativePipelineProgram &program,
                                      plume::RenderGraphicsPipelineDesc &desc) {
  desc.pipelineLayout = program.Layout();
  desc.vertexShader = program.Vertex();
  desc.pixelShader = program.Pixel();
  const auto inputs = program.Input()->Elements();
  desc.inputElements = inputs.data();
  desc.inputElementsCount = uint32_t(inputs.size());
  const auto constants = program.Specializations();
  desc.specConstants = constants.empty() ? nullptr : constants.data();
  desc.specConstantsCount = uint32_t(constants.size());
}
} // namespace bd::gpu
