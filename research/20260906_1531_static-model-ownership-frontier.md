# Static-model ownership frontier and reusable source index

2026-09-06, EDT. Previous goal turn: progress (`f956abd`, `99e6757`). This is a
source-only implementation/investigation checkpoint, not another renderer
qualification. It does not remove additional runtime rendering execution yet.

## The next conversion boundary

Read the complete generated `bdBinaryModelLoadRequest` (0x82140178, file 5),
`bdLoadModelDataCallback` (0x8217B970, file 80) and `bdSceneGraphBuild`
(0x8227E790, file 79), after `config/hooks/pso_predictor.toml` and
`physical_buffers.toml`. Read native material discovery and the relevant
load-time predictor, physical lifetime and node-draw hooks. The following is
an implementation dependency map, not a claim that those dependencies are gone.

| Boundary | Current evidence | Required native ownership |
| --- | --- | --- |
| Visual request -> shared model | `bdBinaryModelLoadRequest` calls `sub_8217B678` with model/texture names; `bdModelLoadAssetMapHook` links its result to the requesting visual before async request assignment | Stable asset/instance handles, including preload-before-request, shared assets and visual reloads; no persistent guest-VA identity |
| Loaded HDB -> geometry | Callback initializes the HDB, reads its scene entry, calls `bdSceneGraphBuild`, then stores the result at LoadModel+204 | Compile native model records during load, before readiness, not from first-draw snapshots |
| Build -> nodes/buffers | Builder clones its 28-byte header, allocates scene/physical storage, calls `bdSceneGraphNodeProcess` and both tree builders; registration hooks identify IB/VB/declarations | Native node/primitive records, explicit geometry/material/texture associations and canonical layouts, reusing existing native libraries |
| Lifetime | Physical registration/free hooks couple mirrors to the block; free hook is reached only on the nonzero physical-block branch | Explicit bounded ownership and generation-safe retirement; audit other graph destruction paths before claiming complete lifetime coverage |
| Draw -> submission | `scene_node.cpp` still calls `__imp__bdSceneNodeDrawSingle` when replay is unavailable and captures the resulting templates | Consume native model/instance records directly; no interpreter warm-up or retained register/fetch/guest-resource templates for the converted path |

Important integration constraints:

- `bdModelLoadBeginHook` and declaration prediction are gated by
  `PrecacheEnabled()`. Native asset production must not depend on that optional
  PSO switch. Existing asset/request maps are guest-address predictor caches,
  not a ready-made native asset registry with complete lifetime guarantees.
- `bdModelLoadEndHook()` currently has no register arguments and executes
  **before** LoadModel+204 is written. Do not assume it can read the completed
  graph from that field or receives the return pointer.
- `native_material.cpp` repeatedly matches material/reflection/skin/control
  recipes using mesh command ranges and guest IB/VB tables. `NativeMeshImport`
  still accepts resource wrappers. Consolidate these associations in native
  primitive records; do not introduce another draw-capture cache under a new name.
- `bdSceneTreeDraw` still has guest preparation children even though traversal
  and several lifecycle children have host hooks. The host walk still reads
  `GuestDrawNode`, the engine palette and `NodeTag` data. Native model storage
  must connect to native instance/update inputs as well as to submission.
- Keep recompiled gameplay, recognizable art, full modern-GPU scope and the
  complete desktop/both-eye/animated-effect gate. Original behavior is the
  reference; console resource/register/scratch layouts are not the final API.

Next implementation: the load-time native model/primitive record producer and
its association/lifetime fixtures, then direct consumption by the static-object
path. Fully inspect node processing, layout/control records and nonphysical
destruction before replacing their semantics. Further generic tooling is not
the next renderer milestone.

## Source-index implementation and verification

Extended the existing `tools/callgraph.py` rather than adding a second index:

- Source locations, recursive/multiple same-line calls, original-body calls,
  unresolved indirect call sites and manifest-named instruction-hook sites.
- `frontier` stops at host-hook **declaration candidates** and exposes boundary
  locations. Original-body calls bypass that stop. Local one-level wrapper
  macros and token pasting are recognized, including Toon and sampler hooks.
- Comments/literals and global hook prototypes are not call sites. The first
  real-source pass exposed a prototype leaking into the preceding function;
  scope tracking and a regression fixture corrected it before publication.
- No C++ preprocessor or host-helper call graph: conditional/header/nested
  expansions, linked activation and indirect targets still require inspection.
  A host-hook declaration is not proof of guest-free execution. Static counts
  are not runtime hotness or a rendering-conversion percentage.
- Queries reuse a valid source-stamped cache but do not write by default.
  `--cache` permits one atomic <=8 MiB index replacement, with an equally bounded
  temporary file, a 20 GiB free-space reserve and refusal of foreign partials,
  reparse ancestors and oversized inputs/outputs. Output rows/depth are bounded.

Ten targeted tests pass in 0.053 s: syntax classes, recursion, original bypass,
scope/prototypes, macro declarations, frontier/output bounds, cache reuse and
invalidation, duplicate/input limits, low space, failed replacement and a
competing partial-file race. Tiny test fixtures clean themselves up.

Final real-source index: **18,777 bodies, 100,213 syntactic sites, 5,852 unresolved
indirect sites and 207 host-hook symbol candidates**, across the entire program,
not just rendering. Independent source spot checks verify prototype exclusion,
node-process recursion and Toon macro detection. Reindex 10.7 s; same-process
cached reload **0.147 s**, equal to the rebuilt graph. This is a navigation
measurement, not an overall development-speed claim.

Useful bounded queries:

```powershell
python -B tools/callgraph.py frontier bdLoadModelDataCallback --depth 2 --limit 65
python -B tools/callgraph.py sites bdSceneGraphNodeProcess --limit 40
python -B tools/callgraph.py frontier bdSceneTreeDraw --depth 1
```

## Storage and unchanged renderer evidence

No renderer build, game run, asset conversion, capture, dump or diagnostic log.
Existing Toon binary/runtime evidence remains the latest qualified renderer.
Owner profile is unchanged at 116 bytes; no relevant renderer/build producers.

One ignored index now occupies 8,249,479 B, replacing the old 2,375,990 B index:
**5,873,489 B net retained growth** for new source/boundary coverage and reuse.
SHA-256 `33178ac25156c61e9203408dd3627d4686a046c1a43e9670a89bac6d5abe9e6d`.
No `.partial` survives. The intermediate schema-2 index was replaced, not kept.
The initial 8 MiB additional-overlap estimate covered the first replacement;
retry overlap after retaining the larger index required a corrected bound of
16 MiB total index+temporary payload (14,134,447 B additional over the original
cache for the actual two payload sizes). Actual peak volume use was not sampled.
This remains within the cumulative 2 GiB/100 MiB limits; no budget reset.

Preflight free 65,176,121,344 B; closing pre-publication free 65,139,838,976 B:
36,282,368 B drive-wide use, not all attributable to the index/source edits.
No separate deletion/cleanup savings are claimed. Keep one current index and
replace on invalidation, never archive each generation. See the cumulative
scene-state ledger for prior retained evidence and cleanup already counted.
