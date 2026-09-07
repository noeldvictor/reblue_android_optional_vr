# Owned object primitive inputs

2026-09-07, parent `91d86ce`, existing desktop Vulkan/OpenXR/PCH tree. This
checkpoint removes replay/source-key selection from native object packet
assembly and repeated color reads from supported ordinary material composition.
It does **not** connect the native rigid shaders to game draws.

## Source-first contract

The repository guest-source/devloop skills kept this work in generated C++,
owned-data fixtures and the existing incremental host tree. No guest rewrite,
decompiler, broad asset recook or new test tree was needed.

- `generated/reblue_recomp.88.cpp`, `bdSceneTreeDraw` (9525 onward): copies
  visual color from +3004 to +3404, then runs setup callbacks before traversal.
  Publish the final +3404 value at traversal entry, not the earlier input.
- `generated/reblue_recomp.40.cpp`, `bdSceneNodeDrawSingle`: ordinary color
  reads at11148,13765,14198; mode11/special routing stays excluded by the existing
  object texture reader. The whole4k-line function was not re-audited here.
- `generated/reblue_recomp.9.cpp`, `sub_82173960` (3659 onward): reads the supplied
  float4 into shader staging and marks it dirty; it does not mutate object color.
- Existing physical-buffer/pso hook definitions and host traversal were inspected.
  No hook or generated file changes. Source-to-object publication remains an adapter.

## Implementation

`NativeMaterialObjectInputs` owns finite color and the shininess-write flag.
The checked reader imports them once per supported object scope. `Walk` now
passes its already-found pose to that scope; no second pose lookup is added.
Normal `EvaluateNativeMaterial` uses the snapshot; explicit verification compares
all consumed snapshots with current source values. Unsupported scopes retain
the old adapter and are counted, rather than silently claiming conversion.

`NativeObjectPrimitive<Image>` pins the pose/model, geometry, material and image
leases and copies the world transform, material values, UVs and primitive/shadow
policy. `BuildNativeObjectPrimitive` selects by owned pose/node/primitive ordinals,
not `NodeTag`, graph/mesh/buffer keys or shader registers. Exact live pose identity
is required by `FindNativeObjectPrimitive`, preventing instance/lane crossover.

Shared `PrepareMaterialMesh` now accepts an immutable native program. Its legacy
source-key adapter is explicitly `PrepareReplayMaterialMesh`; both routes share
the same prepared values. Model control-block identity pins program pointers;
the separate replay alias index is limited to4096 entries/128 B each within the
existing4 MiB scope budget and depth4 limit. Actual vector capacities are checked.
No duplicate material cache or unbounded packet archive is introduced.

Unknown texture/policy channels and material write masks remain explicit. An
owned packet is **not** permission to choose a shader, drop deferred siblings,
guess missing lights/fog, or reuse the translated instance-record layout.

## Checks and actual run

245 Python boundary/scenario checks pass (0.051 s execution). New checks cover
owned selection/shared preparation, checked color import and fresh complete
comparison windows. Material20/PID31720 builds in6.010 s; CPU18/PID27944 passes
0.10 s behavior/0.11 s CTest. Packet fixtures cover destroyed source storage,
scope/model/instance retirement, generation/source reuse, malformed ordinals,
nonfinite values and last-lease release. Geometry is an opaque aliased lease in
this CPU test, never dereferenced; actual GPU geometry coverage remains separate.

Host78/PID31256 passes: up-to-date codegen, host objects/link only. CMake's
`GLOB mismatch` is its refresh for the new header, not a build failure. No
guest objects or shaders rebuilt. Tested exe48,359,936 B/PDB107,626,496 B;
exe SHA256 `5724F295E924A3C209F443CD29848341E26F53A610C5D75E6A2D049F587296EC`.

