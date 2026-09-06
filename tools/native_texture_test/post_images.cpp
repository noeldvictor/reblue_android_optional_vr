// CPU contract checks only: interface doubles, no real GPU allocation/submission.
#include "gpu/host_post_inputs.h"
#include "gpu/scene/native_scene_resolves.h"
#include "gpu/scene/fenced_asset_cache.h"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <limits>
#include <type_traits>
using namespace bd::gpu;
using namespace plume;

namespace {
// Real owner/cache types, CPU-only interface doubles. Destruction assertions
// catch ordering errors without allocating GPU objects or retaining artifacts.
struct Lifetime {
  int images = 0, views = 0, framebuffers = 0;
  bool descriptors_live = true;
  std::array<std::weak_ptr<const NativeTargetImage>, 2> sources;
};
struct TestView : RenderTextureView {
  Lifetime &lifetime;
  explicit TestView(Lifetime &value) : lifetime(value) { ++lifetime.views; }
  ~TestView() override {
    assert(!lifetime.descriptors_live && lifetime.framebuffers == 0 && lifetime.images == 2);
    --lifetime.views;
  }
};
struct TestImage : RenderTexture {
  Lifetime &lifetime;
  explicit TestImage(Lifetime &value) : lifetime(value) { ++lifetime.images; }
  ~TestImage() override {
    assert(!lifetime.descriptors_live && !lifetime.views && !lifetime.framebuffers);
    --lifetime.images;
  }
  std::unique_ptr<RenderTextureView> createTextureView(const RenderTextureViewDesc &) const override {
    return std::make_unique<TestView>(lifetime);
  }
  void setName(const std::string &) override {}
};
struct TestFramebuffer : RenderFramebuffer {
  Lifetime &lifetime;
  explicit TestFramebuffer(Lifetime &value) : lifetime(value) { ++lifetime.framebuffers; }
  ~TestFramebuffer() override {
    assert(!lifetime.descriptors_live && lifetime.images == 2 && lifetime.views == 2);
    for (const auto &source : lifetime.sources) assert(!source.expired());
    --lifetime.framebuffers;
  }
  uint32_t getWidth() const override { return 1440; }
  uint32_t getHeight() const override { return 1584; }
};
void ResolveOwnership() {
  using namespace bd::gpu::scene;
  Lifetime lifetime;
  FencedAssetCache<NativeSceneResolves> cache(256, 1);
  auto create = [&] {
    auto owner = std::make_shared<NativeSceneResolves>();
    owner->sources.width = 1440; owner->sources.height = 1584;
    owner->sources.layers = 2; owner->sources.samples = 4;
    owner->sources.color_identity = 5; owner->sources.depth_identity = 6;
    owner->sources.color_format = RenderFormat::R16G16B16A16_FLOAT;
    owner->sources.depth_format = RenderFormat::D32_FLOAT_S8_UINT;
    for (uint32_t i = 0; i < 2; ++i) {
      // Handle-only source doubles: dependency lifetime, not GPU validity.
      owner->source_owners[i] = std::make_shared<NativeTargetImage>();
      lifetime.sources[i] = owner->source_owners[i];
      owner->images[i] = std::make_unique<TestImage>(lifetime);
      owner->views[i] = owner->images[i]->createTextureView({});
      owner->descriptors[i] = 21 + i;
    }
    owner->framebuffer = std::make_unique<TestFramebuffer>(lifetime);
    return owner;
  };
  auto owner = cache.Acquire(1, 256, create);
  const auto sampled = owner->Sampled(.25f);
  assert(sampled.scene && sampled.depth && sampled.opaque_scene_alpha);
  assert(sampled.exposure == .25f && sampled.scene.samples == 1 && sampled.depth.samples == 1);
  assert(sampled.scene.layers == 2 && sampled.depth.layers == 2);
  assert(sampled.scene.width == 1440 && sampled.depth.height == 1584);
  assert(sampled.scene.descriptor_index == 21 && sampled.depth.descriptor_index == 22);
  assert(sampled.scene.format == RenderFormat::R16G16B16A16_FLOAT);
  assert(sampled.depth.format == RenderFormat::D32_FLOAT_S8_UINT);
  assert(sampled.scene.texture == owner->images[0].get());
  assert(sampled.depth.texture == owner->images[1].get());
  *sampled.scene.layout = RenderTextureLayout::SHADER_READ;
  assert(owner->layouts[0] == RenderTextureLayout::SHADER_READ);
  auto generation = owner->sources;
  ++generation.color_identity;
  assert(generation != owner->sources); // same addresses cannot alias another generation
  generation = owner->sources; ++generation.depth_identity;
  assert(generation != owner->sources);
  generation = owner->sources; generation.layers = 1;
  assert(generation != owner->sources);
  auto post_lease = owner;
  owner.reset();
  const auto retire = [&](const NativeSceneResolves &images) {
    assert(images.descriptors[0] == 21 && images.descriptors[1] == 22);
    lifetime.descriptors_live = false;
  };
  cache.MarkUnused(0); cache.AfterFence(0, retire);
  assert(lifetime.images == 2 && lifetime.descriptors_live); // post still owns the pair
  post_lease.reset();
  cache.MarkUnused(0); cache.AfterFence(1, retire);
  assert(lifetime.images == 2 && cache.Stats().bytes == 256);
  auto reused = cache.Acquire(1, 256, create);
  cache.AfterFence(0, retire);
  assert(reused && lifetime.images == 2 && cache.Stats().created == 1);
  reused.reset(); cache.MarkUnused(1); cache.AfterFence(1, retire);
  assert(!lifetime.images && !lifetime.views && !lifetime.framebuffers);
  assert(!cache.Stats().bytes && cache.Stats().retired == 1);
  for (const auto &source : lifetime.sources) assert(source.expired());
}
} // namespace

int main() {
  ResolveOwnership();
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
  assert(!inputs.opaque_scene_alpha); // an ordinary copy preserves its source alpha
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
