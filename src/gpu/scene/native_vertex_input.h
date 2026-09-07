/**
 * @brief Owned vertex input for native geometry, independent of resource
 * wrappers.
 * @copyright Copyright (c) 2026 reblue contributors
 * @license BSD 3-Clause, see LICENSE
 */
#pragma once
#include <array>
#include <cstring>
#include <memory>
#include <plume_render_interface.h>
#include <span>
#include <unordered_map>

namespace bd::gpu::scene {

// Temporary contract with the existing translated shaders, NOT the native
// asset layout. Delete when those shaders consume named native attributes.
struct VertexShaderDecode {
  uint32_t texcoords = 0, normals = 0, binormals = 0, tangents = 0;
  uint32_t blend_weights = 0, positions = 0, integer_texcoords = 0;
  bool operator==(const VertexShaderDecode &) const = default;
};

// Shared by the input assembler and pulling. These codes are the current
// pulling shader ABI, not a serialized asset format.
constexpr uint32_t VertexInputPullEntry(plume::RenderFormat format, uint32_t slot,
                                      uint32_t offset) {
  using F = plume::RenderFormat;
  uint32_t code = 0;
  switch (format) {
  case F::R32_FLOAT:
    code = 1;
    break;
  case F::R32G32_FLOAT:
    code = 2;
    break;
  case F::R32G32B32_FLOAT:
    code = 3;
    break;
  case F::R32G32B32A32_FLOAT:
    code = 4;
    break;
  case F::B8G8R8A8_UNORM:
    code = 5;
    break;
  case F::R8G8B8A8_UINT:
    code = 6;
    break;
  case F::R8G8B8A8_UNORM:
    code = 7;
    break;
  case F::R16G16_SINT:
    code = 8;
    break;
  case F::R16G16_SNORM:
    code = 9;
    break;
  case F::R16G16B16A16_SNORM:
    code = 10;
    break;
  case F::R16G16_UNORM:
    code = 11;
    break;
  case F::R16G16B16A16_UNORM:
    code = 12;
    break;
  case F::R16G16_FLOAT:
    code = 13;
    break;
  case F::R16G16B16A16_FLOAT:
    code = 14;
    break;
  case F::R32_UINT:
    code = 15;
    break;
  case F::R16G16B16A16_SINT:
    code = 16;
    break;
  default:
    return 0;
  }
  return slot < 16 && offset < 65536 ? (code << 24) | (slot << 16) | offset : 0;
}

class NativeVertexInput {
public:
  static constexpr size_t kMaxElements = 32, kSemanticBytes = 16;
  std::span<const plume::RenderInputElement> Elements() const {
    return {elements_.data(), count_};
  }
  uint64_t Id() const { return id_; }
  uint32_t Streams() const { return streams_; }
  // Pulling also binds an explicit zero source for synthetic attributes. An
  // absent table entry loses the format's default lanes (notably float4 w=0).
  uint32_t PullStreams() const { return pull_streams_; }
  const std::array<uint32_t, 16> &PullTable() const { return pull_; }
  bool Pullable() const { return pullable_; }
  const VertexShaderDecode &ShaderDecode() const { return decode_; }
  NativeVertexInput(const NativeVertexInput &) = delete;
  NativeVertexInput &operator=(const NativeVertexInput &) = delete;

private:
  friend class NativeVertexInputLibrary;
  NativeVertexInput() = default;
  std::array<plume::RenderInputElement, kMaxElements> elements_{};
  std::array<std::array<char, kSemanticBytes>, kMaxElements> names_{};
  std::array<uint32_t, 16> pull_{};
  VertexShaderDecode decode_{};
  uint64_t id_ = 0;
  uint32_t count_ = 0, streams_ = 0, pull_streams_ = 0;
  bool pullable_ = true;
};
using NativeVertexInputHandle = std::shared_ptr<const NativeVertexInput>;

// Serialized by the geometry store's mutex. Inputs are immutable and retained
// for its lifetime, including while a background PSO job uses their address.
// No disk IO or first-draw state capture. Equal content shares one owner.
class NativeVertexInputLibrary {
public:
  static constexpr size_t kMaxBytes = 8u << 20, kMaxInputs = 8192;
  static constexpr size_t kOwnerBytes = sizeof(NativeVertexInput) + 256;
  explicit NativeVertexInputLibrary(size_t max_bytes = kMaxBytes,
                                    size_t max_inputs = kMaxInputs)
      : max_bytes_(max_bytes), max_inputs_(max_inputs) {}
  NativeVertexInputHandle
  Resolve(std::span<const plume::RenderInputElement> elements, uint32_t streams,
          VertexShaderDecode decode) {
    if (elements.empty() || elements.size() > NativeVertexInput::kMaxElements ||
        (streams & ~0xffffu))
      return {};
    // One fixed-size unpublished candidate; no unbounded temporary arrays.
    // Names are owned too: input elements must not borrow a loader's string
    // storage.
    auto input = std::unique_ptr<NativeVertexInput>(new NativeVertexInput);
    uint64_t hash = 14695981039346656037ull;
    const auto word = [&](uint32_t value) {
      for (unsigned i = 0; i < 4; ++i)
        hash = (hash ^ uint8_t(value >> (i * 8))) * 1099511628211ull;
    };
    word(streams);
    word(decode.texcoords);
    word(decode.normals);
    word(decode.binormals);
    word(decode.tangents);
    word(decode.blend_weights);
    word(decode.positions);
    word(decode.integer_texcoords);
    word(uint32_t(elements.size()));
    uint32_t locations = 0;
    for (size_t i = 0; i < elements.size(); ++i) {
      const auto &element = elements[i];
      if (!element.semanticName ||
          element.format == plume::RenderFormat::UNKNOWN ||
          element.slotIndex >= 16 || element.location >= 16 ||
          element.alignedByteOffset >= 65536 ||
          (locations & (1u << element.location)))
        return {};
      if (!(streams & (1u << element.slotIndex)) && element.slotIndex != 15)
        return {};
      if (!(streams & (1u << element.slotIndex)) && element.alignedByteOffset != 0)
        return {}; // the synthetic source is a bounded zero buffer, not a stream
      size_t length = 0;
      while (length < NativeVertexInput::kSemanticBytes &&
             element.semanticName[length])
        ++length;
      if (!length || length == NativeVertexInput::kSemanticBytes)
        return {};
      for (size_t n = 0; n < length; ++n)
        word(uint8_t(element.semanticName[n]));
      word(0);
      word(element.semanticIndex);
      word(element.location);
      word(uint32_t(element.format));
      word(element.slotIndex);
      word(element.alignedByteOffset);
      locations |= 1u << element.location;
      std::memcpy(input->names_[i].data(), element.semanticName, length);
      input->elements_[i] = element;
      input->elements_[i].semanticName = input->names_[i].data();
      // Preserve the format for synthetic inputs too: BD_PullF's empty-entry
      // default is (0,0,0,1), whereas a zero float4 has w=0. Native staging
      // supplies its own zero buffer with stride zero, not a retained guest view.
      const auto entry = VertexInputPullEntry(
          element.format, element.slotIndex, element.alignedByteOffset);
      input->pull_[element.location] = entry;
      input->pullable_ &= entry != 0;
      input->pull_streams_ |= 1u << element.slotIndex;
    }
    input->id_ = hash;
    input->count_ = uint32_t(elements.size());
    input->streams_ = streams;
    input->decode_ = decode;
    if (const auto found = inputs_.find(hash); found != inputs_.end()) {
      const auto &existing = *found->second;
      if (existing.streams_ != streams || existing.decode_ != decode ||
          existing.count_ != elements.size())
        return {};
      for (size_t i = 0; i < elements.size(); ++i) {
        const auto &a = existing.elements_[i], &b = elements[i];
        if (std::strcmp(a.semanticName, b.semanticName) ||
            a.semanticIndex != b.semanticIndex || a.location != b.location ||
            a.format != b.format || a.slotIndex != b.slotIndex ||
            a.alignedByteOffset != b.alignedByteOffset)
          return {}; // Never alias a hash collision.
      }
      return found->second;
    }
    if (inputs_.size() >= max_inputs_ || kOwnerBytes > max_bytes_ ||
        inputs_.size() > (max_bytes_ - kOwnerBytes) / kOwnerBytes)
      return {};
    NativeVertexInputHandle result(std::move(input));
    inputs_.emplace(hash, result);
    return result;
  }
  size_t Bytes() const { return inputs_.size() * kOwnerBytes; }
  size_t Size() const { return inputs_.size(); }

private:
  size_t max_bytes_, max_inputs_;
  std::unordered_map<uint64_t, NativeVertexInputHandle> inputs_;
};

template <class LegacyReader>
std::span<const plume::RenderInputElement>
VertexInputElements(const NativeVertexInput *native, LegacyReader &&legacy) {
  return native ? native->Elements() : legacy();
}
template <class LegacyReader>
VertexShaderDecode VertexInputDecode(const NativeVertexInput *native,
                                     LegacyReader &&legacy) {
  return native ? native->ShaderDecode() : legacy();
}

// Render-thread cumulative observations; do not confuse a successful bind
// with full-frame or pixel qualification. No per-draw log/disk output.
struct NativeVertexInputUseStats {
  uint64_t pipelines = 0, decode_blocks = 0, pulled_records = 0;
};
inline NativeVertexInputUseStats &NativeVertexInputUses() {
  static thread_local NativeVertexInputUseStats stats;
  return stats;
}
} // namespace bd::gpu::scene
