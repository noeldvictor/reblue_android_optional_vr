/**
 * @brief Original-order UI publication and bounded geometry ownership fixtures.
 * @copyright Copyright (c) 2026 reblue contributors
 * @license BSD 3-Clause, see LICENSE
 */
#pragma once
#include "gpu/native_ui_vertices.h"
#include "gpu/scene/immediate_ui_import.h"
#include <cassert>
#include <iostream>
#include <vector>

inline void ImmediateUiTests() {
  using namespace bd::gpu;
  using namespace bd::gpu::scene;
  constexpr uint32_t device = 0x100, default_colour = 0x3000, one = 0x3f800000;
  const std::array<uint32_t, 8> patterns{0, 0x80000000, 1, 0x7f800000,
      0xff800000, 0x7fc12345, 0x7f812345, 0xffffffff};
  std::vector<uint32_t> initial(4096);
  for (size_t i = 0; i < initial.size(); ++i) initial[i] = patterns[i % patterns.size()];
  const auto scalar = [](uint32_t bits) {
    // Independent actual LFS/STFS conversion, not the production bit helper.
    volatile double widened = double(std::bit_cast<float>(bits));
    return std::bit_cast<uint32_t>(float(widened));
  };
  size_t cases = 0;
  for (auto flags : {0u, 1u, 0x80000000u, 0xffffffffu}) {
    initial[(default_colour + 16) / 4] = flags;
    // Include lazy-default aliases, sequential overlapping colour copies,
    // translation aliases with colour and dirty masks, and null inheritance.
    for (uint32_t colour : {0u, 0x40u, default_colour, default_colour + 4,
        device + 0x172c, device + 0x1730, device + 0x1734, device}) {
      for (uint32_t translation : {0u, 0x60u, device, device + 4,
          device + 0x1730, device + 0x83c, device + 0x840, device + 0x844}) {
        auto actual = initial, expected = initial;
        auto read = [&](uint64_t address) -> std::optional<uint32_t> {
          if ((address & 3) || address / 4 >= actual.size()) return {};
          return actual[size_t(address) / 4];
        };
        const auto p = BuildImmediateUiImport(device, colour, translation, default_colour, one, read);
        assert(p && actual == initial); // planning has no partial side effects
        auto at = [&](uint32_t address) -> uint32_t & { return expected.at(address / 4); };
        if (!(at(default_colour + 16) & 1)) {
          for (uint32_t i = 0; i < 4; ++i) at(default_colour + i * 4) = one;
          at(default_colour + 16) |= 1;
        }
        const auto source = colour ? colour : default_colour;
        for (uint32_t i = 0; i < 4; ++i)
          at(device + 0x1730 + i * 4) = scalar(at(source + i * 4));
        at(device + 8) |= 0x80000000;
        if (translation) {
          const auto y = scalar(at(translation + 4));
          const auto x = scalar(at(translation));
          const auto z = scalar(at(translation + 8));
          at(device + 0x840) = x; at(device + 0x844) = y;
          at(device + 0x848) = z; at(device + 0x84c) = one;
          at(device) |= 0x04000000;
        }
        p->Apply([&](uint32_t address, uint32_t word) { actual[address / 4] = word; });
        assert(actual == expected && p->Matches(read));
        for (uint32_t i = 0; i < 4; ++i) {
          assert(p->colour[i] == actual[(device + 0x1730) / 4 + i]);
          if (translation) assert(p->translation[i] == actual[(device + 0x840) / 4 + i]);
        }
        assert(p->translated == (translation != 0));
        ++cases;
      }
    }
  }
  auto read = [&](uint64_t address) -> std::optional<uint32_t> {
    if (!address || (address & 3) || address / 4 >= initial.size()) return {};
    return initial[size_t(address) / 4];
  };
  for (auto invalid : {1u, 0xfffffffcu, 0x10000u}) {
    assert(!BuildImmediateUiImport(invalid, 0, 0, default_colour, one, read));
    assert(!BuildImmediateUiImport(device, invalid, 0, default_colour, one, read));
    assert(!BuildImmediateUiImport(device, 0, invalid, default_colour, one, read));
  }
  assert(!BuildImmediateUiImport(device, 0, 0, default_colour, one,
      [](uint64_t) -> std::optional<uint32_t> { return {}; }));
  NativeUiVertices<4> owner;
  std::array<uint8_t, 4 * kImmediateUiStride + 16> source{};
  for (size_t alignment = 0; alignment < 16; ++alignment) {
    for (size_t i = 0; i < 4 * kImmediateUiWords; ++i)
      for (uint32_t byte = 0; byte < 4; ++byte)
        source[alignment + i * 4 + byte] = uint8_t(patterns[i % patterns.size()] >> (24 - byte * 8));
    assert(owner.Import({source.data() + alignment, 4 * kImmediateUiStride}, 4));
    source.fill(0xcd);
    for (size_t i = 0; i < owner.Words().size(); ++i)
      assert(owner.Words()[i] == patterns[i % patterns.size()]);
    assert(owner.Count() == 4);
  }
  assert(!owner.Import(source, 4) && owner.Count() == 0 && owner.Words().empty());
  assert(!owner.Import({}, UINT32_MAX) && owner.Words().empty());
  assert(!owner.Import({source.data(), 24}, 5) && owner.Words().empty());
  assert(owner.Import({}, 0) && owner.Words().empty());
  assert(ImmediateUiCountSupported(kImmediateUiMaxVertices));
  assert(!ImmediateUiCountSupported(kImmediateUiMaxVertices + 1));
  std::cout << cases << " original-order UI publications and 16 poisoned-source geometry alignments passed\n";
}
