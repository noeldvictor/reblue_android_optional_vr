# Selected animation source checkpoint

2026-09-08, EDT. Parent471b554. The owner requested immediate commit/push while
the next animation bundle was in progress. This is an explicit source-only
checkpoint, not a new tested binary or runtime qualification.

## Source connection

- `NativeAnimationClip::SampleTrack` shares existing keyed/cubic math, selecting
  an immutable track ordinal rather than a model pose identity.
- `NativeAnimationAsset::SampleTarget` resolves named/indexed ownership. Native
  controller layers sample selected joints without an eager whole-clip temporary.
  Nonfinite sampling time still refuses even if no selected named track matches.
- `SampleRootMotion` produces one native channel with boundary flag/name sidecars.
  Named clips select by authored key; indexed root requests use descriptor0,
  independently of the selected model joint's pose identity.
- Hook8218FC98 prepares the actual selected clip in the existing bounded residency
  owner, selects the root using load-owned model names, and reuses owned rest data.
  Eligible requests bypass guest node lookup/key dispatch. Strict verification
  calls the original once and compares output/state before publication; unsupported
  inputs retain the original request. No new residency owner or lookup-wide cook.

Prior investigation recovered8218FC98 in generated file19, its one-joint sampler
8228A4E8 in file85, and named helper8228A310 in file25. Cutscene caller contexts
in file21 and821F4C58 in file6 consume the outgoing root record for placement.
These callers remain original; the hook does not establish cutscene ownership.
Other direct one-joint sampler callers are not replaced by this hook.

## Verification and next work

`python -B tools/host_checks.py`:266 passed.
`python -B tools/host_checks.py --all-boundaries`:415 passed in0.225s.
`git diff --check`:pass. These are source/scenario guards, not C++ behavior tests.

No C++ fixture, host build, game run, image or GPU test was launched for this
checkpoint. Before claiming the changed path works, add selected/whole keyed and
cubic parity fixtures, sparse ordinal coverage, indexed descriptor0 root coverage,
missing-target/dormant-payload tests and transactional refusal coverage. Build
the existing fixture and host target, then obtain positive fresh root-motion
comparison and controller/late-layer regression evidence. Unreached root content
is pending coverage, not a pass or a reason to lower the existing gates.

Last tested artifacts remain material62/CPU60 and host179/run984, as recorded in
[the late-animation report](20260908_2021_native-late-animation.md). Do not restamp
them as this source. Existing failure evidence, desktop/both-eye acceptance and
Quest prohibition remain unchanged. The owner's116B profile retained SHA256
`2F1BC38D763A1B7BDBA31F560684FD4AA7E42A714600B8D344F19DA7F38E23B0`.

No build/runtime producer was present at inspection. No artifacts were deleted,
no new build tree/log/capture was made, and no storage allowance was reset. Only
source, guard tests and small Markdown changes are retained for this checkpoint.
