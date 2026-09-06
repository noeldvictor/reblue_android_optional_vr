# Correct loader observations and post-event field material evidence

2026-09-06, EDT. Previous strategy review yielded actionable source evidence:
the loading predicate confused an allocated icon with a visible one. This turn
implements the correction and clears the failed scenario gate. It is a renderer
verification prerequisite, not additional native geometry/frame ownership.

## Original contracts and changes

- `Game::LoadingScreenUp()` previously checked Loader+136 for nonzero. Original
  `sub_82129380`, generated file 23, 0x82129660..0x82129724, gates drawing on
  float fade+132. The persistent task at +136 has its visibility at task+104;
  `SetVisibleAndPlay(task, 0)` hides it without clearing the pointer. The forced
  or missing-task fallback draws only with strip tick+120 >=4 and positive fade.
  The corrected reader observes these conditions, including the fallback flag
  at 0x82DC9B48. `config/hooks/cutscene.toml` confirms the fade hook boundary.
- The original `bdLoaderInit` initializes **128**, not eight, 124-byte records
  at Loader+144. Complete `sub_82129030` traverses the same 128 slots. Complete
  `bdAssetSlotCheckLoaded` confirms unsigned `(state-1)<=2` means busy. The
  shared, dependency-free observation helper now covers the full range.
- Complete `bdScriptOpScreenFade` (generated file 84) writes its state argument
  to FieldSceneController+1696 and resets fade timing. Relevant branches of
  `ScriptManTask__vf02_Update` (file 105) gate field input checks on state **0**
  at 0x8219F084; at 0x8219FE44 they unpause NPCs and restore state 0. Its state-4
  branch is a fade/wait path, not interactive readiness. Corrected the facade
  comment; did not modify the original state machine or claim state 0 alone
  proves input readiness. This was a targeted branch audit, not a reread of
  the entire approximately 7,900-line update body.

Native-material diagnostics now report loader-busy and icon-visible separately.
The existing ignored runtime supervisor requires two consecutive sampled idle
contexts in `bg41_01`, no event/movie/load/icon, after an observed opening event.
It requires new diffuse/specular comparisons and model hits between the two
samples, and rejects the latest mismatch/missing/unsupported/failure record.
Water activity is no longer the model-material scenario prerequisite.

## Verification and limits

- Loader fixture covers each of 128 slots, states 0..6 and UINT32_MAX, scan
  bounds/early exit, persistent hide/show/fade, NaN/nonpositive fade and fallback
  strip precedence. Fixture build 02 and CPU 01 pass; CPU suite **0.07 s**.
- Initial sandboxed fixture build 01 stalled after CMake regeneration. Verified
  live cmake PID 29180 and Ninja child; terminated that exact owned tree before
  retrying. It exited 1, not a successful build. Permission-enabled build 02
  passed, PID 19804. No build tree was deleted or guest objects rebuilt.
- Host build **42**, PID 30204/session 9296, passed; log spans about **16 s**.
  Header/CMake dependency changes rebuilt host objects only. Existing settings
  designator/deprecated CRT warnings remain. Codegen reports zero files written.
- All **120 source-boundary tests** pass in 0.029 s. The existing runner's actual
  parsed loop passes **15 in-memory gate cases**: positive deltas, missing event,
  wrong stage/state, loading/icon, absent fresh comparisons/hits, unsupported or
  missing models, later mismatches/events, incomplete sample and only one idle
  context. The first test invocation had a shell brace typo; corrected invocation
  passed, with no fixture files or runtime launched by these tests.
- Host binary: 47,811,584 B, linked 16:34:14, source `7e320e0` plus this reviewed
  change, SHA-256 `c514ecb4cc46d9997d252c649dafa817dadacd312e04efc58f91e0bb130d0147`.
- Loader fixture executable: 27,648 B, SHA-256
  `4d46e1a431cfd09b3f6294e3f2db44d752b25865cb7bf37e7fc7f9f8f05884ca`.

Flat diagnostic **890**, PID 29612/session 98449, 16:36:47..16:37:38:
all seven settings audited (autoplay on, material comparison on, PSO precache
off, perf/capture-after off, capture thresholds 600/120 dormant). No raw/perf/
image captures. The original 116-byte owner profile was restored byte-for-byte.

- Frame 868: Loading, fade-state 11, busy 0, visible icon 1.
- Frames 1168/1468: FieldActive, state 0, event 1, busy/icon both 0.
- Frames **1768/2068**, five seconds apart: FieldActive, state 0, `bg41_01`,
  player present, event/movie/busy/icon all 0.
- Between those last two samples: **15,124 diffuse**, **14,514 specular** checks,
  zero wrong; **58,572 model lookup hits**. Total 114 publications, one retirement,
  113 live owners /730,584 B; no missing/load/unsupported/input/budget failures.
- Reflection remains unexercised. This proves sampled post-event field material
  execution, **not movement/input readiness, new field pixels, both-eye or full
  desktop qualification**. Autoplay still waits 150 s before walking; this 51 s
  run cannot qualify movement. No Quest or rendering-speedup result.

Log 176,343 B, SHA-256
`de19c8eba871ec6f603ca6ff1a10448afce4833fce5ccbfa9164bc8a57ac5665`.
Existing opening-cinematic pixels,
normal Toon flat/XR evidence and unresolved scenery/text sequences are preserved.

## Storage and next work

Preflight free 65,061,658,624 B; compile/link overlap reserved <=256 MiB, no new
raw allowance. Loader exe/PDB/object total **718,242 B**; retain this one reusable
behavioral fixture, replace on change. Existing runner tool reservation grows
from 48 to 49 MiB to include it; the 75 MiB stop /100 MiB hard diagnostic ceiling
is unchanged. No downloads, shader rebuild or bulk asset cooking.

Gross new build/runtime logs **195,304 B** across both fixture attempts, CPU,
host and runtime. Removed five exact superseded agent-created logs after their
replacements passed: failed reader run 889, host 41 stdout/stderr, fixture build
01 stdout/stderr. Reader failure is diagnosed above and in its original report;
the exact old logs are gone, future runs can regenerate equivalent diagnostics.
Logical removal **292,376 B**; immediate free 65,049,759,744 ->65,050,058,752 B:
**299,008 B actually reclaimed**, counted once. Net retained log payload shrinks
97,072 B; including the new fixture gives **621,170 B retained diagnostic growth**
for new behavioral coverage. No protected raw/image/build/game data was removed.
Post-cleanup drive-wide use **11,599,872 B** from preflight, not all attributable
to this task. Final source/Git writes still count in the cumulative scene ledger.

Next renderer work remains load-owned geometry/material associations, native
instances and direct scene/shadow submission. Source inspection confirms
`ImportNativeMesh` still takes guest wrappers during replay, and its current
disk writer has no aggregate bound despite the GPU arena's 256 MiB limit.
Resolve that prerequisite before eager load-time cooking; do not introduce
another first-draw cache or grow the native mesh archive without limits.
