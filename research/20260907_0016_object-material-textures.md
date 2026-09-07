# Object-published native material images and UV values

2026-09-07; parent2aaf549, Plume unchanged3094b35. This checkpoint connects an
owned producer/data/consumer path inside transitional submission. It does not
complete direct static-object scene/shadow drawing or the full host frame.

## Source contract and implementation

Used guest-source and devloop: exact generated C++/PPC and existing hook map,
focused CPU fixtures, one incremental host build and one bounded desktop run.
No generated source, game assets or shaders were edited or rebuilt.

- `bdSceneTreeDraw` in generated/reblue_recomp.88.cpp prepares visual, graph,
  palette, selected table and phase in the traversal context, then runs pass
  preparation before traversal. `HostWalk` now scopes one owned texture input
  publication around that object traversal; nested scopes restore the parent.
- `bdSceneNodeDrawSingle`, generated/reblue_recomp.40.cpp, loc_822816B0 onward:
  ordinary6000 texture commands select channel/selector, apply ordered152-byte
  early UV/image records, special scene selection, then84-byte late image
  records. The first UV match ends the early scan, even for channels beyond
  the two UV pairs, and skips that record's own image. Earlier image overrides
  skip special/late selection. The first matching late image wins.
- A null texture is a no-op in `Video::SetTexture`, not an unbind. Consequently
  storing only the last selector is wrong: imageA followed by a null choice
  retainsA. The load-owned ordered assignment program preserves history and
  distinguishes Bind/Keep/Unknown. Unknown writers invalidate ownership.
  Reflection0600 assignments invalidate ordinary slot5 ownership; repeated
  reflection commands and255 disable retain their original no-rebind semantics.
- `ReadMaterialTextureInputs` imports finite UV values and owned image leases
  once per object pass. It preserves initial/default UV versus the distinct
  reset-to-object-base rule. Counts are bounded to256 early/late records; special
  callback/technique11/phase1 families remain explicitly unconverted. Scene-image
  writers not yet owned invalidate their channel rather than freezing a table
  binding. Active late bindings may still supply a known replacement.
- Native table generations and model programs are pinned during the object
  scope. Per-mesh composition consumes only owned program/table/override values;
  it does not read guest words or resolve image resources. Primitive matching
  remains a temporary source-index boundary and rejects conflicting matches.
  Scope storage is capped at4 MiB, four nested scopes per thread,4096 output
  primitives and65,536 assignments. Existing library budgets account for shared
  image/model owners, including pinned retired generations.
- Draw capture checks claimed image channels and UV values against interpreted
  state; mismatches disable that template's replay. Replay preflights the entire
  node's fresh values before issuing draws, then supplies native image handles
  and current UV values. It skips sibling/stale UV-register history for ownedc2.
  Reflection and scene-image slots retain their separate producers. Conversion
  to the old shaderc2 interface is still an explicit temporary ABI boundary;
  this does not claim complete ownership of all shader UV parameters.

The material program stores runtime import recipes, not a new persistent image
identity schema. Original object/pass setup, visibility/culling,1000/2000/3000
direct/deferred routing, unsupported material families, source lookup, captured
templates and named-native-shader/direct submission remain required. The next
slice should complete participation/pass records and direct consumption for a
real rigid family, retaining all participants rather than treating strips as opaque.

## Verification

Material14 /PID30064 passes; CPU12 /PID18444 fixture0.10 s, ctest0.12 s. Fixtures
cover source destruction, ordered inheritance, unavailable/null images, channel
matching, early/late precedence, UV reset, reflection invalidation/repetition,
fresh image values, malformed extents, nonfinite inputs and bounded transactional
failure. Existing model/instance lifetime, concurrent lease, reload and storage
fixtures also pass.162 boundary guards and44 scenario cases pass. The new gate
requires fresh publications, matching checks and image/UV draws in consecutive
ready-field windows; startup counts, stale consumers, mismatches/refusals,
counter resets, oversized logs and lost readiness cannot qualify the path.

Host66 /PID30292 passes; in-place codegen checks its already-up-to-date module,
with no guest objects or shader compilation. Exe48,250,368 B/PDB106,885,120 B.
Tested host SHA256:
`4b0b66a181a1ceda4a8a359e2f675e9c6c2d940e33a9596ecf66050a085a23a0`.
Material fixture SHA256:
`dd74d843c8904029b2aad4270dc3c3d518a5fdc11c92e582010f80927b4eb6f5`.

