/**
 * @brief Temporary checked pose-source boundary; native poses contain no addresses.
 * @copyright Copyright (c) 2026 reblue contributors
 * @license BSD 3-Clause, see LICENSE
 */
#pragma once
#include "gpu/scene/native_instance.h"
#include <optional>

namespace bd::gpu::scene::instance_source {
// InitBones, AllocBoneArray and model unload all use visual+2632. The
// sub_820FC3F8 getter selects container+8/+20, then dereferences the vector.
constexpr uint32_t kPaletteContainer = 2632;
constexpr uint32_t kUpdateThread = (uint32_t(-32035) << 16) - 26664;
struct Publication { uint32_t graph, palette, count, lane; };
struct Transfer { uint32_t source, destination, count; };
struct Binding {
  NativeInstanceId instance = 0;
  uint64_t model_generation = 0;
  uint32_t palettes[2]{};
};
inline bool TransferReady(std::optional<uint32_t> flags) {
  return flags && *flags == 3;
}

template <class ReadWord>
std::optional<uint32_t> Palette(uint32_t container, uint32_t lane, ReadWord &&read) {
  if (!container || lane >= 2) return {};
  const auto holder = read(uint64_t(container) + 8 + lane * 12);
  if (!holder || !*holder) return {};
  const auto palette = read(*holder);
  return palette && *palette ? palette : std::nullopt;
}

// Reader handles alignment, address overflow and BE decoding. The same
// decoder runs in the producer and the source-layout/handoff fixture.
template <class ReadWord>
std::optional<Publication> ReadPublication(uint32_t visual, uint32_t thread, ReadWord &&read) {
  if (!visual || uint64_t(visual) + kPaletteContainer > UINT32_MAX) return {};
  const auto graph = read(uint64_t(visual) + 2620);
  const auto count = read(uint64_t(visual) + 1868);
  const auto update_thread = read(kUpdateThread);
  if (!graph || !*graph || !count || !*count ||
      *count > NativeInstanceRegistry::kMaxTransforms || !update_thread) return {};
  const uint32_t lane = thread == *update_thread ? 0 : 1;
  const auto palette = Palette(visual + kPaletteContainer, lane, read);
  if (!palette) return {};
  return Publication{*graph, *palette, *count, lane};
}

template <class ReadWord>
std::optional<Transfer> ReadTransfer(uint32_t container, ReadWord &&read) {
  // sub_8213F5E8 (the actual derived vtable) copies only when +28 == 3.
  // Its copy and the ungated base sub_82144D10 use vector0.data -> vector1.data
  // and vector0.count (+8), not the calling thread's lane or a pointer swap.
  const auto source = Palette(container, 0, read);
  const auto destination = Palette(container, 1, read);
  const auto holder = read(uint64_t(container) + 8);
  const auto count = holder && *holder ? read(uint64_t(*holder) + 8) : std::nullopt;
  if (!source || !destination || !count || !*count ||
      *count > NativeInstanceRegistry::kMaxTransforms) return {};
  return Transfer{*source, *destination, *count};
}

inline bool ApplyTransfer(NativeInstanceRegistry &registry, Binding &binding,
                          const std::optional<Transfer> &transfer) {
  if (transfer && binding.palettes[0] == transfer->source &&
      registry.Transfer(binding.instance, 0, 1, transfer->count)) {
    binding.palettes[1] = transfer->destination;
    return true;
  }
  registry.Invalidate(binding.instance, 1);
  binding.palettes[1] = 0;
  return false;
}

inline bool PublishCompletedTransfer(NativeInstanceRegistry &registry, Binding &binding,
    const std::optional<Transfer> &transfer, std::span<const RenderMatrix> completed_pose) {
  if (transfer && transfer->count == completed_pose.size() &&
      registry.Publish(binding.instance, 0, completed_pose)) {
    binding.palettes[0] = transfer->source;
    return ApplyTransfer(registry, binding, transfer);
  }
  registry.Invalidate(binding.instance, 0);
  registry.Invalidate(binding.instance, 1);
  binding.palettes[0] = binding.palettes[1] = 0;
  return false;
}

inline std::shared_ptr<const NativeInstancePose> Find(
    const NativeInstanceRegistry &registry, const Binding &binding,
    uint64_t generation, uint32_t palette) {
  if (!generation || generation != binding.model_generation || !palette) return {};
  // Publication represents the completed render copy. The update-side source
  // may already be changing again; it is never exposed as a current draw pose.
  return binding.palettes[1] == palette ? registry.Read(binding.instance, 1) : nullptr;
}
} // namespace bd::gpu::scene::instance_source
