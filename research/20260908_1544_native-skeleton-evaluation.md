# Native ordinary skinned hierarchy evaluation

2026-09-08. Connected implementation after17f5c33, not full animation/frame
completion. Host166 includes the root-ABI decoder and bounded failure provenance.
`bd_native_skeleton` remains opt-in pending complete desktop qualification.

Host167 adds the causal water/ordinary receipt partition described below;
it does not change skeleton math, admission or the strict comparison threshold.

## Producer, owner and consumers

`PublishModelMaterials` imports immutable joint topology, authored translation,
scale and pre/selected/post rotations into `NativeModelRenderData`, sharing its
generation, leases and existing8MiB aggregate budget. The native representation
contains preorder parent ordinals and dense model-local pose IDs, not pointers
or console flags. Unsupported camera-facing/sparse/cyclic source hierarchies are
not admitted; an empty skeleton cannot enter the evaluator.

The existing `bdVisualObjectInitBones` scope supplies object identity. The new
`bdBoneInitSkinned` hook admits only its matching ordinary update-side palette,
matching load-owned model/count, complete checked channel input and valid root.
Native whole-pose evaluation replaces the original recursive bone walk for this
admitted path when comparison is off. Evaluated values publish to the existing
instance update lane and export through a temporary outgoing palette for
collision/effects and unconverted late writers. No draw-time discovery/cache is
introduced. Completed handoff still occurs after all late writers; existing
render interpolation, animated culling, scene/shadow skin plans and water retain
that completed instance/model lifetime. This does not remove the late-copy adapter.

## Recovered source contract

Reference: generated/reblue_recomp.86.cpp `bdAnimBoneEvaluate` (0x822834E8),
file5 `bdBoneInitSkinned`, file90 `sub_82284AD0`, and file30
`bdVisualObjectInitBones`. Full recursion/control-flow audit preceded17f5c33;
this integration rechecked the by-value wrapper rather than repeating that audit.
The relevant instruction hooks in guest_census/frame_interp/pso_predictor/
physical_buffers are unchanged; no generated source or hook TOML was edited.

- Root: five BE register pairs r6..r10, then six float words at caller SP+88.
- Node: ID+0, flags+8, translation+16, radians+28, scale+44, child/sibling+56/+60,
  optional pre/post radians+80/+92 in the104-byte extended layout.
- Channels:48-byte records indexed by pose ID, flags+0, translation+8,
  quaternion xyz/w+20, scale+36. Only enabled values are inputs.
- Euler construction appends Z,Y,X as Hamilton qZ*qY*qX. Authored quaternions
  are not normalized. Row-vector matrices use pre/selected/post left-products.
- Children receive the parent's pre-scale world matrix and a separate scale:
  either translation-only compensation or explicit full scale inheritance.
  Dynamic parent reset does not discard that separate scale. Siblings retain
  their incoming parent, not the previous sibling's output.
- Global0x82DC99DC can suppress a particular node's children; the current native
  path requires it to be zero. Camera-facing flags0x00600000 need a future owned
  view contract, not guessed identity rotations.

`bd_native_materials_verify` calls the original once before export and compares
every component (1e-4 relative with unit absolute floor). Drift throws without
native replacement/fallback and records the first joint/component. Other
preflight-unavailable cases retain the whole original call with bounded reason
examples. No threshold was relaxed to obtain passing evidence.

## Local verification

400 Python guards/scenario tests pass, including production-owner wiring and
fresh `--skeleton` requirements independently in both reload epochs. C++
material46/CPU44 passes native_material_data in0.14s. Coverage includes BE root
packing/stack overflow, off-axis noncommuting rotations, non-unit quaternions,
negative/zero scales, parent compensation, reordered IDs/root siblings, active
channel overrides, all-or-nothing failure, source destruction, model retirement,
render interpolation and shared residency limits. Fixtures do not prove pixels.

Host166 compiled and linked successfully, no guest objects or shader changes.
EXE49138688B SHA256
20666DA6D6F939A10EBE0E68F53D218D3CBC777C8C5F0A0EBEA70DA111AB8BF9.
PDB112095232B SHA256
48E9D305FBC26874BC1368591A70DBAE0CC9B94DF5E2D12C377ACE30CAAF32BD.
Material fixture723456B SHA256
138295188EEE037458683475B095512A35D9E3D634E21123072B9A5201F4EBB7.

## First live integration: run974, preserved failure

