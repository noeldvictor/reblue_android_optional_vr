// CPU contract checks: native interface doubles, no GPU allocation/submission.
#include "gpu/host_post_output.h"
#include "gpu/native_post_images.h"
#include "gpu/native_image_lease.h"
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

struct ImageLife {
  bool image = false, view = false, framebuffer = false, descriptor = true;
};
struct PoolImage : RenderTexture {
  std::shared_ptr<ImageLife> life;
  explicit PoolImage(std::shared_ptr<ImageLife> value) : life(std::move(value)) { life->image = true; }
  ~PoolImage() override {
    assert(!life->descriptor && !life->framebuffer && !life->view);
    life->image = false;
  }
  std::unique_ptr<RenderTextureView> createTextureView(const RenderTextureViewDesc &) const override { return {}; }
  void setName(const std::string &) override {}
};
struct PoolView : RenderTextureView {
  std::shared_ptr<ImageLife> life;
  explicit PoolView(std::shared_ptr<ImageLife> value) : life(std::move(value)) { life->view = true; }
  ~PoolView() override {
    assert(!life->descriptor && !life->framebuffer && life->image);
    life->view = false;
  }
};
struct PoolFramebuffer : RenderFramebuffer {
  NativePostRecipe recipe;
  std::shared_ptr<ImageLife> life;
  PoolFramebuffer(NativePostRecipe shape, std::shared_ptr<ImageLife> value)
      : recipe(shape), life(std::move(value)) { life->framebuffer = true; }
  ~PoolFramebuffer() override {
    assert(!life->descriptor && life->view && life->image);
    life->framebuffer = false;
  }
  uint32_t getWidth() const override { return recipe.width; }
  uint32_t getHeight() const override { return recipe.height; }
};
void PoolOwnership() {
  NativePostImagePool pool(2048, 2); // two 16x8 mono outputs, or one stereo output
  std::vector<std::shared_ptr<ImageLife>> lives;
  const NativePostRecipe recipe{16, 8, 1};
  const auto acquire = [&](const NativePostRecipe &shape) {
    return pool.Acquire(shape, [&] {
      auto result = std::make_shared<NativePostImage>();
      auto life = std::make_shared<ImageLife>();
      result->recipe = shape;
      result->descriptor = uint32_t(lives.size());
      lives.push_back(life);
      result->image = std::make_unique<PoolImage>(life);
      result->view = std::make_unique<PoolView>(life);
      result->framebuffer = std::make_unique<PoolFramebuffer>(shape, life);
      return result;
    });
  };
  const auto retire = [&](const NativePostImage &image) {
    auto &life = *lives[image.descriptor];
    assert(life.image && life.view && life.framebuffer && life.descriptor);
    life.descriptor = false;
  };
  for (const NativePostRecipe invalid : {
           NativePostRecipe{}, {0, 8, 1}, {16, 0, 1}, {16, 8, 0}, {16, 8, 3},
           {UINT32_MAX, UINT32_MAX, 2}, {17, 8, 2}}) {
    assert(!acquire(invalid));
    assert(lives.empty()); // bad shape/overflow rejected before allocation
  }
  auto first = acquire(recipe);
  assert(first && first->Output() && first->Output().image.descriptor_index == 0);
  auto reader = first; first.reset();
  auto second = acquire(recipe);
  assert(second && second->image != reader->image && lives.size() == 2);
  assert(!acquire(recipe)); // neither live published/read lease can be overwritten
  assert(pool.Stats().bytes == 2048 && pool.Stats().refused == 1);
  pool.MarkUnused(0); pool.AfterFence(0, retire);
  assert(pool.Stats().resident == 2); // even a proven fence does not invalidate readers
  const auto *old = reader->image.get();
  reader->layout = RenderTextureLayout::SHADER_READ;
  reader.reset();
  first = acquire(recipe);
  assert(first->image.get() == old && lives.size() == 2);
  assert(first->layout == RenderTextureLayout::UNKNOWN); // next writer must issue a barrier
  *first->Output().image.layout = RenderTextureLayout::COLOR_WRITE;
  assert(first->layout == RenderTextureLayout::COLOR_WRITE);
  second.reset(); first.reset();
  pool.MarkUnused(1); pool.AfterFence(0, retire);
  assert(pool.Stats().resident == 2 && lives[0]->descriptor && lives[1]->descriptor);
  first = acquire(recipe); // reuse cancels this image's pending retirement
  pool.AfterFence(1, retire);
  assert(pool.Stats().resident == 1 && first->image.get() == old);
  assert(!lives[1]->image && !lives[1]->descriptor);
  int density_identity = 0;
  auto different = recipe;
  different.density_map = reinterpret_cast<RenderTexture *>(&density_identity);
  second = acquire(different);
  assert(second && second->image != first->image && lives.size() == 3);
  first.reset(); second.reset();
  pool.MarkUnused(0); pool.AfterFence(1, retire);
  assert(pool.Stats().bytes == 2048); // pending retirements still consume the budget
  assert(!acquire({16, 8, 2}));
  pool.AfterFence(0, retire);
  assert(pool.Stats().bytes == 0 && pool.Stats().retired == 3);
  first = acquire({16, 8, 2});
  assert(first && first->Output().image.layers == 2 && pool.Stats().bytes == 2048);
  first.reset(); pool.MarkUnused(0); pool.AfterFence(0, retire);
  for (const auto &life : lives)
    assert(!life->descriptor && !life->image && !life->view && !life->framebuffer);
  auto failure = pool.Acquire(recipe, [] { return NativePostImageHandle{}; });
  assert(!failure && pool.Stats().failed == 1 && pool.Stats().bytes == 0);
}
void SharedLayoutAndLease() {
  ImageLayoutRecord local;
  assert(local == RenderTextureLayout::UNKNOWN);
  RenderTextureLayout native = RenderTextureLayout::SHADER_READ;
  local.Bind(native);
  assert(&local.Get() == &native && local == RenderTextureLayout::SHADER_READ);
  local = RenderTextureLayout::COPY_DEST;
  assert(native == RenderTextureLayout::COPY_DEST);
  native = RenderTextureLayout::DEPTH_WRITE;
  assert(local == RenderTextureLayout::DEPTH_WRITE);
  const auto transition = [](RenderTextureLayout &record) { record = RenderTextureLayout::SHADER_READ; };
  transition(local); // existing compatibility transition helpers also update the owner
  assert(native == RenderTextureLayout::SHADER_READ);
  ImageLayoutRecord snapshot = local;
  snapshot = RenderTextureLayout::COLOR_WRITE;
  assert(local == RenderTextureLayout::SHADER_READ); // copy construction never shares a binding
  RenderTextureLayout other_native = RenderTextureLayout::UNKNOWN;
  ImageLayoutRecord other;
  other.Bind(other_native);
  other = local; // assignment copies the value into the destination's own record
  assert(other_native == native && &other.Get() == &other_native);
  other = RenderTextureLayout::COPY_SOURCE;
  assert(native == RenderTextureLayout::SHADER_READ);
  local.Unbind();
  native = RenderTextureLayout::DEPTH_WRITE;
  assert(local == RenderTextureLayout::SHADER_READ && &local.Get() != &native);

  int identity = 0;
  auto owner = std::make_shared<RenderTextureLayout>(RenderTextureLayout::SHADER_READ);
  const std::weak_ptr<const void> weak = owner;
  NativeImageLease lease{owner, {reinterpret_cast<RenderTexture *>(&identity), owner.get(),
      16, 8, 2, RenderFormat::D32_FLOAT_S8_UINT, 0, 1}};
  assert(lease && lease.Fits(16, 8, 2));
  assert(!lease.Fits(16, 8, 1) && !lease.Fits(8, 8, 2) && !lease.Fits(16, 0, 2));
  local.Bind(*lease.image.layout);
  owner.reset();
  assert(!weak.expired()); // type-erased ownership retains the native layout/image lifetime
  local = RenderTextureLayout::COPY_SOURCE;
  assert(*lease.image.layout == RenderTextureLayout::COPY_SOURCE);
  auto second_reader = lease;
  lease = {};
  assert(!weak.expired());
  local.Unbind(); // do this before releasing the final owner
  second_reader = {};
  assert(weak.expired() && local == RenderTextureLayout::COPY_SOURCE);
  assert(!lease && !lease.Fits(16, 8, 2));
  owner = std::make_shared<RenderTextureLayout>();
  lease = {owner, {reinterpret_cast<RenderTexture *>(&identity), owner.get(),
      16, 8, 1, RenderFormat::R16G16B16A16_FLOAT, 4, 1}};
  assert(lease.Fits(16, 8, 1));
  auto invalid = lease; invalid.owner.reset(); assert(!invalid.Fits(16, 8, 1));
  invalid = lease; invalid.image.layout = nullptr; assert(!invalid);
  invalid = lease; invalid.image.samples = 4; assert(!invalid);
  invalid = lease; invalid.image.descriptor_index = ~uint32_t{0}; assert(!invalid);
}
} // namespace
int main() { OutputContract(); PoolOwnership(); SharedLayoutAndLease(); }
