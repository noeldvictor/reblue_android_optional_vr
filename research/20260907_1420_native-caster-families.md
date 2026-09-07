# Native opaque caster families

2026-09-07 EDT, source parent213011a plus this bundle; host107/run945.

## Delivered connection

Opaque rigid shadow casting is no longer admitted by one geometry/material ID
or restricted to a sole primitive. Load-owned model programs and the existing
object-pass policy select the supported family before pose fallback or culling.
Every primitive's geometry, world/pass matrices, winding and pipeline is
preflighted before the first sibling enters the existing native draw queue.
The queue retains resources through the real frame-slot fence and emits native
structured instance records/indexed indirect commands; no interpreter, capture,
replay, translated gather or legacy pipeline warm-up supplies these casters.

The original geometry258694267A8DBAEE remains the strict scene/caster regression,
not the broad caster selector. Its lifecycle counters are explicitly isolated
from other primitives; family growth cannot satisfy its900-emission windows or
overflow its eight-generation diagnostic index. Batch compatibility includes
that diagnostic distinction. No new renderer registry, shader family, asset
format, cooker or build tree was introduced.

## Contract and limits

The guest-source skill reused the established phase1 policy/depth contract in
[direct caster evidence](20260907_0846_direct-rigid-shadow.md) and the owned
[declaration/shader-input findings](20260907_0535_owned-primitive-shader-inputs.md).
It avoided importing phase0 colour/UV/lights into the depth-only consumer.
The existing source adapter reads object policy once before traversal; the
load owner supplies primitive associations and the pose owner supplies matrices.
The legacy node entry independently rejects an admitted family, including callers
outside the host walk. Missing pose, GPU allocation, camera or native pass cannot
silently select legacy for an admitted node.

Family admission requires technique0, phase0/1, explicit zero declaration bones,
no skin binding and known non-alpha/non-deferred participation for every sibling.
Texture-dependent effect routing is explicitly unsupported, not guessed from
missing images. Depth-only opaque rendering needs no scene texture-layer/material
identity restriction. Unknown/unsupported skin or deformation, wind, alpha,
deferred and volume/effect paths stay unconverted and keep their existing route.
Selected-regression contract changes still refuse instead of falling back.

GPU preparation requires complete canonical rigid input, triangle-list ranges,
owned buffers and finite affine transforms. Whole-node preparation is bounded
at4096 primitives and the existing4096 records per frame slot/eight program
variants. Intentionally suppressed participation stays suppressed; late failure
returns no partial CPU plan or partial queued replacement.

Remaining: source tree/object policy/pose/pass producers and inherited-state
adapters, broader scene shading (currently one selected one-texture consumer),
alpha/wind/skin/effects, repeated-instance groups, culling/occlusion ownership,
explicit per-eye inputs and the full desktop frame/sequence/event gate. This
does not establish independent source-free scene loading or a complete host frame.

## Verification

- 310 Python source/scenario checks pass, final0.146 s. New family parser requires
 fresh field-window node/multi-primitive participation, actual emissions and
 retirement outside the selected regression. It rejects startup-only/stale/reset/
 wrong-scene/refused/impossible counts and is required independently in both epochs.
- C++ output28 stalled before compilation in sandbox. Its confirmed owned
 CMake25096/Ninja30156 tree was stopped before the escalated output29 retry.
 Output29/CPU12 passed. Output30/PID31364 and CPU13/PID31052 pass after final
 suppression/capacity fixtures:0.38 s assertions/0.40 s CTest. Three-range owned
 geometry retains distinct index/base-vertex ranges and winding after retirement;
 late alpha/skin/GPU/command failures cannot omit siblings. Missing poses, stale
 generations, unsupported techniques, effect routing, ordered suppression,
4097-range refusal and regression/non-regression batch separation are covered.
- Host107/PID30172/session19411 passed incremental host compilation/link;
 codegen0 written/module up-to-date, no guest object or shader rebuild. Exe
48,622,080 B SHA256`663715CBFD7E69C378A7F50C8F219149CAAE2899CAD128D9001ECA48A5F6757A`;
 PDB109,068,288 B. Unchanged production shaders reuse existing GPU26/rigid05
 five-case/two-eye fixture evidence; not a new live stereo qualification.
