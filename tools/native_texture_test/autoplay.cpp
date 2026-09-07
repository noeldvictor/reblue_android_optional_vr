/**
 * @brief Tests the production autoplay and gameplay-input observation policies.
 * @copyright Copyright (c) 2026 reblue contributors
 * @license BSD 3-Clause, see LICENSE
 */
#include "engine/field_input.h"
#include "xr/autoplay.h"
#include <cmath>
#include <limits>
#include <map>
#include <optional>
#include <stdexcept>

namespace {
void Check(bool value) {
  if (!value) throw std::runtime_error("autoplay check failed");
}
bd::xr::AutoplayObservation Ready() {
  bd::xr::AutoplayObservation observation;
  observation.field_active = observation.idle = observation.player = true;
  observation.directional_input = true;
  observation.stage = (2ull << 32) | 4101;
  observation.position = {1, 2, 3};
  return observation;
}
void Walk(bd::xr::Autoplay &policy, const bd::xr::AutoplayObservation &observation,
          double start = 10) {
  for (int i = 0; i <= 5; ++i) {
    const auto input = policy.Step(start + i * 0.125, observation);
    Check(input.walking == (i >= 4));
    if (!input.walking) Check(input.x == 0 && input.y == 0);
  }
  Check(policy.Episode() > 0);
}
} // namespace

void TestAutoplay() {
  using namespace bd::xr;
  Check(AutoplayOverlayBlocksInput(std::nullopt));
  Check(AutoplayOverlayBlocksInput(0));
  Check(!AutoplayOverlayBlocksInput(1));
  Check(!AutoplayOverlayBlocksInput(UINT32_MAX));
  // Synthetic read-only source layout: the production bridge passes swapped
  // words through this same reader; each missing required field fails closed.
  const std::map<uint32_t, uint32_t> words{
      {1000 + 1672, 0}, {1000 + 1676, 0}, {1000 + 1824, 5000},
      {5000 + 304, 0}, {9000 + 112, 0}, {9000 + 116, 0}, {9000 + 628, 0}};
  auto actual = words;
  const auto read = [&](uint32_t key) -> std::optional<uint32_t> {
    auto it = actual.find(key);
    return it == actual.end() ? std::nullopt : std::optional(it->second);
  };
  const auto allowed = [&] { return bd::engine::ReadFieldDirectionalInput(1000, 9000, read); };
  Check(allowed());
  for (const auto &[key, value] : words) {
    actual.erase(key);
    Check(!allowed());
    actual = words;
  }
  for (auto key : {2672u, 5304u, 9116u, 9628u}) {
    actual[key] = 1;
    Check(!allowed());
    actual = words;
  }
  actual[2672] = actual[2676] = 1;
  Check(allowed()); // original inhibit/grace precedence
  actual = words;
  for (uint32_t state = 0; state <= 12; ++state) {
    actual[9112] = state;
    Check(allowed() == (state != 5 && state != 6 && state != 8 && state != 9));
  }
  actual = words;
  actual[2824] = 0;
  Check(!allowed());
  actual[2824] = UINT32_MAX;
  Check(!allowed());
  Check(!bd::engine::ReadFieldDirectionalInput(0, 9000, read));
  Check(!bd::engine::ReadFieldDirectionalInput(UINT32_MAX, 9000, read));
  Check(!bd::engine::ReadFieldDirectionalInput(1000, UINT32_MAX, read));

  Autoplay policy;
  auto ready = Ready();
  Check(!policy.Step(0, {}).start);
  Check(policy.Step(6.1, {}).start);
  Check(policy.Step(6.45, {}).confirm);
  // Elapsed time alone never causes walking, including long cold starts.
  for (double seconds : {10.0, 150.0, 600.0})
    Check(!policy.Step(seconds, {}).walking);
  policy.Reset();
  Walk(policy, ready);
  Check(policy.Distance() == 0 && policy.MovedSamples() == 0);
  ready.position[0] += 0.25f;
  Check(policy.Step(10.75, ready).walking);
  Check(std::abs(policy.Distance() - 0.25) < 1e-8 && policy.MovedSamples() == 1);
  Check(policy.Step(10.875, ready).walking);
  Check(policy.MovedSamples() == 1); // stationary polls are not movement
  Check(std::abs(std::hypot(policy.Step(11, ready).x, policy.Step(11, ready).y) - 1) < 1e-6);

  for (int condition = 0; condition < 10; ++condition) {
    policy.Reset();
    ready = Ready();
    Walk(policy, ready);
    auto blocked = ready;
    switch (condition) {
    case 0: blocked.field_active = false; break; // includes title/battle/loading
    case 1: blocked.idle = false; break;
    case 2: blocked.player = false; break;
    case 3: blocked.event = true; break;
    case 4: blocked.movie = true; break;
    case 5: blocked.panel = true; break;
    case 6: blocked.directional_input = false; break;
    case 7: blocked.stage = 0; break;
    case 8: blocked.stage++; break;
    case 9: blocked.position[0] = std::numeric_limits<float>::quiet_NaN(); break;
    }
    const auto stopped = policy.Step(10.75, blocked);
    Check(!stopped.walking && stopped.x == 0 && stopped.y == 0);
    Check(policy.Distance() == 0 && policy.MovedSamples() == 0);
    Walk(policy, ready, 11);
    Check(policy.Episode() == 2);
  }
  policy.Reset();
  Walk(policy, Ready());
  Check(!policy.Step(12, Ready()).walking); // stale polling loses readiness
  Walk(policy, Ready(), 13);
  auto teleport = Ready();
  teleport.position[0] += 11;
  Check(!policy.Step(13.75, teleport).walking && policy.Distance() == 0);
  Walk(policy, teleport, 14);
  Check(policy.MovedSamples() == 0);
  Check(!policy.Step(std::numeric_limits<double>::quiet_NaN(), ready).walking);
  Check(policy.Episode() == 0);
  Walk(policy, Ready());
  policy.Reset(); // disabling/re-enabling cannot inherit episode or clock state
  Check(policy.Episode() == 0 && !policy.Step(0, Ready()).walking);
}
