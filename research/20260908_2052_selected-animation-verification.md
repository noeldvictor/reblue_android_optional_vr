# Selected animation verification, root coverage pending

2026-09-08 EDT; source parentcecde3e. This continues the connected selected-track
and root-motion source checkpoint, not a new ownership framework. The devloop
and guest-source skills kept verification in existing fixtures/source contracts
and one bounded desktop observation. No guest rebuild, shader change or Quest run.

## Behavior and built artifacts

`TestSelectedTrackAndRootMotion` destroys source storage, then compares all12
output words for3 selected joints across131 times in both keyed and cubic clips.
It covers clamped endpoints, missing named targets, preserved dormant NaN bits,
nonfinite clocks and invalid single-record extents. Reordered model-bound clip
ordinals are checked separately. Existing independent keyed/Hermite math tests
remain; selected/whole parity is a representation comparison, not an independent
numerical oracle. Indexed fixtures assert descriptor0 for model pose2 and exact
TR-only/TRS activation/header/payload behavior.

A two-track finite-at-import fixture has one curve that overflows when sampled.
Whole sampling refuses; selecting only the other subtree succeeds. Selecting
the offending curve refuses transactionally. This causally checks selection
before evaluation; source guards also prohibit the old whole-clip temporary.

Material63 failed due solely to fixture nested-array CTAD; explicit
`std::array<ChannelRecord,1>` extents fix it. Material64/PID38820 build0;
CPU61/PID34880 PASS0.12s, CTest0.13s. All415 Python source/scenario checks pass
in0.226s; these do not substitute for C++ or GPU tests.

Host180/PID14964/session49891 build0, animation bridge and version consumers,
codegen0writes; no guest objects or shaders compiled. EXE49301504B,
PDB113389568B. EXE SHA256:
`00370FA4BA7FBD8E7F0E135880C8273C82CC9EDAC67A9944B9D7CDE74F9D25B6`.
Fixture EXE1325056B SHA256:
`85F283CF0EFACB3E609A59E54B1C624751E87B0D3336FEAD3502377200CBABB0`.

## Run985: controller comparison passes, combined root observation fails

The inspected existing operator ran mono native model/geometry/instance,
skeleton and animation comparisons, controller and late-layer observations,
plus a new positive root-comparison requirement. Previous controller/late gates
were unchanged.60s,400KiB log,192MiB free-drop; captures/perf/dumps/caches off.
PID22788/session2473 terminated20:51:26 with operator exit1: requested root
observation not reached. No unchanged retry or reduced gate followed.

At frame2290:108986 completed/checked controller transactions,18116 admission
refusals,13815 samples,23 mixes,13240 interior-CLIP samples,14601 advancing
clocks,13805 native channel handoffs,changed0. Late6completed/checked,refused0,
samples6,reused6,handoffs6. Skeletonframe2251 has14421 evaluated/published/checked,
wrong0,unavailable0. Context2266 is FieldActive bg41_01,event0 after the observed
opening event. This is not independent reload, motion-pixel or both-eye evidence.
Canonical import122->121tracks/93432B is re-observed;prepare-refused0.

No root completion, absence or refusal observation appears. That does not prove
the original helper was never called under comparison suppression, and does not
qualify native root selection/publication. CPU root math and a successful build
cannot replace actual cutscene integration. The next root probe needs evidence
of a reachable caller/content condition or a different observation, not another
identical opening boot. Guest outgoing48B root data, original cutscene placement,
other one-joint callers, slot selection, special bones and palette adapters remain.
No FPS gain is measured: strict comparisons still execute the original once.

Run985 log267079B SHA256:
`EE9C04E7E05A1529B8A946417356946D2026715724D46E15B3B160DFF30E7B44`.
Retain it for current controller/late comparisons and unresolved root coverage.
Owner profile restored and independently hashed to
`2F1BC38D763A1B7BDBA31F560684FD4AA7E42A714600B8D344F19DA7F38E23B0`.
No producer remains live. Scoped timestamp inventory found no new raw/perf/cache/
dump artifacts. Storage/cleanup accounting remains in the
[shared cumulative ledger](20260906_0333_native-scene-state-bridge.md#2026-09-08-selected-trackroot-motion-verification-aftercecde3e).