Host166 ran normal mono/MSAA/native post with temporary `bd_fps_limit=60`, native
skeleton enabled and original comparison, plus the full strict instance/material/
skin scene+shadow/deferred/reload chain. PID36188 stopped at15:45:17; exact116B
owner profile restored. Log551337B SHA256
CA9C48D1A60FBFBB982582B3655B78078B55B9DF9EF0695E57FDFF8B15C677AB.

Separate fresh gates establish actual native evaluation: cold1879..2179 adds
2904 evaluated/published/checked poses; reload4157..4457 adds2640 each. Final
sample36331, unavailable0/wrong0. Interpolation windows add854/1202 blends;
skin scene and shadow each emit/fence-retire13200 in both epochs. Generation93/
instance144 retires before205/403. These scoped results do not override the full
run's FAIL: the ordinary deferred scope/input conservation verifier rejects
frame600 before ordinary packets have appeared.

Exact failure: ordinary staged/consumed/effect reads0, but ordinary scope begin/
end1 and input batch/identity/refresh1. Independent water receipts show one
staged/consumed packet and one native water scope. Source review confirms water
events incremented counters labelled ordinary. The draw/effect scope itself is
balanced; the family domain of the accounting is wrong. It must not be ignored
as harmless startup activity or avoided by running until the sample shifts.

Host167 partitions begins/ends by scope family and deduplicates ordinary queue
identities before adding water to the SAME input publication. Ordinary batch/
identity/refresh receipts now describe only batches containing ordinary inputs;
water still publishes, refreshes and draws through the shared owner. No rendering
branch, existing scenario threshold or comparison changed. C++ output81/CPU53
passes water-only/mixed/duplicate identity fixtures;402 Python checks pass,
including a causal scenario regression that still rejects the old contamination.
Host167 compiles/links with no guest objects or shader changes.

Host167 EXE49139712B SHA256
8FAA6A8A66E157B7EF3B354073A2B5C438BDF0F6E6017F589572B1A75D3D8E51;
PDB112099328B SHA256
1D4B2ACD5FF9D2C33C3B546E0C5CCCF1313C589CB6DD1B348712931C8AABF145.
Output fixture1847296B SHA256
CBA85F4D8F3B81EE1FBF3CA247174C258D94F37E071C1F77058AF2254954DD2B.

## Corrected run975: scoped cold evidence, reload failure

Same180s/800KiB/192MiB guarded operator and settings, PID35800. No images,
raws, perf files or new caches. Supervisor stopped at15:55:29 and restored the
exact profile. An earlier sandbox stop attempt was denied; a subsequent approved
check found the process already terminal. Log531187B SHA256
4E7BFEE8952482F056934299C9C75E2928109D2708D5CE62C16AAAC9BB603517.

Before generation93 source retirement, cold2163..2463 adds2784 matching native
evaluations/publications, unavailable0, and1111 blends/53370 shared render reads.
Skin scene2096..2396 emits/fence-retires11813; shadow emits5676/retires5720 (some
earlier submissions retire in this window). The corrected ordinary deferred
gate2100..2400 passes7750 packet/effect reads,1625 paired scopes and289 input
batches/905 identities/289 refreshes. These are explicitly the observed cold
epoch, not a truncated log offered as whole-run acceptance.

Full run975 FAILS the requested reload. Readiness starts at selected scene755/
shadow756. At15:53:36.565, a260432us poll gap resets walking and the window at
1598/1599, only843 of900 new emissions. A new window starts1617/1618, then
autoplay encounters a field transition at1860/1861. Generation93 retires; stage
changes tobg01_01 without the required title request/return. Readiness logs
identify this exact interruption; they do not establish its scheduling cause.
The900-emission and250ms freshness requirements are unchanged. No additional
unchanged retry, combined PASS claim or fabricated causal performance fix.

The skeleton path remains opt-in. Source/fixture work on the next native channel/
late-writer boundary can continue, while fresh complete reload and motion/pixel
qualification stay open. Run974's and975's different failures are both retained.

## Remaining interfaces and acceptance

Original animation curves/channel production, object/collision/effect side
effects, secondary/unskinned/camera-facing/partial-walk routes, outgoing palettes,
late-writer source copy and legacy draw interpolation remain. Native model/skin
asset ownership and GPU consumers are reused, not a replacement whole-game
animation framework. No speedup, achieved60FPS, complete host frame, animated
pixel parity, sequence, both-eye or Quest qualification is claimed.

The cumulative storage ledger remains
[native-scene-state-bridge](20260906_0333_native-scene-state-bridge.md), section
"native skeleton host integration after17f5c33". Preserve the971/962 window vs
logged-field discrepancy; no unchanged PrintWindow retry or new raw/image allowance.
