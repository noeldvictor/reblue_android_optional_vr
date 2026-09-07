# Native model-node associations and the first concrete rigid target

2026-09-07, parent `44cb12d`, existing Windows Vulkan-only desktop tree.
The previous turn was verified progress and pushed. This checkpoint removes the
missing load-owned model/instance-to-primitive association, with a live native
bounds consumer. It does not complete direct scene/shadow submission.

## Source and ownership

Read the complete generated `bdSceneGraphNodeProcess` in
`generated/reblue_recomp.46.cpp:9719`, relevant loader/physical/predictor hooks,
existing model publication, pose handoff and host walk. The processor copies the
36-byte mesh header (including sphere), resolves node matrix indices, processes
children and relocates sibling links. Existing full-build/full-destroy hooks
publish/retire native model programs independently of optional PSO precaching.
No generated source or hook TOML changes, asset extraction or new decompiler.

- Load traversal now records every geometry node's matrix-index/mesh association,
  including shared meshes. `NativeModelRenderData` resolves those import keys
  once into references to owned programs; its public lookup has no source keys.
  Duplicate matrix indices are ambiguous, never arbitrary first/last-writer wins.
- Mesh spheres join the immutable material/geometry program. Finite/nonnegative
  source bounds are copied at load; missing bounds stay unavailable.
- `NativeInstanceRegistry::Create` accepts the actual model lease, checks its
  generation, and publishes it with each immutable pose. Model retirement,
  source reuse and later pose writes cannot repoint an existing reader's data.
- Node storage and retired-but-pinned models remain charged to the existing
  bounded registry. No ownership cycle: node pointers borrow programs within
  the model, while external aliasing handles/poses pin that model.
- `FindNativeInstanceNode(pose,index)` selects the owned program without source
  graph, mesh/buffer keys or palette addresses. Host culling consumes its bounds;
  source bounds are read only for explicit comparison or unavailable native data.
  The source tree still supplies traversal/visibility/node indices. Existing
  replay/material consumers still use `NodeTag` and source buffer/range matching.

The repository guest-source/devloop skills kept investigation source-first and
verification in existing fixtures, incremental targets and bounded field runs.
No parallel renderer or draw-capture cache was added.

## Tests and actual runtime evidence

Material19/PID15800 builds; CPU17/PID28244 passes in 0.10 s behavior /0.11 s CTest.
New cases cover shared nodes, source destruction, pose-pinned models, generation
reuse, ambiguity, missing/out-of-range associations, transactional traversal and
node residency backpressure. Existing material/instance/light/fog tests remain.
238 Python guards/scenario tests pass in 0.050 s and eight runner tests in 0.007 s.
The new `--model-nodes` scenario gate rejects stale/startup/wrong-scene samples,
missing checks, resets, mismatches hidden by later success and oversized logs.

Host74/PID28140 passes: CMake refresh, up-to-date codegen probe, host objects/link
only. No guest objects or shaders rebuilt. Exe48,338,432 B/PDB107,388,928 B;
exe SHA256 `7C9F82CC452818E186F1B8C80D5B79C6E4E3C901FC93BC80D0F5BFD6503FCDCA`.

Run919/PID8932,03:03:12..03:04:11 EDT: all14 temporary settings effective,
flat1920x1080 native4xMSAA, precache/pulling on, material/instance comparisons on,
normal texture tables, readiness-driven walking, capture/perf off. Post-event
contexts2042/2342 plus fresh following samples pass the complete existing field
gate. Native bounds add1,761,600 reads and comparisons, unavailable0/wrong0.
Poses+100,401; canonical draws+122,160; load-owned geometry draws+29,515;
shadow/policy checks+15,144; explicit draw commands+198,373. Movement adds30
observations/+37.791045 units. No new cache/perf/dump/raw files.

Inspected `out/verification/native_model_nodes_window.jpg`:1920x1080,128,634 B,
SHA256 `686666C23AB5828263CBAAF91A709E2741691ED03D0D13383C82CDBD6896F2A2`.
Shu running, ground/fence/structure/vegetation and cast shadows are coherent.
Known cliff marks and distant blur remain. One image is not sequence stability
or both-eye qualification. The image belongs to host74, not a later binary.
Run919 log233,579 B, SHA256
`407C4ECB57255FA6DB578B83DAD63E8AE83CE2521AD332842B21DA90A3D799F3`, was retired
after run920 replaced its text gate; this image and these observations remain.

The initial candidate filter yielded zero IDs. It required one primitive and an
explicit empty skin command; that is not evidence no static candidates exist.
Host75/PID28804 passes4.298 s after changing **only bounded candidate diagnostics**
to expose primitive count, actual technique and unknown skin state. Rendering/
ownership code is unchanged. Exe48,338,944 B, SHA256
`17C08409D4D1DCD93E23B7C81C1027E0E1F5A0F46400EF3B0F34BABF60137898`;
PDB107,388,928 B. No extra image was justified for this diagnostic-only change.

