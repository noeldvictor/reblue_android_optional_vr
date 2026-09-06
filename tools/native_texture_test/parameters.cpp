/**
 * @file    parameters.cpp
 * @brief   Bit-preserving engine parameter publication contract tests.
 * @copyright Copyright (c) 2026 reblue contributors
 * @license BSD 3-Clause, see LICENSE
 */
#ifdef NDEBUG
#undef NDEBUG
#endif
#include "gpu/scene/shader_parameter_import.h"
#include "gpu/native_parameter_buffer.h"
#include "immediate_ui.h"
#include "visual_schedule.h"
#include <algorithm>
#include <bit>
#include <cassert>
#include <cstring>
#include <iostream>
#include <random>
#include <vector>
using namespace bd::gpu::scene;

void NativeStorage() {
  using bd::gpu::NativeParameterBuffer;
  NativeParameterBuffer<16> owner;
  std::array<uint32_t, 16> source, output, expected;
  for (uint32_t i = 0; i < source.size(); ++i) source[i] = i + 11;
  output.fill(0xfeedface);
  assert(owner.Missing() == 16 && !owner.Copy(output.data()));
  assert(output[0] == 0xfeedface); // no partial/uninitialized upload
  assert(owner.Publish(4, 4, source.data() + 4));
  assert(owner.Missing() == 12);
  expected = source;
  std::fill(source.begin() + 4, source.begin() + 8, 0xdeadbeef);
  size_t imported = 999, reads = 0;
  assert(!owner.ImportMissing([&](size_t i, uint32_t &word) {
    ++reads; word = source[i]; return i != 12;
  }, imported));
  assert(owner.Missing() == 12 && !owner.Copy(output.data()));
  assert(owner.ImportMissing([&](size_t i, uint32_t &word) {
    assert(i < 4 || i >= 8); word = source[i]; return true;
  }, imported));
  assert(imported == 12 && owner.Missing() == 0);
  assert(owner.Copy(output.data()) && output == expected);
  source.fill(0xbaadf00d); // producer memory may be overwritten/reused
  assert(owner.ImportMissing([](size_t, uint32_t &) {
    assert(false); return false;
  }, imported) && imported == 0);
  assert(owner.Copy(output.data()) && output == expected);
  assert(owner.Invalidate(7, 2));
  assert(owner.ImportMissing([&](size_t i, uint32_t &word) {
    assert(i == 7 || i == 8); word = source[i]; return true;
  }, imported) && imported == 2);
  expected[7] = expected[8] = 0xbaadf00d;
  assert(owner.Copy(output.data()) && output == expected);
  for (auto [first, count] : {std::pair<size_t, size_t>{17, 0}, {16, 1},
                            {SIZE_MAX, 1}, {1, SIZE_MAX}}) {
    assert(!owner.Publish(first, count, source.data()));
    assert(!owner.Invalidate(first, count));
    assert(owner.Copy(output.data()) && output == expected);
  }
  assert(owner.Publish(16, 0, nullptr));
  assert(!owner.Publish(0, 1, nullptr) && !owner.Copy(nullptr));
  owner.Zero();
  assert((owner.Copy(output.data()) && output == std::array<uint32_t, 16>{}));
  owner.Clear();
  assert(owner.Missing() == 16 && !owner.Copy(output.data()));
  const std::array<uint32_t, 8> bits{0, 0x80000000, 1, 0x7f800000,
                                   0xff800000, 0x7fc12345, 0x7f812345, 0xffffffff};
  for (size_t alignment = 0; alignment < 16; ++alignment) {
    std::array<uint8_t, 48> bytes{};
    std::array<uint32_t, 8> imported_words;
    for (size_t i = 0; i < bits.size(); ++i) {
      for (size_t lane = 0; lane < 4; ++lane)
        bytes[alignment + i * 4 + lane] = uint8_t(bits[i] >> ((3 - lane) * 8));
      imported_words[i] = ImportParameterWord(bytes.data() + alignment + i * 4);
    }
    assert(imported_words == bits);
    assert(owner.Publish(0, 8, imported_words.data()));
    assert(owner.Publish(8, 8, imported_words.data()));
    bytes.fill(0xdd);
    assert(owner.Copy(output.data()));
    for (size_t i = 0; i < 16; ++i) assert(output[i] == bits[i % 8]);
  }
  assert(owner.Publish(0, bits.size(), bits.data()));
  assert(owner.Publish(8, bits.size(), bits.data()));
  assert(owner.Copy(output.data()));
  for (size_t i = 0; i < output.size(); ++i) assert(output[i] == bits[i % 8]);
  // Stage independence and a complete 4 KiB ABI-sized block. Clearing one
  // owner (device change/compatibility scope) cannot leak a previous lifetime.
  NativeParameterBuffer<1024> stages[2];
  stages[0].Zero(); stages[1].Zero();
  assert(stages[0].Publish(200, 8, bits.data()));
  std::array<uint32_t, 1024> block;
  assert(stages[1].Copy(block.data()));
  for (auto word : block) assert(word == 0);
  stages[0].Clear();
  assert(!stages[0].Copy(block.data()));
  stages[0].Zero();
  assert(stages[0].Copy(block.data()));
  for (auto word : block) assert(word == 0);
  std::cout << "native storage: owned bits, poisoned producer, missing-only import, "
               "transactional refusal, invalidation, stages and lifetime passed\n";
}

