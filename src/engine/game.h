/**
 * @file    engine/game.h
 * @brief   Process-wide facade over the engine state objects, plus the coarse
 *          engine mode roll-up.
 *
 * @copyright Copyright (c) 2026 Tom Clay <tomc@tctechstuff.com>
 *            All rights reserved.
 * @license   BSD 3-Clause License
 */
#pragma once

#include "engine/battle.h"
#include "engine/cutscene.h"
#include "engine/field.h"
#include "engine/inventory.h"
#include "engine/language.h"
#include "engine/party.h"

namespace bd::engine {

// Blue Dragon has no single mode integer. This is derived from which task
// singletons are live.
enum class EngineMode : u8 {
  Unknown,
  TitleOrMenu,
  FieldActive,
  FieldTransition,
  Battle,
  Loading,
};

const char *ToString(EngineMode mode);

class Game {
public:
  static Game &Get();

  // Guest memory is mapped. Every accessor below returns defaults until it is.
  bool IsReady() const;

  EngineMode Mode() const;

  // Field predicates that no single object owns.
  bool FieldSessionLive() const;    // GameTask != 0
  bool FieldControllerLive() const; // FieldSceneController != 0
  bool FieldGameplayActive() const; // GameTask && !shutdownFlag
  // FSC+0x6A0 is the fade/scene state: 0 idle, NOT a complete input-ready flag.
  // The update resumes NPCs then writes 0 at 0x8219FE44. State 4 is not idle.
  u32 FieldState() const;
  bool IsLoading() const;           // any loader slot state in {1,2,3}
  bool LoadingScreenUp() const;     // actual loading icon/strip visibility
  bool MindowsPanelActive() const;  // Mindows panel focused, NOT the camp menu
  u32 CurrentModuleAddress() const; // SequenceControl+0x70, identity unresolved

  engine::Stage Stage() const; // forwards to Field().Stage()
  engine::Field Field() const { return {}; }
  engine::Party Party() const { return {}; }
  engine::Roster Roster() const { return {}; }
  engine::Inventory Inventory() const { return {}; }
  engine::Battle Battle() const { return {}; }
  engine::Cutscene Cutscene() const { return {}; }
  engine::Movie Movie() const { return {}; }
  engine::Language Language() const { return {}; }

private:
  Game() = default;
};

} // namespace bd::engine
