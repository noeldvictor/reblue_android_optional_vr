# Native late animation layers into skeleton ownership

2026-09-08, after827d36a. Full host-renderer/desktop gate remains incomplete.
Native animation/skeleton remain opt-in. No Quest or speedup claim.

## Connected change

Main controller -> completed native channels -> authored late layers -> native
skeleton -> existing immutable instance/render pose. The late phase owns clocks
and channel working values, not source scratch or per-slot guest sampler calls.
Clips, model names/hierarchy, blending and instance lifetime reuse existing owners.
No new renderer/asset framework, generated source edit or hook-site TOML change.

`PlanNativeAnimationLateLayers` retains order4,5,3, unit-step clocks independent
of the main animation delta, upper-only weight clamp, single duration subtraction
and incoming global inclusion. Source gates and slot5 selection504 stay at the
adapter. `ExecuteController(...,begin_update=false)` continues the same working
layer without clearing dirty flags a second time. An admitted complete late
transaction executes no original body in normal mode. Strict mode runs the whole
original once, checks clocks/globals/channels before export and throws on drift.

`ControllerHandoff::TakeLayer` moves native payloads and restores only outgoing
flag/name sidecars; no packed payload decoding for a valid continuation. Each
late writer republishes before bones. Standalone slot samplers also use native
working layers and republish only when visual/graph/output/count identify that
visual's real channel array. Their scratch samplers cannot masquerade as a visual
publication. Strict comparisons suppress inner native hooks through shared
ReferenceScope. Address/generation/retirement/one-shot guards remain.

Every outgoing word is STILL checked against untracked writers at continuation
and skeleton boundaries. This bundle removes the observed stale late handoff,
not validation/export or all source selection. At most six differing-word
messages per process identify joint/word/expected/actual/readability for any
remaining refusal. No threshold or mismatch check was relaxed.

## Source decisions (main read translated bodies, not a new binary audit)

- `sub_822D3CB0`, generated/reblue_recomp.12.cpp:11866, complete: gates visual
  +5452/+5456/+5528 select slots4/5/3. Slot5 ID+1872==504 skips before entry read.
  Present slots add weight_rate/time_rate once, clamp upper weight, wrap once or
  clamp duration, then SlotUpdate. No other gameplay or effect side effects.
  Three callers: AnimeData_method_4638, sub_823CA880, sub_822B8DF0.
- `AnimeData_method_4638`, file100:11750, complete: main AnimationUpdate, then
  AnimeData_method_1A60, late phase, InitBones, helper_928. Its root-translation
  zeroing happens BEFORE the main controller in this body. Other root-motion
  sampling and matrix/gameplay work remain original.
- `bdVisualObjectAnimSlotUpdate`, file89:2624, complete: scalar clamping and
  replacement dispatch, no independent clock advancement. Only main controller,
  late phase and attachment sub_822B9BF0 call it statically.
- `sub_822B9BF0`, file23:11555, complete: two attachments, slot0 lookup/selection,
  time/weight adoption, clear all flags if an old clip disappears, otherwise
  SlotUpdate. The standalone sampler connection does NOT replace this selection
  and clear-flags producer; live attachment coverage remains pending.
- Offset search alone is unsafe: many +2628 references have visual-8 as base and
  identify the model GRAPH, not the channel array. For example player dispatch
  and AnimeData's attached object use graph+16/name lookup. Do not install a
  blanket channel writer hook based on that offset.
- `sub_8228A4E8` one-joint/root-motion sampler has five callers: AnimeData_method_4638,
  sub_8218FC98, sub_822B8DF0, AnimeData_method_EBA8, bdPlayerFieldUpdateDispatch.
  Its dispatch was only partially read; exact complete contract/relevant callees
  still need reading before implementation. Not the full weighted dispatcher.

## Verification

- 413 artifact-free source guards/scenario tests PASS0.239s. Updated guards
  require native layer publication while still forbidding sampler-side asset
  import. Added late runtime wiring/ordering guards, not pixel qualification.
- Material62/PID37152 build0, CPU60/PID33600 PASS0.12s/CTest0.13s. Tests include
  literal late order/unit clocks/disabled and absent slots/nonpositive weights,
  replacement and invalid clocks, plus65 advancing controller ->late ->skeleton
  ->immutable render-instance chains against the independent prior ABI layer
  implementation. Untouched dirty flags, lifetime, exactly-once consumption and
  untracked writes remain covered. Source curve storage is destroyed first.
- Host179/PID22264/session13205 build0: private bridge and version consumers;
  codegen0writes, no guest objects or shaders. Source base827d36a dirty.
- Run984/PID32680/session49310 terminal20:21:19. Operator exit1 is the explicit
  observation-complete stop, not a crash/pass of broader scene gates. Extra late
  criterion requires positive matching transactions, samples, reuse and native
  skeleton handoffs ON TOP OF the unchanged controller observation.
- Frame1110: main30460checked/completed,5028refused,3996samples,8mixes,3839interior
  CLIP times,4012advancing clocks,3991handoffs,changed0. Late6completed/checked,
  refused0,6samples,6native continuations,6skeleton handoffs. Standalone0samples,
  slot-publications0. Combined source ordering, causal CPU regressions and the
  same main counts/+6 handoffs establish the fix for983's six late refusals;
  this is not a claim that all other scene writers are gone.
- Skeleton frame1084:evaluated/published/checked3706,wrong0,unavailable0. Context
  frame1085:FieldActive,bg41_01,event1. Not interactive/reload/motion-sequence or
  both-eye qualification. Canonicalization122->121tracks/93432B,prepare-refused0;
 15resident clips1079656B. No new images or performance measurement.

HostEXE49293824B SHA256
`F35100C8A4B33124B9B2970AD1934976012BC379643E58E9F1C7593160773A98`;
PDB113348608B. FixtureEXE1285632B SHA256
`FF58E847694976E102236BA500A3809607AC06D2AD1FE1709847A174CB9C81CB`;
tree10470066B/43files. Run984118995B SHA256
`03C9EECD1E85766AFC93A5ABDDEB9A3C0A1D8D97576FA4C2DB91D6EDA80AAC91`.
Exact owner profile restored and independently verified:
`2F1BC38D763A1B7BDBA31F560684FD4AA7E42A714600B8D344F19DA7F38E23B0`.

## Remaining boundary and storage

Next use this established route as regression, not another unchanged boot.
Classify remaining controller model/admission refusals alongside the next real
producer/consumer removal. Attachment flag/selection writers, one-joint root
motion, other character/cutscene writes and special bones remain. Do not remove
the boundary comparison until their consumers migrate. Persistent cooked clip
identities, dense/mode/content coverage, fields/battles/cutscenes/menus/events/
reloads/motion/both-eyes and all requested modern GPU techniques remain required.
Prior975reload,979/980coverage and971/962pixel failures remain protected.

Same cumulative ledger/floor62509998080B; no reset. Runtime preflight78089008B
diagnostics,409600B full log overlap reserved under75MiB;60s/192MiB growth cap.
No new raw/image/perf/cache/dump files. Verified179/62/60 supersedes178/61/59
build/test text. Run984 replaces177/983 controller/import provenance and resolves
the six observed late refusals; their reports/hashes remain historical evidence.
Completed cleanup and final accounting are in the shared scene-state ledger.
