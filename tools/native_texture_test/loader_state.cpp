/**
 * @file    loader_state.cpp
 * @brief   Loading observations: hidden persistent icons and all asset slots.
 * @copyright Copyright (c) 2026 reblue contributors
 * @license BSD 3-Clause, see LICENSE
 */
#include "engine/loader_state.h"
#include <array>
#include <iostream>
#include <limits>
#include <stdexcept>

using namespace bd::engine;

static void Require(bool value) {
  if (!value)
    throw std::runtime_error("loader-state observation check failed");
}

int main() {
  std::array<uint32_t, 128> states{};
  uint32_t reads = 0;
  const auto busy = [&] {
    reads = 0;
    return AnyAssetSlotLoading([&](uint32_t i) {
      ++reads;
      return states.at(i);
    });
  };
  Require(!busy() && reads == 128);
  // Exercise every slot, including the 120 omitted by the former reader.
  for (uint32_t i = 0; i < states.size(); ++i) {
    for (uint32_t state = 0; state <= 6; ++state) {
      states[i] = state;
      const bool expected = state >= 1 && state <= 3;
      Require(busy() == expected);
      Require(reads == (expected ? i + 1 : 128));
    }
    states[i] = UINT32_MAX;
    Require(!busy());
    states[i] = 0;
  }
  for (bool force_strip : {false, true}) {
    for (bool task : {false, true}) {
      for (uint32_t visible : {0u, 1u}) {
        for (int32_t tick : {-1, 0, 3, 4, 128}) {
          Require(!LoadingIconVisible(0.0f, task, visible, force_strip, tick));
          Require(!LoadingIconVisible(-0.01f, task, visible, force_strip, tick));
          Require(!LoadingIconVisible(std::numeric_limits<float>::quiet_NaN(),
                                      task, visible, force_strip, tick));
          const bool expected = force_strip || !task ? tick >= 4 : visible != 0;
          Require(LoadingIconVisible(0.02f, task, visible, force_strip, tick) == expected);
          Require(LoadingIconVisible(1.0f, task, visible, force_strip, tick) == expected);
        }
      }
    }
  }
  // Same allocation through visible -> faded out -> hidden -> visible again.
  Require(LoadingIconVisible(1.0f, true, 1, false, 10));
  Require(!LoadingIconVisible(0.0f, true, 1, false, 10));
  Require(!LoadingIconVisible(0.0f, true, 0, false, 10));
  Require(!LoadingIconVisible(0.5f, true, 0, false, 10));
  Require(LoadingIconVisible(0.5f, true, 1, false, 10));
  std::cout << "128 loader slots and persistent icon/strip visibility passed\n";
}
