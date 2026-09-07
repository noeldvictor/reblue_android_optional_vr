# Geometry-owned runtime vertex inputs

2026-09-06 EDT. Parent `bfc324e`; Plume unchanged at `3094b35`. The preceding
read-only review established the next dependency, not an implementation result.

## Boundary changed

`NativeGeometry` now pins a content-shared immutable `NativeVertexInput`, built
by the geometry importer, not from a captured draw. It owns input elements and
semantic-name bytes, stream participation, derived pulling entries and a
separately named **temporary** translated-shader decode contract. The library
has 8 MiB/8,192-owner limits, validates before publication, checks hash collisions
and reuses existing entries at capacity. Store lifetime pins queued PSO inputs.
Field residency is only **two inputs /3,824 B**.

For converted dispatch, `host_draw.cpp` clears `PipelineState::vertexDeclaration`,
publishes the explicit native input and binds geometry-owned strides. Pipeline
creation, shared shader parameters and vertex-pull staging use that owner.
Engine draws clear native input ownership rather than inherit it. The pulled
pipeline also uses an owned zero-input description. Its filler semantics still
come from the existing translated-shader ABI, not a finished native shader.

The raw-hashed pipeline grows 158 ->166 bytes. Reviewed its precache/capture
consumers: the appended field is runtime-only, so legacy generated CSV/header
entries default it to null. `RecordPipelineState` explicitly excludes native
rows; no console declaration hash is fabricated, and no generated guest or
shader cache regeneration is needed. Background native PSO requests reuse the
existing queue, whose lifetime is covered by the geometry input store.

**Not completed:** self-describing `.bdmesh` layouts, canonical vertex packing,
named native shader parameters, native object/texture/pass contracts, removal of
`NodeTag`/source indexing and draw templates, and direct static scene/shadow
submission. Packed bytes and old shader masks are unchanged. The new runtime
owner is a dependency removal, not the final native asset/shader API.

## Verification

- Mesh build 04 PID 24120 and CPU 04 PID 17576 pass (CPU 0.10 s). The production
  fixture destroys source elements/name storage and the library, then exercises
  native IA/decode selectors with legacy readers that throw if called. It checks
  pulling metadata, synthetic zero attributes, identity/reuse and byte/count
  budgets. Existing topology, payload and disk-storage checks still pass.
- Draw-intent build 01 PID 31000 and test pass (0.03 s), including clearing stale
  native ownership when returning to engine intent.
- **152 boundary checks and 23 scenario cases pass.** Text checks supplement,
  not replace, behavioral and runtime verification. The new scenario modes use
  two consecutive fresh post-event field windows; pulling requires positive
  native pulled-record deltas rather than merely enabled cvars.
- Host build 60 PID 29620 passes, Ninja elapsed **44.663 s**: the pipeline/header
  change rebuilds host consumers, not guest objects or shader payloads. Exe
  48,186,368 B, SHA-256
  `dc225af11729753fe83344e65d5774722e5de1ee1dfa55192ee762b69097ab58`.

Both bounded runs use normal flat MSAA, autoplay, material/geometry/pose checks,
native texture tables with original table comparison disabled. All ten profile
settings audited; 75 s/400 KiB log limits, raw/perf off, mesh persistence off via
the material diagnostic, owner profile restored exactly (116 B). No retries
failed and no Quest/XR run was made.

| Run | Configuration and fresh post-event evidence |
| --- | --- |
| 907 /PID 27672, 20:32:14..20:33:12 | Precache off: 170,037 native pipeline uses and decode blocks; zero pulling. 119,041 matching poses, 22,330 normal table lookups, 51,806 load-owned geometry draws /2,700 matching geometry checks, diffuse/specular 15,423/14,683 matching checks. |
| 908 /PID 26572, 20:38:03..20:39:00 | Precache on: 170,020 native pipeline uses/decode blocks/**pulled records**. 118,987 matching poses, 22,333 normal table lookups, 51,785 geometry draws /2,700 matching geometry checks, diffuse/specular 15,426/14,686 matches. |

No pose misses, table fallback/refusal or geometry mismatch. The separate
unconverted replay lookup count remains 761; not all submission is load-owned.
908's recent queue report averages 320 incoming ->313 issued draws per flush,
three instanced groups covering nine draws, with pulled and indirect dispatch
active. Queue counters have their existing accounting scope; the 170,020 counter
specifically observes **native input staging**, not a full guest-free frame.
Zero pulling in 907 was expected: precache-off suppresses the instanced/pulled
twins. 908 explicitly exercises the changed GPU path. No speedup is claimed.

Both 1920x1080 JPEGs were inspected: Shu, terrain/rocks, foliage, fences and
shadows remain visible without an obvious new geometry/texture break. Distant
blur remains. These standing images do not qualify motion, sequences, reloads,
authored effects, both eyes or the complete desktop gate. Autoplay still waits
150 s before walking; its readiness-driven replacement remains pending.

Retained evidence:

- `reblue_907.log`, 203,569 B, SHA-256
  `5df6c2b5f0bf7851a4b2dbb297043019a621c8b4e0f41967116d30dd78e281f8`.
- `reblue_908.log`, 212,374 B, SHA-256
  `5b633a9514b2602825e58d8b956c2f85485d5eab12e057470bcc4d8c6a498e63`.
- Current `out/verification/native_vertex_input_pull_window.jpg`, 388,098 B,
  SHA-256 `0093fee9df6564a4f7a299df1d881d94a6f7966496fe38c9144527b41376fda7`.
- Mesh fixture exe 203,776 B, SHA-256
  `b8ef5223f0a9a3a43a8663979e1c240118cb9a6ae7c1e1e8b5f22c620fa52fb3`.

The superseded 907 image was 389,788 B, SHA-256
`325829e87b00454e1feb08203bfad2620a88199d8dfb229b6e1775621c2e29d4`;
it is no longer retained. Keep both small logs for distinct precache-off/on
coverage, one current field image, and final host60/mesh04/draw-intent01 logs.

## Storage

Same cumulative ledger: `20260906_0333_native-scene-state-bridge.md`; original
2 GiB/100 MiB ceilings and zero new raw allowance are unchanged. Preflight free
64,168,857,600 B; <=256 MiB build overlap and <=4 MiB fixture/log growth planned.
No new build tree, downloads, raw/perf/cache/dump files. Existing mesh cache
remains 3,510 files /36,510,144 B. Aggregate build logs 175,358 B.

After replacement validation, deleted **nine** exact superseded agent files:
normal table JPEG/log906, host59 stdout/stderr, mesh build03/CPU03 stdout/stderr,
and the intermediate PSO-off vertex-input JPEG. Logical payload **992,997 B**;
immediate measured reclaim **1,007,616 B** (614,400 +393,216), counted once.
They are reproducible diagnostics, not game data or protected VR/failure evidence.

Comparable runtime-log/build-log/image retention grows **233,145 B** for the
new native-input and pulling coverage. Known mesh exe/main-object/new-input-object
growth is **83,458 B**; other modified fixture objects/build metadata were not
fully baselined, so these subtotals are not an exact total-artifact delta.
Mesh fixture exe plus all five objects now total 865,196 B. No broad cleanup or
unrelated free-space change is credited as savings.

Post-cleanup free **64,135,020,544 B**, drive-wide use **33,837,056 B** from
preflight. Source/docs/Git writes follow and remain charged. Keep one latest
representation per verification purpose; broader protected evidence stays.
