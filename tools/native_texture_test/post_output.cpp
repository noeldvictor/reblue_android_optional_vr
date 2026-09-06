// CPU contract checks: native interface doubles, no GPU allocation/submission.
#include "gpu/host_post_output.h"
#include "gpu/post_sequence.h"
#include <array>
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
using namespace bd::gpu;
using namespace plume;

namespace {
struct OutputFramebuffer : RenderFramebuffer {
  uint32_t width = 1440, height = 1584;
  uint32_t getWidth() const override { return width; }
  uint32_t getHeight() const override { return height; }
};
void OutputContract() {
  int identities[5]{};
  const auto texture = [&](uint32_t i) {
    return reinterpret_cast<RenderTexture *>(&identities[i]);
  };
  std::array<RenderTextureLayout, 5> layouts{};
  const auto sampled = [&](uint32_t i, uint32_t layers) {
    return SampledImage{texture(i), &layouts[i], 1440, 1584, layers,
        RenderFormat::R16G16B16A16_FLOAT, i, 1};
  };
  OutputFramebuffer framebuffer;
  HostPostOutput output{sampled(2, 1), &framebuffer};
  HostPostInputs inputs{sampled(0, 1), sampled(1, 1), .25f};
  inputs.depth.format = RenderFormat::D32_FLOAT_S8_UINT;
  assert(output && output.CanRender(inputs));
  assert(!HostPostOutput{} && !HostPostOutput{}.CanRender(inputs));
  const auto rejects = [&](HostPostOutput invalid) {
    assert(!invalid && !invalid.CanRender(inputs));
    assert(!invalid.CanSampleMono(sampled(4, 1)));
  };
  auto invalid = output;
  invalid.framebuffer = nullptr; rejects(invalid);
  invalid = output; invalid.image.texture = nullptr; rejects(invalid);
  invalid = output; invalid.image.layout = nullptr; rejects(invalid);
  invalid = output; invalid.image.descriptor_index = ~uint32_t{0}; rejects(invalid);
  for (auto format : {RenderFormat::UNKNOWN, RenderFormat::R8G8B8A8_UNORM,
                     RenderFormat::D32_FLOAT, RenderFormat::R16G16B16A16_UINT}) {
    invalid = output; invalid.image.format = format; rejects(invalid);
  }
  for (uint32_t samples : {0u, 2u, 4u, 8u}) {
    invalid = output; invalid.image.samples = samples; rejects(invalid);
  }
  for (uint32_t layers : {0u, 3u, 32u}) {
    invalid = output; invalid.image.layers = layers; rejects(invalid);
  }
  invalid = output; invalid.image.width = 0; rejects(invalid);
  invalid = output; invalid.image.height = 0; rejects(invalid);
  --framebuffer.width; rejects(output); ++framebuffer.width;
  --framebuffer.height; rejects(output); ++framebuffer.height;
  assert(output.CanRender(inputs));
  auto feedback = inputs;
  feedback.scene.texture = output.image.texture;
  assert(!output.CanRender(feedback));
  feedback = inputs; feedback.depth.texture = output.image.texture;
  assert(!output.CanRender(feedback));
  feedback = inputs; feedback.scene.layers = 2;
  assert(!output.CanRender(feedback));
  auto optical = sampled(4, 1);
  assert(output.CanSampleMono(optical));
  optical.descriptor_index = 0;
  assert(output.CanSampleMono(optical));
  // Different wrapper/descriptor/layout identities cannot hide physical feedback.
  optical.texture = output.image.texture;
  assert(!output.CanSampleMono(optical));
  optical = sampled(4, 2); assert(!output.CanSampleMono(optical));
  optical = sampled(4, 1); optical.samples = 4; assert(!output.CanSampleMono(optical));
  optical = sampled(4, 1); optical.layout = nullptr; assert(!output.CanSampleMono(optical));
  assert(!output.CanSampleMono({}));

  for (uint32_t layers : {1u, 2u}) {
    std::array<HostPostOutput, 2> targets{{
        {sampled(2, layers), &framebuffer}, {sampled(3, layers), &framebuffer}}};
    for (uint32_t count : {1u, 2u, 3u, PostSequence::kCapacity}) {
      inputs = {sampled(0, layers), sampled(1, layers), .25f};
      const auto plan = *MakePostSequence(count);
      for (uint32_t i = 0; i < count; ++i) {
        if (i) {
          inputs.scene = targets[plan.Output(i - 1)].image;
        }
        inputs.exposure = plan.Exposure(i, .25f);
        const auto &target = targets[plan.Output(i)];
        assert(target.CanRender(inputs));
        assert(inputs.exposure == (i ? 1.f : .25f));
        *target.image.layout = RenderTextureLayout::SHADER_READ;
        assert(layouts[2 + plan.Output(i)] == RenderTextureLayout::SHADER_READ);
      }
    }
  }
}
} // namespace
int main() { OutputContract(); }