Run914 /PID25128,00:13:59..00:14:58 EDT, flat1920x1080 normalMSAA/precache-on.
All12 settings took effect: autoplay, native instance/shadow/material textures/
tables and material checks on; table comparison, automatic captures and perf
CSV off. Original116-byte profile restored exactly, SHA256
`2f1bc38d763a1b7bdba31f560684fd4aa7e42a714600b8d344f19da7f38e23b0`.
No new raw/perf/cache/dump files. All owned producers terminal.

Fresh post-event samples frames2054/2354: bg41_01,state0,player present,
event/movie/loader/icon inactive. Values below are deltas unless marked cumulative.

| Observation | Result |
| --- | --- |
| Native object material publications |33,600;300 contain override records |
| Native material reads / matching checks |76,625 /58,641;0 mismatches/refusals |
| Native UV-composed replays / image slots |59,927 /62,690 |
| Peak scope memory |110,640 B cumulative;4 MiB cap |
| Unsupported scopes / unavailable draw lookups |167,660 /72,463 cumulative; not converted |
| Canonical owners / draws / native pulled records |2,206 owners;139,362 draws /152,384 records |
| Source-free GPU loads |0; not qualified |
| Model geometry draws / checks |46,142 /1,446 matching;953 unavailable cumulative |
| Diffuse / specular / reflection checks |15,498 /14,758 /0;0 wrong |
| Native pose reads/checks |110,362 each;0 unavailable/refused/drift |
| Normal table lookups |20,912;0 original comparison/fallback |
| Shadow policies / receiver checks / receiving checks |2,973 policies;15,498 /14,883 matching |
| Walking |same episode;150 observations;172.916238 world units;11.999 s walking |

Native instancing/indirect dispatch is active. Table hook's separate native-image
reader counter remains0; the new object material consumers above use the pinned
table API, not that hook. No direct native reflection image consumption is proved.
Presence of300 override-bearing publications is not evidence that every animated
override family changed correctly; authored event/lifecycle qualification remains.

Inspected `out/verification/native_material_textures_window.jpg`,1920x1080,
quality60,134,083 B: character running beside the fence/building, ground/foliage,
rocks and cast shadows coherent. Thin dark cliff/background artifacts and
distant blur remain. This single image is component sanity evidence, not
same-camera parity, a stability sequence, reloads, both eyes or full-game proof.
Image SHA256 `44179d9d64955a15fa571e814552b10d9723716e6027831d3fd5da73b48e6364`.
Log `out/build/win-amd64-release/logs/reblue_914.log`,226,970 B, SHA256
`fe5d37ca33c2b9dccae6846f031fab6e4b7a69df2cf35d1814352d13b95c117d`.
No FPS, bandwidth saving or overall conversion percentage is claimed.

## Storage and retention

Same cumulative ledger `20260906_0333_native-scene-state-bridge.md`; owner-approved
3 GiB checkpoint exception, floor62,509,998,080 B, diagnostics100 MiB/logs10 MiB/
images10 MiB/raw0 unchanged. The new image/log requalify the previous shadow/
canonical/material/pose/movement component purpose. After validation, removed
eight exact obsolete files: shadow sanity JPEG/run913 log, material13/CPU11/
host65 stdout/stderr.364,409 logical B; immediate free63,459,553,280 ->
63,459,926,016 B, measured reclamation372,736 B, counted once. Old reports and
hashes remain, but those exact prior image/log files no longer do. Build/test
outputs are reproducible. Run911 motion images, standing/failure evidence,
protected raw archive, assets and active builds were not touched.

Material fixture7,129,853 B (+167,233 B); aggregate build logs151,538 B (+3,689 B).
New run/image replace equivalent evidence (+1,076 B/+1 B). Comparable retained
fixture/log/image growth171,999 B supports the newly owned texture/UV path and
its tests. Host exe/PDB growth410,936 B; build objects/metadata, source/docs,
helpers and Git lack complete byte baselines and are not falsely counted as zero.
No duplicate build or asset cache. Retain new component evidence until equivalent
replacement, and new focused fixtures while they cover production behavior.
Images10,240,483 B;245,277 B under the unchanged aggregate limit.
Ending free at cleanup63,459,926,016 B (~59.10 GiB); drive-wide gain47,939,584 B
from the initial read-only sample, not attributed to our372,736 B cleanup alone.
Low sampled free before runtime63,063,465,984 B stayed above the supervised floor.
