# Native single-joint animation consumers

2026-09-08. Source and verification checkpoint after042c75e.

## Connection and source contract

The existing native clip/model/rest owners now supply the single-joint sampler
used by placement and attachment updates. Admitted calls bypass guest packed-key
and weighted-helper dispatch. Original graph lookup, caller placement/root
transforms and the outgoing48-byte channel record remain temporary adapters.

Complete translated8227EF60 and8228A4E8 bodies, the named A310 sampler and
AnimeData_method_4638 were inspected; the other caller input/output blocks were
read, not claimed as complete caller ports. The four non8218FC98 sites pair
lookup then sample in generated files100:11924/11942,63:11507/11525,
65:20217/20235 and88:7016/7034. They correspond to AnimeData_method_4638,
sub_822B8DF0, AnimeData_method_EBA8 and bdPlayerFieldUpdateDispatch.

The lookup hook publishes a thread-local one-shot graph/node/pose/generation
selection only after checked agreement with the loaded native model. Sampling
consumes it once, rechecks generation and uses the existing selected r5 clip
residency. No reverse-node cache, retained model lease or eager catalog cook was
added. Intervening/null lookup, wrong node and retired/reused generation refuse.

SampleRootMotion now shares weighted native channel blending: nonpositive and
sub-epsilon weights preserve all bytes, weights above one clamp, and missing
channels blend against owned authored rest. Indexed dispatch uses descriptor0,
not the selected pose ordinal. Named missing tracks preserve channels while
updating their boundary name. Dense inherited compression and nonzero Euler
modes still refuse; ambiguous model-name admission was not relaxed.

Strict verification executes the original sampler once before publication and
throws on drift. Active sampled floats retain the existing1e-4 relative/absolute
tolerance and finite requirement; every unsampled/header word requires exact
bits, including dormant NaN payloads in TR-only or no-op records. The separate
whole-root comparator uses the same channel-aware rule. No tolerance was raised.

## Existing-fixture and host evidence

- Material65/PID33868 build0; CPU62/PID33524 PASS0.12s, CTest0.13s. Fixtures cover
  keyed/cubic selected-track parity, weights -1/0/2^-24/.125/.5/1/2, owned rest,
  generation reuse, one-shot invalidation and exact dormant payloads.
- 416 Python source/scenario guards PASS0.227s; these are not C++/GPU evidence.
- Host182/PID29180 build0, only private bridge plus link; codegen0writes and no
  guest objects or shaders. Host181 was superseded by sampled/no-op provenance.
- HostEXE49311232B/PDB113426432B; EXE SHA256
  `BC0D87BCC2E0742916DB91F5F553F6E45A33C453B38FCF39F41C75F648600F00`.
- FixtureEXE1329664B SHA256
  `67F0C7E4C11F15A6504A0443E5F5AFD8A1B71DC2C4D051781229BF1BB1D91492`.

## Live run986: scoped observation passes

Existing bounded desktop operator, NativeImages/ModelMaterialsVerify/
ModelGeometryVerify/InstanceVerify/AnimationProbe/ControllerAnimationProbe/
LateAnimationProbe/SingleJointProbe; maximum60s,400KiB text,192MiB free-drop.
No capture, raw, perf, cache or dump production. PID34464/session14460 ran
21:11:48..21:12:21; exit1 is the explicit observation-complete stop, not a crash.

At frame1144, single-joint completed/checked/sampled are all289, weighted0 and
refused0. Main controllers completed/checked30364, samples3984, mixes8,
interior-CLIP3827, advancing clocks4000 and native handoffs3979,changed0;
5012 model/controller admission refusals remain unclassified. Late updates,
checks, samples, reused channels and handoffs are all6,refused0. Skeleton1119
has3706 matching evaluations/publications,wrong0/unavailable0. Context1120 is
FieldActive bg41_01,event1. Canonical import again maps122descriptors to121tracks
in93432B,prepare-refused0.

This establishes matched full-weight samples during the opening event. It does
not attribute live coverage to every caller, exercise fractional weights, prove
post-event/reload/motion pixels or qualify both eyes. Original-once comparisons
are correctness evidence, not an FPS measurement. Separate8218FC98 whole-root
coverage remains missing: run986 does not supersede run985's failed root gate.
The authored cutscene root-request gates need a targeted observation, not another
unchanged boot merely because an event is active.

Run log120282B SHA256
`62CF629B8D43403A712B87B8E1D0F64B3A9FFDF7BE302551B5F77058437C9B90`.
Profile restored exactly, independently checked SHA256
`2F1BC38D763A1B7BDBA31F560684FD4AA7E42A714600B8D344F19DA7F38E23B0`.
All producers terminal; timestamp inventory found no new prohibited artifacts.

## Remaining work and storage

Connect remaining placement/source-selection consumers to owned values, then
retire lookup/export and late-writer adapters when their last consumer migrates.
Special bones, authored indexed/subtree content, persistent cooking, palettes,
scene-wide pixels and the complete desktop/both-eye gate remain open. No Quest
or headset optimization/qualification is claimed.

Reused existing fixture/build trees and one bounded live observation, following
guest-source/devloop guidance. Eight superseded ordinary build/test logs removed
after replacement validation:7219B logical,12288B observed reclaimed. No runtime
failure, game data, profile or active build tree was removed. Partial attributed
retained growth178953B includes the new distinct986log and fixture/binary changes.
Keep985 for its separate root failure/post-event baseline. Calculated diagnostics
78340544B means another full400KiB runtime overlap exceeds the unchanged75MiB
stop by106944B; reconcile eligible evidence before another producer, without
resetting budgets. Measurements and cumulative limits remain in the
[shared ledger](20260906_0333_native-scene-state-bridge.md#2026-09-08-one-joint-consumer-connection-after042c75e).
