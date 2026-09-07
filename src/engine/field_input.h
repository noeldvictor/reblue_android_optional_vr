/**
 * @brief Shared field directional-input observations, not player movement logic.
 * @copyright Copyright (c) 2026 reblue contributors
 * @license BSD 3-Clause, see LICENSE
 */
#pragma once
#include <cstdint>

namespace bd::engine {

// Temporary gameplay reader; the automation policy receives only native values.
// Reader returns optional<uint32_t> in host byte order. Missing data fails closed.
// bdPlayerFieldCanMove, 0x82207250..0x822072DC, gates the active leader this way.
// ScriptManTask::Update at 0x8219EB80 is more conservative about script mode:
// automation requires mode 0, not just !=1. This does NOT replace CanMove's
// per-character animation/action tests or establish that a stick actually moved it.
template <typename Reader>
bool ReadFieldDirectionalInput(uint32_t controller, uint32_t entity, Reader &&read,
                               uint32_t *blockers = nullptr) {
  if (blockers) *blockers = 32; // missing/unreadable observation
  if (!controller || !entity || controller > UINT32_MAX - 1824 ||
      entity > UINT32_MAX - 628)
    return false;
  const auto inhibit = read(controller + 1672);
  const auto grace = read(controller + 1676);
  const auto script = read(controller + 1824);
  if (!inhibit || !grace || !script || !*script ||
      *script > UINT32_MAX - 304)
    return false;
  const auto script_mode = read(*script + 304);
  const auto state = read(entity + 112);
  const auto hold = read(entity + 116);
  const auto lock = read(entity + 628);
  if (!script_mode || !state || !hold || !lock) return false;
  const uint32_t mask = ((!*grace && *inhibit) ? 1 : 0) |
      (*script_mode != 0 ? 2 : 0) | (*hold ? 4 : 0) | (*lock ? 8 : 0) |
      (*state == 5 || *state == 6 || *state == 8 || *state == 9 ? 16 : 0);
  if (blockers) *blockers = mask;
  return mask == 0;
}

} // namespace bd::engine