uint64_t OriginalMask(uint32_t first, uint32_t count) {
  const uint32_t begin = std::rotr(first, 2) & 0x3fffffff;
  const uint32_t end = std::rotr(uint32_t(first + count - 1), 2) & 0x3fffffff;
  const auto shift = std::min((end - begin) & 127u, 63u);
  uint64_t arithmetic = 0;
  for (uint32_t bit = 0; bit <= shift; ++bit)
    arithmetic |= uint64_t(1) << (63 - bit);
  return (begin & 64) ? 0 : arithmetic >> (begin & 127);
}
void Copy(std::vector<uint8_t> &bytes, uint32_t source, uint32_t destination,
          uint32_t count) {
  CopyParameterRows(
      count,
      [&](uint32_t offset) {
        uint32_t word;
        std::memcpy(&word, bytes.data() + source + offset, 4);
        return word;
      },
      [&](uint32_t offset, uint32_t word) {
        std::memcpy(bytes.data() + destination + offset, &word, 4);
      });
}
void OriginalCopy(std::vector<uint8_t> &bytes, uint32_t source,
                  uint32_t destination, uint32_t count) {
  // Model the original four LVLX/LVRX loads followed by four STVX stores,
  // then its single-vector tail loop. Snapshot one instruction group only.
  for (uint32_t row = 0; row < count;) {
    const uint32_t length = count - row >= 4 ? 64 : 16;
    uint8_t group[64];
    std::memcpy(group, bytes.data() + source + row * 16, length);
    std::memcpy(bytes.data() + destination + row * 16, group, length);
    row += length / 16;
  }
}
int main() {
  visual_schedule_test::Run();
  ImmediateUiTests();
  NativeStorage();
  uint32_t ranges = 0;
  for (uint32_t first = 0; first <= 257; ++first)
    for (uint32_t count = 0; count <= 257; ++count) {
      assert(ParameterRangeSupported(first, count) ==
             (uint64_t(first) + count <= 256));
      assert(ImportParameterDirtyMask(first, count) ==
             OriginalMask(first, count));
      if (ParameterRangeSupported(first, count) && count) {
        uint64_t groups = 0;
        for (uint32_t row = first; row < first + count; ++row)
          groups |= uint64_t(1) << (63 - row / 4);
        assert(ImportParameterDirtyMask(first, count) == groups);
        ++ranges;
      }
    }
  assert(!ParameterRangeSupported(UINT32_MAX, 1));
  assert(!ParameterRangeSupported(1, UINT32_MAX));
  assert(ImportParameterDirtyMask(0, 0) == UINT64_MAX);
  assert(ImportParameterDirtyMask(1, 0) == (uint64_t(1) << 63));
  assert(ImportParameterDirtyMask(256, 0) == 0);
  std::mt19937 random(0x73580);
  for (uint32_t trial = 0; trial < 10000; ++trial) {
    const uint32_t first = random(), count = random();
    assert(ImportParameterDirtyMask(first, count) ==
           OriginalMask(first, count));
  }
  std::vector<uint8_t> initial(8704);
  for (auto &byte : initial)
    byte = uint8_t(random());
  // Explicit payloads: +/- zero, infinities, denormals and signaling/quiet NaNs
  // must remain bits, never enter floating-point arithmetic during a copy.
  const uint32_t payloads[]{0,          0x80000000, 1,          0x7f800000,
                            0xff800000, 0x7fc12345, 0x7f812345, 0xffffffff};
  std::memcpy(initial.data() + 192, payloads, sizeof(payloads));
  uint32_t copies = 0;
  for (uint32_t count :
       {0u, 1u, 2u, 3u, 4u, 5u, 6u, 7u, 8u, 9u, 15u, 16u, 17u, 255u, 256u})
    for (uint32_t alignment = 0; alignment < 16; ++alignment)
      for (int32_t delta = -80; delta <= 80; ++delta) {
        auto actual = initial, expected = initial;
        const uint32_t destination = 256;
        const uint32_t source =
            uint32_t(int32_t(destination) + delta + int32_t(alignment));
        OriginalCopy(expected, source, destination, count);
        Copy(actual, source, destination, count);
        assert(actual == expected); // includes both untouched guards
        ++copies;
      }
  // PS must see earlier VS writes when both stages share an overlapping source.
  auto actual = initial, expected = initial;
  OriginalCopy(expected, 240, 256, 17);
  OriginalCopy(expected, 240, 4352, 17);
  Copy(actual, 240, 256, 17);
  Copy(actual, 240, 4352, 17);
  assert(actual == expected);
  // Assert observable load/store order independently of final byte equality.
  std::vector<bool> events;
  CopyParameterRows(
      7,
      [&](uint32_t) {
        events.push_back(false);
        return 0u;
      },
      [&](uint32_t, uint32_t) { events.push_back(true); });
  size_t event = 0;
  for (const auto words : {16u, 4u, 4u, 4u}) {
    for (uint32_t i = 0; i < words; ++i)
      assert(!events[event++]);
    for (uint32_t i = 0; i < words; ++i)
      assert(events[event++]);
  }
  assert(event == events.size());
  std::cout << ranges
            << " nonempty masks, 66564 range/mask pairs, 10000 wrapped masks, "
            << copies << " guarded overlapping/unaligned copies passed\n";
}
