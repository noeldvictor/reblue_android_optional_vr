/**
 * @brief Bounded, CPU-owned immediate UI geometry in host byte order.
 * @copyright Copyright (c) 2026 reblue contributors
 * @license BSD 3-Clause, see LICENSE
 */
#pragma once
#include <array>
#include <bit>
#include <cstdint>
#include <cstring>
#include <span>

namespace bd::gpu {
// The current UI shader input is six 32-bit lanes. Do not infer the format of
// other sprite/text/post producers from its size. Their layouts differ.
inline constexpr uint32_t kImmediateUiWords = 6;
inline constexpr uint32_t kImmediateUiStride = kImmediateUiWords * 4;
inline constexpr uint32_t kImmediateUiMaxVertices = 65536;
inline constexpr bool ImmediateUiCountSupported(uint32_t count) {
  return count <= kImmediateUiMaxVertices;
}
template <uint32_t Capacity = kImmediateUiMaxVertices> class NativeUiVertices {
  std::array<uint32_t, size_t(Capacity) * kImmediateUiWords> words_;
  uint32_t count_ = 0;
public:
  // All source alignments are supported; never convert packed colour/float
  // bit patterns through floating-point arithmetic. A refusal exposes no old
  // geometry, so a failed import cannot redraw the preceding batch.
  bool Import(std::span<const uint8_t> source, uint32_t count) {
    count_ = 0;
    if (count > Capacity || source.size() != uint64_t(count) * kImmediateUiStride)
      return false;
    for (size_t i = 0; i < size_t(count) * kImmediateUiWords; ++i) {
      uint32_t word;
      std::memcpy(&word, source.data() + i * 4, 4);
      words_[i] = std::endian::native == std::endian::little ? std::byteswap(word) : word;
    }
    count_ = count;
    return true;
  }
  uint32_t Count() const { return count_; }
  std::span<const uint32_t> Words() const {
    return {words_.data(), size_t(count_) * kImmediateUiWords};
  }
};
} // namespace bd::gpu
