// CPU contract checks: native interface doubles, no GPU allocation/submission.
#include "gpu/native_target_images.h"
#include "gpu/host_post_output.h"
#include "gpu/native_post_images.h"
#include "gpu/native_image_lease.h"
#include "gpu/post_sequence.h"
#include "gpu/scene/native_scene_framebuffer.h"
#include "gpu/scene/native_scene_commands.h"
#include "gpu/scene/native_scene_snapshot.h"
#include "gpu/scene/native_rigid_shadow.h"
#include "gpu/scene/native_rigid_scene.h"
#include "gpu/scene/native_rigid_batch.h"
#include "gpu/scene/native_rigid_route.h"
#include "gpu/scene/native_rigid_lifecycle.h"
#include "gpu/scene/native_shadow_receiver_bridge.h"
#include <array>
#include <limits>
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include "refraction_material_cases.h"
#include "water_update_cases.h"
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
  assert(lease.CanPublishExtent(16, 8, 2));
  assert(!lease.CanPublishExtent(1280, 720, 1)); // strict by default
  assert(lease.CanPublishExtent(1280, 720, 1, NativeImageExtentPolicy::AdoptSource));
  assert(!lease.CanPublishExtent(16, 8, 2, NativeImageExtentPolicy(99)));
  for (auto extent : {NativeImageExtentPolicy::MatchDestination, NativeImageExtentPolicy::AdoptSource}) {
    assert(!lease.CanPublishExtent(0, 8, 2, extent));
    assert(!lease.CanPublishExtent(16, 0, 2, extent));
    assert(!lease.CanPublishExtent(16, 8, 0, extent));
    assert(!lease.CanPublishExtent(16, 8, 3, extent));
    assert(!NativeImageLease{}.CanPublishExtent(16, 8, 2, extent));
    auto invalid = lease;
    invalid.image.samples = 4;
    assert(!invalid.CanPublishExtent(16, 8, 2, extent));
    invalid = lease;
    invalid.owner.reset();
    assert(!invalid.CanPublishExtent(16, 8, 2, extent));
  }
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
void NativeTargetOwnership() {
  NativeTargetImageStore store(4096, 2);
  const NativeTargetShape shape{16, 8, 1, RenderFormat::D32_FLOAT_S8_UINT};
  std::vector<std::shared_ptr<ImageLife>> lives;
  const auto acquire = [&](uint64_t id, const NativeTargetShape &recipe) {
    return store.Acquire(id, recipe, [&] {
      auto result = std::make_shared<NativeTargetImage>();
      result->shape = recipe;
      result->descriptor = uint32_t(lives.size());
      auto life = std::make_shared<ImageLife>();
      lives.push_back(life);
      result->image = std::make_unique<PoolImage>(life);
      result->view = std::make_unique<PoolView>(life);
      return result;
    });
  };
  const auto retire = [&](const NativeTargetImage &owned) {
    auto &life = *lives[owned.descriptor];
    assert(life.image && life.view && life.descriptor);
    life.descriptor = false;
  };
  for (const NativeTargetShape invalid : {NativeTargetShape{},
       {16, 8, 3, shape.format}, {UINT32_MAX, UINT32_MAX, 2, shape.format},
       {16, 8, 1, shape.format, 0}, {16, 8, 1, shape.format, 3},
       {16, 8, 1, shape.format, 16}, {16, 8, 1, shape.format, UINT32_MAX},
       {16, 8, 1, RenderFormat::R8G8B8A8_UNORM}}) {
    assert(!acquire(1, invalid) && lives.empty()); // reject before any GPU allocation
  }
  assert(!acquire(0, shape));
  auto first = acquire(1, shape);
  assert(first && first->Sampled() && lives.size() == 1);
  auto *physical = first->image.get();
  auto *sampling = first->view.get();
  assert(first->layout == RenderTextureLayout::UNKNOWN);
  ImageLayoutRecord adapter;
  adapter.Bind(first->layout);
  NativeImageLease getter{first, first->Sampled()};
  adapter = RenderTextureLayout::SHADER_READ;
  assert(*getter.image.layout == RenderTextureLayout::SHADER_READ);
  first.reset();
  auto reused = acquire(1, shape);
  assert(reused && reused->image.get() == physical && store.Stats().reused == 1);
  assert(reused->view.get() == sampling && lives.size() == 1);
  assert(reused->layout == RenderTextureLayout::SHADER_READ); // acquisition is not a GPU barrier
  assert(!acquire(1, {8, 16, 1, shape.format})); // same bytes cannot hide a shape mismatch
  assert(!acquire(1, {8, 8, 1, shape.format, 2})); // sample count is part of identity
  reused.reset(); adapter.Unbind();
  auto next = acquire(2, shape);
  assert(next && next->image.get() != physical && getter.image.texture == physical);
  assert(store.Stats().bytes == 2048);
  assert(!acquire(3, shape) && lives.size() == 2); // entry cap, despite ample bytes
  next.reset(); store.MarkUnused(1); store.AfterFence(0, retire);
  assert(lives[0]->image && lives[1]->image && store.Stats().bytes == 2048);
  store.AfterFence(1, retire);
  assert(lives[0]->image && !lives[1]->image && getter);
  auto msaa = shape; msaa.samples = 4;
  assert(!acquire(3, msaa) && lives.size() == 2); // byte cap with a free entry
  getter = {};
  store.MarkUnused(0); store.AfterFence(1, retire);
  assert(lives[0]->image && !acquire(3, msaa)); // pending retirement still counts
  store.AfterFence(0, retire);
  assert(!lives[0]->image && store.Stats().bytes == 0);
  uint64_t id = 3;
  for (uint32_t layers : {1u, 2u}) for (uint32_t samples : {1u, 2u, 4u, 8u}) {
    const NativeTargetShape recipe{8, 4, layers, RenderFormat::R16G16B16A16_FLOAT, samples};
    auto target = acquire(id++, recipe);
    assert(target && target->Sampled().samples == samples && target->Sampled().layers == layers);
    assert(store.Stats().bytes == uint64_t(8 * 4 * 8 * layers * samples));
    NativeImageLease sampled{target, target->Sampled()};
    assert(bool(sampled) == (samples == 1)); // MSAA is never an ordinary sampled-image lease
    sampled = {}; target.reset(); store.MarkUnused(0); store.AfterFence(0, retire);
  }
  assert(!store.Acquire(id, shape, [] { return NativeTargetImageHandle{}; }));
  assert(store.Stats().failed == 1 && store.Stats().bytes == 0);
  for (const auto &life : lives) assert(!life->descriptor && !life->image && !life->view);
}
struct SceneSource : RenderTexture {
  std::unique_ptr<RenderTextureView> createTextureView(const RenderTextureViewDesc &) const override { return {}; }
  void setName(const std::string &) override {}
};
struct SceneFramebuffer : OutputFramebuffer {
  std::array<std::weak_ptr<const NativeTargetImage>, 2> sources;
  std::array<bool, 2> present;
  explicit SceneFramebuffer(const std::array<NativeTargetImageHandle, 2> &images)
      : sources{images[0], images[1]}, present{bool(images[0]), bool(images[1])} {}
  ~SceneFramebuffer() override {
    for (uint32_t i = 0; i < 2; ++i) if (present[i]) assert(!sources[i].expired());
  }
};
void SceneFramebufferOwnership() {
  using namespace bd::gpu::scene;
  assert(!NativeSceneFramebuffer{}.Matches(nullptr, nullptr));
  const auto source = [](bool depth, uint32_t layers) {
    auto owner = std::make_shared<NativeTargetImage>();
    owner->shape = {16, 8, layers, depth ? RenderFormat::D32_FLOAT_S8_UINT : RenderFormat::R16G16B16A16_FLOAT, 1};
    owner->image = std::make_unique<SceneSource>();
    owner->descriptor = depth ? 8 : 7;
    return owner;
  };
  for (const auto layers : {1u, 2u}) {
    NativeSceneFramebufferStore store(2);
    std::array<NativeTargetImageHandle, 2> sources{source(false, layers), source(true, layers)};
    std::array<std::weak_ptr<const NativeTargetImage>, 2> weak{sources[0], sources[1]};
    SceneSource density;
    uint32_t created = 0;
    const auto acquire = [&](const std::array<NativeTargetImageHandle, 2> &pair,
                             const RenderTexture *map = nullptr) {
      return store.Acquire(pair, map, [&](const RenderFramebufferDesc &desc) {
        ++created;
        assert(desc.colorAttachmentsCount == 1 && desc.colorAttachments[0] == pair[0]->image.get());
        assert(desc.depthAttachment == pair[1]->image.get() && !desc.depthAttachmentReadOnly);
        assert(desc.viewMask == (layers == 2 ? 3u : 0u));
        assert(desc.fragmentDensityMap == map);
        assert(!desc.colorAttachmentViews && !desc.depthAttachmentView && !desc.fragmentDensityMapView);
        return std::make_unique<SceneFramebuffer>(pair);
      });
    };
    assert(!acquire({}) && !acquire({sources[0], {}}) && !acquire({sources[0], sources[0]}));
    for (uint32_t field = 0; field < 7; ++field) {
      auto invalid = source(true, layers);
      if (field == 0) ++invalid->shape.width;
      if (field == 1) ++invalid->shape.height;
      if (field == 2) invalid->shape.layers = layers == 1 ? 2 : 1;
      if (field == 3) invalid->shape.samples = 4;
      if (field == 4) invalid->shape.format = RenderFormat::R16G16B16A16_FLOAT;
      if (field == 5) invalid->descriptor = ~uint32_t{0};
      if (field == 6) invalid->image.reset();
      assert(!acquire({sources[0], invalid}));
    }
    auto multisampled = source(false, layers); multisampled->shape.samples = 4;
    assert(!acquire({multisampled, sources[1]}) && created == 0);
    multisampled.reset();
    auto first = acquire(sources);
    assert(first && created == 1 && first->Matches(sources[0]->image.get(), sources[1]->image.get()));
    assert(!first->Matches(sources[1]->image.get(), sources[0]->image.get()));
    sources[0]->layout = RenderTextureLayout::SHADER_READ;
    auto reused = acquire(sources);
    assert(reused == first && created == 1 && sources[0]->layout == RenderTextureLayout::SHADER_READ);
    auto foveated = acquire(sources, &density);
    assert(foveated && foveated != first && created == 2); // map identity is part of the recipe
    std::array<NativeTargetImageHandle, 2> recreated{source(false, layers), source(true, layers)};
    assert(!acquire(recreated) && created == 2); // bounded even with retained/pending owners
    foveated.reset(); store.MarkUnused(0); store.AfterFence(1);
    assert(store.Stats().resident == 2 && !acquire(recreated));
    foveated = acquire(sources, &density); // reacquisition cancels retirement
    store.AfterFence(0);
    assert(store.Stats().retired == 0 && created == 2);
    foveated.reset(); store.MarkUnused(1); store.AfterFence(1);
    assert(store.Stats().retired == 1);
    auto next = acquire(recreated);
    assert(next && next != first && created == 3); // same extent is never image identity
    first.reset(); reused.reset(); sources = {};
    store.MarkUnused(0); store.AfterFence(1);
    for (const auto &owner : weak) assert(!owner.expired());
    store.AfterFence(0);
    for (const auto &owner : weak) assert(owner.expired());
    next.reset(); recreated = {}; store.MarkUnused(1); store.AfterFence(1);
    assert(!store.Stats().resident && !store.Stats().bytes && store.Stats().retired == 3);
    assert(store.Stats().refused == 2);
  }
  std::array<NativeTargetImageHandle, 2> sources{source(false, 1), source(true, 1)};
  NativeSceneFramebufferStore failure(1);
  assert(!failure.Acquire(sources, nullptr, [](const RenderFramebufferDesc &) {
    return std::unique_ptr<RenderFramebuffer>{};
  }));
  assert(failure.Stats().failed == 1 && failure.Stats().bytes == 0 && failure.Stats().resident == 0);
}
struct SceneCommandRecorder {
  std::vector<char> events;
  std::vector<RenderTextureBarrier> writes;
  RenderFramebuffer *bound = nullptr;
  RenderColor color;
  float depth = 0;
  uint8_t stencil = 0;
  void barriers(RenderBarrierStages stage, const RenderTextureBarrier *values, uint32_t count) {
    assert(stage == RenderBarrierStage::GRAPHICS && count > 0 && count <= 4);
    writes.assign(values, values + count);
    events.push_back('b');
  }
  void discardTexture(RenderTexture *image) {
    assert(image && !events.empty() && (events.back() == 'b' || events.back() == 'd'));
    events.push_back('d');
  }
  void setFramebuffer(RenderFramebuffer *fb) { assert(fb); bound = fb; events.push_back('f'); }
  void clearColor(uint32_t index, const RenderColor &value) {
    assert(bound && index == 0); color = value; events.push_back('c');
  }
  void clearDepthStencil(bool clear_depth, bool clear_stencil, float z, uint8_t s) {
    assert(bound && clear_depth && clear_stencil); depth = z; stencil = s; events.push_back('z');
  }
};
struct SnapshotRecorder {
  std::vector<char> events;
  RenderTexture *source = nullptr, *destination = nullptr;
  std::vector<RenderTextureBarrier> barriers_seen;
  void setFramebuffer(RenderFramebuffer *fb) { assert(!fb); events.push_back('e'); }
  void barriers(RenderBarrierStages stage, const RenderTextureBarrier *values, uint32_t count) {
    assert((stage == RenderBarrierStage::COPY && count == 2) ||
           (stage == RenderBarrierStage::GRAPHICS && count == 1));
    barriers_seen.insert(barriers_seen.end(), values, values + count);
    events.push_back('b');
  }
  void copyTexture(RenderTexture *dst, RenderTexture *src) {
    assert((events == std::vector<char>{'e','b'}));
    source = src; destination = dst; events.push_back('c');
  }
};
void DepthOnlyCommands() {
  using namespace bd::gpu::scene;
  for (uint32_t layers : {1u, 2u}) {
    NativeSceneFramebufferStore store(2);
    auto source = std::make_shared<NativeTargetImage>();
    source->shape = {1440, 1584, layers, RenderFormat::D32_FLOAT_S8_UINT, 1};
    source->image = std::make_unique<SceneSource>();
    source->descriptor = 12;
    std::array<NativeTargetImageHandle, 2> pair{NativeTargetImageHandle{}, source};
    uint32_t created = 0;
    const auto create = [&](const RenderFramebufferDesc &desc) {
      ++created;
      assert(!desc.colorAttachmentsCount && !desc.colorAttachments && !desc.fragmentDensityMap);
      assert(desc.depthAttachment == source->image.get() && desc.viewMask == (layers == 2 ? 3u : 0u));
      return std::make_unique<SceneFramebuffer>(pair);
    };
    SceneSource density;
    assert(!store.Acquire(pair, &density, create) && !created);
    for (uint32_t field = 0; field < 5; ++field) {
      const auto shape = source->shape;
      if (field == 0) source->shape.samples = 4;
      if (field == 1) source->shape.layers = 3;
      if (field == 2) source->shape.format = RenderFormat::R16G16B16A16_FLOAT;
      if (field == 3) source->shape.width = 0;
      if (field == 4) source->shape.width = UINT32_MAX;
      OutputFramebuffer fb;
      assert(!store.Acquire(pair, nullptr, create));
      assert(!NativeSceneCommands::CreateDepthOnly(source, &fb));
      source->shape = shape;
    }
    auto framebuffer = store.Acquire(pair, nullptr, create);
    assert(framebuffer && created == 1 && framebuffer->Matches(nullptr, source->image.get()));
    assert(!framebuffer->Matches(source->image.get(), nullptr));
    assert(store.Acquire(pair, nullptr, create) == framebuffer && created == 1);
    auto *fb = framebuffer->framebuffer.get();
    assert(!NativeSceneCommands::CreateDepthOnly({}, fb));
    assert(!NativeSceneCommands::CreateDepthOnly(source, nullptr));
    for (float bad : {-1.f, 2.f, std::numeric_limits<float>::quiet_NaN()})
      assert(!NativeSceneCommands::CreateDepthOnly(source, fb, bad));
    auto scope = NativeSceneCommands::CreateDepthOnly(source, fb);
    assert(scope && scope->ClearPending() && !scope->ColorReadImage());
    assert(scope->Matches(nullptr, source->image.get()) && !scope->Matches(source->image.get(), nullptr));
    SceneCommandRecorder recorder;
    assert(scope->Bind(recorder) == 1 && scope->ApplyClear(recorder));
    assert((recorder.events == std::vector<char>{'b', 'd', 'f', 'z'}));
    assert(recorder.depth == 1.f && !recorder.stencil && !scope->ClearPending());
    assert(recorder.writes[0].texture == source->image.get());
    assert(recorder.writes[0].layout == RenderTextureLayout::DEPTH_WRITE);
    recorder.events.clear();
    assert(scope->Bind(recorder) == 0 && !scope->ApplyClear(recorder));
    assert((recorder.events == std::vector<char>{'f'}));
    SnapshotRecorder snapshot;
    assert(!CopySceneSnapshot(snapshot, *scope, source->Sampled()) && snapshot.events.empty());
    // End-of-pass publication borrows the actual native image/layout. Next
    // scope clears the reused image, while resize/retirement retain old readers.
    NativeImageLease receipt{source, source->Sampled()};
    ImageLayoutRecord getter;
    getter.Bind(source->layout);
    source->layout = RenderTextureLayout::SHADER_READ;
    auto next = NativeSceneCommands::CreateDepthOnly(source, fb);
    recorder.events.clear();
    assert(next->Bind(recorder) == 1 && next->ApplyClear(recorder));
    assert((recorder.events == std::vector<char>{'b', 'f', 'z'}));
    assert(*receipt.image.layout == getter.Get() && getter.Get() == RenderTextureLayout::DEPTH_WRITE);
    std::weak_ptr<const NativeTargetImage> weak = source;
    getter.Unbind(); source.reset(); pair = {}; scope.reset(); next.reset(); framebuffer.reset();
    store.MarkUnused(0); store.AfterFence(1);
    assert(store.Stats().resident == 1 && !weak.expired());
    store.AfterFence(0);
    assert(!store.Stats().resident && !weak.expired() && receipt);
    receipt = {};
    assert(weak.expired());
  }
}
void CameraAndRigidCaster() {
  using namespace bd::gpu::scene;
  const RenderMatrix identity{1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1};
  RenderTransformInputs inputs{identity, identity, identity};
  inputs.view[12] = 7; inputs.projection[0] = 2;
  RenderCameraState state;
  state.Publish(inputs, false, false, false); assert(!state.Read());
  state.Publish(inputs, true, false, false); assert(!state.Read());
  state.Publish(inputs, false, true, false);
  assert(state.Read() && state.Read()->world_to_clip[12] == 14);
  const auto retained_camera = *state.Read();
  inputs.world[0] = std::numeric_limits<float>::quiet_NaN();
  state.Publish(inputs, false, false, false); assert(state.Read()); // world isn't a camera input
  inputs.view[12] = 9;
  state.Publish(inputs, false, false, false); assert(!state.Read()); // unseen late writer
  state.Publish(inputs, true, false, false); assert(state.Read());
  state.Publish(inputs, true, true, true); assert(!state.Read());
  state.Publish(inputs, true, false, false); assert(!state.Read());
  state.Publish(inputs, false, true, false); assert(state.Read());
  inputs.projection[0] = std::numeric_limits<float>::infinity();
  state.Publish(inputs, false, true, false); assert(!state.Read());
  inputs.projection = identity;
  state.Publish(inputs, false, true, false); assert(!state.Read()); // invalidation lost both
  state.Publish(inputs, true, false, false); assert(state.Read());
  inputs.view[0] = inputs.projection[0] = (std::numeric_limits<float>::max)();
  state.Publish(inputs, true, true, false); assert(!state.Read()); // derived overflow
  inputs = {identity, identity, identity};
  NativeSceneCommands pass;
  pass.PublishCamera(inputs, true, true, false, 10, 1);
  assert(pass.Camera(10, 1) && !pass.Camera(11, 1) && !pass.Camera(10, 3));
  pass.PublishCamera(inputs, false, false, false, 11, 1); assert(!pass.Camera(11, 1));
  pass.PublishCamera(inputs, true, true, false, 11, 1);
  pass.PublishCamera(inputs, true, false, false, 11, 3); assert(!pass.Camera(11, 3));
  pass.PublishCamera(inputs, false, true, false, 11, 3); assert(pass.Camera(11, 3));
  NativeSceneCommands nested;
  nested.PublishCamera(inputs, false, false, false, 11, 3); assert(!nested.Camera(11, 3));
  assert(pass.Camera(11, 3));
  pass.InvalidateCamera(); assert(!pass.Camera(11, 3));

  auto geometry = std::make_shared<NativeGeometry>();
  geometry->id = 0x258694267A8DBAEEull; geometry->canonical_vertices = true;
  geometry->stream_mask = 1; geometry->count = 474; geometry->strides[0] = 96;
  int buffer_token = 0;
  geometry->streams[0].buffer.ref = reinterpret_cast<RenderBuffer *>(&buffer_token);
  geometry->index.buffer.ref = geometry->streams[0].buffer.ref;
  NativeVertexInputLibrary library;
  RenderInputElement element{}; element.semanticName = "POSITION";
  element.format = RenderFormat::R32G32B32A32_FLOAT;
  geometry->rigid_vertex_input = library.Resolve(std::span(&element, 1), 1, {});
  auto material = std::make_shared<NativeMaterial>(); material->id = 0x63B8D67932573E51ull;
  NativeModelMaterialProgram program;
  program.valid = true; program.geometries = {geometry}; program.materials = {material};
  program.ranges.resize(1); program.ranges[0].shader.vertex_bones = 0;
  PrimitivePolicyInputs policy; policy.phase = 1; policy.pass_cull = PrimitiveCull::Back;
  assert(SelectedNativeRigidShadow(program));
  const auto build = [&] { return PrepareNativeRigidShadow(program, identity, policy, retained_camera); };
  auto caster = build();
  assert(caster && caster->draw && caster->cull == PrimitiveCull::Back);
  assert(caster->pass.world_to_shadow.rows[3].x == 14 && caster->object.flags.x == 0);
  program.ranges.push_back(program.ranges[0]); assert(!build()); program.ranges.pop_back();
  program.ranges[0].shader.vertex_bones.reset(); assert(!build());
  program.ranges[0].shader.vertex_bones = 2; assert(!build()); program.ranges[0].shader.vertex_bones = 0;
  program.ranges[0].skin = NativeSkinBinding{}; assert(!build()); program.ranges[0].skin.reset();
  program.policy_steps = {{PrimitivePolicyOperation::Alpha, 1, 0}};
  program.ranges[0].policy_step_end = 1; assert(!build()); // sorted/alpha isn't a solid caster
  program.policy_steps = {{PrimitivePolicyOperation::Texture, 1, 0}};
  policy.texture_effects = true; assert(!build()); // no invented texture routing
  policy.texture_effects = false; assert(build());
  auto bad_world = identity; bad_world[15] = 0;
  assert(!PrepareNativeRigidShadow(program, bad_world, policy, retained_camera));
  auto bad_camera = retained_camera; bad_camera.world_to_clip[0] = std::numeric_limits<float>::quiet_NaN();
  assert(!PrepareNativeRigidShadow(program, identity, policy, bad_camera));
  program = {}; material.reset(); geometry.reset();
  assert(caster->geometry && caster->geometry->count == 474); // source/model lifetime independent
}
void RigidHardOffRouting() {
  using namespace bd::gpu::scene;
  const RenderMatrix identity{1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1};
  const auto mesh = [](uint32_t key, bool selected, bool bounds = true) {
    ModelMaterialImport result; result.source_mesh = key;
    auto &p = result.program; p.valid = true; p.ranges.resize(1);
    p.materials.resize(1); p.shadow_policies.resize(1); result.source_bindings.resize(1);
    auto geometry = std::make_shared<NativeGeometry>();
    geometry->id = selected ? 0x258694267A8DBAEEull : 42;
    p.geometries.push_back(std::move(geometry));
    if (bounds) p.bounds = std::array<float,4>{0,0,0,1};
    return result;
  };
  ModelMaterialRegistry models;
  const ModelNodeSourceBinding nodes[]{{0,100}, {1,200}};
  assert(models.Publish(10, {mesh(100,true),mesh(200,false)}, nodes));
  auto model = models.FindModel(10);
  NativeInstancePose pose{1,model->Generation(),model,{identity,identity}};
  const auto route = [&](uint32_t node, uint32_t view) {
    return PrepareNativeRigidRoute(model,&pose,node,view).route;
  };
  assert(route(0,1) == NativeRigidRoute::Shadow && route(0,3) == NativeRigidRoute::Scene);
  assert(route(1,3) == NativeRigidRoute::Legacy);
  // Selection exists before the first pose handoff, not after an old draw.
  assert(PrepareNativeRigidRoute(model,nullptr,0,3).route == NativeRigidRoute::Refused);
  assert(PrepareNativeRigidRoute(model,nullptr,1,3).route == NativeRigidRoute::Legacy);
  assert(PrepareNativeRigidRoute({},&pose,0,3).route == NativeRigidRoute::Refused);
  for (uint32_t view : {0u,2u,4u,~0u}) assert(route(0,view) == NativeRigidRoute::Refused);
  assert(route(2,3) == NativeRigidRoute::Refused);
  pose.transforms.clear(); assert(route(0,3) == NativeRigidRoute::Refused);
  pose.transforms = {identity,identity};
  pose.instance = 0; assert(route(0,3) == NativeRigidRoute::Refused); pose.instance = 1;
  ++pose.model_generation; assert(route(0,3) == NativeRigidRoute::Refused); --pose.model_generation;
  assert(!NativeRigidLegacyAllowed(nullptr));
  assert(!NativeRigidLegacyAllowed(models.Find(10,100).get()));
  assert(NativeRigidLegacyAllowed(models.Find(10,200).get()));
  auto invalid = mesh(300,false); invalid.program.valid = false;
  assert(!NativeRigidLegacyAllowed(&invalid));
  invalid = mesh(300,false); invalid.program.geometries[0].reset();
  assert(!NativeRigidLegacyAllowed(&invalid)); // Missing native geometry must not hide selection.
  auto unknown_id = std::make_shared<NativeGeometry>();
  invalid.program.geometries[0] = unknown_id; assert(!NativeRigidLegacyAllowed(&invalid));
  invalid.program.geometries.clear(); assert(!NativeRigidLegacyAllowed(&invalid));
  const ModelNodeSourceBinding incomplete_node[]{{0,300}};
  invalid = mesh(300,true); invalid.program.geometries[0].reset();
  assert(models.Publish(20,{invalid},incomplete_node));
  assert(PrepareNativeRigidRoute(models.FindModel(20),nullptr,0,3).route == NativeRigidRoute::Refused);
  models.Retire(20);

  // Teardown and source-address reuse: the old lease is valid for already
  // queued work, but cannot authorize new traversal against the replacement.
  const auto retained = model;
  models.Retire(10);
  assert(!models.FindModel(10) && !models.Find(10,100));
  assert(PrepareNativeRigidRoute(models.FindModel(10),&pose,0,3).route == NativeRigidRoute::Refused);
  assert(models.Publish(10,{mesh(100,true),mesh(200,false)},nodes));
  model = models.FindModel(10);
  assert(model != retained && model->Generation() != retained->Generation());
  assert(route(0,3) == NativeRigidRoute::Refused);
  pose.model_generation = model->Generation(); // Even a forged matching stamp is insufficient.
  assert(route(0,3) == NativeRigidRoute::Refused);
  pose.model = model; assert(route(0,3) == NativeRigidRoute::Scene);
  assert(models.Publish(10,{mesh(100,true,false),mesh(200,false)},nodes));
  model = models.FindModel(10); pose.model = model; pose.model_generation = model->Generation();
  assert(route(0,3) == NativeRigidRoute::Refused); // No source-bounds fallback.
  const ModelNodeSourceBinding ambiguous[]{{0,100},{0,200}};
  assert(models.Publish(10,{mesh(100,true),mesh(200,false)},ambiguous));
  model = models.FindModel(10); pose.model = model; pose.model_generation = model->Generation();
  assert(route(0,3) == NativeRigidRoute::Refused);
  assert(!models.Publish(10,{mesh(100,true)},nodes));
  assert(!models.FindModel(10)); // Failed replacement cannot expose the prior generation.
}
void ReceiverSetupOrder() {
  using namespace bd::gpu::scene;
  struct Adapter {
    std::vector<char> events;
    LightingVector authored{.1f,.2f,.3f,.4f}, owned{};
    bool refuse = false;
    void Reset() { events.push_back('z'); owned = {}; }
    void Preflight() { events.push_back('p'); if (refuse) throw 1; }
    void BindCompatibilityImage() { events.push_back('b'); }
    void FlushCompatibilityParameters() { events.push_back('f'); authored[0] = .75f; }
    LightingVector ReadColour() { events.push_back('r'); return authored; }
    void PublishCompatibilityColour(const LightingVector &value) {
      events.push_back('c'); assert(value[0] == .75f); authored[0] = .125f;
    }
    void PublishNative(const LightingVector &value) { events.push_back('n'); owned = value; }
  } adapter;
  RunNativeReceiverSetup(false,adapter);
  assert(adapter.events == std::vector<char>{'z'});
  adapter.events.clear(); adapter.refuse = true;
  try { RunNativeReceiverSetup(true,adapter); assert(false); } catch (int) {}
  assert((adapter.events == std::vector<char>{'z','p'}));
  adapter.events.clear(); adapter.refuse = false;
  RunNativeReceiverSetup(true,adapter);
  assert((adapter.events == std::vector<char>{'z','p','b','f','r','c','n'}));
  assert(adapter.owned[0] == .75f && adapter.authored[0] == .125f);
  for (uint32_t phase=0;phase<16;++phase) for (uint32_t technique=0;technique<16;++technique)
    assert(ImportReceiverParticipation(technique,phase) ==
        (technique != 14 && (phase == 0 || phase == 3 || phase == 5 || phase == 6)));
  assert(!ImportReceiverParticipation(0,~0u));
}
void RigidScenePacket() {
  using namespace bd::gpu::scene;
  const RenderMatrix identity{1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1};
  const LightingVector colour{.1f,.2f,.3f,.5f};
  {
  NativeReceiverPublication publication;
  NativePrimaryReceiver receiver{std::make_shared<NativeTargetImage>(),identity,colour};
  assert(!publication.Read(12,4,3));
  publication.Publish(receiver,12,4,3);
  auto retained_receiver = publication.Read(12,4,3);
  assert(retained_receiver && !publication.Read(13,4,3) && !publication.Read(12,5,3) && !publication.Read(12,4,1));
  receiver.colour[0] = .2f; receiver.world_to_shadow[12] = 7;
  publication.Publish(receiver,12,4,3);
  assert(publication.Read(12,4,3)->colour[0] == .2f && retained_receiver->colour[0] == .1f);
  assert(publication.Read(12,4,3)->world_to_shadow[12] == 7 && retained_receiver->world_to_shadow[12] == 0);
  receiver.colour[0] = std::numeric_limits<float>::quiet_NaN();
  publication.Publish(receiver,12,4,3);
  assert(!publication.Read(12,4,3));
  receiver.colour = colour; receiver.world_to_shadow[0] = std::numeric_limits<float>::infinity();
  publication.Publish(receiver,12,4,3);
  assert(!publication.Read(12,4,3));
  receiver.world_to_shadow = identity;
  publication.Publish(receiver,12,4,3); publication.Reset(); assert(!publication.Read(12,4,3));
  publication.Publish(receiver,0,4,3); assert(!publication.Read(0,4,3));
  publication.Publish(receiver,12,4,16); assert(!publication.Read(12,4,16));
  receiver.image.reset(); publication.Publish(receiver,12,4,3); assert(!publication.Read(12,4,3));
  auto lifetime_image = std::make_shared<NativeTargetImage>();
  const std::weak_ptr<NativeTargetImage> weak_image = lifetime_image;
  publication.Publish({lifetime_image,identity,colour},12,4,3);
  auto retained = publication.Read(12,4,3);
  lifetime_image.reset(); publication.Reset();
  assert(!weak_image.expired() && retained->world_to_shadow == identity);
  retained.reset(); assert(weak_image.expired());
  }

  NativeModelMaterialProgram program; program.valid = true; program.ranges.resize(1);
  auto geometry = std::make_shared<NativeGeometry>();
  geometry->canonical_vertices = true; geometry->stream_mask = 1;
  geometry->count = 474; geometry->strides[0] = 96;
  int buffer_token = 0;
  geometry->streams[0].buffer.ref = reinterpret_cast<RenderBuffer *>(&buffer_token);
  geometry->index.buffer.ref = geometry->streams[0].buffer.ref;
  NativeVertexInputLibrary library;
  RenderInputElement element{}; element.semanticName = "POSITION";
  element.format = RenderFormat::R32G32B32A32_FLOAT;
  geometry->rigid_vertex_input = library.Resolve(std::span(&element,1),1,{});
  auto material = std::make_shared<NativeMaterial>(); material->id = 0x63B8D67932573E51ull;
  program.geometries = {geometry}; program.materials = {material};
  auto albedo = std::make_shared<NativeTextureGpu>();
  albedo->image = std::make_unique<SceneSource>(); albedo->view = std::make_unique<RenderTextureView>();
  albedo->dimension = RenderTextureViewDimension::TEXTURE_2D_ARRAY;
  auto depth = std::make_shared<NativeTargetImage>();
  depth->shape = {1024,1024,1,RenderFormat::D32_FLOAT_S8_UINT,1}; depth->descriptor = 1;
  depth->layout = RenderTextureLayout::SHADER_READ;
  depth->image = std::make_unique<SceneSource>(); depth->view = std::make_unique<RenderTextureView>();
  NativeRigidReceiver receiver{depth,identity,colour,{true,true,true}};
  NativeObjectPrimitive<NativeTextureBinding> packet;
  packet.geometry = geometry; packet.material = material; packet.world = identity;
  packet.shader.vertex_bones = 0; packet.shader.vertex_colour = true;
  packet.features = NativeMaterialFeatures{true,true,false,false,true};
  packet.lights = NativeSelectedLights{};
  // Disabled layers still have a fully defined native payload; do not depend on
  // prior stack contents when BuildRigidPass validates the complete record.
  NativeFogLayers fog{}; for (auto &layer : fog) layer.disabled = true; packet.fog = fog;
  packet.camera = RenderCamera{identity,identity,identity};
  packet.lighting = NativeLightingPass{};
  packet.lighting->inputs.color_scale = {0,0,0,1};
  packet.lighting->inputs.shadow_bias = .001f;
  packet.policy = {PrimitiveCull::Back,true,true,false,false,true};
  packet.material_mask = 3; packet.material_values[0] = {1,.8f,.7f,1}; packet.material_values[1] = {0,0,0,0};
  packet.textures.image_mask = 1; packet.textures.images[0].primary = albedo;
  packet.textures.owns_uv = true; packet.textures.uv = {.25f,-.5f,0,0};
  packet.receiver_shadow = NativeShadowPolicy::Receive;
  packet.samplers[0] = NativeMaterialSampler2D{{},MaterialSampleAddress::Wrap,MaterialSampleAddress::Clamp};
  const auto build = [&] { return PrepareNativeRigidScene(program,packet,receiver); };
  auto plan = build(); assert(plan && plan->draw && plan->cull == PrimitiveCull::Back);
  assert(plan->object.flags.x == 63 && plan->object.uv_scale_offset.x == 1.f/512);
  assert(plan->object.uv_scale_offset.z == .25f+1.f/512 && plan->object.uv_scale_offset.w == -.5f+1.f/512);
  assert(plan->pass.shadow_filter.z == .65f/1024 && plan->shadow == depth && plan->albedo == albedo);
  auto good = packet;
  for (uint32_t fault=0;fault<15;++fault) {
    packet = good;
    if (fault == 0) packet.lights.reset();
    if (fault == 1) packet.fog.reset();
    if (fault == 2) packet.camera.reset();
    if (fault == 3) packet.lighting.reset();
    if (fault == 4) packet.shader.vertex_bones.reset();
    if (fault == 5) packet.shader.vertex_bones = 1;
    if (fault == 6) packet.shader.texture_layers = 2;
    if (fault == 7) packet.policy.deferred = true;
    if (fault == 8) packet.policy.alpha_test = true;
    if (fault == 9) packet.textures.owns_uv = false;
    if (fault == 10) packet.features->reflection = true;
    if (fault == 11) packet.features->normal_mapping = true;
    if (fault == 12) packet.samplers[0].reset();
    if (fault == 13) packet.material_mask = 1;
    if (fault == 14) packet.receiver_shadow = NativeShadowPolicy::Unknown;
    assert(!build());
  }
  packet = good;
  program.ranges.push_back(program.ranges[0]); assert(!build()); program.ranges.pop_back();
  albedo->dimension = RenderTextureViewDimension::TEXTURE_2D; assert(!build());
  albedo->dimension = RenderTextureViewDimension::TEXTURE_2D_ARRAY;
  depth->layout = RenderTextureLayout::DEPTH_WRITE; assert(!build()); depth->layout = RenderTextureLayout::SHADER_READ;
  depth->shape.layers = 2; assert(!build()); depth->shape.layers = 1;
  receiver.visibility.receiver_visible = false;
  assert(!(build()->object.flags.x & RigidReceiveShadow));
  packet.policy.direct = false; assert(!build()->draw);
  const std::weak_ptr<NativeGeometry> retired_geometry = geometry;
  const std::weak_ptr<NativeTextureGpu> retired_albedo = albedo;
  const std::weak_ptr<NativeTargetImage> retired_depth = depth;
  program = {}; packet = {}; good = {}; receiver = {};
  geometry.reset(); material.reset(); albedo.reset(); depth.reset();
  assert(retired_geometry.use_count() == 1 && retired_albedo.use_count() == 1 && retired_depth.use_count() == 1);
  assert(plan->geometry->count == 474 && plan->albedo->view && plan->shadow->image);
  plan.reset();
  assert(retired_geometry.expired() && retired_albedo.expired() && retired_depth.expired());
}
void RigidLifecycle() {
  using namespace bd::gpu::scene;
  using Event = NativeRigidLifecycle::Event;
  NativeRigidLifecycle lifecycle;
  assert(!lifecycle.Loaded(0) && !lifecycle.Find(0));
  assert(lifecycle.Loaded(93) && !lifecycle.Loaded(93));
  assert(!lifecycle.Note(Event::Submitted,94,144,3));
  assert(!lifecycle.Note(Event::Submitted,93,0,3));
  assert(!lifecycle.Note(Event::Submitted,93,144,2));
  assert(!lifecycle.Note(Event::Submitted,93,144,3,2));
  assert(!lifecycle.Note(Event::Emitted,93,0,3));
  assert(!lifecycle.Note(Event::FenceRetired,93,0,3));
  for (auto view : {1u,3u}) for (int i=0;i<2;++i) assert(lifecycle.Note(Event::Submitted,93,144+i,view));
  assert(lifecycle.Find(93)->first_instance == 144);
  assert(lifecycle.SourceRetired(93) && !lifecycle.SourceRetired(93));
  assert(!lifecycle.SourceRetired(94) && !lifecycle.Find(93)->Closed());
  assert(!lifecycle.Note(Event::Submitted,93,144,3));
  assert(lifecycle.Loaded(94)); // New load while old native commands are pending.
  for (auto view : {1u,3u}) {
    assert(lifecycle.Note(Event::Emitted,93,0,view,2));
    assert(!lifecycle.Note(Event::Emitted,93,0,view));
    assert(lifecycle.Note(Event::FenceRetired,93,0,view));
    assert(!lifecycle.Find(93)->Closed());
    assert(lifecycle.Note(Event::FenceRetired,93,0,view));
    assert(!lifecycle.Note(Event::FenceRetired,93,0,view));
  }
  assert(lifecycle.Find(93)->Closed() && !lifecycle.Find(94)->Closed());
  assert(lifecycle.Note(Event::Submitted,94,288,3));
  assert(lifecycle.Note(Event::FenceRetired,94,0,3));
  assert(lifecycle.Find(94)->views[1].emitted == 0); // Retirement is not output.
  assert(lifecycle.SourceRetired(94) && lifecycle.Find(94)->Closed());
  for (uint64_t i=0;i<NativeRigidLifecycle::kCapacity-2;++i) assert(lifecycle.Loaded(100+i));
  assert(!lifecycle.Loaded(999) && !lifecycle.Loaded(93)); // Never evict proof.
  NativeRigidEpoch epoch; epoch.generation = 1;
  NativeRigidOutputWindow window;
  assert(!window.Step(true,epoch,2));
  epoch.views[0].emitted = 2;
  assert(!window.Step(true,epoch,2));
  epoch.views[1].emitted = 2;
  assert(window.Step(true,epoch,2));
  assert(!window.Step(false,epoch,2));
  assert(!window.Step(true,epoch,2)); // Readiness loss discards prior output.
  epoch.views[0].emitted = epoch.views[1].emitted = 4;
  assert(window.Step(true,epoch,2));
  ++epoch.generation;
  assert(!window.Step(true,epoch,2)); // New generation gets its own baseline.
  epoch.views[0].emitted = epoch.views[1].emitted = 6;
  assert(window.Step(true,epoch,2));
  epoch.source_retired = true;
  assert(!window.Step(true,epoch,2));
  window = {}; epoch = {}; epoch.generation = 2;
  assert(!window.Step(true,epoch));
  epoch.views[0].emitted = epoch.views[1].emitted = 899;
  assert(!window.Step(true,epoch));
  epoch.views[0].emitted = epoch.views[1].emitted = 900;
  assert(window.Step(true,epoch));
}
void RigidBatches() {
  using namespace bd::gpu::scene;
  int token = 0;
  NativeRigidBatchItem a;
  a.geometry = std::make_shared<NativeGeometry>();
  a.pipeline = reinterpret_cast<RenderPipeline *>(&token);
  a.layout = reinterpret_cast<RenderPipelineLayout *>(&token);
  a.framebuffer = reinterpret_cast<RenderFramebuffer *>(&token);
  a.viewport = RenderViewport(0,0,32,16); a.scissor = RenderRect(0,0,32,16);
  a.frame = 8; a.slot = 1; a.view = 1;
  a.model_generation = 93; a.instance = 144;
  NativeRigidBatchItem b = a;
  b.instance = 145;
  b.input.object_data.world.rows[3].x = 7;
  b.input.object_data.diffuse = {.2f,.4f,.6f,1};
  b.input.pass_data.lights[0].colour_strength.x = .7f;
  b.input.pass_data.fog[0].colour_opacity.w = .4f;
  const NativeRigidBatchItem *pair[]{&a,&b};
  std::array<NativeRigidInstanceGPU,2> packed{};
  assert(NativeRigidBatchLength(pair,8,1) == 2 && PackNativeRigidBatch(pair,packed,8,1));
  assert(packed[0].object_data.world.rows[3].x == 0 && packed[1].object_data.world.rows[3].x == 7);
  assert(packed[1].object_data.diffuse.z == .6f && packed[1].pass_data.lights[0].colour_strength.x == .7f);
  assert(packed[1].pass_data.fog[0].colour_opacity.w == .4f);
  const auto good = b;
  for (uint32_t fault=0;fault<16;++fault) {
    b = good;
    if (fault == 0) ++b.frame;
    if (fault == 1) ++b.slot;
    if (fault == 2) b.view = 3;
    if (fault == 3) b.geometry = std::make_shared<NativeGeometry>(); // even an identical reloaded asset is a new lease
    if (fault == 4) b.pipeline = nullptr;
    if (fault == 5) b.layout = nullptr;
    if (fault == 6) b.framebuffer = nullptr;
    if (fault == 7) b.viewport.x = 1;
    if (fault == 8) b.viewport.width = 64;
    if (fault == 9) b.viewport.minDepth = .2f;
    if (fault == 10) b.scissor.right = 8;
    if (fault == 11) b.albedo = std::make_shared<NativeTextureGpu>();
    if (fault == 12) b.shadow = std::make_shared<NativeTargetImage>();
    if (fault == 13) b.albedo_sampler = reinterpret_cast<RenderSampler *>(&token);
    if (fault == 14) ++b.model_generation;
    if (fault == 15) b.instance = 0;
    const auto before = packed;
    assert(NativeRigidBatchLength(pair,8,1) == 1 && !PackNativeRigidBatch(pair,packed,8,1));
    assert(std::memcmp(before.data(),packed.data(),sizeof(packed)) == 0);
  }
  b = good;
  const NativeRigidBatchItem *barrier[]{&a,nullptr,&b};
  assert(NativeRigidBatchLength(barrier,8,1) == 1);
  assert(!NativeRigidBatchLength(pair,9,1) && !NativeRigidBatchLength(pair,8,0));
  std::array<const NativeRigidBatchItem *,kNativeRigidBatchLimit+1> many;
  many.fill(&a); assert(NativeRigidBatchLength(many,8,1) == kNativeRigidBatchLimit);
  assert(!PackNativeRigidBatch(pair,std::span(packed).first(1),8,1));
  a.view = b.view = 3;
  assert(!NativeRigidBatchLength(pair,8,1));
  a.albedo = std::make_shared<NativeTextureGpu>(); a.shadow = std::make_shared<NativeTargetImage>();
  a.albedo_sampler = a.shadow_sampler = reinterpret_cast<RenderSampler *>(&token);
  b = a; assert(NativeRigidBatchLength(pair,8,1) == 2);
  b.shadow_sampler = nullptr; assert(NativeRigidBatchLength(pair,8,1) == 1);
  for (const uint32_t alignment : {1u,16u,64u,256u,1024u}) for (uint32_t count : {1u,2u,256u}) {
    const auto placement = PlanNativeRigidStorage(count,alignment); assert(placement);
    for (uint64_t base : {0ull,16ull,1024ull,4193792ull}) {
      const auto offset = placement->Offset(base);
      assert(offset >= base && offset%alignment == 0 && offset%sizeof(NativeRigidInstanceGPU) == 0);
      assert(offset-base+placement->bytes <= placement->reserve);
    }
  }
  assert(!PlanNativeRigidStorage(0,16) && !PlanNativeRigidStorage(257,16));
  assert(!PlanNativeRigidStorage(2,0) && !PlanNativeRigidStorage(2,3) && !PlanNativeRigidStorage(2,65536));
  a = {}; b = {};
  assert(packed[1].object_data.world.rows[3].x == 7); // packed CPU data survives source retirement
}
void SceneCommands() {
  using namespace bd::gpu::scene;
  const NativeSceneClear clear{{.125f, .25f, .5f, 1.f}, .75f, 23};
  for (uint32_t layers : {1u, 2u}) for (uint32_t samples : {1u, 2u, 4u, 8u}) {
    std::array<NativeTargetImageHandle, 2> sources;
    std::array<std::shared_ptr<NativeTargetImage>, 2> resolved_owners;
    std::array<SampledImage, 2> resolved{};
    for (uint32_t i = 0; i < 2; ++i) {
      const auto make = [&](uint32_t sample_count) {
        auto image = std::make_shared<NativeTargetImage>();
        image->shape = {1440, 1584, layers,
            i ? RenderFormat::D32_FLOAT_S8_UINT : RenderFormat::R16G16B16A16_FLOAT, sample_count};
        image->image = std::make_unique<SceneSource>();
        image->descriptor = i;
        return image;
      };
      sources[i] = make(samples);
      if (samples > 1) {
        resolved_owners[i] = make(1);
        resolved[i] = resolved_owners[i]->Sampled();
      }
    }
    OutputFramebuffer framebuffer;
    const auto create = [&](const std::array<SampledImage, 2> &outputs) {
      return NativeSceneCommands::Create(sources, &framebuffer, outputs, clear);
    };
    assert(!NativeSceneCommands::Create({}, &framebuffer, resolved, clear));
    assert(!NativeSceneCommands::Create(sources, nullptr, resolved, clear));
    --framebuffer.width; assert(!create(resolved)); ++framebuffer.width;
    auto invalid_clear = clear; invalid_clear.depth = -1.f;
    assert(!NativeSceneCommands::Create(sources, &framebuffer, resolved, invalid_clear));
    invalid_clear = clear; invalid_clear.color.a = std::numeric_limits<float>::quiet_NaN();
    assert(!NativeSceneCommands::Create(sources, &framebuffer, resolved, invalid_clear));
    if (samples > 1) {
      assert(!create({}));
      for (uint32_t field = 0; field < 9; ++field) {
        auto bad = resolved;
        if (field == 0) bad[0].texture = sources[0]->image.get();
        if (field == 1) bad[1].texture = bad[0].texture;
        if (field == 2) bad[0].layout = &sources[0]->layout;
        if (field == 3) bad[1].layout = bad[0].layout;
        if (field == 4) ++bad[0].width;
        if (field == 5) ++bad[1].height;
        if (field == 6) bad[0].layers = layers == 1 ? 2 : 1;
        if (field == 7) bad[1].format = sources[0]->shape.format;
        if (field == 8) bad[1].samples = samples;
        assert(!create(bad));
      }
    } else assert(!create({sources[0]->Sampled(), sources[1]->Sampled()}));
    auto scope = create(resolved);
    assert(scope && scope->ClearPending());
    assert(scope->Matches(sources[0]->image.get(), sources[1]->image.get()));
    assert(!scope->Matches(sources[1]->image.get(), sources[0]->image.get()));
    SceneCommandRecorder recorder;
    const uint32_t attachment_count = samples > 1 ? 4u : 2u;
    assert(scope->Bind(recorder) == attachment_count && recorder.writes.size() == attachment_count);
    for (uint32_t i = 0; i < attachment_count; ++i) {
      assert(recorder.writes[i].texture == (i < 2 ? sources[i]->image.get() : resolved[i - 2].texture));
      assert(recorder.writes[i].layout == (i % 2 ? RenderTextureLayout::DEPTH_WRITE : RenderTextureLayout::COLOR_WRITE));
    }
    // Zero-draw scenes use exactly these same bind/clear commands before readout.
    assert(scope->ApplyClear(recorder) && !scope->ClearPending());
    assert((recorder.events == std::vector<char>{'b','d','d','f','c','z'}));
    assert(recorder.color.r == .125f && recorder.color.g == .25f && recorder.color.b == .5f);
    assert(recorder.color.a == 1.f && recorder.depth == .75f && recorder.stencil == 23);
    recorder.events.clear();
    assert(scope->Bind(recorder) == 0 && !scope->ApplyClear(recorder));
    assert((recorder.events == std::vector<char>{'f'})); // resumed LOAD, no discard/reclear
    sources[0]->layout = RenderTextureLayout::SHADER_READ;
    recorder.events.clear();
    assert(scope->Bind(recorder) == 1 && !scope->ApplyClear(recorder));
    assert((recorder.events == std::vector<char>{'b','f'}));
    for (const auto &image : sources) image->layout = RenderTextureLayout::SHADER_READ;
    for (const auto &image : resolved) if (image.texture) *image.layout = RenderTextureLayout::SHADER_READ;
    auto next = create(resolved);
    recorder.events.clear();
    assert(next->Bind(recorder) == attachment_count && next->ApplyClear(recorder));
    assert((recorder.events == std::vector<char>{'b','f','c','z'})); // next scene clears persistent images
    // Actual snapshot core, mono/layered and 1/2/4/8 samples. The copy must
    // sample the ordinary resolved colour for MSAA, not its write attachment.
    auto snapshot = std::make_shared<NativeTargetImage>();
    snapshot->shape = {1440, 1584, layers, RenderFormat::R16G16B16A16_FLOAT, 1};
    snapshot->image = std::make_unique<SceneSource>();
    snapshot->descriptor = 20;
    const auto output = snapshot->Sampled();
    const auto input = next->ColorReadImage();
    assert(input && input.texture == (samples > 1 ? resolved[0].texture : sources[0]->image.get()));
    for (uint32_t field = 0; field < 9; ++field) {
      auto bad = output;
      if (field == 0) bad.texture = input.texture;
      if (field == 1) bad.layout = input.layout;
      if (field == 2) ++bad.width;
      if (field == 3) ++bad.height;
      if (field == 4) bad.layers = layers == 1 ? 2 : 1;
      if (field == 5) bad.format = RenderFormat::R8G8B8A8_UNORM;
      if (field == 6) bad.samples = 2;
      if (field == 7) bad.descriptor_index = ~0u;
      if (field == 8) bad.texture = nullptr;
      SnapshotRecorder refused;
      assert(!CopySceneSnapshot(refused, *next, bad) && refused.events.empty());
      assert(snapshot->layout == RenderTextureLayout::UNKNOWN);
    }
    SnapshotRecorder copied;
    assert(CopySceneSnapshot(copied, *next, output));
    assert((copied.events == std::vector<char>{'e','b','c','b'}));
    assert(copied.source == input.texture && copied.destination == output.texture);
    assert(copied.barriers_seen.size() == 3 && copied.barriers_seen[0].texture == input.texture);
    assert(copied.barriers_seen[0].layout == RenderTextureLayout::COPY_SOURCE);
    assert(copied.barriers_seen[1].layout == RenderTextureLayout::COPY_DEST);
    assert(copied.barriers_seen[2].layout == RenderTextureLayout::SHADER_READ);
    assert(*input.layout == RenderTextureLayout::COPY_SOURCE && snapshot->layout == RenderTextureLayout::SHADER_READ);
    recorder.events.clear();
    assert(next->Bind(recorder) == 1 && !next->ApplyClear(recorder));
    assert((recorder.events == std::vector<char>{'b','f'})); // resume writes, preserve scene contents
    assert(snapshot->layout == RenderTextureLayout::SHADER_READ); // snapshot is independent
  }
  for (bool same : {false, true}) for (bool shared : {false, true}) for (bool ready : {false, true}) {
    assert((PlanSceneSnapshot(SceneSnapshotPhase::Scene, same, shared, ready) ==
            SceneSnapshotPlan{true, !((same || shared) && ready), true}));
    assert((PlanSceneSnapshot(SceneSnapshotPhase::Reflection, same, shared, ready) ==
            SceneSnapshotPlan{true, true, false}));
    assert((PlanSceneSnapshot(SceneSnapshotPhase::Inactive, same, shared, ready) == SceneSnapshotPlan{}));
  }
}
} // namespace
int main() {
  OutputContract(); PoolOwnership(); SharedLayoutAndLease(); NativeTargetOwnership();
  SceneFramebufferOwnership();
  DepthOnlyCommands();
  CameraAndRigidCaster();
  RigidHardOffRouting();
  RigidLifecycle();
  RigidScenePacket();
  ReceiverSetupOrder();
  RigidBatches();
  SceneCommands();
  refraction_material_tests::Run();
  water_update_tests::Run();
}
