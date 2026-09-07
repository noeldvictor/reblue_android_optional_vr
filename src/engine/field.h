/**
 * @file    engine/field.h
 * @brief   The live field session: current stage, and the player transform.
 *
 * @copyright Copyright (c) 2026 Tom Clay <tomc@tctechstuff.com>
 *            All rights reserved.
 * @license   BSD 3-Clause License
 */
#pragma once

#include <array>
#include <string>
#include <string_view>

#include <rex/types.h>

namespace bd::engine {

// World units for a position, radians for a YXZ euler rotation with yaw in the
// middle slot.
using Vec3 = std::array<f32, 3>;

// ScriptMan area category, named for the stage-name prefix each one selects.
enum class AreaCategory : u32 {
  Gr = 0,
  Bg = 1,
  Bi = 2,
  Dg = 3,
  Wd = 4,
  Wc = 5,
  Eb = 6,
  Sp = 7,
  Bt = 8,
};

// Mirrors bdStageNameBuild: the stem from a category and combinedNum
// (= area*100 + sub), empty for a category with no stem.
void BuildStageName(char *out, size_t cap, u32 cat, u32 num);

// The script variable block. bdSaveBlockCapture copies its first 20480 bytes
// straight into the save block, so reading it live is reading save state. Two
// flag namespaces share it: 32-bit globals, and a two-bit array above them.
// Falsy off-field.
class ScriptVars {
public:
  // The highest two-bit flag the block has room for.
  static constexpr u32 kFlagMax = 0x257F;

  ScriptVars() = default;
  explicit ScriptVars(u32 ea) : ea_(ea) {}

  explicit operator bool() const { return ea_ != 0; }

  // Global script variable N, indexed from 0 for what the script calls 256.
  // Chest and barrier flag ids index this directly.
  u32 Global(u32 index) const;

  // Two-bit script flag, set by a search point when it is used up.
  u32 Flag(u32 id) const;

private:
  u32 ea_ = 0;
};

// The stage descriptor hanging off the field scene controller's ScriptManTask.
// Falsy off-field.
class Stage {
public:
  Stage() = default;
  explicit Stage(u32 script_man_ea) : ea_(script_man_ea) {}

  explicit operator bool() const { return ea_ != 0; }

  u32 Category() const;    // area kind, selects the stage-name prefix
  u32 CombinedNum() const; // area*100 + sub
  u32 Area() const;        // CombinedNum / 100
  u32 Sub() const;         // CombinedNum % 100

  // Built the way bdStageNameBuild does, e.g. "bg03_01". Empty when the
  // category has no known prefix.
  std::string Name() const;

  // What the game calls this stage on screen, e.g. "Ancient Hospital Ruins -
  // 1F". Empty when namelist_map has no row for it.
  std::string DisplayName() const;

  // Allocation-free equivalent of Name() == name, for per-tick polling.
  bool Is(std::string_view name) const;

private:
  u32 ea_ = 0;
};

class Field {
public:
  Field() = default;

  // True once the field scene controller and its ScriptManTask both resolve.
  explicit operator bool() const;

  engine::Stage Stage() const;
  engine::ScriptVars Vars() const;

  u32 NothingsCollected() const; // lifetime total from save data, 0 off-field

  bool HasPlayer() const;
  // Shared controller/script/leader input gates, not full per-character CanMove.
  bool DirectionalInputAvailable(u32 *blockers = nullptr) const;
  Vec3 Position() const;
  Vec3 Rotation() const;
};

} // namespace bd::engine
