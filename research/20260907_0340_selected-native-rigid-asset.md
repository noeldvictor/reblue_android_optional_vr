# Selected native rigid asset and retained shader input

2026-09-07 EDT, parent a4b4555. Previous turn: verified progress and pushed.
Repository guest-source/devloop skills kept this in existing source, fixture,
cache and incremental desktop build. No guest/shader rebuild or new build tree.

## Ownership change

`NativeGeometry` now retains `rigid_vertex_input`, constructed from the canonical
CPU payload during upload for both import and `LoadNativeGeometry`. It uses the
GPU-tested production shader schema and existing bounded input library. Missing
inputs stay explicitly unavailable; translated consumers remain unchanged.
No per-draw declaration lookup or GPU upload-memory readback was added.

Optional `bd_native_mesh_cook_target` defaults empty and accepts one exact nonzero
16-digit content ID. During material verification's persistence suppression,
only that canonical asset can write, capped at2 MiB by the existing disk writer.
The256 MiB/16384-file aggregate cache/reserve/leases still apply. No path/glob,
bulk recook, second cache allowance or output on invalid selection. Normal
persistence remains unchanged. A read-only `--inspect` mode in the existing mesh
fixture validates one bounded asset and reports native schema/value ranges.

## Verification and exact asset

Mesh06 first failed from Windows min/max macros. Mesh10 built after fixing the
calls; CPU09 caught a new scratch directory interfering with an older inventory
assertion. Separate private scratch state fixed it. Mesh11/PID25416 and CPU10/
PID30208 pass0.12 s behavior/0.13 s CTest, including exact selection, malformed
IDs, byte caps before writes, reuse timestamps, previous storage/cook/lifetime
tests and symlink ancestor checks.240 Python guards/scenario tests pass0.053 s.

Host77/PID28004 passes: codegen up-to-date, host objects/link only. Exe48,346,624 B,
PDB107,438,080 B; exe SHA256
`3265CFF832D1EB6C0E1B6F5065EE95120E00727EF663D44C3F9BEF371A4EADE8`.
Mesh fixture exe260,608 B, SHA256
`4E06965644D60794DB94304206FCF56C9D5E85FDB088F528FF1381E3CA791ED4`.

Run921/PID27672,03:34:38..03:35:38: flat1920x1080/native4xMSAA, precache/pulling
on, all15 temporary settings effective. Full existing post-event bg41_01 text
gate passes at fresh contexts2029/2329 (opening event1429). Bounds reads/checks
+1,761,600 unavailable0/wrong0; poses+113,082; canonical draws+144,336;
geometry draws+49,008, geometry comparisons+1,687 wrong0; policy/shadow checks
+15,360. Movement30 samples/+37.772134 units/13.131 s walking. No source-free GPU
load, direct shader route, live cull-policy change or compound refresh exercised.

At03:34:57.268 selected geometry258694267A8DBAEE persisted true/reused false/
rigid input true. Existing cache now3511 files/36,527,716 B: exactly one write,
zero failures/refusals/conflicts. The field candidate remains node64's sole
primitive, material63B8D67932573E51, view3/technique0; runtime IDs144/93 identify
this observation, not an asset identity. Disk file:
`out/build/win-amd64-release/cache/native_meshes/v1/258694267a8dbaee.bdmesh`,
17,572 B, SHA256 `9C14828E26FBD3D40D40D3A01A7B011249B4A66E116CAFE4B7CAF578A877BB66`.

Independent fixture inspection, with no game/source buffers/GPU: content ID
valid,162 vertices/474 indices (158 triangles), stride96, native rigid input1.
After discarding CPU mesh bytes, retained input still has locations0..3/slot0
and zero translated unpack masks. Actual six attributes:

| Semantic/index | Offset | Observed values |
| --- | ---: | --- |
| Position0 | 0 | x -1057.99..274.553, y -130.354..-114.26, z -1074.76..681.146, w1 |
| Normal0 | 16 | y0.92053..1, w1; full finite xyz normals |
| Tangent0 | 32 | all zero |
| TexCoord0 | 48 | xy16383..16895, zw0 |
| TexCoord2 | 64 | all zero |
| Color0 | 80 | constant approximately(0.0117647,0.0156863,0.0745098,1) |

UV values are not yet sampling coordinates. Existing dump
`hlsl_dump/bd_normal_cs_vs_norm.hlsl:953` and:959 computes `(uv+1)/512+offset`;
the constant0x3B000000 is1/512. Read its complete main and the material offset
publication. This is a related-family lead, not this object's exact live shader
proof: the original normal/CS-normal VSO container hashes differ, and no exact
normal VS dump was found. Do not bake this conversion, enable vertex color or
normal mapping merely from attribute presence. Resolve the actual family flags,
then compose owned object color/UV/images and live lights/fog, direct scene/shadow
submission, interpreter/template-free cold-load/reload and both-eye game pixels.
No compatibility consumer is deleted yet; native shader game submission remains
the next outcome, not more generic adapters or broad asset cooking.

Run921 log236,311 B, SHA256
`06506AD321F602A3A8CAE9485C41B4E69FA8E16D51266E3A4B81F246C4A55017`.
No new raw/image/perf/dump. Exact116 B profile restored, SHA256
`2F1BC38D763A1B7BDBA31F560684FD4AA7E42A714600B8D344F19DA7F38E23B0`.
No game render consumer changed, so no duplicate image was taken for persistence
and an unused input handle. Run919's inspected image stays tied to host74, not
restamped as host77. This is not new sequence/both-eye or performance evidence.

## Storage and retention

Same cumulative ledger in `20260906_0333_native-scene-state-bridge.md`:
original3 GiB exception/floor62,509,998,080 B/raw0, no reset. First current free
63,298,228,224 B; run-end62,976,528,384 B. Scoped caches/dumps/perf initially had
no new files despite greater drive-wide changes; per-build256 MiB and per-run
192 MiB growth stops supplement the original floor. No unrelated-drive cleanup
is claimed. Selected17,572 B file is retained for the next direct consumer; no
duplicate payload/recook is needed for read-only inspection. Keep the current
mesh fixture and one current field text log; replace equivalent logs after
validation. Protected raw/motion/GPU failure evidence and game data are untouched.

Completed cleanup after replacement checks passed:13 superseded logs (mesh
build09/06/10, CPU08/09, host76 stdout/stderr and run920 full text),241,190 logical B.
Immediate volume check63,316,140,032 ->63,316,393,984 B:253,952 B/248 KiB actually
reclaimed once. Test failure causes above remain; build logs reproducible, old
field text gone but prior report/hash retained. No protected GPU/raw/image data
deleted. Current mesh11/CPU10/host77 logs, run921 text/run919 image remain.
Known retained growth147,821 B: fixture70,466, logs1,922, field replacement1,029,
native asset17,572, host exe/PDB56,832. These add the selected native asset/input
coverage; other object/metadata/source/Git changes are unmeasured. Ending free
58.968 GiB, drive-wide gain18,165,760 B, not all attributable to cleanup. No active
producer or temporary profile remains.240 Python checks pass again0.049 s.
