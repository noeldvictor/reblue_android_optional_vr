// CPU contract checks only: no GPU objects are allocated or dereferenced here.
#include "gpu/host_post_inputs.h"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <limits>
#include <type_traits>
using namespace bd::gpu;
using namespace plume;

int main() {
  static_assert(std::is_trivially_copyable_v<SampledImage>);
  // Distinct address tokens model physical image identity, not guest headers.
  int identities[3]{};
  auto *color = reinterpret_cast<RenderTexture *>(&identities[0]);
  auto *depth = reinterpret_cast<RenderTexture *>(&identities[1]);
  auto *output = reinterpret_cast<RenderTexture *>(&identities[2]);
  RenderTextureLayout color_layout = RenderTextureLayout::COLOR_WRITE;
  RenderTextureLayout depth_layout = RenderTextureLayout::DEPTH_WRITE;
  SampledImage scene{color, &color_layout, 1920, 1080, 1,
      RenderFormat::R16G16B16A16_FLOAT, 17, 1};
  SampledImage z{depth, &depth_layout, 1920, 1080, 1,
      RenderFormat::D32_FLOAT_S8_UINT, 18, 1};
  assert(scene && z && !SampledImage{});
  HostPostInputs inputs{scene, z, .25f};
  assert(inputs.CanRenderTo(output, 1));
  assert(!inputs.CanRenderTo(nullptr, 1));
  assert(!inputs.CanRenderTo(output, 0));
  assert(!inputs.CanRenderTo(output, 2));
  assert(!inputs.CanRenderTo(color, 1));
  assert(!inputs.CanRenderTo(depth, 1));
  auto aliased = inputs;
  aliased.depth.texture = color;
  assert(!aliased.CanRenderTo(output, 1));

  const auto rejects = [&](SampledImage invalid) {
    assert(!invalid);
    assert((!HostPostInputs{invalid, z, 1}.CanRenderTo(output, 1)));
    assert((!HostPostInputs{scene, invalid, 1}.CanRenderTo(output, 1)));
  };
  auto invalid = scene;
  invalid.texture = nullptr; rejects(invalid);
  invalid = scene; invalid.layout = nullptr; rejects(invalid);
  invalid = scene; invalid.width = 0; rejects(invalid);
  invalid = scene; invalid.height = 0; rejects(invalid);
  invalid = scene; invalid.format = RenderFormat::UNKNOWN; rejects(invalid);
  invalid = scene; invalid.descriptor_index = ~uint32_t{0}; rejects(invalid);
  // A native Texture2DArray reader cannot accept an unresolved MSAA image.
  for (uint32_t samples : {0u, 2u, 4u, 8u, 16u, 32u, 64u}) {
    invalid = scene; invalid.samples = samples; rejects(invalid);
  }
  for (uint32_t layers : {0u, 3u, 32u}) {
    invalid = scene; invalid.layers = layers; rejects(invalid);
  }
  // Slot zero is a valid native descriptor; only the explicit sentinel fails.
  invalid = scene; invalid.descriptor_index = 0; assert(invalid);
  for (float exposure : {0.0f, -1.0f, std::numeric_limits<float>::infinity(),
                         -std::numeric_limits<float>::infinity(),
                         std::numeric_limits<float>::quiet_NaN()}) {
    inputs.exposure = exposure;
    assert(!inputs.CanRenderTo(output, 1));
  }
  inputs.exposure = .25f;
  inputs.scene.layers = 2;
  assert(!inputs.CanRenderTo(output, 2));
  inputs.depth.layers = 2;
  assert(inputs.CanRenderTo(output, 2));
  // Colour and depth may have different extents: consumers sample normalized UV.
  inputs.scene.width = 1440; inputs.scene.height = 1584;
  assert(inputs.CanRenderTo(output, 2));

  // Copies still update the owner's live layout, never a cached layout value.
  auto borrowed = inputs.scene;
  *borrowed.layout = RenderTextureLayout::SHADER_READ;
  assert(color_layout == RenderTextureLayout::SHADER_READ);
  assert(*scene.layout == color_layout && *inputs.scene.layout == color_layout);
  color_layout = RenderTextureLayout::COLOR_WRITE;
  assert(*borrowed.layout == RenderTextureLayout::COLOR_WRITE);
  assert(inputs.exposure == .25f); // validation does not consume/apply exposure
}