- Run945/PID28908/session67557 terminal14:20:22-14:22:20. All21 settings took
 effect; exact owner profile restored. Both full existing epochs plus receiver,
 owned scene lights and the new caster-family gates pass independently. Cold
 generation93/instance144 closes all1687 selected scene/shadow records before
 title; new207/384 reaches a new900-emission interactive window. The old941
 dirty-bit mismatch did not occur; this does not identify its cause or fix it.

| Fresh300-frame window | Non-regression node visits | Multi-primitive node visits | Native primitives submitted/emitted/fence-retired |
| --- | ---: | ---: | ---: |
| Cold field2041->2341 | 9,588 | 2,096 | 12,530 / 12,530 / 12,529 |
| Reloaded field4669->4969 | 9,590 | 2,096 | 12,532 / 12,532 / 12,532 |

These are repeated visits, not distinct assets. Suppressed counts remain0 in
this observed run. The newer window adds300 selected native scene emissions and
12,832 total native shadow emissions; all are singleton indirect calls, merged
instances0. No draw-call reduction, FPS improvement or Quest speed estimate.
Owned scene lighting adds300 publications/reads with2,898 bindings and zero
missing reads; receiver original/refused/missing remain0. Strict legacy source
comparisons remain enabled. The broader family's retirement is aggregate evidence,
not an asset-by-asset reload census.

Inspected the renderer-owned1920x1080 image: Shu moving by the fence/bell support,
coherent terrain, trees, stream and shadows, existing black cliff marks. JPEG
was encoded in memory within110 KiB without lowering resolution;105,191 B.
This is one mono sanity image, not an isolated caster pixel oracle, visual
sequence, same-pose comparison, animated-effect or both-eye acceptance.

Retained log`out/build/win-amd64-release/logs/reblue_945.log`,505,192 B,
SHA256`BBBB860A54C78D6E802C66D0C2B08C254BA9337BC30B1E1C73370C13DAE1CD8E`.
Image`out/verification/native_caster_family_window.jpg`, SHA256
`DB5B45846632841877805FC646165E9CEF207417E431BF204DD72BF92E0C408C`.
Owner profile116 B SHA256
`2F1BC38D763A1B7BDBA31F560684FD4AA7E42A714600B8D344F19DA7F38E23B0`.

## Storage and next connection

The dev-loop skill kept verification in the existing fixture/host trees and
bounded run supervisor. Same cumulative ledger,3 GiB exception and
62,509,998,080 B operational floor; no reset. Build supervision300 s/256 MiB
free-drop, planned192 MiB peak overlap; runtime180 s/800 KiB text/192 MiB drop,
110 KiB JPEG/10 MiB aggregate images, incoming raw0. Scoped audit finds no new
raw/perf/cache/HLSL/dump outputs and no new >1 MiB install-tree file during the run.

After equivalent945 text/pixel acceptance, removed944 log/receiver image,
host106 logs and superseded fixture logs (including the stopped empty attempt).
Fourteen exact agent-created outputs:633,704 logical B,647,168 B measured recovery
across two deletions. Exact retired runtime text/image are gone; hashes/findings
remain. Protected940/image,941 failure, historical raw/VR/movement/other baseline
evidence, game data and profiles were not removed. An already absent935 image
was detected by read-only checks and was neither deleted nor credited again.

Known retained net growth568,300 B: texture fixture+467,033 (69,596,023 B/129files),
exe+12,288/PDB+106,496, build logs-1,021 (191,620 B/136files), runtime log
replacement+14,306, image replacement-30,802 (aggregate10,336,287 B).
Other host objects/source/Git and drive activity are not fully attributed.
First measured free63,162,793,984 ->cleanup-end62,970,253,312 B:183.62 MiB
drive-wide use,58.646 GiB free, not all attributable to the renderer. Recheck the
same cumulative ledger before any next producer; no next job remains running.

Next extend the same object/primitive owners to representative scene material
families and repeated-instance groups. Do not reuse the one-texture scene shader
for the observed three-layer materials without implementing that contract.
Keep the selected regression and unsupported participation visible, preserve941,
and continue toward the unchanged full desktop gate before Quest work.
