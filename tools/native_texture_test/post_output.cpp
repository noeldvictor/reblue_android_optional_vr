// CPU contract checks: native interface doubles, no GPU allocation/submission.
#include "gpu/native_target_images.h"
#include "gpu/host_post_output.h"
#include "gpu/native_post_images.h"
#include "gpu/native_image_lease.h"
#include "gpu/post_sequence.h"
#include "gpu/scene/native_scene_framebuffer.h"
#include "gpu/scene/native_scene_commands.h"
#include "gpu/scene/native_scene_snapshot.h"
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
  explicit SceneFramebuffer(const std::array<NativeTargetImageHandle, 2> &images)
      : sources{images[0], images[1]} {}
  ~SceneFramebuffer() override {
    for (const auto &source : sources) assert(!source.expired());
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
  SceneCommands();
  refraction_material_tests::Run();
  water_update_tests::Run();
}