Run920/PID15876,03:08:48..03:09:47, same14 settings/full field gate, text-only.
Contexts2049/2349 and fresh following samples pass: native bounds+1,761,600
reads/checks, unavailable0/wrong0; poses+100,482; normal table lookups+29,988
with zero original comparisons/fallback; canonical draws+123,849; native geometry
draws+29,530; shadow/policy checks+15,145; explicit draws+201,528. Movement adds31
observations/+39.002439 units. No source-free GPU load, live cull-policy change,
compound refresh or native shader route was exercised.
Current log235,282 B, SHA256
`4C15C400A34F1D6699BF788D2156B59878E0D51A9F538198249176F2087251F9`.
Both runs restored the exact116 B profile, SHA256
`2F1BC38D763A1B7BDBA31F560684FD4AA7E42A714600B8D344F19DA7F38E23B0`.

Final host76/PID29896 passes3.201 s after guarding the diagnostic call itself,
so verification-off rendering does not evaluate its source technique arguments.
The verified-on behavior is unchanged; no further game run/capture. Host76 exe
SHA256 `DEBDDB399C8E096EEFB359202D953C9DA8F40836896E48C4CF77DB4F2D3AF17A`,
same exe/PDB sizes as75.238 Python checks pass again in0.049 s. Runtime evidence
remains tied explicitly to74/75, not restamped as76.

## Actual next target, not a completed direct object

At03:09:32.934, after interactive `bg41_01` readiness, view3/technique0:

| Model-local node | Primitive count | First geometry content ID | Material content ID |
| --- | ---: | --- | --- |
| 48 | 3 | `30931DB266E1EA94` | `EF2B8C178E636AB9` |
| **64** | **1** | **`258694267A8DBAEE`** | **`63B8D67932573E51`** |
| 65 | 3 | `3A7E1F8E1FAF4E04` | `EF2B8C178E636AB9` |
| 4 | 14 | `FCA411E16FD37CAB` | `EF2B8C178E636AB9` |

First three observations are instance144/model generation93; last is143/92.
These runtime numbers are not persistent identities. Select node64's sole
primitive by its stable content IDs for the next end-to-end integration.
Local sphere approximately(-391.72,-122.31,-196.81), radius962.05; its visual/art
name has not been identified. No hard-coded source address is an asset ID.

Read the existing68 B `native_materials/v1/63b8d67932573e51.bdmat`, SHA256
`0319E777F8F5F2E4C022219ABB51B9FDBD2E1244878B3099949984E51A1796D4`.
Lighting model0, flags20: diffuse modulation **off**, no multiplier, known black
specular and power0. `ComposeNativeMaterial` therefore uses the live object color
directly; an absent multiplier is not a missing diffuse result in this mode.
The skin command is unspecified, not explicitly empty. Canonical cooking rejects
nonzero-position-index deformation streams; still verify the selected schema,
input defaults, UV meaning and live flags before declaring shader eligibility.

No native mesh file exists for geometry`258694267A8DBAEE`: current verification
disables persistence, so canonicalization occurs in memory. The existing3510
mesh files remain36,510,144 B. The material is persisted; the selected geometry
has **not** passed source-free disk/GPU loading. Reuse the loader's CPU payload
and existing bounded writer to persist/validate only this target, not a bulk
recook or GPU readback. Then connect owned object color/UV/image/policy inputs and
actual light/fog producers to the GPU-tested rigid shader programs/shared cache.
Route the fully supported node before interpreter/replay with whole-node preflight;
retain unsupported siblings. Cold load, native updates, scene/shadow output,
teardown/reload and both-eye game pixels remain required. No FPS gain is claimed.

## Storage

Same cumulative ledger/cap/floor/raw0 in `20260906_0333_native-scene-state-bridge.md`.
Known comparable retained fixture/log/runtime/image growth121,979 B; host exe/PDB
growth136,192 B. Other objects, metadata, source and helper deltas lack complete
baselines. One material tree and one field log remain; no new build tree/tool/
cook/raw archive. Keep run920 text and run919 image by their separate verification
purposes, replacing them when the next equivalent/direct-object checks qualify.

Completed cleanup: nine superseded outputs,604,999 logical B, **618,496 B
(604 KiB) measured reclaimed** across two immediately measured deletions.
Removed run918 log/image, material18/CPU16/host73 stdout/stderr, then run919's
full text log after920 replaced the same field gate. Old images/logs are gone;
reports/hashes remain and build logs can be regenerated. Native shader/snapshot
GPU evidence, baseline/motion/protected raw sets and game data remain untouched.

Drive free space fluctuated independently of the small measured artifacts; a
scoped cache/perf/dump inventory found no new files. Before the second run,
growth exceeded the initial compile-only overlap estimate, so producers were
stopped/reconciled and a per-run192 MiB drive-growth stop was added alongside the
original floor. Cleanup-end free63,315,546,112 B (58.97 GiB), drive-wide gain
33,120,256 B from first63,282,425,856 B; only618,496 B is credited to cleanup.
No active renderer/build producer remains. No budget reset or repeated savings.
