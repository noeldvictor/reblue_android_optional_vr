# Shared animated material UV ownership

2026-09-09. Production source dd6a2dc; test migration and connected verification
follow its explicit WIP checkpoint. Host190 banner dd6a2dcdirty. No behavior
change after the tested build (only boundary precondition comments were clarified);
no prior binary/live evidence is restamped.

## Connected outcome and boundaries

Native controller channels -> native UV evaluation -> immutable NativeMaterialUVs
in NativeInstanceRegistry -> late eye patch -> next controller -> actual native
object/material composition and scene packets. The fixed eye-only publication is
replaced, not retained alongside a second source-address cache or renderer owner.

The native value is a bounded sparse, sorted list of model-local slots with
selector/channel, UV, enable and writer provenance; full table count is retained.
The existing source index carries only association with the outgoing table.
Instance/model generation governs publication and lifetime. Pinned allocations
remain charged under the existing16MiB registry budget through replacement,
instance retirement and registry destruction. Identical bitwise publications
reuse their lease. Refused replacements clear stale visibility without replaying
the original producer's side effects; the explicit current-source route remains.

Controller scrolling consumes validated prior native offsets when available;
translation/rotation consume native channels. Eye publication patches the first
two slots and retains untouched later controller slots in that same owner.
Actual ReadMaterialTextureInputs consumes owned UV values, and composition tracks
the provenance actually selected/reset per channel. No per-draw UV re-import as
native material input. Remaining table reads validate the outgoing adapter.

This is not all material ownership. Authored rates/divisors, table bindings and
joint-driver selection still cross the source boundary. Eye caller scratch/table
exports, timeline/catalog inputs, other effect callers, viewer/image/late writers
and unsupported material routes remain. Unknown writes/rebinding invalidate the
entire shared publication until republished; old matching bytes cannot revive it.

Contracts are reused from the fully audited effect/parser/binder and all three
eye callers, rather than another unchanged whole-function audit:
[effect transaction](20260908_2305_native-effect-animation.md),
[authored parser/binder coverage](20260908_2331_effect-input-coverage.md),
[eye and late caller order](20260909_0000_native-eye-materials.md).
Relevant render_tweaks TOML was reread; no hook/codegen/shader input changed.

## Verification

423 artifact-free Python source/scenario checks PASS0.232s; wiring only.
Existing C++ material fixture76/PID25344 builds successfully, CPU73/PID29188
passes0.12s (CTest0.13s). Tests use production clip import/controller, instance
publication, checked source association and actual material importer/composer.

- Three UV modes feed native material values; exported UV reads inside the
  importer throw, and next-tick scrolling also forbids its exported offset reads.
- Eye patches preserve a third controller slot, combine provenance in actual
  material order, and feed the next controller with new eye offsets.
- Unknown third-slot writes invalidate the whole publication; restoring bytes
  cannot resurrect it. Existing eye/table/count/selector/channel/enable, missing
  input, overflow, nonfinite, subnormal and reload regressions are preserved.
- Sparse slots up to255, duplicates/order/bounds, provenance and signed-zero
  equality, pinned retirement and exact accounted budget/refusal/retry are tested.
  Pinned values survive source/controller/registry destruction.

Host190/PID35952/session69911 builds through link step32, no guest objects or
shader compilation; codegen reports0writes. EXE49382912B, PDB113823744B;
EXE SHA256829B80D48E8126D14F069F5873F72F049F4323339E151F74A3D7AAAE8ECC1CC7.
Fixture1537536B SHA2561FA2EF3A8D9E0D74697877A52F94A7F725D44FA6D5B40EED44F90DE315858DC6.

## Purposeful live observation, not complete desktop acceptance

Host190/PID38184/session94844,00:36:06..00:37:02. Existing supervisor flags:
NativeImages, ModelMaterialsVerify, ModelGeometryVerify, InstanceVerify,
AnimationProbe, ControllerAnimationProbe, LateAnimationProbe, EyeMaterialProbe,
AnimatedUVProbe. Mono, capture/perf/cook off; native scene/skin/deferred enabled.
All14settings took effect. The116B owner profile restored byte-for-byte, SHA256
2F1BC38D763A1B7BDBA31F560684FD4AA7E42A714600B8D344F19DA7F38E23B0.

The added observation requires the unchanged eye/controller/late regressions,
positive native scroll-input reuse and increasing UV material packets after two
idle bg41_01 contexts. Requested observation reached; supervisor intentionally
exits1 through diagnostic-stop/finally, not a CTest success code or crash.

- Animation-material1982:1922publications,refused0,1918owned scroll inputs.
  Earlier1682:1622publications,1618owned inputs; both counters advance300.
- Eye1982:959completed/959exact comparisons,refused0,225off-center.
- Idle contexts1662->1962: UV/eye scopes632->932,composed/packets27808->41008.
  Owner1647->1947:1588->1888publications,2201->2801reads,changed0.
- Skin1664->1964:emitted/fence-retired33513->46713. This separate whole-skin
  counter does not identify eye-specific GPU draws or prove pixels.
- Controller1982:94106matching,15636boundary refusals,11955samples,23mixes,
  11457interior,11945handoffs,changed0,12447advancing. Late6matching/reused/
  handoffs. Skeleton1947:12585matching,unavailable0,wrong0.
- Material1962:19333checks,wrong0. Effects1982:963matching,1924UVs.
  Live translation/rotation/queued transitions and eye-preserved non-eye slots
  remain0. Synthetic C++ coverage does not qualify those missing live routes.

No new raw/window/perf/cache/dump files. Motion pixels, reload/stereo, full host
frame, every scene and Quest2 acceptance remain open. No measured speedup claim.
Next publish authored material/joint bindings into existing owners and migrate
remaining UV/image writers before deleting their final outgoing checks/exports.

## Retention and storage

Full253093B log retained losslessly in retained-animated-uv190.zip51516B.
ZIP SHA2565CF0DDB4FABA6AEA3850951360417EF4583F415CAC3D2EF31C84CBA9AAC0D7C8;
sole member reblue_981.log SHA256
0AEAAC79E344B9DA6F9D32C113287DFAA2EA3B8DA39D2AE8CF3391A8CC5D53BE.
Member name/length/full hash verified before plaintext removal. This replaces
eye188's eye/consumer verification purpose; its ZIP was removed, historical
results remain documented. Keep effect187, placement988, root985 and earlier
unresolved reload/pixel evidence. No new raw allowance or budget reset.

Six superseded75/72/189 receipts plus plaintext and old eyeZIP removed:
8files311863Blogical,319488B measured deletion minus53248B newZIP allocation =
266240B net260KiB cleanup. Current plaintext is recoverable from the ZIP;
superseded receipts are reproducible, prior188raw log is no longer retained.
Partial retained growth260661B:fixture+163674,EXE/PDB+96256,ZIPreplacement-2563,
buildlogs+3294. Excludes host objects/metadata/source/Git and drive-wide activity.
Post-cleanup63559647232B free (~59.2GiB),drive-wide+17055744B from first check;
only the measured deletion/allocation is claimed cleanup. Diagnostics78149834B,
next400KiB overlap has83766B headroom below75MiB; fresh preflight mandatory.
See the [cumulative ledger](20260906_0333_native-scene-state-bridge.md#2026-09-09-shared-animated-material-ownership-afterdd6a2dc).