Run922/PID29112,04:05:04–04:06:01, flat1920x1080/native MSAA. All14 temporary
settings took effect: autoplay, native instance/shadow/material/policy/texture
paths, host materials, pulling/precache enabled; materials verification on,
texture-table verification off; raw captures and perf CSV disabled. No selected
cook. The bounded wrapper restores the original116 B profile byte-for-byte in
`finally`; SHA256 `2F1BC38D763A1B7BDBA31F560684FD4AA7E42A714600B8D344F19DA7F38E23B0`.

Fresh `bg41_01`, view3 post-event windows: opening event frame1445, comparison
contexts2045/2345, field-state0/player1/event0/movie0/loader0/icon0. Deltas:

- Object publications+33,600, owned color reads/checks+79,716, zero wrong/unavailable.
- Instance reads/checks+113,514; model bounds reads/checks+1,761,600, zero wrong.
- Canonical draws+145,428; load-owned geometry draws+49,280/checks+1,686, zero wrong.
- Shadow checks+15,352; material image/UV, primitive policy, named lighting,
  explicit bindings and native pulling gates all pass. These remain the existing
  translated game shader route, not new native shader submissions.
- Movement31 samples/+37.499395 units over12.5 s walking.
- Zero cache writes, source-free GPU loads, new raw frames, perf CSV or shader dumps.

The four existing bounded candidate diagnostics now assemble owned packets.
Selected geometry `258694267A8DBAEE`/material `63B8D67932573E51`, node64/primitive0:
material mask3, image mask0001, known UV offsets `(0,0,0,0)`, diffuse `(1,1,1,1)`,
known direct/non-deferred/non-alpha participation. These are observed live values,
not asset defaults. Its canonical UV0 remains16383..16895; the related cached
shader's `(uv+1)/512+offset` has **not** been proven as this exact variant. Do not
bake a guessed conversion or white color. The previous17,572 B asset is unchanged
(SHA256 `9C14828E26FBD3D40D40D3A01A7B011249B4A66E116CAFE4B7CAF578A877BB66`).

Actually inspected `out/verification/native_object_inputs_window.jpg`,1920x1080,
quality60,137,491 B: Shu running beside the fence/blue bell, readable terrain,
vegetation and character shadow. Known black cliff marks/distant blur remain.
One sanity image is not sequence stability, exact selected-object identification,
both-eye/full-game qualification or a measured speedup.

Evidence hashes: run922 log223,889 B SHA256
`054E696F66C93CEEF9A8A44720D43335EBB21633B71D0F52D883E34ECB7DEF9B`;
image SHA256 `5228AD577396C9D0AA9698005F6F0D1F41AC28394971A61227129D89155BF24F`.

## Retention and next consumer

Cumulative storage accounting stays in
`20260906_0333_native-scene-state-bridge.md`; original3 GiB exception and raw0
gate unchanged. After replacement checks/pixels passed, removed8 exact
superseded artifacts: material19/CPU17/host77 stdout/stderr, run921 text and
run919's model-node sanity image.370,066 logical B; measured376,832 B (368 KiB)
reclaimed once. Build logs can be regenerated; those exact historical runtime
files are gone, while their reports/hashes and the selected cooked asset remain.
Distinct baseline, GPU/failure, motion and raw evidence is preserved.

Known retained growth270,552 B: material fixture+70,985, build logs+1,404,
replacement image+8,857, replacement field log−12,422, exe/PDB+201,728. Retention
adds object-packet lifetime and color-consumer coverage; replace passing outputs
by purpose at the next equivalent checkpoint. Other object/metadata/source/Git
deltas are unknown. Cleanup-end free63,266,369,536 B (58.92 GiB), drive-wide net
use+20,201,472 B (19.27 MiB) from this turn's first measurement; not all attributable
to the task. No owned producer remains and no new asset/cache tree was created.

Next: consume this packet with exact material-family UV/shader flags and owned
live light/fog records; preflight the complete node and submit native scene and
shadow records through the existing backend. Then prove cold-load/reload with
this family's interpreter, template capture and replay explicitly disabled.
The source publication boundary, replay alias index, translated shaders/instance
gathering and remaining game paths still exist. Quest work remains gated on full
desktop qualification; no FPS gain is claimed.
