/**
 * @file    native_parameter_buffer.h
 * @brief   Bounded, address-free CPU ownership of shader parameter words.
 * @copyright Copyright (c) 2026 reblue contributors
 * @license BSD 3-Clause, see LICENSE
 */
#pragma once
#include <array>
#include <bitset>
#include <cstddef>
#include <cstdint>
#include <cstring>

namespace bd::gpu {
template <size_t Words> class NativeParameterBuffer {
public:
  static constexpr bool Contains(size_t first, size_t count) {
    return first <= Words && count <= Words - first;
  }
  void Clear() { known_.reset(); }
  void Zero() { words_.fill(0); known_.set(); }
  bool Invalidate(size_t first, size_t count) {
    if (!Contains(first, count)) return false;
    for (size_t i = first; i < first + count; ++i) known_.reset(i);
    return true;
  }
  // Own the bytes, never the producer's pointer. memmove also permits a
  // caller's source range to overlap its output when adapting a native block.
  bool Publish(size_t first, size_t count, const void *words) {
    if (!Contains(first, count) || (count && !words)) return false;
    if (count) std::memmove(words_.data() + first, words, count * 4);
    for (size_t i = first; i < first + count; ++i) known_.set(i);
    return true;
  }
  size_t Missing() const { return Words - known_.count(); }
  // Temporary import boundary: request only unpublished/invalidated words.
  // The callback must return false for unavailable input. A failed import
  // changes nothing, including words successfully read before the failure.
  template <class Read> bool ImportMissing(Read read, size_t &imported) {
    imported = 0;
    if (known_.all()) return true;
    auto next = words_;
    for (size_t i = 0; i < Words; ++i) {
      if (known_[i]) continue;
      if (!read(i, next[i])) return false;
      ++imported;
    }
    words_ = next;
    known_.set();
    return true;
  }
  bool Copy(void *out) const {
    if (!out || !known_.all()) return false;
    std::memcpy(out, words_.data(), Words * 4);
    return true;
  }
private:
  std::array<uint32_t, Words> words_{};
  std::bitset<Words> known_;
};
} // namespace bd::gpu
