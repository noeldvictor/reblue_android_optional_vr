# Load-owned model material records and a rejected field gate

2026-09-06, EDT. Implements the first data-owner part of the static-object
frontier, not the complete native model/instance/submission path. Core owner
and CPU tests published as `d723d70`; loader/consumer integration follows.

## Source and implementation

Read the hook manifests first, then the complete generated node processor
(`reblue_recomp.46.cpp:9719`), both tree builders (files 43 and 68), and the
complete graph destructor `sub_8227EBE8` (`.94.cpp:9482`). Node processing copies
80/104-byte nodes, clones command streams, constructs IB/VB tables and recurses
through child/sibling links. Tree A renumbers nodes; tree B remaps joint operands
and marks feature flags. The import therefore runs **after the full builder**,
not at the early physical-block hook. The previously mapped load-end hook has
no result argument and precedes the LoadModel+204 result store.

- `native_model_materials.*` owns immutable decoded primitive recipes and shared
  native material handles. Its temporary source-model/source-mesh index is
  explicitly an adapter, never a persistent asset identity. Material IDs/files
  reuse the existing native material library. No copied command/register stream.
- The `bdSceneGraphBuild` wrapper publishes once at completed load, independently
  of optional PSO precaching. All nodes, including initially hidden/pruned ones,
  participate; visibility remains live instance state. Shared meshes import once.
- Four material/reflection/skin/shadow consumers now obtain immutable leases.
  The old thread-local first-draw command cache and physical-generation clearing
  are removed. A missing model cannot cause a draw-time decode/import.
- The complete `sub_8227EBE8` entry retires the source index before original
  node/declaration release, including the nonphysical destruction branch. Aliasing
  leases pin their model and keep retired generations charged until last release.
- Limits: 8 MiB logical recipe storage plus bookkeeping allowance, 4096 live
  models, 4096 visited nodes/model and one million source words/model. Malformed
  trees, missing inputs, duplicate keys and full budgets refuse bounded work.
  Reused keys cannot expose an old generation after failed publication.

The original graph builder/destructor still execute. IB/VB matching, model-local
geometry/control/texture recipes, live control reads, `NodeTag`, native instance
updates, shader ABI and retained draw/list templates remain. Technique 11,
phase 1 and deferred-list imports retain their previous explicit exclusions.
No complete guest-free static-object draw or fully host-owned frame is claimed.

## Verification

Material test build 03 / CPU 03 pass: suite 0.13 s, including existing material
format/storage/skin/reflection tests and new source-free lifetime tests. New
cases cover preload, sharing, explicit unskinned recipes, whole-model pinning,
retirement/address reuse, failed replacement, count/byte/overflow limits,
registry destruction, coordinated loader/render leases, cyclic/missing/aliased
nodes and transactional traversal. No persistent fixture data is added.

120 source-boundary guards pass in 0.030 s. Five new guards check post-builder
publication, complete destructor placement, no draw-time discovery, dependency-
free owner implementation and diagnostic scene-state inputs. They are not a
substitute for behavioral or pixel tests.

Host 38 failed on missing original-entry declarations; corrected in 39. Host 40
includes final checked node spans. Host 41 adds diagnostic-only semantic scene
context and passes (about 3 s wrapper time). No guest objects or shaders rebuilt;
the codegen freshness check reports no files written and module up to date.

- Host 40: 47,808,512 B, SHA-256
  `e2d2c430bc1cd3e2b45c86e3dd3f0d67042cecd3c37168d8f304d1f1de87ed75`.
- Host 41: 47,811,072 B, linked 16:06:15, source `d723d70` plus reviewed dirty
  integration; SHA-256
  `17b9a6c433874a058735b956b878086d34ebcf32a27cb1cf9e9aa3e417e95bd5`.
- Material test: 465,920 B, SHA-256
  `27f8181442711cd807451b154801dfb96111f49b3662d3d50234f3c6faccf5c8`.

