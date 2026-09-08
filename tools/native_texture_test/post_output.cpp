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
#include "gpu/scene/native_deferred_queue.h"
#include "gpu/scene/native_deferred_effects.h"
#include "gpu/scene/native_rigid_program.h"
#include "gpu/scene/native_rigid_batch.h"
#include "gpu/scene/native_rigid_route.h"
#include "gpu/scene/native_rigid_lifecycle.h"
#include "gpu/scene/native_shadow_receiver_bridge.h"
#include "gpu/scene/blend_import.h"
#include <array>
#include <limits>
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include "refraction_material_cases.h"
#include "water_update_cases.h"
#include "native_occlusion_cases.h"
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
  assert(caster && caster->size() == 1 && caster->at(0).draw && caster->at(0).cull == PrimitiveCull::Back);
  assert(caster->at(0).pass.world_to_shadow.rows[3].x == 14 && caster->at(0).object.flags.x == 0);
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
  // A representative three-range node needs no scene material or texture-layer
  // restriction for depth-only casting. All siblings retain exact GPU ranges.
  geometry->id = 42;
  program.materials.clear();
  program.ranges.resize(3, program.ranges[0]);
  program.ranges[0].shader.texture_layers = 3;
  program.ranges[1].winding = PrimitiveWinding::Reverse;
  program.ranges[2].winding = PrimitiveWinding::TwoSided;
  auto sibling = std::make_shared<NativeGeometry>(*geometry);
  sibling->id = 43; sibling->count = 12; sibling->start_index = 9; sibling->base_vertex = -3;
  program.geometries = {geometry, sibling, geometry};
  const auto classify = [&] { return PrepareNativeRigidCasterAdmission(program, policy).route; };
  assert(classify() == NativeRigidCasterRoute::Native);
  auto family = build();
  assert(family && family->size() == 3 && family->at(1).geometry == sibling);
  assert(family->at(1).geometry->start_index == 9 && family->at(1).geometry->base_vertex == -3);
  assert(family->at(0).cull == PrimitiveCull::Back && family->at(1).cull == PrimitiveCull::Front &&
      family->at(2).cull == PrimitiveCull::None);
  assert(PrepareNativeRigidCasterAdmission(program, {}).route == NativeRigidCasterRoute::Refused);
  for (uint32_t technique : {3u, 8u, 9u, 11u, 14u}) {
    policy.technique = technique; assert(classify() == NativeRigidCasterRoute::Legacy && !build());
  }
  policy.technique = 0;
  policy.texture_effects = true; assert(classify() == NativeRigidCasterRoute::Legacy);
  policy.texture_effects = false;
  policy.pass_mode = 2; assert(classify() == NativeRigidCasterRoute::Legacy);
  policy.pass_mode = 0;
  program.ranges[2].shader.vertex_bones = 1; assert(classify() == NativeRigidCasterRoute::Legacy && !build());
  program.ranges[2].shader.vertex_bones = 0;
  program.ranges[2].skin = NativeSkinBinding{}; assert(classify() == NativeRigidCasterRoute::Legacy);
  program.ranges[2].skin.reset();
  program.policy_steps.push_back({PrimitivePolicyOperation::Alpha, 17, 0});
  program.ranges[2].policy_step_end = 2;
  assert(classify() == NativeRigidCasterRoute::Legacy && !build()); // Do not omit an alpha sibling.
  program.policy_steps.pop_back(); program.ranges[2].policy_step_end = 1;
  sibling->canonical_vertices = false;
  assert(classify() == NativeRigidCasterRoute::Native && !build()); // Missing GPU data cannot select legacy.
  sibling->canonical_vertices = true;
  program.geometries[2].reset(); assert(classify() == NativeRigidCasterRoute::Native && !build());
  program.geometries[2] = geometry;
  program.ranges[2].policy_step_end = 2; assert(classify() == NativeRigidCasterRoute::Refused);
  program.ranges[2].policy_step_end = 1;
  assert(build());
  // Mixed opaque/cutout siblings are prepared atomically. No scene material,
  // light, fog or receiver input is needed by the alpha-only caster.
  program.policy_steps.push_back({PrimitivePolicyOperation::Alpha,17,0});
  program.ranges[2].policy_step_end = 2;
  program.ranges[2].shader.vertex_colour = true;
  assert(PrepareNativeRigidShadowAdmission(program,policy).route == NativeRigidCasterRoute::Native && !build());
  std::vector<NativeRigidShadowCutout> cutouts(3);
  auto &cutout = cutouts[2];
  const auto covered = [&] { return PrepareNativeRigidShadow(program,identity,policy,retained_camera,cutouts); };
  assert(covered() && !covered()->at(2).object.flags.x && !covered()->at(2).albedo);
  auto image = std::make_shared<NativeTextureGpu>();
  image->image = std::make_unique<SceneSource>(); image->view = std::make_unique<RenderTextureView>();
  image->dimension = RenderTextureViewDimension::TEXTURE_2D_ARRAY;
  cutout.textured = true; cutout.owns_uv = true; cutout.uv = {.25f,.5f,0,0}; cutout.image.primary = image;
  cutout.sampler = NativeMaterialSampler2D{{},MaterialSampleAddress::Wrap,MaterialSampleAddress::Wrap};
  assert(!covered()); // direct callback selects shadownull, not textured shadowmap
  policy.shadow_modulates_colour = true;
  assert(classify() == NativeRigidCasterRoute::Legacy); // scene admission stays strict
  assert(PrepareNativeRigidShadowAdmission(program,policy).policies[2].deferred);
  program.ranges[2].shader.vertex_colour.reset(); // not a shadow shader input
  auto textured = covered();
  assert(textured && textured->at(2).draw && textured->at(2).albedo == image &&
      textured->at(2).object.diffuse.w == 0 && !(textured->at(2).object.flags.x & RigidVertexColour) &&
      std::bit_cast<float>(textured->at(2).object.flags.w) == .6f &&
      textured->at(2).object.flags.y == 1 && textured->at(2).object.uv_scale_offset.z == .25f+1.f/512 &&
      !textured->at(0).albedo && textured->at(1).geometry == sibling);
  const auto valid_cutout = cutout;
  for (uint32_t fault = 0; fault < 8; ++fault) {
    cutout = valid_cutout;
    if (fault == 0) cutout.image = {};
    if (fault == 1) cutout.owns_uv = false;
    if (fault == 2) cutout.sampler.reset();
    if (fault == 3) cutout.sampler->u = MaterialSampleAddress::Clamp;
    if (fault == 4) cutout.sampler->v = MaterialSampleAddress::Clamp;
    if (fault == 5) cutout.uv[0] = std::numeric_limits<float>::infinity();
    if (fault == 6) cutout.image.cube = image;
    if (fault == 7) cutout.image.slice_2d = image;
    assert(!covered());
  }
  cutout = valid_cutout;
  image->dimension = RenderTextureViewDimension::TEXTURE_2D; assert(!covered());
  image->dimension = RenderTextureViewDimension::TEXTURE_2D_ARRAY;
  policy.phase = 0; assert(PrepareNativeRigidShadowAdmission(program,policy).route == NativeRigidCasterRoute::Legacy);
  policy.phase = 1; policy.pass_mode = 2;
  assert(PrepareNativeRigidShadowAdmission(program,policy).route == NativeRigidCasterRoute::Legacy);
  policy.pass_mode = 0;
  cutouts.pop_back(); assert(!covered()); // never use a shorter sibling packet
  image.reset();
  assert(textured->at(2).albedo && textured->at(2).object.diffuse.w == 0);
  policy.shadow_modulates_colour = false;
  program.policy_steps.pop_back(); program.ranges[2].policy_step_end = 1;
  program.policy_steps = {{PrimitivePolicyOperation::Alpha,17,0}, {PrimitivePolicyOperation::Alpha,0,0}};
  for (auto &range : program.ranges) range.policy_step_end = 2;
  policy.special_shadow_block = true;
  const auto suppressed_family = build();
  assert(suppressed_family && suppressed_family->size() == 3);
  for (const auto &plan : *suppressed_family) assert(!plan.draw); // A later opaque command does not restore casting.
  program.ranges.resize(4097, program.ranges[0]);
  program.geometries.resize(4097, geometry);
  assert(classify() == NativeRigidCasterRoute::Refused && !build());
  program = {}; material.reset(); geometry.reset();
  assert(caster->at(0).geometry && caster->at(0).geometry->count == 474);
  sibling.reset();
  assert(family->at(1).geometry->count == 12); // Whole-node leases survive source/model retirement.
}
void RigidHardOffRouting() {
  using namespace bd::gpu::scene;
  const RenderMatrix identity{1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1};
  const auto mesh = [](uint32_t key, bool selected, bool bounds = true) {
    ModelMaterialImport result; result.source_mesh = key;
    auto &p = result.program; p.valid = true; p.ranges.resize(1); p.ranges[0].shader.vertex_bones = 0;
    p.ranges[0].shader.vertex_colour = true;
    p.materials.resize(1); p.shadow_policies.resize(1); result.source_bindings.resize(1);
    p.materials[0] = std::make_shared<NativeMaterial>();
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
  assert(route(1,3) == NativeRigidRoute::Refused); // Missing pass policy cannot select legacy.
  PrimitivePolicyInputs scene_policy;
  assert(PrepareNativeRigidRoute(model,&pose,1,3,scene_policy).route == NativeRigidRoute::Scene);
  assert(PrepareNativeRigidRoute(model,nullptr,1,3,scene_policy).route == NativeRigidRoute::Refused);
  assert(!NativeRigidLegacyAllowed(models.Find(10,200).get(),3,scene_policy));
  scene_policy.technique = 3;
  assert(PrepareNativeRigidRoute(model,nullptr,1,3,scene_policy).route == NativeRigidRoute::Legacy);
  assert(NativeRigidLegacyAllowed(models.Find(10,200).get(),3,scene_policy));
  PrimitivePolicyInputs caster_policy; caster_policy.phase = 1;
  assert(PrepareNativeRigidRoute(model,&pose,1,1,caster_policy).route == NativeRigidRoute::Shadow);
  assert(PrepareNativeRigidRoute(model,nullptr,1,1,caster_policy).route == NativeRigidRoute::Refused);
  assert(!NativeRigidLegacyAllowed(models.Find(10,200).get(),1,caster_policy));
  assert(PrepareNativeRigidRoute(model,&pose,1,1).route == NativeRigidRoute::Refused);
  caster_policy.technique = 3;
  assert(PrepareNativeRigidRoute(model,nullptr,1,1,caster_policy).route == NativeRigidRoute::Legacy);
  assert(NativeRigidLegacyAllowed(models.Find(10,200).get(),1,caster_policy));
  // Selection exists before the first pose handoff, not after an old draw.
  assert(PrepareNativeRigidRoute(model,nullptr,0,3).route == NativeRigidRoute::Refused);
  assert(PrepareNativeRigidRoute(model,nullptr,1,3).route == NativeRigidRoute::Refused);
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
void DeferredEffectOwnership() {
  using namespace bd::gpu::scene;
  const RenderMatrix identity{1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1};
  NativeDeferredEffects effects{7, {.alphaBlendEnable = true}, true,
      {std::make_shared<NativeTargetImage>(), identity, {.1f,.2f,.3f,.4f}}};
  NativeRigidScenePlan plan{};
  plan.draw = plan.deferred = true; plan.shadow = effects.receiver.image;
  plan.pass.world_to_shadow = PackRigidMatrix(identity);
  NativeRigidSceneSubmission submission{11, 19, 3, 7, false, {plan,plan}};
  for (uint32_t fault = 0; fault < 14; ++fault) {
    auto bad = submission; auto late = effects;
    if (fault == 0) bad.instance = 0;
    if (fault == 1) bad.model_generation = 0;
    if (fault == 2) ++bad.frame;
    if (fault == 3) ++late.frame;
    if (fault == 4) late.blend.alphaBlendEnable = false;
    if (fault == 5) late.receiver.image.reset();
    if (fault == 6) late.receiver.colour[3] = std::numeric_limits<float>::quiet_NaN();
    if (fault == 7) late.receiver.world_to_shadow[12] = std::numeric_limits<float>::infinity();
    if (fault == 8) bad.plans.back().shadow = std::make_shared<NativeTargetImage>();
    if (fault == 9) bad.plans.back().pass.world_to_shadow = PackRigidMatrix(RenderMatrix{});
    if (fault == 10) bad.plans.back().deferred = false;
    if (fault == 11) bad.plans.back().draw = false;
    if (fault == 12) bad.plans.clear();
    if (fault == 13) bad.plans.resize(4097, plan);
    assert(!FinalizeNativeDeferredEffects(bad, late, 7));
    if (!bad.plans.empty()) {
      assert(bad.plans.front().blend == plan.blend && !bad.plans.front().alpha_to_coverage);
      assert(!std::memcmp(&bad.plans.front().pass.shadow_colour_strength, &plan.pass.shadow_colour_strength, sizeof(RigidFloat4)));
    }
  }
  // A retained effects value survives replacement of the author's publication.
  auto retained = effects;
  effects.receiver.colour[0] = .9f;
  assert(FinalizeNativeDeferredEffects(submission, retained, 7));
  for (const auto &item : submission.plans) {
    assert(item.blend == retained.blend && item.alpha_to_coverage);
    const RigidFloat4 expected{.1f,.2f,.3f,.4f};
    assert(!std::memcmp(&item.pass.shadow_colour_strength, &expected, sizeof(expected)));
    assert(!std::memcmp(&item.object, &plan.object, sizeof(plan.object))); // No legacy zeroed material input.
  }
  assert(FinalizeNativeDeferredEffects(submission, effects, 7)); // Later same-visual update is consumed.
  const auto image = effects.receiver.image;
  effects.receiver.image.reset(); retained.receiver.image.reset();
  assert(submission.plans.front().shadow == image);
}
void RigidScenePacket() {
  using namespace bd::gpu::scene;
  const RenderMatrix identity{1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1};
  const LightingVector colour{.1f,.2f,.3f,.5f};
  {
  NativeReceiverPublication publication;
  const NativeVisualIdentity visual{12,7}, another{13,7}, reused{12,8};
  NativePrimaryReceiver receiver{std::make_shared<NativeTargetImage>(),identity,colour};
  assert(!publication.Read(visual,4,3));
  publication.Publish(receiver,visual,4,3);
  auto retained_receiver = publication.Read(visual,4,3);
  assert(retained_receiver && !publication.Read(another,4,3) && !publication.Read(visual,5,3) && !publication.Read(visual,4,1));
  assert(!publication.Read(reused,4,3));
  receiver.colour[0] = .2f; receiver.world_to_shadow[12] = 7;
  publication.Publish(receiver,visual,4,3);
  assert(publication.Read(visual,4,3)->colour[0] == .2f && retained_receiver->colour[0] == .1f);
  assert(publication.Read(visual,4,3)->world_to_shadow[12] == 7 && retained_receiver->world_to_shadow[12] == 0);
  receiver.colour[0] = std::numeric_limits<float>::quiet_NaN();
  publication.Publish(receiver,visual,4,3);
  assert(!publication.Read(visual,4,3));
  receiver.colour = colour; receiver.world_to_shadow[0] = std::numeric_limits<float>::infinity();
  publication.Publish(receiver,visual,4,3);
  assert(!publication.Read(visual,4,3));
  receiver.world_to_shadow = identity;
  publication.Publish(receiver,visual,4,3); publication.Reset(); assert(!publication.Read(visual,4,3));
  publication.Publish(receiver,{},4,3); assert(!publication.Read({},4,3));
  publication.Publish(receiver,{12,0},4,3); assert(!publication.Read({12,0},4,3));
  publication.Publish(receiver,visual,4,16); assert(!publication.Read(visual,4,16));
  receiver.image.reset(); publication.Publish(receiver,visual,4,3); assert(!publication.Read(visual,4,3));
  auto lifetime_image = std::make_shared<NativeTargetImage>();
  const std::weak_ptr<NativeTargetImage> weak_image = lifetime_image;
  publication.Publish({lifetime_image,identity,colour},visual,4,3);
  auto retained = publication.Read(visual,4,3);
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
  NativeMeshData asset;
  asset.streams.push_back({0,96,{}});
  asset.attributes = {{MeshSemantic::Position,0,0},{MeshSemantic::Normal,0,16},
      {MeshSemantic::TexCoord,0,32},{MeshSemantic::Color,0,48},{MeshSemantic::TexCoord,2,64}};
  geometry->rigid_vertex_input = NativeRigidVertexInput(asset,library);
  geometry->layered_rigid_vertex_input = NativeRigidVertexInput(asset,library,true);
  assert(geometry->rigid_vertex_input->Elements().size() == 4 &&
      geometry->layered_rigid_vertex_input->Elements().size() == 5);
  assert(geometry->layered_rigid_vertex_input->Elements()[4].semanticIndex == 2 &&
      geometry->layered_rigid_vertex_input->Elements()[4].alignedByteOffset == 64);
  asset.attributes.back().index = 1;
  assert(!NativeRigidVertexInput(asset,library,true) && NativeRigidVertexInput(asset,library));
  asset.attributes.back().index = 2; asset.attributes.back().offset = 84;
  assert(!NativeRigidVertexInput(asset,library,true));
  asset.attributes.back().offset = 64; asset.attributes.push_back(asset.attributes.back());
  assert(!NativeRigidVertexInput(asset,library,true));
  asset = {}; // Both immutable shader signatures survive source retirement.
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
  assert(plan->pass.shadow_filter.z == .65f/1024 && plan->shadow == depth && plan->albedo[0] == albedo);
  auto good = packet;
  {
    // Production packet preparation can outlive the object scope without
    // resolving an unseeded Keep or retaining a pose/source lookup key.
    NativeSceneLightingPublication publication;
    NativeSceneLightSet authored;
    authored.count = 2; authored.mode = 2; authored.scoring.scale = 255;
    for (uint32_t n = 0; n < 2; ++n) {
      auto &light = authored.lights[n];
      light.candidate.id = int32_t(n); light.candidate.kind = LitDirectional;
      light.candidate.enabled = true; light.candidate.views = 1u << n;
      light.candidate.intensity = 1;
      LitLight value{}; value.kind = LitDirectional; value.direction = {0,-1,0};
      value.colour = n ? LitVector{0,0,1} : LitVector{1,0,0}; light.value = value;
    }
    assert(publication.Publish(7, authored, {{11,93,0,NativeObjectLightInputs{}},{11,93,1,std::nullopt}}));
    const auto update = publication.Update(7);
    const auto red = publication.Capture(7,update,11,93,0,0);
    const auto blue = publication.Capture(7,update,11,93,0,1);
    const auto keep = publication.Capture(7,update,11,93,1,0);
    assert(red && blue && keep && !publication.Resolve(7,*keep));
    packet.lights.reset();
    assert(!build());
    const auto captured = PrepareNativeRigidScene(program,packet,receiver,nullptr,{}, {},keep);
    assert(captured && captured->light_recipe && !captured->light_ticket);
    assert(captured->pass.lights[0].kind.x == LitDisabled);
    assert(!PrepareNativeRigidScene(program,packet,receiver,nullptr,{}, {},NativeSceneLightRecipe{}));
    NativeRigidSceneSubmission pending{11,93,1,7,false,{*captured,*captured}};
    pending.plans[1].primitive = 1;
    const auto before = pending.plans.front().pass;
    const auto object_before = pending.plans.front().object;
    packet = {}; authored = {}; // finalization cannot consult the producer
    const auto first = publication.Resolve(7,*red);
    assert(first && first->revision == 0 && publication.Commit(7,*first));
    const auto stale = publication.Resolve(7,*keep);
    const auto next = publication.Resolve(7,*blue);
    assert(stale && next && publication.Commit(7,*next) && !publication.CanCommit(7,*stale));
    const auto final = publication.Resolve(7,*keep);
    assert(final && final->lights[0].colour.z == 1);
    for (uint32_t fault = 0; fault < 7; ++fault) {
      auto rejected = pending;
      auto ticket = *final;
      if (fault == 0) ++ticket.update;
      if (fault == 1) ticket.inherited = false;
      if (fault == 2) rejected.plans[1].light_recipe.reset();
      if (fault == 3) rejected.plans[1].light_recipe = *red;
      if (fault == 4) rejected.plans[1].light_ticket = ticket;
      if (fault == 5) ticket.lights[2].colour.x = std::numeric_limits<float>::quiet_NaN();
      if (fault == 6) ticket.revision = UINT64_MAX;
      assert(!FinalizeNativeRigidSceneLights(rejected.plans,ticket));
      for (const auto &sibling : rejected.plans)
        assert(std::memcmp(&sibling.pass,&before,sizeof(before)) == 0);
      assert(!rejected.plans.front().light_ticket); // no partially finalized sibling
    }
    assert(FinalizeNativeRigidSceneLights(pending.plans,*final) && publication.Commit(7,*final));
    assert(!FinalizeNativeRigidSceneLights(pending.plans,*final));
    for (const auto &sibling : pending.plans) {
      assert(sibling.pass.lights[0].colour_strength.z == 1 && sibling.pass.lights[0].colour_strength.x == 0);
      assert(sibling.pass.lights[1].kind.x == LitDisabled && sibling.pass.lights[2].kind.x == LitDisabled);
      assert(std::memcmp(&sibling.object,&object_before,sizeof(object_before)) == 0);
      assert(std::memcmp(&sibling.pass,&before,offsetof(NativeRigidPassGPU,lights)) == 0);
      assert(std::memcmp(sibling.pass.fog,before.fog,sizeof(before.fog)) == 0);
      assert(sibling.geometry == geometry && sibling.albedo[0] == albedo && sibling.shadow == depth);
    }
    auto explicit_bind = *captured; explicit_bind.light_recipe = *red;
    const auto bind_ticket = publication.Resolve(7,*red);
    assert(bind_ticket && FinalizeNativeRigidSceneLights(std::span(&explicit_bind,1),*bind_ticket));
    assert(explicit_bind.pass.lights[0].colour_strength.x == 1);
    {
      NativeDeferredQueue queue;
      auto delayed = *captured;
      delayed.deferred = delayed.draw = true; delayed.depth = 20;
      NativeRigidSceneSubmission mixed{11,93,1,7,false,{*captured,delayed,delayed}};
      mixed.plans[1].primitive = 1; mixed.plans[2].primitive = 2;
      assert(!queue.Take(0) && !queue.EndDrain());
      for (uint32_t fault = 0; fault < 6; ++fault) {
        auto rejected = mixed;
        if (fault == 0) rejected.frame = 8;
        if (fault == 1) rejected.plans[2].depth = std::numeric_limits<float>::quiet_NaN();
        if (fault == 2) rejected.plans[2].light_ticket = *final;
        if (fault == 3) rejected.plans[2].light_recipe.reset();
        if (fault == 4) rejected.plans[2].draw = false;
        assert(!queue.Stage(rejected, fault == 5 ? 5139 : 1, 7));
        assert(queue.Entries().empty() && rejected.plans.size() == 3);
      }
      assert(queue.Stage(mixed,1,7) && mixed.plans.size() == 1 && !mixed.plans[0].deferred);
      assert(queue.Entries().size() == 2);
      auto later = NativeRigidSceneSubmission{11,93,1,7,false,{delayed}};
      assert(!queue.Stage(later,0,7) && later.plans.size() == 1); // invalid insertion order
      auto next_frame = later; next_frame.frame = 8;
      assert(!queue.Stage(next_frame,2,8)); // undrained preceding frame
      mixed = {}; delayed = {}; // producer destroyed; queue owns the original packets
      assert(!queue.BeginDrain(8) && queue.BeginDrain(7));
      assert(!queue.BeginDrain(7) && !queue.EndDrain() && !queue.Stage(later,2,7));
      // Stable mixed sort consumes two native siblings around a legacy writer.
      // A late blue Bind must affect Keep, even though red was current at stage.
      const auto late = publication.Resolve(7,*blue);
      assert(late && publication.Commit(7,*late));
      for (uint32_t i = 0; i < 2; ++i) {
        auto value = queue.Take(i);
        assert(value && value->plans[0].primitive == i + 1 && !queue.Take(i));
        const auto inherited = publication.Resolve(7,*value->plans[0].light_recipe);
        assert(inherited && FinalizeNativeRigidSceneLights(value->plans,*inherited));
        assert(value->plans[0].pass.lights[0].colour_strength.z == 1);
        assert(value->plans[0].geometry == geometry && value->plans[0].albedo[0] == albedo);
      }
      assert(queue.EndDrain() && queue.Entries().empty());
      assert(queue.Stage(next_frame,0,8) && queue.BeginDrain(8) && queue.Take(0) && queue.EndDrain());
    }
    publication.Reset();
    assert(!publication.Resolve(7,*keep) && pending.plans.front().pass.lights[0].colour_strength.z == 1);
    packet = good;
  }
  packet.material_mask = 7;
  packet.material_values[2][0] = std::numeric_limits<float>::quiet_NaN();
  assert(build()); // Inactive reflection channel is not a diffuse/specular dependency.
  for (uint32_t mask : {0u,1u,2u,4u,5u,6u}) {
    packet.material_mask = mask;
    const char *reason = nullptr;
    assert(!PrepareNativeRigidScene(program,packet,receiver,&reason) &&
        std::string_view(reason) == "active diffuse/specular material values unavailable");
  }
  packet = good; packet.features->specular = false;
  packet.material_values[1][0] = std::numeric_limits<float>::quiet_NaN();
  for (uint32_t mask : {1u,3u,5u,7u}) {
    packet.material_mask = mask;
    const auto non_specular = build();
    assert(non_specular && !(non_specular->object.flags.x & RigidSpecular) &&
        non_specular->object.specular.x == 0 && non_specular->object.specular.w == 0);
  }
  packet.features->specular = true; packet.material_mask = 1;
  assert(!build()); // Missing specular data cannot disable an authored feature.
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
  {
    const auto saved_bounds = program.bounds;
    program.bounds = std::array<float,4>{0,0,-10,2};
    packet.policy.direct = false; packet.policy.deferred = packet.policy.alpha_test = true;
    NativeRigidCutoutInputs cutout{128,RigidCutoutAlways,false,{}};
    cutout.blend.alphaBlendEnable = true;
    NativeRigidDeferredInputs deferred;
    const auto prepare = [&] { return PrepareNativeRigidScene(program,packet,receiver,nullptr,cutout,deferred); };
    auto sorted = prepare();
    assert(sorted && sorted->draw && sorted->deferred && sorted->depth_write && sorted->depth == 8);
    assert(sorted->object.flags.z == RigidCutoutGE); // sorted pass owns comparison
    cutout.comparison = UINT32_MAX; // unrelated preceding comparison is not consumed
    assert(prepare() && prepare()->object.flags.z == RigidCutoutGE);
    packet.world[14] = -5;
    assert(prepare()->depth == 13 && sorted->depth == 8);
    packet.camera->view[14] = 7;
    assert(prepare()->depth == 6);
    deferred = {true,-123};
    program.bounds.reset();
    assert(prepare()->depth == -123);
    packet.policy.shadow_allowed = false;
    assert(!prepare()); // no depth write uses real bounds, not the fixed key
    program.bounds = std::array<float,4>{0,0,-10,2};
    auto no_depth_write = prepare();
    assert(no_depth_write && !no_depth_write->depth_write && no_depth_write->depth == 6);
    packet.camera->view[0] = std::numeric_limits<float>::quiet_NaN();
    assert(!prepare());
    packet = good;
    program.bounds = saved_bounds;
    assert(sorted->geometry == geometry && sorted->albedo[0] == albedo && sorted->depth == 8);
  }
  {
    packet.policy.alpha_test = true;
    NativeRigidCutoutInputs cutout{128,RigidCutoutGE,true,{}};
    cutout.blend.alphaBlendEnable = true;
    cutout.blend.srcBlend = RenderBlend::SRC_ALPHA;
    cutout.blend.destBlend = RenderBlend::INV_SRC_ALPHA;
    const auto cutout_plan = PrepareNativeRigidScene(program,packet,receiver,nullptr,cutout);
    assert(cutout_plan && (cutout_plan->object.flags.x & RigidCutout) && cutout_plan->alpha_to_coverage &&
        cutout_plan->blend == cutout.blend && cutout_plan->object.flags.z == RigidCutoutGE &&
        std::bit_cast<float>(cutout_plan->object.flags.w) == 128.f*std::bit_cast<float>(0x3b808081u));
    cutout.reference = 255; cutout.comparison = RigidCutoutAlways;
    const auto changed = PrepareNativeRigidScene(program,packet,receiver,nullptr,cutout);
    assert(changed && std::bit_cast<float>(changed->object.flags.w) == 1.f &&
        cutout_plan->object.flags.z == RigidCutoutGE); // retained plan is immutable
    cutout.comparison = 8;
    assert(!PrepareNativeRigidScene(program,packet,receiver,nullptr,cutout));
    cutout.comparison = 0; cutout.blend.alphaBlendEnable = false;
    assert(!PrepareNativeRigidScene(program,packet,receiver,nullptr,cutout));
    const float nan = std::numeric_limits<float>::quiet_NaN();
    for (uint32_t compare=0;compare<8;++compare) {
      assert(RigidCutoutPasses(compare,nan,.5f) == (compare == RigidCutoutAlways));
      assert(RigidCutoutPasses(compare,.5f,nan) == (compare == RigidCutoutAlways));
      assert(RigidCutoutPasses(compare,.5f,.5f) ==
          (compare == RigidCutoutGE || compare == RigidCutoutEqual || compare == RigidCutoutLE || compare == RigidCutoutAlways));
    }
    assert(!RigidCutoutPasses(8,1,0));
    // Requested factors must survive an opaque predecessor's disabled COPY
    // output. Shared-alpha folding and separate-alpha modes remain distinct.
    BlendShadow blend;
    blend.requested = 6u | (7u<<8) | (1u<<16);
    const auto enabled = DecodeEnabledBlendImport(blend);
    assert(enabled && enabled->alphaBlendEnable && enabled->srcBlend == RenderBlend::SRC_ALPHA &&
        enabled->destBlend == RenderBlend::INV_SRC_ALPHA && enabled->srcBlendAlpha == RenderBlend::SRC_ALPHA);
    blend.flags = 0x40000000u;
    assert(DecodeEnabledBlendImport(blend)->srcBlendAlpha == RenderBlend::ONE);
    blend.requested = 31;
    assert(!DecodeEnabledBlendImport(blend)); // no ZERO fallback for unsupported constant factors
    packet = good;
  }
  program.ranges.push_back(program.ranges[0]); assert(!build()); program.ranges.pop_back();
  // No asset-ID ceiling and no dropped sibling: semantic admission precedes
  // resources. The individual packet builder refuses missing active inputs.
  material->id = 19; program.ranges[0].shader = packet.shader;
  PrimitivePolicyInputs scene_policy;
  const auto classify = [&] { return PrepareNativeRigidSceneAdmission(program,scene_policy).route; };
  assert(classify() == NativeRigidCasterRoute::Native && build());
  auto detail1 = std::make_shared<NativeTextureGpu>();
  detail1->image = std::make_unique<SceneSource>(); detail1->view = std::make_unique<RenderTextureView>();
  detail1->dimension = RenderTextureViewDimension::TEXTURE_2D_ARRAY;
  auto detail2 = std::make_shared<NativeTextureGpu>();
  detail2->image = std::make_unique<SceneSource>(); detail2->view = std::make_unique<RenderTextureView>();
  detail2->dimension = RenderTextureViewDimension::TEXTURE_2D_ARRAY;
  packet.textures.images[1].primary = detail1; packet.textures.images[2].primary = detail2;
  packet.textures.image_mask = 7; packet.textures.uv = {1,2,3,4}; packet.textures.secondary_uv = {5,6,7,8};
  packet.samplers[1] = *packet.samplers[0]; packet.samplers[2] = *packet.samplers[0];
  for (uint8_t layers=0;layers<=3;++layers) {
    packet.shader.texture_layers = layers; program.ranges[0].shader = packet.shader;
    auto layered = build(); assert(layered && layered->object.flags.y == layers && classify() == NativeRigidCasterRoute::Native);
    assert(layered->object.detail_uv_scale_offset[0].z == 3+1.f/512 && layered->object.detail_uv_scale_offset[1].z == 5+1.f/512);
    for (uint32_t n=0;n<3;++n) assert(bool(layered->albedo[n]) == (n<layers));
  }
  auto layered = build();
  const auto layered_input = geometry->layered_rigid_vertex_input;
  geometry->layered_rigid_vertex_input.reset(); assert(classify() == NativeRigidCasterRoute::Native && !build());
  geometry->layered_rigid_vertex_input = layered_input;
  for (uint32_t n=0;n<3;++n) {
    packet.textures.image_mask &= ~(1u<<n); assert(!build()); packet.textures.image_mask |= 1u<<n;
    auto saved = packet.samplers[n]; packet.samplers[n].reset(); assert(!build()); packet.samplers[n] = saved;
  }
  packet.textures.images[2].slice_2d = detail2; assert(!build()); packet.textures.images[2].slice_2d.reset();
  packet.textures.secondary_uv[0] = std::numeric_limits<float>::quiet_NaN(); assert(!build()); packet.textures.secondary_uv[0] = 5;
  program.ranges.push_back(program.ranges[0]); program.geometries.push_back(geometry); program.materials.push_back(material);
  packet.primitive = 1; assert(classify() == NativeRigidCasterRoute::Native && build());
  program.ranges[1].reflection.enabled = true; assert(classify() == NativeRigidCasterRoute::Legacy);
  program.ranges[1].reflection.enabled = false; program.ranges[1].features.normal_mapping_requested = true;
  assert(classify() == NativeRigidCasterRoute::Legacy); program.ranges[1].features.normal_mapping_requested = false;
  program.ranges[1].shader.vertex_colour.reset(); assert(classify() == NativeRigidCasterRoute::Refused);
  program.ranges[1].shader = program.ranges[0].shader;
  program.policy_steps = {{PrimitivePolicyOperation::Alpha,17,0}};
  program.ranges[1].policy_step_end = 1;
  assert(classify() == NativeRigidCasterRoute::Native &&
      PrepareNativeRigidCasterAdmission(program,scene_policy).route == NativeRigidCasterRoute::Legacy);
  program.policy_steps.clear();
  program.policy_steps.push_back({PrimitivePolicyOperation::Alpha,1,0}); program.ranges[1].policy_step_end = 1;
  assert(classify() == NativeRigidCasterRoute::Legacy); // Unsupported sibling cannot be omitted.
  assert(PrepareNativeRigidSceneAdmission(program,scene_policy,true).route == NativeRigidCasterRoute::Native);
  scene_policy.pass_mode = 2;
  assert(PrepareNativeRigidSceneAdmission(program,scene_policy,true).route == NativeRigidCasterRoute::Legacy);
  scene_policy.pass_mode = 0;
  program.policy_steps.clear(); program.ranges.resize(1); program.geometries.resize(1); program.materials.resize(1);
  packet = good;
  const std::weak_ptr<NativeTextureGpu> retired_detail1 = detail1, retired_detail2 = detail2;
  detail1.reset(); detail2.reset();
  assert(!retired_detail1.expired() && !retired_detail2.expired());
  layered.reset(); assert(retired_detail1.expired() && retired_detail2.expired());
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
  assert(plan->geometry->count == 474 && plan->albedo[0]->view && plan->shadow->image);
  plan.reset();
  assert(retired_geometry.expired() && retired_albedo.expired() && retired_depth.expired());
}
void RigidLifecycle() {
  using namespace bd::gpu::scene;
  NativeRigidOutputReceipt receipt;
  assert(!receipt.Visible() && !receipt.Culled() && !receipt.Recorded());
  assert(!receipt.Resolve(true) && !receipt.Retire());
  assert(receipt.Record() && !receipt.Record() && !receipt.Retire());
  assert(!receipt.Visible()); // CPU-recorded / generated is not visible output.
  assert(receipt.Resolve(false) && receipt.Culled() && !receipt.Visible());
  assert(!receipt.Resolve(true) && receipt.Retire() && !receipt.Retire());
  receipt = {};
  assert(receipt.Record() && receipt.Resolve(true) && receipt.Visible() && !receipt.Culled());
  assert(receipt.Retire());
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
  NativeRigidOutputReceipt hidden;
  assert(hidden.Record() && hidden.Resolve(false) && hidden.Retire());
  assert(!window.Step(true,epoch)); // Retiring a GPU-zeroed draw cannot supply emission900.
  epoch.views[0].emitted = epoch.views[1].emitted = 900;
  assert(window.Step(true,epoch));
  NativeRigidReloadReadiness readiness{false,true,(2ull<<32)|4101};
  assert(readiness.Blockers(0) == 0 && readiness.Blockers(249999999) == 0);
  assert(readiness.Blockers(250000000) == NativeRigidReloadReadiness::Stale);
  assert(readiness.Blockers(-1) == NativeRigidReloadReadiness::Stale);
  // A stale observer must discard even899 emissions, not resume that baseline
  // when input starts polling again. Readiness reporting cannot qualify output.
  window = {}; epoch.views[0].emitted = epoch.views[1].emitted = 0;
  assert(!window.Step(readiness.Blockers(1) == 0,epoch));
  assert(window.Generation() == epoch.generation);
  epoch.views[0].emitted = epoch.views[1].emitted = 899;
  assert(!window.Step(readiness.Blockers(250000000) == 0,epoch));
  assert(window.Generation() == 0);
  assert(!window.Step(readiness.Blockers(1) == 0,epoch));
  epoch.views[0].emitted = epoch.views[1].emitted = 900;
  assert(!window.Step(true,epoch));
  epoch.views[0].emitted = epoch.views[1].emitted = 1799;
  assert(window.Step(true,epoch));
  readiness.walking = false;
  assert(readiness.Blockers(1) == NativeRigidReloadReadiness::NotWalking);
  readiness.paused = true; ++readiness.stage;
  assert(readiness.Blockers(250000000) == 15);
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
  for (uint32_t fault=0;fault<17;++fault) {
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
    if (fault == 11) b.albedo[0] = std::make_shared<NativeTextureGpu>();
    if (fault == 12) b.shadow = std::make_shared<NativeTargetImage>();
    if (fault == 13) b.albedo_samplers[0] = reinterpret_cast<RenderSampler *>(&token);
    if (fault == 14) ++b.model_generation;
    if (fault == 15) b.instance = 0;
    if (fault == 16) b.regression = true;
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
  a.albedo[0] = std::make_shared<NativeTextureGpu>(); a.shadow = std::make_shared<NativeTargetImage>();
  a.scene_depth = std::make_shared<NativeTargetImage>();
  a.input.object_data.flags = {RigidAlbedo,1,0,0};
  a.shadow_sampler = reinterpret_cast<RenderSampler *>(&token); a.albedo_samplers.fill(a.shadow_sampler);
  b = a; assert(NativeRigidBatchLength(pair,8,1) == 2);
  b.scene_depth = std::make_shared<NativeTargetImage>(); assert(NativeRigidBatchLength(pair,8,1) == 1);
  b = a; b.scene_depth.reset(); assert(!b.Ready(8,1));
  b = a; b.input.pass_data.world_to_clip[0].rows[3].x = 1;
  assert(NativeRigidBatchLength(pair,8,1) == 1); // One visibility command has one exact camera.
  b = a;
  a.world_bounds = NativeBounds{{-2,-3,-4},{0,1,2}}; b.world_bounds = NativeBounds{{1,2,3},{4,5,6}};
  auto union_bounds = NativeRigidBatchBounds(pair);
  assert((union_bounds && *union_bounds == NativeBounds{{-2,-3,-4},{4,5,6}}));
  b.world_bounds.reset(); assert(!NativeRigidBatchBounds(pair)); // Unknown sibling must keep batch visible.
  b = a;
  b.shadow_sampler = nullptr; assert(NativeRigidBatchLength(pair,8,1) == 1);
  a.albedo[1] = std::make_shared<NativeTextureGpu>(); a.albedo[2] = std::make_shared<NativeTextureGpu>();
  a.input.object_data.flags.y = 3; b = a;
  assert(NativeRigidBatchLength(pair,8,1) == 2);
  for (uint32_t n=0;n<3;++n) {
    b = a; b.albedo[n].reset(); assert(NativeRigidBatchLength(pair,8,1) == 1);
    b = a; b.albedo_samplers[n] = nullptr; assert(NativeRigidBatchLength(pair,8,1) == 1);
    b = a; b.albedo[n] = std::make_shared<NativeTextureGpu>(); assert(NativeRigidBatchLength(pair,8,1) == 1);
  }
  b = a; b.input.object_data.flags.y = 4; assert(!b.Ready(8,1));
  b.input.object_data.flags.y = 0; assert(!b.Ready(8,1));
  b.input.object_data.flags.x = 0; b.albedo = {}; assert(b.Ready(8,1));
  NativeRigidBatchItem shadow_cutout = good;
  shadow_cutout.input.object_data.flags = {RigidCutout | RigidAlbedo,1,RigidCutoutGE,0};
  shadow_cutout.albedo[0] = a.albedo[0]; shadow_cutout.albedo_samplers[0] = a.albedo_samplers[0];
  assert(shadow_cutout.Ready(8,1));
  shadow_cutout.albedo[0].reset(); assert(!shadow_cutout.Ready(8,1));
  shadow_cutout.input.object_data.flags = {RigidCutout,0,RigidCutoutGE,0};
  shadow_cutout.albedo_samplers[0] = nullptr; assert(!shadow_cutout.Ready(8,1)); // no invented alpha-only program
  shadow_cutout.input.object_data.flags = {}; assert(shadow_cutout.Ready(8,1));
  shadow_cutout.shadow = a.shadow; assert(!shadow_cutout.Ready(8,1)); // no attachment feedback
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
void CheckScreenshotContracts();
int main() {
  CheckScreenshotContracts();
  native_occlusion_tests::Run();
  OutputContract(); PoolOwnership(); SharedLayoutAndLease(); NativeTargetOwnership();
  SceneFramebufferOwnership();
  DepthOnlyCommands();
  CameraAndRigidCaster();
  RigidHardOffRouting();
  RigidLifecycle();
  RigidScenePacket();
  ReceiverSetupOrder();
  DeferredEffectOwnership();
  RigidBatches();
  SceneCommands();
  refraction_material_tests::Run();
  water_update_tests::Run();
}