Flat diagnostic 888 (host 40), PID 27768/session 60001, 16:02:26-16:02:55:
all seven settings audited, material comparison enabled, PSO precache disabled,
perf/raw off. 114 builds/publications, 738,768 B live recipes, 14,208 lookup hits;
zero missing/load/input/budget failures or unsupported meshes. Diffuse 3,951 and
specular 3,467 checks match; reflection has zero checks. The inspected 1920x1080
PNG shows the tower/rocks/sky/character and opening credits: **opening cinematic,
not interactive field**. Its 1,794,992 B image is
`out/verification/native_material_pass_window.png`, SHA-256
`a5f158a89958e1bc9674e557b8f81b2f6df47abf8e01d0086414eaa770b7cb90`.
The filename was vacant after the prior checkpoint's documented cleanup.

This exposed a false scenario inference: 301 water updates also occur in the
opening cinematic. Water counters, including earlier dated "field marker"
wording, do not independently identify an interactive field. Prior actual images
remain evidence of what they show; no prior both-eye/full-game gate is upgraded.

The supervisor now requires the existing semantic readers to report FieldActive,
field state 4, a named stage/player and no event/movie, followed by matching
material/model samples. Nine in-memory fixtures execute its actual parsed gate,
rejecting missing/stale/loading/event/later-failure contexts. It does not merely
select the last matching good sample while ignoring a later failure.

Run 889 (host 41), PID 24436/session 10626, 16:07:31-16:08:47, is **a failed
field-scenario gate**, not a renderer pass. The readers report stage `bg41_01`,
Loading / field state 0; the event flag clears, but interactive readiness is
never established within the 75 s bound. Do not relax the predicate to make it
green: distinguish actual scenario state from a stale/incorrect state reader
against the generated writers before another attempt. No image/raw/perf output.

Run 889 still supplies scoped material/lifetime evidence: 114 publications,
one retirement, 113 live models /730,584 B; 488,116 hits and zero missing/load/
input/budget failures or unsupported meshes. Diffuse 126,735 and specular 120,369
comparisons match; reflection is unexercised. These cumulative samples are not
all field-only. Nonphysical teardown coverage is source/CPU, not isolated live
coverage. Reload/battle/both-eye/animated-effect/full-desktop gates remain open;
older scenery/text failures remain unsuperseded. No speedup or Quest claim.

## Storage and next dependency

Both processes are terminal and owner profile restored byte-for-byte. No new
raw, perf, geometry, texture or HLSL outputs. Six newly reached material files
add 408 B; the library now holds 36 files /2448 B. They supply new load-time
coverage, not a per-run duplicate asset set.

Gross new logs/image: 2,208,640 B (11,719 build/test log bytes, 401,929 runtime
log bytes, 1,794,992 image bytes). Retired ten exact ignored/inactive/reparse-free
logs: old material build/CPU 02, and host attempts 38/39/40. Replacement CPU 03
and host 41 pass; the fixed compile failure is recorded above. Logical removal
10,405 B; immediate volume free 65,095,716,864 ->65,095,737,344 B, **20,480 B
measured reclaimed**, once. Logs can be regenerated; exact old logs are gone.
Current normal Toon flat/XR evidence and all protected failure/raw sets remain.

Net new retained diagnostic payload is 2,198,235 B for distinct opening-material
pixels and the failed semantic field gate; retain until the relevant checks are
replaced/diagnosed. Host grows 23,552 B over the prior Toon binary. The cumulative
scene-state ledger remains authoritative: no checkpoint-budget reset. Preflight
65,140,396,032 B; post-cleanup free 65,095,737,344 B (44,658,688 B drive-wide use,
not all attributable to these outputs). Final source/docs/Git writes follow.

Next: reconcile scenario-state readers while advancing load-owned geometry/
material bindings and native instances/direct scene+shadow submission. Do not
replace this missing path with another first-draw resource/register cache.
