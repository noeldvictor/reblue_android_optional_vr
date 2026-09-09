# Host renderer transition

The owner's active goal is to move **all rendering** to the host, remove the
Xbox 360 rendering model, and use modern GPU and VR techniques on desktop.
Gameplay remains recompiled. Preserve Blue Dragon's art style and readability;
materials, lighting, geometry, effects and asset formats may change for performance.
Quest 2 optimization follows the completed desktop transition.

## Completion requirements

All of these remain required; shipping an intermediate component is not completion.

- Host frame scheduling and pass construction, with no guest rendering execution
  or per-draw D3D/Xenos state translation in the finished frame.
- Host scene and object data, materials, animation and GPU skinning, shadows,
  reflections, particles/effects, post-processing, UI and presentation.
- Native asset conversion with explicit versioned formats, stable identities,
  desktop cooking and persistent output. Meshes, textures/mips/compression and
  materials must not depend on transient guest allocation addresses.
- Multiview stereo and correctly sized layered targets; host frustum and
  occlusion culling, instancing, indirect draws and suitable generated LODs,
  merged geometry and impostors.
- Remove EDRAM allocation, tile matching, seed copies and emulated resolves
  from the frame. Ordinary GPU resolves needed for native MSAA are distinct.
- Verify representative fields, battles, cutscenes, menus, scene transitions
  and reloads on desktop, including both eyes and animated effects. Record
  remaining guest rendering calls and resource dependencies explicitly.
- Only after that desktop gate: Quest 2 qualification and VR optimization,
  including foveation, toward the recorded 72 Hz / 1440x1584-per-eye target.

## Active work queue

Updated 2026-09-07 after the owner's request to work faster and smarter.
The work unit is a **connected rendering subsystem**, not a console callback or
a longer list of counters. Reuse the native owners, assets and backend already
built. The selected rigid object remains a regression target, not a permanent
ceiling on scene-level implementation.

Host107/run945 extends native opaque rigid casting to multi-primitive nodes,
with 12,532 additional native shadow emissions/retirements in a fresh300-frame
reloaded-field window. Both epochs pass the new non-regression family gate and
all existing reload/receiver/owned-lighting checks; mono pixels were inspected.
That last live scene-shading result remains the selected object's route.
The new source connects whole opaque rigid nodes with0..3 texture layers to
native submission. Host110 also connects ordered inherited light tickets, an
outbound mirror for retained legacy consumers, and active material-channel
requirements;312 Python checks and expanded C++ fixtures pass. The preceding12
two-eye GPU shader cases remain unchanged. Run948's fresh cold-field window adds
1,265 wider-family native scene emissions/retirements and cold teardown completes.
It then fails a strict legacy UV comparison in the reloaded field (visual23820098,
channel16). No new pixels; sampled layered/inherited draws0. Preserve948 and
investigate that exact producer/consumer boundary before another probe.
The host link fit after unrelated free-space recovery within the unchanged
allowance; full scene/reload/inheritance/pixel acceptance remains pending.
Do not restamp host107/run945 or enable normal acceptance
switches on fixture evidence alone. Run941's legacy dirty-bit mismatch remains unexplained;
keep its strict comparison and failure evidence. There is no qualified complete
host frame, broad scene, full-game stereo or Quest result.
[Current caster-family evidence](../research/20260907_1420_native-caster-families.md),
[owned-lighting contract](../research/20260907_1342_owned-scene-lighting.md),
[preserved receiver/failure evidence](../research/20260907_1224_native-receiver-setup.md).
[New layered-scene connection and pending gates](../research/20260907_1501_layered-rigid-scene.md).
[Ordered light ownership, host110 and preserved UV failure](../research/20260907_1544_ordered-scene-lighting.md).

Host111 follow-up adds bounded failure provenance without changing UV semantics.
Runs949..951 stopped at time/storage limits before the needed UV observation;
no non-reproduction is a fix.313 Python checks pass, no new pixels or reload
acceptance. The local supervisor now checks full diagnostic overlap before boot.
Next observation needs the mismatching node/technique and exact owned/live/staging/
shader values to identify the first divergence; no speculative outgoing mirror
or guessed causal test. [Current investigation and retained evidence](../research/20260907_1638_uv-boundary-provenance.md).

Independent scene ownership progress: direct blended cutouts now connect the
load-owned ordered cutoff recipe and object/pass inputs to native scene shader,
pipeline and indirect submission. 314 Python checks, expanded material/output
C++ fixtures and 23 two-eye GPU modes pass (2.87 s, zero validation errors/warnings).
This is not game acceptance: that first host link and later syntax checks could
not start within the unchanged storage reserve. No game run or capture. Preserve
sorted siblings, legacy interoperation and all outstanding
UV/light/pixel gates. The subsequent phase1 caster connection is described below.
[Cutout connection and current evidence](../research/20260907_1713_native-scene-cutouts.md).

Historical phase1 shadow connection: host112 added owned images, forced-wrap
samplers and retained indirect batches, but its object/vertex alpha and generic
cutoff model was incorrect. The earlier37/41 GPU modes verified that implementation,
not the recovered authored shadow contract. Preserve those results as history,
not current equivalence evidence. [Earlier shadow connection](../research/20260907_1742_native-cutout-shadows.md).

Host115/run955 follow-up: the first live cutout refusal was caused by treating
the light-space colour flag as a cutoff override. The exact translated branch
skips colour writes, not cutoff defaulting/setters; the importer and causal C++
fixture now reflect that distinction.320 Python checks pass.41 GPU modes pass
in1.65 s, validation0/0; four new modes sample cutout shadow depths through the
native receiver in both eyes, including filtered edges and instanced casters.
The fresh cold-field1842..2142 window adds1,198 textured scene cutout emissions /
1,196 retirements and8,778 zero-texture shadow cutout emissions /8,766 retirements.
All prior cold rigid/receiver/lighting/caster gates pass; old generation93 closes.
The disk guard stops the reloaded opening event, with no image or reload acceptance.
Textured shadow emissions remain0, so the new strict cutout-family gate stays
pending. Keep948/941 failures and the normal profile unchanged.
[Current integration, source correction and evidence](../research/20260907_1838_cutout-integration.md).

Current correction (host116/run956): `sub_82174270` selects the
phase1 shadowmap route for ordinary deferred casters; their texture-enabled
participation was excluded by native admission. The original shadowmap PS uses
fixed base alpha0.6, no object/vertex alpha, generic cutoff or negative-U sentinel.
Ordinary phase1 depth-only deferred casters now join the existing native queue;
zero-texture casters use the solid shader. Unsupported effects/skin/wind/forced
passes remain excluded, and scene deferred ordering is unchanged.
Material CPU36/output CPU20 pass. Rigid14 passes all46 GPU cases (1.30 s,
validation0/0), including corrected overlap placement and reversed instance order.
Host116 links without guest object compilation. Run956 passes the full strict
cold/reloaded-field chain: each fresh300-frame epoch emits/retires300 textured
shadow casters; scene cutouts also advance, and generation93 fully retires before
generation207. Batches remain singletons and sampled layered draws remain0.
The new mono image has visible tree-trunk gaps, so pixel acceptance is open.
Identify affected node/material/pose/participation and camera relation; preserve
a matched-state correctness question rather than retrying for nicer framing.
Keep948/941 failures and host107/run945's last accepted pixels unchanged.
[Corrected contract, current live proof and pixel question](../research/20260907_1927_corrected-shadow-coverage.md).

Host118/run958 connects native scene-depth queries and fence collection to owned
node observations and the native rigid draw consumer. The translated query
shader/upload, guessed target trigger and source-address draw filter are removed.
Plume2d206ee fixes deferred render-pass query ordering. A failed first live run957
also exposed the outgoing descriptor handoff; the production binding scope now
restores its explicit resume snapshot, tested with a real dynamic descriptor
consumer before the successful retry. Query inputs remain fully native; the
outgoing compatibility handoff is still needed by unmigrated consumers.

325 Python checks, C++ history/geometry contracts and eight Vulkan query/pixel
cases pass. Run958 passes the full strict cold/reload chain with generation93
retired before206. Its last query sample has450,896 submitted /450,679 collected,
101,924 zero results but **zero native draw skips**. This proves real query
execution, not useful culling or speedup. Next: distinguish queries for nodes
with no native consumer from camera/bounds/depth/history refusal; query only
eligible native consumers, then prove actual skipped draws with controlled
visibility and pixels. Do not weaken camera/generation safety to inflate skips.
Stereo queries remain unqualified. The attempted JPEG found the renderer already
terminal; no image was written. At that checkpoint the image archive had only
47,920 B headroom below10 MiB; the reviewed cleanup below subsequently made room.
Keep957 failure evidence,956 tree-gap evidence,945 accepted pixels and948/941.
[Native occlusion connection and exact evidence](../research/20260907_2007_native-occlusion.md).

Prior follow-up, host119/run959: query admission now happens only at the
preflighted, nonempty native rigid consumer, with owned world bounds passed by
value from the walk. The walk's broad query publication and camera lookup bridge
are removed; legacy-only nodes cannot consume query/history capacity. Empty
passes avoid query pipeline creation and binding changes. Current/history bounds,
camera, depth-generation, age and duplicate checks remain strict. 326 Python
checks and the expanded C++ fixture pass; eight Vulkan cases now feed two actual
submissions/fences into the production decision and cull only the hidden candidate
(validation0/0). This is a fixture decision, not skipped game geometry/pixel proof.

Run959 passes the full strict cold/reload chain, generation93 retired before207.
At query frame4800: 31,649 native requests, 8,960 submitted /8,954 collected,
3,034 zeros and0 native skips. Decisions: 22,689 invalid bounds, 8,613 changed
camera, 304 missing history, 43 visible; other reasons0. Bounds rejection includes
missing/nonfinite/near-clipped inputs; this sample does not isolate those subtypes.
Those refusal counts motivated the indexed native bounds connection below;
they are historical node-level counts, not directly comparable to primitive-level
counts or a controlled performance measurement.

The independent reloaded4352..4652 material window now proves real scene batching:
9,515 instances /7,893 indirect calls, 2,813 instances in merged groups. Wider
scene emissions advance, layered draws remain0. This replaces the current
"singletons only" observation, not historical measurements or pixel acceptance.
No measured speedup and no claim that the admission edit caused the observed
batching; the moving scene differs from958. Preserve all existing visual failures.

Current connection, host120/run960: native geometry now owns bounds derived from
checked, indexed v2 Position.xyz before GPU upload, including signed base vertex.
The production object matrix produces conservative world boxes; no new file
format, sidecar or bulk recook is needed. Native occlusion no longer consumes the
walk's radius-scale bounds handoff. Preflight/order remain whole-node, but query
identity and culling are per primitive. The walk's other source/frustum adapters
remain. Exact camera, depth generation, near-plane and temporal checks stay strict.

327 Python checks, mesh/output CPU fixtures and eight two-fence GPU cases pass
(validation0/0). Host120 links without guest objects. Run960 records49,457 native
requests,24,011 submitted/collected queries,12,091 zeros and1,026 primitive skips.
Cold generation93 fully retires before207, but the300-second supervisor terminates
the run before reload qualification. The selected reloaded object emits2,228 scene
and2,228 shadow primitives; totals alone do not satisfy continuous readiness.
No image was captured. Neither culling correctness nor a cause for the missing
reload gate is proven. Host119/run959 remains the preceding full strict reload
pass; host107/run945 remains the accepted game-pixel checkpoint.

Host121/run961 follow-up now passes the full strict cold/reload chain with the
indexed native bounds: generation93/instance144 retires before207/385, both
900-emission windows pass, and fresh material/receiver/light/cutout gates pass.
330 Python checks and output CPU24 pass; query shaders are unchanged. Bounded
readiness transition reporting is connected, but this run observes only two
window starts and no reset. It does not explain960's timeout or fix a known cause.

Its separate fresh-culling image trigger sees no positive skip delta in the
sampled interactive-field windows (304 cumulative skips are already present at
the end of the opening event). The supervisor exits1 for the missing image after
the strict text gate passes. Its early-stop condition is corrected locally to
wait for both requested outcomes within the same limit; no unchanged retry.
Readiness reporting remains bounded64 lines, and the900-emission/250 ms gates
remain strict. Captured pixels, if any, cannot satisfy or bypass reload acceptance.

Current connected bundle: moving-view/depth visibility through current-frame
native depth and the existing rigid queue, indexed bounds and native image
owners. The production exact-camera query-history path is now removed; its
historical evidence below is not the current implementation. Controlled
hidden/visible game pixels, the tree-gap investigation, stereo and the full
desktop gate remain required.

The GPU side of this bundle is now built and verified: `native_depth_visibility.*`
and `native_visibility_*` build a current-depth maximum pyramid and native indexed
commands. GPU46/visibility3 pass72 Vulkan cases, including perspective, odd sizes,
all1/2/4/8 MSAA samples, ordered same-image camera/depth changes and actual indirect
pixels. Shared byte/owner budgets include readback; scratch refresh allocates no
new buffers. `DrawCommand` records exactly once; `Seal`/`CollectAfterFence` validate
the real command fields and distinguish generated/culled/drawn instances. GPU47
adds a compile-verified noncopyable reservation (no shader/behavior change).
334 Python checks,46 rigid cases and8 query regressions pass; Vulkan validation0/0.

Host123/run962 now connects that runtime consumer: retained depth/world bounds,
exact batch camera compatibility, shared work across all slots, refresh after
non-native depth writes and complete emitter-state restoration after compute.
Actual native draws use `DrawCommand`; all three submission paths seal work,
and real slot-fence completion precedes receipt collection and retirement.
Production query pools/history/filter and their obsolete build entry are removed.
339 Python checks, expanded output42/CPU25 and the host link pass.

Per-item pending/visible/culled/retired classification replaces assumed equality.
Both strict cold/reload epochs pass without reducing900 actual visible emissions
or250 ms readiness; cold generation93 closes1658 scene/shadow submissions at the
real fence, followed by new generation207. In the fresh reloaded4634..4934 window,
3,248 generated/draw-recorded commands yield3,247 collected receipts:3,236 visible
and11 culled instances. These are actual indirect draws, not a speedup or broad
scene qualification. This sample has no merged native batches;959 retains that
distinct proof. The run terminates normally and restores the exact owner profile.

The captured97,976 B window JPEG **fails visual acceptance**: title-logo pixels
conflict with the logged interactive field/movement context. No cause or fix is
claimed. Keep945 as last accepted mono pixels and956's tree-gap evidence.
`gpu/screenshot.*`/`ServiceOnPresent` now retain frame/request/source identities
and collect at the actual slot fence, replacing present-counter retirement.
Eight production-readback Vulkan cases pass, including padded row widths and
later source writes/release. The existing scenario checker validates matching
frame/fence/JPEG receipts separately from reload and visual acceptance.

Host125/run963 passes the strict cold/reload chain and records post-gamma
frame5070, but the saved65,536 B JPEG is truncated; strict decode refuses it.
No pixels qualify and962 remains unexplained. Host126 now uses a fixed-capacity
output stream with distinct written extent, overflow refusal, complete JPEG
markers and explicit4:4:4 chroma.350 Python checks and the expanded CPU fixture
pass:153,370 B full-resolution JPEG fully decodes, block-centre colour error4
within the unchanged limit12. Tiny-image tests had missed the larger boundary.
Run964 reuses host126 and passes the strict cold/reload chain. The frame5005
probe is refused by the bounded encoder/export; no JPEG is written. This does
not qualify pixels or explain962. Its log/request and963's truncated export
remain preserved; the latter now has explicit truncated-evidence filenames.

The larger per-image budget is awaiting owner approval; the unchanged10 MiB
aggregate archive also needs independently fitting overlap before another probe.
Do not overwrite preserved probe paths or boot until the existing archive fits.
That observation must distinguish stale window capture from a wrong presented
frame; repeating PrintWindow or relaxing context checks cannot answer that.
Host164/run971 reproduces this discrepancy during the Toon integration: fresh
scene/reload receipts pass, but the full-resolution window image shows the title
logo. Preserve that image and962; neither qualifies character pixels or proves
the renderer-fence image would be wrong. No further unchanged PrintWindow retry.
Do not build another capture/renderer framework or make the selected asset a
permanent ceiling on scene ownership. Current-depth controlled pixels, broader
scene/animation/event/both-eye gates and Quest readiness remain open.
[Current-depth GPU contract and integration findings](../research/20260907_2209_native-depth-visibility.md).
[Runtime receipts, live checks and image discrepancy](../research/20260907_2256_current-depth-runtime.md).
[Frame ownership, encoder correction and remaining gate](../research/20260908_0006_native-frame-provenance.md).

Independent ownership connection, host127: immediate draws now bind their
complete cached pipeline/vertex/index inputs, independent of logical dirty bits.
Queued native/reordered/pulled work can no longer leave those physical bindings
implicitly owned by the previous draw. CPU preparation/caching and deferred
value snapshots remain; no new guest imports or forced pipeline compilation.
351 Python checks, the expanded C++ draw fixture and52 two-eye Vulkan cases
pass (1.45 s, validation0/0). Six added cases independently poison PSO/VB/IB:
controls without restoration lose geometry, production restoration matches the
unchanged colour/depth oracle. Host127 links without guest object compilation.
No new game run or image: live interoperation and all existing visual failures
remain unqualified. This is a verified handoff contract, not evidence that it
caused962. Continue scene ownership using the same native consumers while the
bounded post-gamma observation awaits its budget; preserve all final gates.
[Implementation and causal regression evidence](../research/20260908_0033_immediate-geometry-bindings.md).

Host128 validates the source-only deferred groundwork committed in8d6f4e4,
plus independent sorted cutoff handling and delayed Bind/Keep light recipes.
The existing direct lighting path now shares capture/resolve; inherited values
are resolved at actual draw order, not frozen during speculative preparation.
Three C++ fixtures and351 Python checks pass. GPU rigid17 passes55 two-eye
cases (1.33 s, validation0/0), including overlapping source-over draws driven by
mixed-order metadata, an unsorted control and depth-write-off. These use native
production shaders, not actual compatibility shaders or the live deferred list.
Host128 links without guest objects; no game run or image was produced.

Host129 now connects `PrepareNativeRigidSceneForObject` -> owned
`NativeRigidSceneSubmission` -> `SubmitNativeRigidScenePackets` -> existing
queue/fence ownership for the current direct path. Backend consumption has no
pose, model lookup or active object-scope dependency. The producer captures only
Bind/Keep recipes; the consumer resolves and transactionally packs whole-node
lights before the outgoing compatibility mirror and backend lock. Packet camera
rows are checked against the active command scope, and depth-write policy reaches
both the pipeline and queue metadata. The immediate-only light getter/scope-bound
commit adapter are removed. Light-read counters now count whole-node resolutions,
not speculative per-primitive preparations; do not compare those totals as FPS.
Two C++ fixtures,352 Python checks and55 two-eye Vulkan cases pass (1.26 s,
validation0/0); host129 links without guest objects. No game run or new pixels.
[Owned consumer implementation and evidence](../research/20260908_0135_owned-scene-consumption.md).

Host130 connects the ordinary rigid deferred producer, bounded owned packet queue
and mixed payloads in `ConsumeDeferredList` to that same native consumer. The
temporary opt-in is `bd_native_rigid_deferred` (default false, requires native
scene and deferred consumer). Deferred siblings allocate no816-byte guest entry;
direct siblings remain immediate. One stable mixed order preserves submission
ties and resolves each delayed light action at its draw position. Native entries
skip model callbacks, per-entry resource/world/constant imports and D3D draws.

Live callback slots are checked against the recovered ordinary contract: lights
then shader for models, known shadow participants then shader for visuals, and
no-op visual resource begin/end. Visual transitions remain an explicit temporary
adapter, with source identities only in its sidecar. Native blend/alpha and
receiver-colour publications are consumed after visual begin, not frozen at the
walk. Unknown/changed contracts, stale/reentrant queues, capacity overflow or a
lost consumer refuse without replaying accepted packets through the guest list.
Two changed C++ fixtures and353 Python checks pass; host130 links without guest
objects. Existing55 two-eye shader cases are reused, not rerun or claimed as
mixed-list integration. No new game run or image.

Run966 now passes the full strict mono cold-field/title/reload text chain with
the deferred opt-in. Cold1800..2100 stages/consumes10,815 native packets alongside
5,570 legacy draws; reloaded4500..4800 stages/consumes3,825 alongside7,609 legacy
draws, with no pending native packets in either sampled window. Generation93/
instance144 retires, then207/388 supplies the new scene. The existing scene/caster/
receiver/light/cutout and source/GPU regression checks pass independently in both
epochs. Native scene GPU counters include direct and deferred instances; they
are not per-deferred-family fence receipts. No pixels or speedup are claimed.
`--rigid-deferred` now uses fresh mixed-consumer receipts and the existing
consecutive-ready-window rules; it also checks both epochs with `--rigid-reload`.
357 Python checks pass. Renderer sources/binary are unchanged from host130.

Host132 removes registry/participant and blend-setter callback dispatch from
ordinary visual transitions opened by native deferred work. Existing lifecycle,
receiver, blend/alpha and scene-light owners feed `NativeDeferredEffects`, with
transactional frame/image/matrix checks. A separate outgoing port preserves
material zero/late-restore and active bytes for unmigrated draws. Same-visual
native/legacy alternation does not add transitions. Two rebuilt C++ fixtures,
32 CPU tests,360 Python checks and host build pass; no guest objects rebuilt.
Run966 remains evidence for host130, not this binary.

Run967 now passes the strict host132 cold-field/title/reload chain, including
`--deferred-effects` independently in both epochs. Cold1800..2100 consumes10,957
native packets through3,383 paired native visual scopes alongside6,124 legacy
draws. Reload4500..4800 consumes2,758 through234 scopes alongside7,667 legacy
draws. No native pending work, fallback or refusal; old93/144 retires before the
new207/388 scene. Existing receiver/light/caster/cutout and source/GPU gates pass.
These are consumer/lifetime receipts, not pixel or timing qualification.

Source checkpoint after host132: the native per-entry visual-address sidecar is
removed. `NativeVisualPublication` owns bounded instance/generation-keyed blend
and diffuse-class values, collected through the existing instance source index
after scene preparation and before list drain. Receiver publication/read uses
the same native keys; dynamic receiver colour, blend/alpha and lights stay late.
Source retirement before this handoff refuses; retained values cannot alias a
reused generation. Unknown legacy resource callbacks inside the batch refuse.
Host133 builds, but run968 fails at the first water resource callback before
field readiness. Dump/source analysis identifies82454720/824548A8, not an
arbitrary writer. Host134 separates native no-op omission from executing water
callbacks: the existing whole-function writer republishes native input values
after all effects, including indirect parameter aliases. Native scopes retain
no source identity; unknown resource writers and failed refresh still terminate.
Deferred C++ failures now log and use the existing fatal shutdown rather than
creating an unhandled-exception dump.32 CPU tests,364 Python checks and host134
pass. Run969 now passes strict independent cold/title/reload epochs, including
fresh initial publications AND completed-writer refreshes: cold1800..2100 consumes
10,865 native packets with300 batches/959 visual publications/175 refreshes;
reload4500..4800 consumes3,053 with300/322/139. Native pending0, visual scopes
balanced, old93/144 retires before new207/434. Other receiver/light/caster/cutout
and source/GPU gates pass. No new crash dump/capture; profile restored. Run968
and its symbolized cause remain failure evidence; no pixel/speedup claim.

Water shader/input checkpoint (host135): `native_water_inputs.h`,
`native_water_material_source.h`, `native_water_program.*` and the native water
VS/PS now consume semantic materials, tangent-bearing native meshes, six explicit
image bindings and existing native light/fog data. The existing GPU fixture runs
16 two-eye cases with real indexed instancing and native scene snapshots;
55 unchanged rigid cases also pass, validation0/0. This is a built native consumer,
not live water conversion: no game hook or queue admission changed. Native wave
deformation/material changes need authored-scene/art review. See
[water program evidence](../research/20260908_0435_native-water-program.md).

Water queue/geometry checkpoint (host138): `SubmitNativeWaterScenePackets` now
uses the existing native store, upload arena, descriptor emitter, current-depth
visibility and fence retirement. Water packets retain all six image/view owners,
have their own976B instance ABI and can coalesce consecutively without reordering.
Native mesh load supplies tangent inputs and indexed maximum absolute colour-red
weight; queue bounds include world-Y wave displacement. Active colour/depth and
MSAA resolve destinations cannot be sampled. Legacy depth/eye sorters, blended
gathering and depth-prepass scheduling now preserve ordered native boundaries.
16 two-eye GPU cases exercise the
shared packer/binder and owner retirement; CPU scene/batch tests and55 rigid GPU
regressions pass. The fixture does not execute `DrawQueueFlush`, and no game
producer calls the new submission entry yet. No new live family is converted.
[Queue evidence and limitations](../research/20260908_0509_native-water-queue.md).

Image handoff checkpoint (host139): all live native target/shadow, post,
scene-snapshot and resolved-depth publishers use typed `NativeImageLease::From`
handoffs retaining their own full-array sampling views. The water queue now
consumes these leases directly, including pooled post images and resolve owners;
it no longer requires dynamic inputs to be `NativeTargetImage` allocations.
Image-only leases cannot supply an adapter-owned view. CPU tests cover retained
resolve outputs/source-framebuffer lifetime, distinct roles, shape and pool reuse;
16 two-eye GPU cases now sample the real post-image pool with queued-reader
overwrite refusal. Host139 and371 Python checks pass. No live water admission
or additional converted family is claimed.
[Image-owner connection evidence](../research/20260908_0940_native-water-image-leases.md).

Bottom-depth checkpoint (host141): complete begin/end hooks now reuse native
depth, framebuffer, command, image-lease and fence owners, replacing the eligible
pass's console surface create/clear/resolve/release lifecycle. The explicit
WaterBottomDepth slot is not inferred from square dimensions or shared with sun
shadows. Completion retains D32 depth plus its producing world projection;
camera fit and caster/sampling adapters remain. Testing real depth instead of
the fixture's previous HDR placeholder exposed Plume's cross-framebuffer deferred
clear merge. Fork commit191c31c preserves the original framebuffer/view owner.
17 two-eye water,55 rigid,8 snapshot GPU cases, CPU owner tests and372 Python
checks pass. Host141/run970 passes strict mixed cold/reload regression, but has
no observed bottom-pass invocation/allocation. It does not qualify this new pass
in game or add a native water-family caller.
[Bottom producer and causal backend evidence](../research/20260908_1031_native-water-bottom.md).

Planar-reflection checkpoint (2026-09-08, host142/run971):
`native_reflection_pass_bridge.cpp` replaces eligible begin/end
attachment/clear/completion work with an exclusive HDR lease from the existing
post-image pool and a native ReflectionDepth target. The existing framebuffer
and command owners now accept leased colour attachments, retaining them through
fences; completion publishes the exact rendered image without a copy or console
resolve. Active reflection scopes also expose the existing native snapshot path.
CPU59/output35 passes mono/layered clear/completion, output identity, reader/fence
retention and shared MSAA regressions. GPU65/water12 passes18 two-eye cases using
the actual native reflection colour/depth/framebuffer/completion producer, including
an empty reflection;55 rigid and8 snapshot cases also pass, validation0/0.
373 Python checks pass. Host142 links without guest object rebuilds. Run971 passes
the strict mixed cold/reload chain and observes3,900 completed reflection publications,
zero compatibility/faults and fresh300-publication windows after readiness in each
epoch. Nine initial camera misses stop increasing; their cause remains unqualified.
No scene-snapshot call was observed, so reflection-phase snapshot integration remains
open, as do actual game pixels and HDR art parity. Do not infer those from fixtures.
Authored camera calculation, extent/getter update and legacy draws remain adapters.
This does not add a game caller to the native water queue or qualify a full native
frame. [Reflection producer and evidence](../research/20260908_1110_native-reflection-pass.md).

Water draw connection (2026-09-08, host144/run972): completed resource writers now
publish copied water values and typed bump/environment/planar/snapshot leases in a
single ordered, instance/generation/frame-keyed scope. The mixed consumer calls
`SubmitNativeWaterScenePackets` before `BindEntry`/`SubmitSurface`; admitted entries
skip legacy geometry/constant binding and translated drawing, and cannot replay an
older captured water entry. The existing model index supplies exact node/range
associations, not a new address cache. Native packets retain model geometry, pose,
images, lighting and current alpha/blend/depth inputs. Water alpha comparison and
MSAA coverage intent are connected; early undisplaced walk culling is deferred until
the shared queue has finalized wave-expanded bounds. Native water sampling explicitly
uses wrapping linear normals, clamped screen/cube inputs, point D32 shore sampling
and comparison sun sampling; no slots0..4 filter-publication assumption is made.

CPU61/output37 checks sorted node identity, retirement/reuse and ambiguity. GPU67/
water14 passes20 two-eye cases, including actual discard colour/depth behavior and
material/image publication retirement;55 rigid and8 snapshot cases also pass,
validation0/0.375 Python guards/scenario checks pass. Host143/144 link without guest
object compilation. Run972 passes the strict mixed cold/reload chain and observes
native water in both epochs: last sample3179 submitted,3084 emitted,93 culled,3177
fence-retired, zero admission refusals. Fresh post-event windows advance emissions
and retirement. The new generation starts with every preceding water packet retired.
No new images/raws: this is live draw/lifetime evidence, not game-pixel or HDR art
qualification. [Implementation, failure correction and evidence](../research/20260908_1146_native-water-draw.md).

Direct water material connection (2026-09-08, host145/run973): admitted main-view
entries now call the native producer/queue without model/resource callback dispatch
or translated shader selection. Owned light tickets commit once BEFORE the water
writer, preserving late aliased parameters instead of exporting lights again after
the writer. Completed output refresh, native submission, outgoing depth intent and
slot7/12/optional13 cleanup remain ordered. Model registry state stays idle; native
draws do not fake active participants. Changed/unknown model contracts refuse before
effects; lost late ownership is terminal, never a partially replaced replay.

The existing deferred CPU fixture tests the production lifecycle, all admitted
technique branches, dirty/unknown registry refusal, late snapshot selection and
failure at every stage.376 Python checks and the host build pass. Run973 passes the
strict mixed cold/reload chain, including neighboring legacy inputs/light checks:
last sample3159 balanced direct begin/end pairs,3159 submissions,3071 emissions,
86 culled,3157 retired, no admission refusals. Cold1551..1851 and reload3948..4248
both have300 new begin/end/submission/retirement pairs; emissions293/300. Old water
generation94 fully retires before208's first submission. Unchanged GPU shader/backend
evidence is reused; no new game pixels/HDR/stereo or bottom/snapshot qualification.
[Direct material source contracts and evidence](../research/20260908_1212_native-water-material.md).

Shared water visual connection (2026-09-08, host147/run975): known sorted water
visuals now reuse the existing native receiver/class/blend scope and late identity/
generation publication, including water-only lists. Receiver writes precede a fresh
current-visual import; completed water writers still refresh cross-visual aliases.
The material output owns late object inputs, and the unused callback-capture pointer/
legacy publication branch is retired. Ordinary rigid model admission is unchanged.
Host146/run974 passed its regression chain but did NOT reach the new water scope:
resource class was incorrectly assumed to imply visual type8. Exact extra1 branches
and exhaustive type tests fix that admission gap. Run975 observes actual type5,
3109 paired water visual scopes at4500 and3159 direct materials/3066 emissions/
3157 retirements at4549,0 admission refusals. Strict cold/reload/mixed checks pass;
generation94 fully retires before208 submits.377 Python checks and deferred CPU
tests pass. No new images, HDR/game-stereo or authored bottom/snapshot qualification.
[Source contracts, causal regression and evidence](../research/20260908_1236_native-water-visual.md).

Native water producer connection (2026-09-08, host151/run978): admitted whole nodes
now bypass the command interpreter and source-list allocation. The existing shared
queue owns pose/model leases, primitive identity, sort depth, alpha/cull, light recipes
and image leases; late material production still occurs at its ordered consumption
point. Load-time recipes now capture normal-table commands and environment samplers.
Run977 identified the actual normal-map command that blocked eligibility; focused
CPU tests cover normal repetition, disable/Keep and phase1 selection before retry.
Run978 passes strict cold/reload/mixed checks. Fresh ready-field water queue windows
1800..2100 and4200..4500 each stage/consume300 records. GPU windows1563..1863 and
4250..4550 each submit/retire300, with282/272 emissions respectively. Old water
generation94 fully retires before208 submits. Latest queue3111 paired records;
later GPU3162 submitted/3065 emitted/3160 retired.377 Python checks, material and
post-output CPU behavior fixtures and incremental host build pass. No new game
pixels/HDR/stereo qualification; unchanged shader evidence is reused.
The opaque per-record bridge still retains source image keys for checked outgoing
mixed-frame exports and a visual key for late material production. This is not an
entirely source-free queue or frame. Special image overrides, direct/skinned and
other unadmitted producers remain explicit legacy families.
[Producer verification and storage](../research/20260906_0333_native-scene-state-bridge.md#native-water-deferred-producer-2026-09-08-source681f1fd-plus-edits).

Completed water frame-image connection (2026-09-08, host155/run980): the named
reflection pass now publishes its exact completed native HDR lease directly to
water. Incomplete/stale/compatibility writes invalidate future reads; queued leases
survive replacement and retirement. Other reflection planes cannot replace this role.
The water snapshot call now receives the producer's exact result, not a callback
followed by a getter lookup. Its authored scheduling/getter/slot exports remain
producer adapters. An outgoing reflection-mirror check refuses unknown late aliases.
378 guards, CPU publication/lifetime tests and strict cold/reload/mixed checks pass.
Each ready-field epoch advances300 reflection publications/reads; latest3900/3171.
Water3158 submitted/3069 emitted/3156 retired, unavailable0, old generation94 fully
retired before208. No authored snapshot/bottom or game-pixel/HDR/stereo qualification.
[Contracts, current verification and remaining adapters](../research/20260908_1345_native-water-images.md).

Next connected work: remove remaining water producer/late-material image and state
adapters, including outgoing sampler/world/state exports when their mixed consumers
no longer need them. Reuse the connected publication/queue, water update/refraction
setup and existing instance/model owners. Native water's completed planar/snapshot
selection is now connected; replace remaining bump-table, authored feature and
producer-side timing/getter imports with native
producer inputs while preserving late aliases, Bind/Keep and snapshot ordering;
do not freeze them at the initial walk or recreate owners. Complete image-role
production for currently unavailable cases, not a permanent fallback family.
The observed water had no admission refusals, but no authored bottom/snapshot call
was observed. Establish their scheduling gate or an authored active scenario before
another live probe; unchanged bg41_01 retries add no evidence. Shore must retain its
completed bottom depth/projection pair, never a guessed transform or colour snapshot.
Reflection still has authored camera/extent/getter/draw adapters and initial camera
misses (10 in972, unchanged after startup), not a solved full-frame path. The shader
supports per-eye inputs, but actual game stereo producers, water shadow variants,
authored controls and game-pixel/HDR/mixed-order sequence coverage remain open.
Keep strict unknown-writer refusal, outgoing compatibility state and fence ownership.
Do not repeat the callback census or build a second renderer.
Full native frame, broader authored effects/scenes/reloads and both-eye acceptance
remain open.
[Native effect scope connection and verification](../research/20260908_0251_native-deferred-effects.md).
[Writer-ordered native inputs and causal failure](../research/20260908_0337_native-visual-inputs.md).
[Native deferred live qualification and archive locations](../research/20260908_0225_native-deferred-live.md).
[Sorted producer/consumer connection and evidence](../research/20260908_0206_native-deferred-connection.md).
[Deferred contracts and pending runtime connection](../research/20260908_0109_deferred-packet-order.md).

The superseded940 mono reload JPEG was reviewed and retired, leaving172,616 B
image headroom;945 accepted and956 tree-gap images remain. The planned110 KiB
run960 image was not produced. Preserve960's timeout evidence along with957/948/941.
Run961 also produced no image; its506,703 B log is the current strict reload proof,
while959 retains distinct real multi-instance batching evidence.
[Indexed native bounds, current result and storage](../research/20260907_2107_native-indexed-bounds.md).

### Delivered connection: owned scene lighting consumed by native rigid draws

The direct consumer now uses `NativeSceneLightingPublication`: semantic lights
and native instance/model/node bindings published after DrawEnd, scene preparation
and pose/light transfer, before DrawStart. `PrepareNativeSelectedLightValues`
and `PreviewSelectedLightValues` were removed. Native consumption no longer reads
dirty masks, selected source slots, cached shader IDs or legacy descriptors.

| Boundary | Existing owner | Current result / remaining work |
| --- | --- | --- |
| Authored updates -> owned scene-light data | `bdLightListUpdateSnapshot` in `frame_interp.cpp`; existing selection/composition math and lighting bridges | Bounded300 semantic records, scoring/priority/exclusion data and update identity. Pass-owned lighting view remains distinct from render view. Original authored updates/snapshot helper still execute at the producer. |
| Object updates -> selection inputs | Existing native instance source index and model/pose owners | Copies class/centre/radius and explicit Bind/Keep at handoff, keyed only by native instance/model/node. Last live result had2,898 explicit bindings; new inherited actions/tickets are built and CPU-tested, pending game acceptance. Missing poses/invalid bindings remain unowned. No guessed bounds or draw-time light reimport. |
| Native light/object inputs -> actual scene packet | Existing `PrepareNativeRigidSceneForObject` and native scene/shadow queue/programs | Pure owned selection/composition, copied packet values survive replacement/retirement. Existing instancing/indirect/image/fence path unchanged. Current live acceptance remains the selected rigid consumer, not all imported bindings or a whole scene. |

CPU fixtures consume after source destruction, reject stale frame/update/native
identities, and exercise changed colours with unchanged IDs, directional/point/
spot, priority/exclusion, per-node overrides, limits and copied-value lifetime.
Run944 proves fresh native consumption in both reload epochs; one inspected image
is sanity evidence, not a sequence, animated-light matrix or both-eye acceptance.
Unchanged shader-fixture evidence is reused. Keep unsupported families explicit.

Keep run941's exact failure and strict comparison. Establish its affected
boundary with a targeted observation when needed; do not assume it is unrelated
or claim a successful rerun fixed it. Independent implementation of the owned
light path can proceed while that regression is open. The affected runtime
behavior remains unqualified until the cause is resolved or the replaced
dependency's removal and equivalent native behavior are proved. No disabling
checks, counter resets, weaker thresholds or relabelled passing evidence.

### Next delivery bundles

1. **Scene-level rigid ownership and representative batches.** Complete remaining
   object/receiver/camera/pass producers, inherited node-light inputs and explicit per-eye inputs using the
   existing owners. Grow beyond the single asset to representative supported
   scene-shaded objects, culling/occlusion and real multi-instance groups. The
   opaque caster family now handles whole multi-primitive nodes without material
   ID or scene texture-layer restrictions; its submission, actual emission and
   fence gates pass in both reload epochs. Skin/wind, phase0 cutout casting, specialized scene deferred and
   texture-dependent effect participation remain explicit unsupported families.
   The new ordinary0..3-layer shader, independent third UV owner and whole-node
   scene submission and ordered light inheritance are implemented. Host110/run948
   clears the observed non-specular object's unused-channel refusal and exercises
   wider native scene emissions. Next, recover the cause of948's reloaded-field
   UV mismatch at `host_draw.cpp`'s channel16 comparison: trace the owned object
   UV recipe and intervening native/legacy writers; do not assume an outgoing
   mirror alone fixes it or rerun unchanged code. Keep the comparison strict,
   add a causal boundary fixture, then verify representative fresh
   layered/multi-primitive scene emission and retained lifetimes with inspected
   game pixels and selected-object reload regression. Verify actual inherited
   consumption and legacy interoperation; an unseeded/invalid inherited chain,
   layouts or other owners remain explicit refusal, not warm-up fallback. Preserve sibling,
   deferred and volume participation; no silent omission or warm-up fallback.
   The selected asset's cold-start hard-off/reload proof stays a regression case.
   Reuse `NativeObjectTextureScope`, owned instance/model/primitive data, existing
   receiver/camera/pass publications and native programs. Exit with representative
   multi-object native emissions using those owners, no legacy warm-up, plus
   culling/participation/lifetime and targeted pixel checks. Do not stop at another
   source-reader helper or claim all2,898 light bindings are native draws.
2. **Material families and characters.** Direct blended-cutout scene ownership
   is connected, host-built and CPU/GPU-tested, with live emission through reload;
   visual/interoperation acceptance remains open. Phase1 casting now has corrected
   source/CPU/GPU and live reload coverage for the actual fixed-alpha shadowmap
   and ordinary deferred participants. Host116/run956 proves fresh textured
   scene/shadow emissions and lifetime handling in both epochs. Its tree-trunk
   image gaps remain an unqualified visual boundary: distinguish authored
   camera fading/clipping, native scene geometry/material participation and
   retained consumers before selecting a fix. Keep representative layered,
   inherited-light and both-eye coverage explicit; do not infer them from reload.
   Preserve the new
   textured emission/fence gate and all ordering/legacy/pixel gates. Integrate these paired
   scene/shadow consumers, then wind and translucent
   materials; deliver native skeleton/skin assets, animation/
   pose production and GPU skinning as connected character paths. Test authored
   changes and field/battle/cutscene/shadow lifetimes. Cook only formats/assets
   actually needed, with stable IDs, persistence and bounded streaming.
   The native skin asset prerequisite is now implemented and CPU-tested:
   `CookSkinMesh` resolves authored palette slots into model-local joints;
   BDMESH v3 stores explicit paired joint-local positions/normals and weights,
   using the existing aggregate mesh budget. Do not collapse these into a single
   bind position without the skeleton/inverse-bind contract. Source-free indexed
   deformation/bounds passed the prerequisite fixture. The connection now passes
   CPU/GPU fixtures and host158 compilation: load-owned skin bindings/counts feed
   `CookSkinMesh` and the shared geometry store; animated bounds and completed
   instance poses feed bounded, deduplicated palettes in the existing queue/fences.
   Six new Vulkan cases cover1/2/3 influences with single and instanced casters;
   all55 prior rigid cases,20 water cases and8 snapshot cases pass, validation0/0.
   CPU coverage includes source retirement, pose replacement, exact palette
   capacity, shared-pose reuse, prefix splitting and conservative FP32 bounds.
   Earlier run981 stopped at the192MiB free-drop
   limit during reload, with no native skin emission. Targeted983 proves native
   skin geometry exists, but generation97/node47 (4primitives) and node62
   (6primitives) reject for `unconverted cutout sibling`. Generation38/node1
   (44primitives) uses technique1 with texture-dependent participation.
   **Current connection verified through cold-field emission (host160):** skin
   deformation shares the shadow cutout image/UV/wrap-sampler producer and fixed
   0.6-alpha shader. Technique1 now consumes the existing exact object/pose/pass
   scope's texture-classified policies; admission checks all siblings against
   native alpha/cull/order semantics. Unknown/volume and mismatched policies do
   not become ordinary draws. Pre-cull animated bounds use that same admission.
   Mesh18/CPU16, output70/CPU45 and host160 pass. GPU70/rigid25 passes85 cases,
   including30 skin cases across1/2/3 influences, cutouts, alpha boundaries and
   overlapping instances (validation0/0). CPU tests include4/6/44-sibling packets,
   missing inputs, late volume/unknown, policy mismatch and source retirement.
   All383 Python checks pass, including fresh skin emission/fence checks required
   independently in each reload epoch.
   Host159/run984 passed the full strict cold/reload/mixed regression, but emitted
   no native skin. Host160's11:37 run (PID37048, reused log978, NOT the older water
   run978) emits/retires13200 skin primitives in the fresh1857..2157 cold-field
   window. It stopped during reload at the192MiB drive-free-drop guard before its
   requested image. Full skin reload/game-pixel/stereo qualification remains open.
   The reached native node consumer bypasses its original node/interpreter/replay
   and bone-register draw path; object/visual setup and animation producers remain.
   **Ordinary skin scene connection (host161):** native joint-local position and
   normal shaders now share the material/light/image owners, animated culling,
   ordered/deferred plans, indirect batches and pose-retaining frame fences.
   CPU47 and host161 pass; GPU27 passes154 cases including69 new skin scene
   cases (1..3 influences, layers, lighting, cutouts, distinct instances, both-eye
   pixels, validation0/0).387 Python checks pass. Host161's12:15 run (PID22748,
   reused log969, not the historical969) passes the full strict cold/reload/mixed
   chain and fresh skin-shadow windows,13200 emissions/fences per300frames in
   each field epoch. Ordinary skin scene draws emit/fence-retire5661 primitives
   during each opening event, then remain flat in interactive fields; the new
   separate scene gate correctly stays Pending. One1920x1080/105571B inspected
   window shows the player/ground shadow and terrain; no character parity,
   stable sequence or both-eye qualification is established. Technique1 scene
   shading, wind and other specialized materials, native animation and remaining
   replay/register producers remain open. Next connect representative ongoing
   character shading (including the distinct technique1 contract) through these
   owners, using the reached event consumer as regression coverage; do not retry
   unchanged code to turn zero post-event scene emissions into a passing gate.
   Preserve all existing visual failures and the complete desktop acceptance.
   No full character/frame completion or speedup is established.
   **Native Toon surface consumer (host162), authored connection (host164):** explicit owned light/ambient
   adjustments, texture tints and alpha policy now feed scene plans and the
   shared rigid/skin GPU program. CPU48 passes; GPU28 passes345 cases including
   191 added Toon/mixed cases, validation0/0;388 Python checks pass. Host164 now
   connects authored scene/object remaps and ordered texture tints to owned
   packets, classified whole-node admission and animated pre-culling. CPU50 and
   the host build pass; shaders are unchanged. Missing specular RGB remains
   unknown, not guessed black. Fur/outline/lattice and sorted Toon stay excluded.
   Host164/run971 now passes the full strict cold/title/reload/mixed chain,
   including fresh scene and shadow skin consumption independently in both
   interactive fields: scene1428..1728 and3964..4264 each emit/fence-retire13200.
   Generation93/instance144 retires before207/480. This replaces host161's
   startup-only scene observation; repeated draws are not unique asset counts.
   The new1920x1080/101438B window image shows the title logo, contrary to the
   logged field context. Pixels fail acceptance; no cause or fix is inferred.
   Preserve this and962's discrepancy. Next distinguish window capture from
   actual post-gamma output through the existing fence-owned frame probe once
   its explicit image allowance/aggregate overlap fit; do not retry PrintWindow
   or relax the scene checks. Native animation and specialized families can
   continue independently, but character parity/sequence/both eyes remain open.
   [Live scene/reload evidence, visual failure and storage](../research/20260908_1334_native-toon-live.md).
   [Authored connection and source correction](../research/20260908_1318_native-toon-producer.md).
   [Earlier GPU contract and fixture evidence](../research/20260908_1237_native-toon-surface.md).
   [Scene connection and verification](../research/20260908_1210_native-skin-scene-shading.md).
   [Current consumer, exact evidence and storage](../research/20260908_1143_native-skinned-shadow-consumer.md).
   [Connection verification and exact refusals](../research/20260908_1100_native-skin-caster-verification.md).
   [Exact contract and evidence](../research/20260908_1410_native-skin-assets.md).
   **Native render-pose connection (2026-09-08, host165):** completed
   handoffs now record host tick endpoints in the existing bounded instance
   owner. Native culling, scene/shadow plans and retained skin palettes request
   the same immutable frame-phase pose; raw completed poses still feed source
   comparisons and unconverted consumers. No guest interpolation scratch is
   added. Admitted native water now retains the same render lease for sorting,
   drawing and wave bounds; raw poses still serve outgoing mixed-frame world
   exports. Material44/CPU42 and output80/CPU52 pass timing, discontinuities,
   late writers, identity, rigid-only/mixed/skin bounds, actual scene/shadow
   plans, water queue retirement and shared-budget tests.390 Python checks and
   host165 pass, without guest object compilation or shader changes. Host165/run973
   now passes the full strict cold/title/reload/mixed/skin scene+shadow chain with
   a temporary 60 FPS cap and independent fresh render-pose checks. Cold 1870..2170
   adds 1,287 blends / 37,961 shared reads; reload 4187..4487 adds 951/37,529, refused 0.
   Skin scene emits/fence-retires 13,198 and 13,200 in its respective fresh windows;
   shadows add 13,200 each. Old 93/144 retires before 207/419. 394 Python checks pass.
   This proves live consumption, not visual motion, achieved 60 FPS or speedup.
   Next qualify renderer-owned motion/pixels under the existing storage gate;
   ordinary native evaluation is connected below; animation channels and late
   writers remain implementation work. The known
   title/window discrepancy still precludes another unchanged PrintWindow run.
   Original animation evaluation/collision/effect side effects, conditional
   source-palette copying, secondary palettes and legacy interpolation remain.
   Do not restamp host164/run971 as verification of this source.
   [Render-pose connection, verification and remaining gates](../research/20260908_1440_native-render-poses.md).
   [Fresh interpolation/reload evidence and storage](../research/20260908_1505_native-render-pose-live.md).
   **Ordinary native skeleton evaluation (host167, opt-in):** load-owned joint
   hierarchy/TRS/pre/post rotations and update-time channels now drive native
   whole-pose evaluation through the existing instance owner, completed handoff,
   interpolation/culling and skin consumers. Root ABI, parent-scale compensation,
   rotation order, source destruction and lifetime/budget fixtures pass;402 Python
   checks, material46/CPU44, output81/CPU53 and host167 pass, no guest/shader build.
   Host166/run974 reaches36331 matching evaluations with zero unavailable/drift;
   fresh cold1879..2179 and reload4157..4457 add2904/2640 checked publications.
   Its full chain FAILS because water-only scopes/inputs polluted ordinary
   counters. Host167 partitions accounting by family, keeping shared publication
   and rendering unchanged; strict verifier and old failed evidence are retained.
   Run975 verifies the corrected cold-field boundary, but does not complete its
   title/reload scenario: a260432us input-poll gap resets the strict900-emission
   readiness window at843, then a natural field exit interrupts the next window.
   Preserve975; do not retry unchanged code or relax readiness to claim PASS.
   Native skeleton math has scoped live comparison evidence, not full integration
   or motion pixels. Normal defaults remain unchanged. Next recover/own animation
   curves/channel inputs and remaining late-writer/secondary/view-dependent bone
   contracts through these owners; investigate the exact readiness interruption
   before another reload probe. Original collision/effect side effects, outgoing
   palettes/completed source copy, unsupported bone routes and full desktop
   sequence/both-eye gates remain. No speedup/full host frame or Quest claim.
   [Skeleton contract, live scopes and preserved failures](../research/20260908_1544_native-skeleton-evaluation.md).
   **Selected-motion residency and sampling (host170, opt-in):** completed
   standalone/packed loads register source lifetime in the same bounded index;
   selected slots prepare immutable named type2/type3 curves before sampling.
   Run977 exposed eager load-order starvation: 124 clips filled the8MiB cap and
   zero selected calls reached native sampling. Dormant catalog entries now
   consume only bounded metadata; active curves, aliases and retired leases share
   the unchanged cap. Recent selections/leases prevent eviction; failed imports
   retry only on increased available budget or source generation changes.
   Steady-state slots/samplers do not decode source keys. Dormant source-backed
   registrations remain a temporary streaming boundary, not native clip cooking.
   Whole reset and weight1/full-root preserve sampling reuse the existing model
   names, channel/skeleton/instance owners and strict original comparison.
   Material53/CPU51,406 source/scenario checks and host170 pass, including
   load-order pressure, pins, retirement, source reuse/destruction, exact budgets
   and existing exhaustive compact-float/Hermite/angular curve fixtures.
   Run978:12989 checked/sampled,wrong0; whole27,preserved12962,cubic1. Reported
   resident peak942672B; two prepare failures remain unclassified. This proves
   reachable sustained channel sampling, NOT the repeated256-cubic observation,
   scoped field/reload/skin/motion pixels or both loader-retirement paths. Preserve
   977's starvation diagnosis and978's recorded insufficient cubic coverage;
   the newer layer bundle below supersedes978's runtime log, not its report.
   **Weighted/subtree/layer connection (host172, opt-in):** owned authored rest
   values, weighted keyed/cubic application, hierarchy masks and native whole-layer
   mixing now feed the same native skeleton/instance consumers. Broader
   bdAnimationUpdate scope prepares all six physical slots before direct
   whole-layer/subtree calls; original clocks/collision/effects remain intact.
   At host172, named/exclusion rules and nonzero Euler modes still refuse
   explicitly; the named-input extension below supersedes that named-rule limit.
   Layer composition preserves lone-channel semantics and in-place union-flags
   ordering in the temporary outgoing adapter; native math stays source-free.
   Material55/CPU53, shared output83/CPU55,408 guards and host172 pass, no guest
   objects/shaders. CPU covers rest/base activation, quaternion weights, subtree
   masks, source destruction,448 layer activation/weight cases and aliasing.
   Run979:13822 matching samples,113 weighted,0 subtree,380 cubic; requested
   >=256 weighted plus positive subtree gate FAILS. Run980:13858 sampled/checked,
   wrong0,23 mixed/checked,113 weighted,0 subtree,382 cubic; requested>=256 mix
   plus>=256 cubic gate FAILS. Earlier repeated-cubic call-count criterion is now
   observed, not advancing/interior-key or diverse-motion qualification. Retain
   new979/980 failures; neither ordinary matching calls nor startup totals prove
   full field/reload/skin/pixels.
   **Named inputs/importer extension:** host173 owns bounded inline model names,
   direct-search versus weighted-prune filters, constant timestamp handling and
   scale-tail hold. Material56/CPU54, output84/CPU56 and409 guards pass. Run981
   records4002 matching samples/wrong0, filtered/subtree0, and identifies both
   refused preparations as the SAME duplicate-descriptor clip, not two clips or
   exhausted residency. First-match descriptor canonicalization now passes
   material57/CPU55 and409 guards, including243 descriptor patterns x6 unique
   model traversal orders. Host174/run982 verifies live canonicalization:
   122 descriptors ->121 tracks/93424B,prepare-refused0,7590 matching samples,
   wrong0. The changed source address is not a stable asset identity. Its host
   build passes; no guest objects/shaders. Run981's superseded text is retired;
   named/excluded routes still need actual live coverage. Duplicate model names,
   nonzero Euler modes and nonzero entry depth still refuse explicitly.
   **Indexed format connection (host175):** type0 T/R and type1 T/R/S import into
   the same immutable clip/residency owner with joint-index bindings, no synthetic
   names or packed storage in the native asset. The existing layer/instance
   consumers handle dense header/scale/global/traversal differences. Material58/
   CPU56 and410 guards pass, including129 advancing samples per format,
   reordered identities, source destruction, ignored filter/scale/header rules,
   exact budgets and retired clip ->completed native pose lifetime. Host175
   passes without guest objects/shaders; no indexed live content observed.
   Indexed pure application supports duplicate names, but the model importer
   still withholds ambiguous name tables, so that runtime admission remains open.
   **Native controller connection (host177/run983, opt-in):** native slot-clock
   plans, TRS working layers and completed channel handoff replace admitted
   controller scratch/memcpy/sampler dispatch. Preserve single clock subtraction,
   adjacent-slot mixing, double-advanced overlays and signed UV phase. Collision/
   effects run once; strict verification executes the complete original once.
   Final material61/CPU59,412 guards andhost178 pass; no guest objects/shaders.
   Host178 adds a source multi-layer overlap guard, not a smaller native joint
   limit; last live evidence is177, not a restamp of the178 binary.
   Run983:30460 matching controller transactions (includes empty plans),3996
   samples,8 mixes,3839 interior-CLIP times,4012 advancing slot clocks,3985 native
   skeleton handoffs. Six changed/unreadable boundary refusals and5028 model/
   controller admission refusals require classification. Frame1137 context is
   FieldActive,event1, not independent interactive/reload qualification.
   The one-shot generation guard still checks outgoing48B data for late writes;
   no claim that those reads, source slot selection or all exports are gone.
   Native working values reach the existing skeleton/instance/render owners;
   temporary flag/name sidecars stay confined to the outgoing adapter.
   Run983 also positively re-verifies first-match import:122->121tracks,
   prepare-refused0. Its current asset layout charges93432B, not174's93424B.
   Superseded174/982 text retired; source/hash findings and979/980 failures remain.
   Next migrate remaining channel writers/source selection, remove validation/
   exports when their last consumer retires, and select actual authored subtree/
   indexed scenarios for motion/interior-key/pixel gates. No unchanged boot or
   reduced threshold. Indexed loader/content qualification,
   inherited dense compression/nonzero Euler modes, late writers/special bones,
   persistent cooking, outgoing48-byte channels/palettes and full desktop/both-eye
   gates remain. Preserve975's reload and971/962's pixel failures; defaults
   unchanged, no Quest work.
   [Controller transaction, handoff, source contracts and live evidence](../research/20260908_1951_native-animation-controller.md).
   [Indexed connection, source boundaries and exact verification](../research/20260908_1916_native-indexed-animation.md).
   [Named/importer source contracts and checkpoint](../research/20260906_0333_native-scene-state-bridge.md#2026-09-08-named-animation-inputs-and-importer-contracts-after52a9510).
   [Weighted/layer contracts and scoped verification](../research/20260908_1824_native-animation-layers.md).
   [Selected residency, evidence and storage](../research/20260908_1745_selected-motion-residency.md).
   [Original runtime connection](../research/20260908_1653_loaded-animation-sampling.md).
   [Recovered clip/loader contracts and angular regressions](../research/20260908_1612_native-animation-clips.md).
3. **Specialized producers and complete host frame.** Dynamic vertices,
   effects/particles, UI, secondary shadows, reflections, remaining frame/pass
   scheduling and presentation. Remove guest rendering, register/resource
   adapters, retained templates, EDRAM/tile inference, seed copies and emulated
   resolves as their final consumers disappear.
4. **Full desktop gate, then Quest 2.** Qualify fields, battles, cutscenes, menus,
   transitions, reloads, animated effects and both eyes. Multiview, frustum/
   occlusion culling, instancing, indirect draws and the remaining required modern
   GPU techniques stay in scope. Only then qualify/optimize Quest, including
   foveation, toward the unchanged 72 Hz target.

These describe dependency order, not a requirement to finish every scene's
visual matrix before writing the next producer sharing the same owner.
Acceptance claims remain scoped to what was actually verified; the full
desktop requirements above are unchanged.

### Working cadence

- Start with a short bundle contract: outcome, interface removed, reused files
  and a falsifiable exit test. Reuse completed source findings when unchanged.
- Group related source/fixture/consumer edits before an integration build.
  Use `python -B tools/host_checks.py` and existing C++/GPU fixtures for the
  inner loop; use a bounded game/pixel run for a coherent connected change.
  Broad reload/scene/sequence/both-eye qualification belongs at integration
  milestones, not after each helper. It is not optional at completion.
- Every diagnostic must answer a named implementation question. If it does not,
  change the hypothesis/method instead of repeating it until a pass. Preserve
  relevant failures, and distinguish investigation from ownership progress.
- Commit/push coherent verified connections frequently. A prerequisite-only
  checkpoint names its pending consumer; source/tests do not restamp the last
  live-tested binary. Keep one active queue and one cumulative storage ledger,
  not a dated report or duplicate worklog for every small edit.
- Read the dependency map below only when its specific source contract is needed.
  It and the linked dated reports preserve detailed provenance; they are not a
  mandatory re-audit on every turn. No new renderer framework or bulk recook.

### Direct rigid-object dependency map

Source audit at `11f5d94`, updated for owned object/light/fog inputs (2026-09-07).
This records why the latest component
checks are not an end-to-end object conversion, and where the next implementation
must connect. It is not a second roadmap or a new renderer framework.

| Required contract | Reuse | Concrete remaining dependency |
| --- | --- | --- |
| An object/primitive packet selected by owned handles | `NativeModelRenderData`, `NativeInstancePose::model`, `FindNativeObjectPrimitive`/`BuildNativeObjectPrimitive`, owned geometry/materials/bounds and object color/image/UV/policy publications | The selected direct scene draw now consumes the owned packet, retaining geometry/images through the fence. Packet assembly selects owned programs without a `NodeTag`/source key. The producer still resolves object bindings and visibility at an explicit source boundary. Only `PrepareReplayMaterialMesh` keeps the bounded replay alias index; remove it when replay's last consumer migrates. Source-to-object publication still needs replacement. |
| Explicit vertex, material and pass inputs | Canonical attributes, pass-local `RenderCameraState`, native image leases, `BuildRigidObject`/`BuildRigidPass`, explicit GPU layout and owned selected lights/fog | Direct scene/caster use fresh cameras, copy-free completed shadow images, late receiver colour and owned scene/node light selection. Production-style array views and D32/S8 sampling pass the GPU fixture and are bound in the live route. Host106/run944 adds accepted receiver/owned-light reload text and mono sanity pixels;941's legacy comparison failure stays open. Authored light updates, inherited node inputs, projection/colour, fog and source camera/object producers and compatibility publication remain. Mono cameras are duplicated only for mono acceptance; layered scene targets refuse until explicit per-eye publication exists. |
| Native shader/pipeline binding | Existing Plume device/framebuffers/queue; `GraphicsBindings`; bounded `NativePipelineProgram`; GPU-tested `CreateNativeRigidPrograms` with scene and position-only shadow inputs | Both programs now use native structured instance storage and indexed indirect commands through the shared cache/queue. CPU batch preflight and two-instance/two-eye GPU pixels pass; live field uses singleton batches. Complete repeated-object runtime/lifecycle coverage. Other families still use engine bindings and translated instance gathering. |
| Direct scene and shadow submission | Existing traversal, culling, instancing/pulling, indirect submission and native pass commands | Both opt-in routes bypass `bdSceneNodeDrawSingle` before interpreter/replay/capture. Host107/run945 replaces the caster asset-ID ceiling with whole-node opaque rigid admission, transactional sibling preparation and native submissions across the wider family. The selected scene/caster reload window stays isolated from those counts. Native batches still emit singletons. Source object/pass publication, unsupported families, repeated-instance groups, sequences and both-eye acceptance remain. |

Selected investigation target from field run920: geometry `258694267A8DBAEE`,
material `63B8D67932573E51`, model-local node64, sole primitive, technique0/view3
in `bg41_01`. Runtime instance144/generation93 identify that observation only,
not persistent asset IDs. The material has modulation off, black specular/power0;
diffuse comes from the live object color. Its skin command is unspecified and
its canonical vertex schema now has all four required native inputs. Its17,572 B
v2 file is persisted and independently inspected;162 vertices/158 triangles.
Upload/load retains `rigid_vertex_input`, independent of translated locations.
Run927 identifies its exact live pair: VS`B5C88BB6295138CC`, PS`FB83DD3F5E67CEB7`.
The matching VS dump is `bd_mirror_vs_norm.hlsl` (deduplicated with
`bd_normal_vs_norm`), not the related CS variant. Its UV formula is
`(uv+1)/512+offset`: UV0 values16383..16895 map to32..33 before the live offset,
so do not subtract32 from the asset. The packet owns one texture layer, enabled
vertex color and zero declaration bones. Run922 observes its owned packet: material mask3,
image mask0001, known UV offsets `(0,0,0,0)`, diffuse `(1,1,1,1)`, known direct
non-deferred/non-alpha participation. These are that object's live values, not
defaults to freeze into its asset or proof of exact shader eligibility. Run924
also observes its owned light kinds `(directional, disabled, disabled)` after
the per-node callback. Run923's object-wide-only snapshot missed these updates;
the consumer gate failed instead of accepting startup publisher checks.
Reuse this selected asset, not a library-wide recook. Its packet now retains
lighting pass values, composed ordinary material features and 2D samplers as well.
Run936 connects fresh receiver/per-node lights and explicit native scene bindings
after whole-node preflight. Other families keep drawing; these IDs identify an
acceptance target, not a complete lifecycle or permission to drop siblings.

[Direct caster evidence](../research/20260907_0846_direct-rigid-shadow.md):
output17/CPU2, GPU24/rigid03,275 Python checks and host93 pass. Run935's first
native caster submission is frame787; later ready-field windows add300 native
submissions and300 fence retirements. It uses the real selected geometry/pose
and native pass camera, not a captured template or translated shader layout.
The old phase0-only colour/texture packet was not reinterpreted as phase1:
caster admission consumes only the shadow contract, with checked per-object
policy publication still a source adapter. Receiver/scene draws, native batching,
source-free cold-load/reload and full-game/both-eye acceptance remain open. The
normal profile keeps this acceptance switch off. No performance claim.

[Shadow image evidence](../research/20260907_0757_native-shadow-images.md):
output15/CPU1,271 Python checks and host92 pass. Run934 adds300 native image
handoffs with exact owner/image/descriptor/layout checks, no resolve link and no
field fallback. The existing complete field gate passes; inspected character,
tree and fence shadows remain coherent with known cliff marks/blur. Depth-only
commands reuse the scene image/framebuffer owners and skip compatibility clear,
seed and write-layout selection. CPU tests cover mono/layered recipes, empty and
repeated clears, invalid inputs, no colour snapshot and fence-retained readers.
This removes the primary shadow allocator/framebuffer/resolve-link dependency,
not caster rendering, engine camera fitting, receiver callbacks or getter headers.
No direct native rigid draw, reload, both-eye or speedup claim.

[Light-selection evidence](../research/20260907_0727_native-light-selection.md):
material29/CPU27,267 Python checks and host91 pass. Run933 adds14,332 exact
selection comparisons,36 full reselections and9,936 candidates in consecutive
post-event windows2017/2317, with no field fallback. Existing light publication,
draw-consumption, sampler/fog/shadow and movement gates pass. CPU tests cover
128,000 ranked candidates, incremental/dirty updates, live-vs-snapshot category,
bounds/aliases and source destruction before native pass consumption. The
inspected image retains known cliff marks/blur; no raw/cache growth. This removes
the supported selection/scoring body, not its old callback timing, authored light
snapshot producer or direct native scene/shadow submission. Cold-load/reload and
both-eye acceptance remain open; no speedup claim.

[Sampler evidence](../research/20260907_0655_owned-material-samplers.md): material28/CPU26,
sampler1/CPU1, binding21/CPU19, 263 Python checks and host90 pass. Run932 adds
3,628 matching sampler comparisons and 41,602 owned-input draws in fresh
post-event windows2032/2332. Ordinary 2D addressing folds at load; filtering comes
from a fresh frame/view publication with late setters. Unsupported slots refuse
ownership. The first run931 correctly failed coverage because a plain-2D-only
gate excluded the uploader's 2D-array views; both types now have boundary coverage.
All prior field/movement/light/fog/shadow gates pass. The inspected image retains
known cliff marks/blur; no new raw/cache outputs. Template/interpreter use, direct
native shader submission, cold-load/reload, both-eye and speedup remain unproven.

[Material feature evidence](../research/20260907_0625_owned-material-features.md):
material27/CPU25,261 Python checks and host88 pass. Run930 adds1,326 matching
feature comparisons and41,061 owned-input draws in post-event windows2009/2309;
all previous field/light/fog/shadow and observed-movement gates pass. Actual pixels
retain known cliff marks/blur. The five ordinary switches use named native
recipes and fresh object/pass gates, not captured boolean history. No new raw
or cache files; replacement verification retires the prior equivalent small set.
Template/interpreter use, direct native shaders, cold-load/reload and both-eye
qualification remain open; no speedup claim.

[Lighting pass evidence](../research/20260907_0602_owned-lighting-pass.md):
lighting1/CPU1, material26/CPU24, 259 Python checks and host87 pass. Run929 adds
1,242 matching pass-input comparisons and38,435 owned-pass draws in consecutive
post-event windows, with existing field/movement gates passing. The packet owns
ambient/camera/color/shadow sampling values; ordinary replay no longer requires
interpreted PS c0/c1 freshness or c2 history. Vertex pass constants and the
template/interpreter branch remain. First attempt928 correctly failed the
coverage gate: the lighting texture-slot ID was not the scene render-view ID.
The corrected producer uses the shared scene identity; no new raw/cache outputs,
and the inspected field image retains known cliff marks/blur. No direct native
shader submission, cold-load/reload, both-eye or speedup claim.

[Primitive shader evidence](../research/20260907_0535_owned-primitive-shader-inputs.md):
material25/CPU23,257 Python checks and host85 pass. Run927 adds1,332 matching
ordinary-pair comparisons and42,166 owned-input draws in fresh field windows,
with existing light/fog/movement gates passing. Actual pixels remain coherent
with known cliff marks/blur. This removes captured texture-enable/vertex-color
values as the source for these supported draws, not template/interpreter use.
The later material-feature checkpoint above owns diffuse/specular/normal-map/
reflection/fog switches; shadow receiving has its separate owned policy. Remaining
shadow pass values are not permission to freeze observed flags. The native UV packer can now use the exact
family formula. No full direct object, reload, both-eye or speedup claim.

[Fog evidence](../research/20260907_0510_owned-fog.md): material24/CPU22,
254 source/scenario checks and host84 pass. Run926 adds2,400 matching fog
publications,33,600 owned snapshots and1,335 matching normal-lit draw checks
(2,670 active layers) in fresh post-event windows, with zero fallback/mismatch.
The selected asset owns both active ranges `0/1600` and `100/-200`; signed fog
endpoints are now accepted by the native pass packer. Light comparisons also
pass (+14,328 publications/+1,335 draw checks). The inspected1920x1080 image
retains known cliff marks/distant blur. This is not direct native shader
submission, source-free GPU loading, reload/sequence/both-eye qualification or
a measured speedup. Authored fog updates and the staging/flush adapter remain.

Work backward from the final submit call, defining the packet/binding contract
first and connecting its missing producers next. Each implementation checkpoint
must name which dependency above is removed and which compatibility consumer can
now be deleted. Further broad adapter expansion, additional math-only rewrites
and bulk recooking are not substitutes for this path.

The acceptance harness now disables the selected family's interpreter, template
capture and replay before its first draw in hard-off mode. Cold-start field
admission/output passes in938;940 adds actual teardown/reload with fresh model,
instance and game-task identities. A failure stays visible rather than warming
a fallback. The opt-in reload extension now validates both field epochs and
old source/GPU retirement; ordinary walking alone still does not prove it.
Complete the remaining receiver/pass producers and explicit per-eye inputs at
this consumer, then expand families and complete the unchanged desktop gate.

### Reusable inner loop

`python -B tools/host_checks.py` runs the selected rigid-path Python guards and
scenario-parser tests. Repeat `--area model|geometry|material|instance|scenario`
to narrow the selection, use `--list` to inspect it, and `--all-boundaries` for
the broader native guard set. No build, game launch, profile edit, output log or
bytecode cache. Empty/missing tests and failures cannot report a passing check.

Verification of the dev-loop change: eight runner tests pass, including failed
imports, empty groups, failure exit status, fail-fast/keep-going and list-only
behavior; 107 rigid-path checks pass in 0.019 s and the broader 222 checks pass in
0.050 s (test execution, not total process startup or overall development speed).
Focused model/instance selection also passes from outside the repository cwd.
Both revised repository skills passed their frontmatter validator. That tooling
checkpoint made no renderer change or new runtime output. The subsequent binding
checkpoint expanded the selection to 113 rigid-path /228 broader checks; native
program guards brought these to 116 /231; rigid shader guards now bring them to
119 /234 respectively; model-node guards/scenarios now bring these to123 /238. Their
separate C++ and runtime evidence follows below.

## Earlier checkpoint evidence by subsystem

Owned object primitive inputs (2026-09-07): object-entry color/shininess snapshots
now feed ordinary material composition; source reads remain only for verification
or unsupported scopes. Owned pose/node/primitive selection shares the same prepared
image/UV/policy data as replay without using its source-key index. Material20/CPU18,
host78 and245 Python checks pass. Run922 has79,716 fresh matching color reads,
33,600 new publications, four bounded owned packet observations and all existing
post-event field/movement gates. Its137,491 B1920x1080 sanity image was inspected:
Shu/terrain/vegetation/shadows coherent, known cliff marks/distant blur remain.
No raw/perf/cache output; profile restored. This is packet/lifetime and ordinary
color-consumer progress, not a native rigid game draw, sequence/both-eye gate,
interpreter-free cold-load/reload or performance result. Evidence:
`research/20260907_0406_owned-object-primitive-inputs.md`.

Selected native rigid asset (2026-09-07): geometry upload/load now resolves and
retains the production native shader vertex input. An opt-in exact content ID
allows one<=2 MiB asset during otherwise persistence-disabled verification, using
the same aggregate disk cache and reserve. Mesh11/CPU10, host77 and240 Python
checks pass. Run921 repeats the complete existing field text gate, writes exactly
one17,572 B file and no raw/image/perf/dump. Independent read validates identity,
actual attributes and input lifetime after CPU data destruction. The schema has
Position/Normal/UV0/Color, but UV units and live material flags still need exact
family resolution. No game draw reroute, source-free GPU load or new game pixel
qualification; its inspected image was still run919 tied to host74. Evidence:
`research/20260907_0340_selected-native-rigid-asset.md`.

Native model-node associations (2026-09-07): load traversal preserves every
node-to-program association, including shared meshes; duplicate matrix indices
cannot select an arbitrary primitive. Instance poses pin the model generation
and its bounds/primitive programs through retirement and source reuse. The host
walk uses owned bounds, with source reads only for comparison or unavailable
native data. Material19/CPU17, host74–76, 238 Python checks and eight runner tests
pass. Run919 adds1,761,600 fresh matching bounds reads and an inspected1920x1080
image; run920 repeats all field gates and identifies four actual content-keyed
targets without another capture. Source-free node/bounds consumption is not
source-free geometry loading or direct rendering. No game shader route, native
light/fog producer, interpreter/template-free cold-load/reload or complete
desktop/both-eye gate is established. No measured speedup. Evidence:
`research/20260907_0312_native-model-node-associations.md`.

Native rigid shaders (2026-09-07): production scene VS/PS and shadow VS use named
geometry, explicit object/pass buffers and texture/sampler sets, with no translated
shader common/register ABI. Material18/CPU16 covers packing/refusals and the
existing independent lighting/fog reference. Vulkan rigid02 compares all 512
color pixels across four two-eye cases plus scene/caster depth; maximum color
error 0.0000404567, zero validation errors/warnings, 1.21 s. Snapshot19 also passes
all eight layer/MSAA combinations. Fixture23, host73, 234 source/scenario checks
and eight runner tests pass. Reuse existing trees, no game run/raw/image export.
This removes the missing native shader/input implementation dependency, not the
live object/light/fog producer or source selection/template dependencies. No
compatibility consumer can be deleted yet. Full cache-selection GPU integration,
actual model identity, cold-load/reload and scene/shadow/both-eye game acceptance
remain next. Run918 stays tied to host72, not the new binary. Evidence:
`research/20260907_0245_native-rigid-shaders.md`.

Native pipeline programs (2026-09-07): the shared cache accepts an immutable
native shader/layout/input/specialization owner without calling the translated
shader linker or selecting the main engine layout. Mixed inputs are refused;
native background jobs and cache entries retain their program leases. Limits:
eight specializations, 256 pending compiles and 2,048 retained native variants.
Binding19/CPU17, intent20/CPU18, host72, 231 source/scenario checks and eight runner
tests pass. Run918 is an **existing engine-path regression**, not native-program
GPU qualification: no game producer creates a native program yet. Fresh field
geometry/material/pose/shadow/movement/binding gates pass; one 1920x1080 image
inspected, known cliff artifacts/blur remain. No raw/perf/cache/dump output,
exact profile restored, superseded sanity/fixture/build evidence retired.
Native rigid shader/input and object producers remain, along with interpreter/
template-free cold-load/reload, scene/shadow and both-eye acceptance.
Evidence: `research/20260907_0217_native-pipeline-programs.md`.

Explicit queued bindings (2026-09-07): the existing emitter now consumes bounded
layout/set/offset snapshots without global constant-set lookup or a fixed
three-offset bind. Grouping includes binding identity and exact comparisons;
flush restores the caller's entry bindings. Fixture18/CPU16, host70/71, 228 Python
guards/scenario checks and eight runner tests pass. Run917 used host70; host71
only caps an unsupported-ABI error message, with no successful draw-path change.
Fresh post-event field windows add 196,347 draw commands, 128,465 descriptor binds
and 900 layout binds; existing material/geometry/pose/shadow/movement gates pass.
Layout binds include flush starts, not proof of live alternative layouts.
One 1920x1080 image inspected; known cliff artifacts/blur remain. No new raw/perf/
cache/dumps, exact profile restored, superseded sanity outputs retired. Native
alternative layouts have CPU coverage only; live engine bindings, translated
instance records, shader wrappers, source selection and templates remain.
This removes a direct-object binding dependency, not the native object/shader
producer or cold-load/reload/sequence/both-eye acceptance gap.
Evidence: `research/20260907_0157_explicit-draw-bindings.md`.

Named lit shading (2026-09-07): the live normal material uses shared named
light/fog arithmetic instead of310 lines of repeated register-machine logic.
All material branches remain. Material17/CPU15, host69,170 source guards and52
scenario tests pass. Independent references cover1,200 light cases, coloured
shadow composition and ordered radial/planar blend/add/subtract fog. Host68's
HLSL struct-ternary compile error was corrected; only the normal host shader
rebuilt, no guest objects/translated shader cache. Run916 adds56,835 normal-lit
queued draws in fresh post-event field windows; all prior geometry/material/
pose/table/shadow/policy/movement gates pass. One1920x1080 image inspected,
known cliff artifacts/blur remain; no numerical GPU parity/sequence/reload/
both-eye claim. No new raw/perf/cache/dumps, exact profile restored, superseded
component and resolved compile-retry outputs retired. Shader ABI/front-end,
source selection/templates and the direct native object gap remain.
Evidence: `research/20260907_0108_named-lit-shading.md`.

Native primitive participation (2026-09-07): material15/CPU13, host67,167 source
guards and49 scenario cases pass. Flat normal-MSAA/precache-on run915 adds45,313
known plans,74,785 direct/14,419 deferred candidates,15,493 matching checks and
59,292 native-cull replays in fresh post-event field windows. Zero mismatches;
scope peak111,300 B under4 MiB. Prior geometry/pulling/material/image/UV/pose/
table/shadow/movement gates also pass. One1920x1080 image inspected; known cliff
artifacts and distant blur remain. No new raw/perf/cache/dumps; exact profile
restored and equivalent predecessor evidence retired. Live cull changes and
compound refreshes were zero: invalidation has pure-condition tests and atomic
erase source guards, not runtime change qualification. Policy publication is
currently phase0 only; volume effects, original pass/material callbacks, source
index, shader ABI and templates remain. This is not direct native object drawing,
reload/both-eye qualification or a measured speedup.
Evidence: `research/20260907_0047_native-primitive-participation.md`.

Object material textures (2026-09-07): material14/CPU12, host66,162 source guards
and44 scenario cases pass. Flat normal-MSAA/precache-on run914 adds33,600 native
object publications (300 containing override records),76,625 material reads,
58,641 matching checks,59,927 UV-composed draws and62,690 native image slots in
fresh post-event field windows. Zero mismatches/refusals; peak object-scope
storage110,640 B under4 MiB. Canonical/geometry/material/pose/table/shadow/movement
gates also pass. One1920x1080 image inspected; prior background artifacts/blur
remain. No new raw/perf/cache/dump output; profile restored exactly, equivalent
prior component evidence retired. Source object/pass setup, unsupported/special
override families, pass/alpha/deferred routing, shader ABI, source index and
templates remain. This is not a direct static-object path, complete animated
override/reload/both-eye qualification or a measured speedup.
Evidence: `research/20260907_0016_object-material-textures.md`.

Load-owned shadow policy (2026-09-06): material13/CPU11, host65, 157 source guards
and39 scenario cases pass. Flat normal-MSAA/precache-on run913 owns2,973 policies
(209 disabled, zero unknown), with15,019 fresh matching receiver checks and
43,037 composed replays after the opening event. Canonical geometry, native
pulling, material/pose checks, normal tables and movement also pass. One inspected
full-resolution JPEG replaces the preceding canonical component sanity image;
this is not sequence/both-eye or whole-game qualification. Runtime reloads and
dynamic control families remain unqualified;71,945 unsupported/unmatched adapter
lookups remain. No new raw/perf/cache/dump output, original profile restored,
superseded component outputs retired after validation. The native policy consumer
no longer reads the model control table; the original interpreter, source index,
templates, live pass/visibility adapter and direct submission gap remain.
Evidence: `research/20260906_2344_load-owned-shadow-policy.md`.

Canonical rigid geometry (2026-09-06): the owner-approved 3 GiB exception resumed
verification under the original cumulative ledger; other storage caps and zero
new raw allowance remain. Mesh build09/CPU08, host64, 156 guards and 34 scenario
tests pass, including the synthetic-pulling default correction. No guest object
or shader rebuild. Flat normal-MSAA/precache-on run912 /PID27288 passes fresh
post-event movement, geometry/material/pose, normal texture-table and pulling
gates. It observes 2,206 canonical meshes /104,787 fresh canonical draws and
117,515 fresh native pulled records. Instancing and indirect calls are active.
One inspected 1920x1080 JPEG adds component sanity coverage, not a new sequence.
No raw/perf/cache/dump output; the exact owner profile is restored.

The schema has named values and content/layout identities independent of the
source declaration, and converted inputs clear shader unpack masks. Missing
native pull attributes use format-aware entries and an owned zero STORAGE
buffer. Source-free disk-to-GPU loads remain **zero**, so the implemented native
content-ID entry point is not qualified by this game's importing producer.
The field retains 980 noncanonical owners among 3,186 meshes; those are not a
whole-game conversion percentage. The GPU arena reserves 64 MiB; compact storage,
bandwidth/performance, reloads, longer sequences and both eyes are unqualified.
Shader-register ABI, source lookup, replay templates and direct scene/shadow
object submission remain the next ownership boundary. Prior cliff-edge/blur
evidence stays protected. Evidence: `research/20260906_2316_canonical-rigid-desktop.md`;
implementation history: `research/20260906_2158_canonical-rigid-mesh.md` and
`research/20260906_2211_native-pull-defaults.md`.

Desktop verification loop (2026-09-06): runtime-independent autoplay now waits
for stable field/controller/player readiness, stops on interruptions, and
reports observed horizontal displacement separately from stick input. Mindows'
selected panel survives while hidden; the corrected policy uses its visibility
flag. CPU fixture, host63, 152 guards and 28 scenario cases pass. Flat run911
observes one uninterrupted walk with fresh geometry/pose/material/table/pulling
checks, and three inspected full-size images. This adds short field movement
coverage, not native rendering ownership or complete sequence/reload/both-eye
qualification. Dark cliff-edge artifacts and distant blur remain unqualified.
Evidence/failures: `research/20260906_2120_readiness-driven-autoplay.md`.

Native vertex inputs (2026-09-06): immutable, bounded, content-shared owners
carry IA elements (including owned semantic names), pulling entries and the
temporary old-shader decoding contract. Geometry pins them; native dispatch
clears the guest declaration, uses owned strides, and gives all three consumers
the explicit input. The legacy PSO CSV excludes runtime-native rows rather than
serializing resource pointers. Host 60, mesh CPU 04, draw-intent test, 152 source
guards and 23 scenario cases pass. PSO-off run 907 adds 170,037 native pipeline/
decode uses, no pulling; normal precache-on run 908 adds 170,020 native pulled
records with instancing and indirect dispatch active. Both retain fresh matching
material/geometry/pose and normal texture-table checks. Two inputs /3,824 B;
normal 1920x1080 pixels inspected. No new raw/perf/cache files. Packed formats,
shader-register ABI, template/source lookup, full native object/pass contracts
and direct scene/shadow submission remain. Movement/reloads/sequences/both eyes
and full desktop qualification remain open. Evidence:
`research/20260906_2040_native-vertex-inputs.md`.

Texture tables (2026-09-06): immutable image leases and native runtime IDs now
publish after actual load completion; replacement/eviction preserves immutable
old generations. Limits are 16,384 tables /16 MiB, including pinned owners/index
accounting. The field's thousands of inline lists are distinct from GPU image
count; source tracing corrected the initial 2,048-table cap. The atomic snapshot
removes global-epoch false refusals and closes the collection/publication race.
Host 59, binding CPU 05, material CPU 10, 146 source guards and 18 scenario cases
pass. Comparison run 905 adds 22,326 matching lookups /22,006 image checks.
Normal-table run 906 adds 21,745 lookups with zero original comparison/fallback,
zero refusals and an inspected field image; 5,736 tables /2,172,192 B. Fresh
pose/geometry/material checks also match. Other material verification remains
enabled to establish the field gate; this is not a whole-renderer diagnostics-off
or performance run. Source selection/return ABI, dynamic overrides, resource
consumers and direct static-object submission remain. Reflection's new direct
native-image consumer has zero observations. Movement/reload/sequences/both-eye
and full-game qualification remain open. Evidence:
`research/20260906_1955_native-texture-tables.md`.

Instance render poses (2026-09-06): native IDs/generations, bounded immutable pose
storage, completed render-handoff publication and traversal/replay consumers.
The final boundary includes late writers after InitBones and the actual derived
copy callback's dirty-state gate. Host 53, Release fixture/CPU 09, 137 source
guards and 11 scenario cases pass. Flat PSO-off run 900: 239 live instances
/368,896 B, 118,851 fresh matching pose reads, zero pose misses/refusals/drift;
fresh geometry/material checks also match. One full-size sanity image inspected.
Original pose calculation/copy, secondary palettes, skin gathering, source index,
templates and direct scene/shadow submission remain. This does not qualify
movement/reload sequences, effects, both eyes or the complete desktop frame.
Evidence and failed-attempt corrections:
`research/20260906_1850_native-instance-render-poses.md`.

Load-owned geometry (2026-09-06): the model-load hook resolves primitive buffer/
declaration associations and produces GPU geometry before publication. Four
material/skin/reflection/shadow-policy consumers stop reading guest buffer tables;
converted base-geometry replays use load-owned handles. All 130 source guards,
Debug/Release material fixtures and host build 47 pass. PSO-off flat run 893 loads
2,973 primitives with no load/geometry budget failures. Two post-event field
samples add 51,173 native-handle draws, 2,650 matching geometry checks and matching
15,253 diffuse /14,541 specular checks. A bounded full-size JPEG sanity image was
inspected; it does not qualify sequences, movement, authored effects or both eyes.
761 unavailable replay lookups still take the unconverted import path. LOD and
other specialized paths, original load adapter, source index, templates, shader
ABI, native instances and direct scene/shadow submission remain. No new cooked
files or raw frames; diagnostics suppress mesh writes. Default persistence keeps
the existing bounded policy. Evidence: `research/20260906_1743_load-owned-model-geometry.md`.

Native mesh storage (2026-09-06): the active importer now uses a bounded disk
cache with independent byte/file/reserve limits, non-waiting writer ownership,
valid-payload reuse/conflict protection and checked repairs. Persistence refusal
does not discard the already-uploaded native geometry. Host build 43, expanded
mesh CPU fixture, all 124 source guards and read-only loading of 3,510 existing
files /36,510,144 B pass. Format, keys and cache payloads are unchanged. No new
game/VR/pixel run at that storage checkpoint; geometry follows above, while
native instance/direct submission still remains.
Evidence: `research/20260906_1701_native-mesh-storage.md`.

Field observations (2026-09-06): loading visibility now observes fade/task/strip
state, and all 128 asset slots are scanned. Generated fade-state writers restore
0 when NPCs resume; the former state-4 readiness assertion was wrong. The host
build, loader CPU fixture, 120 source guards and 15 in-memory scenario-gate cases
pass. Flat PSO-off run 890 observes the opening event end, then two idle field
samples with 15,124 new diffuse /14,514 specular matches and 58,572 new model
lookup hits. No missing/load/unsupported/input/budget failures. No new pixels,
movement, reflection, both-eye or full-desktop qualification. Exact evidence:
`research/20260906_1638_field-state-observations.md`.

Load-owned model materials (2026-09-06): host/material fixtures and 120 source
guards pass; PSO-off diagnostics exercise 114 publications and one retirement,
488,116 lookup hits with no missing/load/input/budget failures, and matching
126,735 diffuse /120,369 specular comparisons. An opening-cinematic image was
inspected, not an interactive field. Its initial field-scenario check failed
because readers reported Loading / state 0; the reader issue is corrected by
the checkpoint above, without retroactively qualifying that run. Reflection has zero
checks. Old normal flat/XR and failure evidence remains. Geometry, instances,
source/resource/recipe adapters and retained templates still prevent claiming
the complete native static-object path. Details and the corrected water-marker
inference: `research/20260906_1610_load-owned-model-materials.md`.

Prior qualified component evidence (scene labels subject to that correction):

Toon material callbacks (2026-09-06): complete update/begin/end replacements
own texture animation and edge-parameter production. Pass dispatch directly
calls native participants. Flat/XR samples record 13,280/3,020 native participant
calls and zero guest participants at that dispatcher, with no fallback/refusal/
faults. Fresh field-camera XR samples match 1,510 original Toon publications and
2,627,009 native parameter blocks, zero mismatch/full legacy blocks. Host build
37, CPU suite 30 (31/31, 6.94 s) and 115 source guards pass; one flat field image
was inspected. Counters/list/edge data, two inherited edge words, texture resource
and shader ABI imports remain. This is not full Toon asset/frame ownership or
both-eye/full-game qualification. No new raw captures or Quest work. Evidence:
`research/20260906_1505_native-toon-materials.md`.

## Historical checkpoints

Entries below describe their dated checkpoint, not the current publication
state or a separate active queue. Later evidence supersedes only what it
explicitly requalifies; unresolved failure evidence remains required.

Material passes (2026-09-06): five complete guest bodies now have host
replacements for begin/end, recipe selection, shader binding and cached vertex
declarations. Native scheduling calls the host binders directly; normal decoding
specialization is centralized in the shared declaration binder. Host build 36,
CPU suite 29 (31/31, 7.10 s) and 111 source guards pass. Flat/XR samples record
616,830/159,375 native starts, zero fallback/refusal/faults; one flat field image
was inspected. The strengthened XR check requires a field-camera marker before
fresh matching parameters: 2,083,519 blocks agree, zero full legacy imports.
Registry/cache/recipe/resource/shader-ABI adapters and participant callbacks
(15,056 flat /2,400 XR sampled) remain. Both-eye pixels, authored effects and
full-game qualification remain open; this is not a fully host-owned frame.
No new raw frames. Twelve obsolete diagnostics removed, 4,886,528 B measured
reclaimed; net retained log/perf/PNG growth 64,717 B for stronger current evidence.
Evidence: `research/20260906_1429_native-material-passes.md`. The owner's
standing normal commit/push approval remains recorded in `AGENTS.md`.

Deferred visuals (2026-09-06): the complete `sub_824252D0` scheduler and primitive
preparation now have a native replacement, with a native scene-sized snapshot
instead of an emulated resolve or 1280x720-only gate. It preserves the 512-entry
bound, live callback-sensitive dispatch, mode-dependent translation and final
queue clear. Host build, 31 CPU tests and 107 source guards pass. The normal
flat regression samples 5,363 empty calls, no fallback/faults, and an inspected
field image; **nonempty authored deferred effects were not observed**. Native
copy/resolve commands retain existing strict eight-mode GPU fixture evidence.
Authored queue/vertex production, shader/state/texture/getter/material callbacks,
unconverted scopes and full desktop/both-eye qualification remain. No Quest/XR
run this checkpoint. Ten obsolete diagnostics removed, 4,259,840 B measured
reclaimed; retained diagnostic payload shrank 12,644 B. Evidence:
`research/20260906_1358_native-deferred-visuals.md`.

Publication follow-up (2026-09-06): the owner granted standing normal commit/
push approval for `noeldvictor/reblue_android_optional_vr:main` and
`noeldvictor/plume:main`. Plume is published through `3094b35`, now recorded
by the parent dependency reference. The scene/post image integration is now
reviewed as one coherent source checkpoint: native source allocations, MSAA and
single-sample framebuffers, snapshot leases, post images and getter publication
share fence-retained ownership and layout state. Review confirmed the dependency
factory rejects null-backed Vulkan framebuffers before they enter native caches.
All 103 source guards pass again; existing host build 34, CPU suite 27, flat/XR
logs 880/881 and strict tiny GPU fixtures supply the unchanged integration's
verification. No new build, runtime or capture was needed. Broad desktop/both-eye
and authored snapshot/deferred-effect qualification remain open. Earlier dated
approval-blocked notes are historical, not the current publication constraint.
Review details: `research/20260906_0333_native-scene-state-bridge.md` (13:42 entry).

Sorted visual scheduling (2026-09-06): the complete `Visual__DrawSortedQueues`
now owns model/primitive order and dispatch on the host, using one bounded
32 KiB key array without guest bucket heads/next pointers. Model preparation,
live callback inputs, shader changes, deferred limits and final native colour
publication are preserved; the enclosing full legacy parameter scope is gone.
Host build, 31 CPU tests and 103 source guards pass. Normal flat/XR samples
record 2,121/425 schedules and 78,861/36,784 primitives without fallback/faults;
952,257 XR parameter blocks match, with zero full legacy blocks in normal
flat/XR. One flat sanity image was inspected. Queued models/deferred effects
have CPU/source coverage only, not authored runtime qualification. Authored
queue/vertex/visual storage and producers, bone/material/pass callbacks, state/
resource/getter adapters and the deferred emulated resolve remain. Full native
frame/game/both-eye gates remain open, before Quest. Eighteen superseded
diagnostics removed, 4,694,016 B measured reclaimed. Evidence and carefully
scoped speed counters: `research/20260906_1323_native-visual-schedule.md`.

Immediate UI submission (2026-09-06): the complete `Visual__DrawVerticesUP`
body now prepares colour/optional translation on the host, publishes native
parameters and owns a bounded CPU vertex copy uploaded directly without guest
Begin/End heap scratch. Host build, 31 CPU tests and 99 source guards pass;
16,664 original preparations/uploads match, normal flat/XR record 84,502/35,350
native submissions without fallback/refusal/upload failure. One standing-field
sanity image was inspected; translated/empty inputs have CPU coverage only.
The original sorted scheduler (including model and deferred-primitive handling)
still has a full legacy parameter scope. Authored vertex production, shader/
texture/state adapters, complete UI/frame ownership and broad desktop/both-eye
qualification remain open. Fourteen obsolete diagnostics removed, 4,685,824 B
measured reclaimed. No raw captures or Quest work. Evidence:
`research/20260906_1252_native-immediate-ui.md`.

Native parameter storage (2026-09-06): float setters/flush, native transforms and
deferred producers publish directly into bounded host CPU owners, now consumed
by ordinary uploads and replay base blocks. Independent flat/XR comparisons
match 34,591/501,224 sampled native blocks with zero mismatch after exposing
missing visual c53, foliage c57 and Toon/fur c50/c51 notifications. Host build,
31 CPU tests and 94 source guards pass; normal flat records 855,492 native blocks
alongside 11,144,896 imported words and 157,468 legacy UI blocks. One flat sanity
image was inspected, not sequence/both-eye/full-game qualification. Shader ABI,
source descriptors, inline/UI imports, guest mirrors/getters and complete native
material/frame ownership remain. Twenty-one superseded diagnostics removed,
4,460,544 B measured reclaimed; no raw captures or Quest work. Evidence:
`research/20260906_1215_native-parameter-storage.md`.

Native material disk ownership (2026-09-06): runtime/cooker persistence now has
independent aggregate byte/file limits, a free-space reserve and a non-waiting
writer lease. Full storage retains usable native resident materials without
evicting files or resetting the budget on restart. The expanded CPU fixture,
including eight two-writer trials, and host link pass. All 30 existing material
assets load source-free; no game run, capture or asset rewrite was needed.
This closes a storage prerequisite, not water parameter or frame ownership.
Evidence: `research/20260906_0757_native-material-storage.md`.

Water animation/parameter update (2026-09-06): complete water-update and shared
sampling-mode callbacks now have native replacements, preserving tick cadence,
23 parameter words, mode counters and sequential alias behavior in a bounded
32-entry plan. Host build, 31 CPU tests and 88 source guards pass; 301 complete
authored publications match the original, followed by 2,701 normal native updates
with zero fallback/refusal. A flat sanity image was inspected. Authored data,
parameter storage and counters/tick source remain imports. Native material assets,
standalone mode dispatch and refraction/snapshot/both-eye/full-game qualification
remain open. Ten obsolete diagnostics were removed, reclaiming 4,239,360 B
measured; no raw captures or Quest runs. Evidence:
`research/20260906_0726_native-water-update.md`.

Water/refraction setup (2026-09-06): two whole callback replacements now execute
host preparation order, checked parameter/image imports and the existing water
highlight clamp, preserving depth-write and other blend policy. Host build,
31 CPU tests and 83 source guards pass. A bounded flat run reports 1,964 native
water preparations with zero material fallback/refusal/faults; one sanity image
was inspected. Refraction and snapshot execution were not observed in that field,
so their authored runtime qualification remains open. Native assets/storage,
child state/parameter/getter adapters and complete scene/
frame ownership remain. Ten superseded diagnostics were removed, reclaiming
4,272,128 B measured; no raw captures or Quest runs. Evidence:
`research/20260906_0700_native-refraction-materials.md`.

Native scene snapshots (2026-09-06): the address-free copy core preserves HDR
and independent mono/layered images across resumed scene writes. A tiny real-GPU
fixture exposed and fixed a Plume pending-depth-clear/color-resolve ordering bug;
all eight mono/stereo x 1/2/4/8-sample cases and the broader resolve suite pass
with zero Vulkan validation errors/warnings. Host build, 31 CPU tests and 77
source guards pass. The pending bridge now adopts the native scene extent instead
of requiring a fixed 1280x720 getter. Authored snapshot-copy execution remains
unproven: the earlier flat check did not report this path, and no repeated game
run or raw capture was made. Integration/getters/reflection
ownership and full desktop qualification remain open. Twenty-four superseded
small outputs were removed, reclaiming 4,177,920 B measured. Local Plume commit
3094b35; dependency/root publication still needs approval. Evidence:
`research/20260906_0629_native-scene-snapshots.md`.

Effect preparation and cleanup (2026-09-06): both preparation groups, paired
resource/participant cleanup and global array teardown now execute in native
code through six whole-function replacements. Host build, 31 CPU tests and 72
source guards pass. Bounded flat/XR field checks record 381,870/723,492 model
prepare/finish pairs and 681,379/1,151,643 visual pairs, zero lifecycle fallback,
refusal or faults. One flat sanity PNG was inspected; timed stops did not exercise
global teardown, which has CPU coverage only. Callback implementations, native
identities/storage, scene ownership and full desktop game/both-eye qualification
remain open. Thirteen superseded diagnostics were removed, reclaiming 6,283,264 B
measured; 60.10 GiB remains free, no new raw captures. Evidence:
`research/20260906_0536_native-effect-lifecycle.md`.

Effect activation and registry mutation (2026-09-06): host replacements cover
all effect selectors and the three-group registration/removal algorithms,
including signed-priority ordering, duplicate/first-removal semantics, live
callback inputs, indexed-view tail cleanup and checked native array mutations.
Authored flags, callback metadata and shared array storage remain imports;
preparation/cleanup now have the follow-up above. This is not complete registry
or frame ownership. Host link,
31 CPU tests and 69 source guards pass. Bounded flat/XR checks each record
28 registrations, 9 removals, 40 insertions and 18 erasures, with zero activation/
registry compatibility, refusals or faults. Existing parent/scene/post checks
also pass. One flat sanity PNG was inspected, not sequence/stereo/full-game
qualification. Thirteen superseded small diagnostics were removed, reclaiming
6,418,432 B measured; 60.11 GiB remains free and no raw captures were added.
Callback implementations, registry/scene ownership and full desktop game/both-eye
gates remain required before Quest. Evidence:
`research/20260906_0516_native-effect-activation.md`.

Whole-view scheduling (2026-09-06): the complete parent `bdRenderViewSubmit`
now selects and schedules its passes in host code, with native reflection
candidate geometry and focus publication. Previously inlined starts enter the
shared host lifecycle dispatcher. All special-view branches remain represented;
temporary authored inputs, descriptors, registry and remaining callbacks are
explicit imports, not a completed native frame. Host link, 31 CPU tests and 65
source guards pass. Bounded flat/XR checks record 3,601/9,601 native views with
zero parent/dispatcher fallback, refusal or fault. Disabling native post exercises
isolated legacy containers/cleanup without replaying the parent. One bounded flat
sanity PNG was inspected, not sequence/stereo/full-game qualification. A bounded
non-MSAA check also reached 3,601 native views without scheduler faults or fallback.
Nineteen superseded small outputs were removed, reclaiming 8,036,352 B measured;
closing free space is 60.14 GiB, with no new raw captures. Native
scene production, callback/registry ownership, animation/materials/UI and the
full desktop game/both-eye gate remain required before Quest 2. Evidence:
`research/20260906_0442_native-view-schedule.md`.

Shared pass lifecycle dispatch (2026-09-06): complete host replacements now
execute pass-start/pass-finish scheduling, including ordered participant calls,
live registry-slot updates and the final pass close. Existing parent-inlined
starts also finish through the native dispatcher. The host link, expanded CPU
suite (31/31) and 60 existing scene/post guards pass. Bounded flat/XR checks record
3,601/9,301 native starts and 9,940/22,428 finishes, with zero dispatcher
fallbacks, refusals or faults; existing scene/post ownership checks also pass.
One flat sanity PNG was inspected, not sequence/stereo/full-game qualification.
Thirteen superseded diagnostics were removed, reclaiming 6,402,048 B measured;
free space was 60.16 GiB with 3.54 MiB net growth for that follow-up. Parent branch
decisions/inlined starts are now converted above; descriptor/participant
registry/callback ownership remains work. This is not a fully native frame. The
complete parent source trace also corrects the misleading AllPasses name: that function inserts
objects into lists, not schedules views. No Quest work. Evidence:
`research/20260906_0412_native-pass-dispatch.md`.

Native scene precision boundary (2026-09-06, local renderer integration): the
remaining state-308 callback is verified as Xbox high-precision blending, not
MSAA. Its two guest calls around scene clear are removed; native FP16 attachment
and pipeline formats remain fixed, without console surface/packet format edits.
Only final engine cache/request getters are published through a preflighted
adapter. Host build, 31 CPU tests and 60 source guards pass. Normal flat/XR/
non-MSAA checks record 3,600/9,600/3,600 native clears, zero scene state-308 calls,
compatibility clears/depth publications or post imports/refusals. One bounded flat
PNG was inspected and replaced its predecessor, not new sequence/stereo/full-game
qualification. Eighteen obsolete/empty diagnostics were removed, reclaiming
7,196,672 B measured. Independent adapter/tests and callback diagnostics form the
local checkpoint; renderer/Plume publication still needs approval. Other state
producers, getters/scaling, complete scene/UI/frame ownership and desktop game
gates remain. No Quest work. Evidence:
`research/20260906_0333_native-scene-state-bridge.md`.

Native scene command ownership (2026-09-06, local renderer integration): native
scene scopes now own source/resolve write layouts, first-use discards, framebuffer
binds and typed colour/depth/stencil clears. Scene begin no longer requests a
console-style clear or stores it on binding headers; the native bind skips
alias/seed/tile-chain selection. Resumed scopes preserve contents; empty scenes
still clear before publication. Host build, 31 CPU tests and 59 source guards
pass. Five bounded MSAA/non-MSAA flat/XR and post-off recovery checks report zero
compatibility clears/depth publications; normal post has zero imports/refusals.
Two flat sanity PNGs were inspected, not new sequence/stereo/full-game evidence.
Twenty-five superseded diagnostics, including those PNG predecessors, were
removed: 13,762,560 B measured reclaimed. Independent command contracts/tests are
the local checkpoint; integration/Plume publication still requires approval.
State 308, complete native draw-state execution, remaining getters/scaling,
scene/UI/frame ownership and the full desktop game gates remain. No Quest work.
Evidence: `research/20260906_0255_native-scene-commands.md`.

Native single-sample scene framebuffers (2026-09-06, local renderer integration):
scene begin now constructs mono/stereo framebuffers from retained native images,
outside the resource-header cache. Bounded residency keys exact source owners and
density-map identity; fence retirement destroys framebuffers before their source
images. The existing native MSAA path remains intact. Host build, 31 CPU tests and
57 source guards pass. Non-MSAA flat/XR record 3,600/10,500 native depth handoffs,
zero compatibility depth publications and no post imports/refusals; non-MSAA
recovery publishes all 3,600 pending colours, and default-MSAA regression passes.
One non-MSAA flat PNG was inspected and replaces its predecessor, not sequence/
stereo/full-game qualification. Twenty-three superseded diagnostics were removed,
reclaiming 8,167,424 B measured. Independent contract/tests are locally checkpointed;
renderer/Plume publishing still requires approval. Native pass command/clear and
draw-state execution, remaining getters/scaling, full scene/UI/frame ownership and
desktop game gates remain open. No Quest work.
Evidence: `research/20260906_0236_native-scene-framebuffers.md`.

Native scene source allocation (2026-09-06, local renderer integration): scene
colour/depth images now originate in the native store for both single-sample and
MSAA, using explicit native formats/extent/layers/sample counts. Scene begin no
longer passes Xbox formats to SurfacePool or adopts already-created images.
Only the temporary binding/GetDesc header remains; it does not own allocations.
Native resolve framebuffers retain source owners through their own destruction.
Host build, 31 CPU tests and 55 source guards pass. Normal flat MSAA/non-MSAA each
record 3,600 native depth handoffs; normal XR MSAA/non-MSAA record 9,600/10,200;
all have zero compatibility depth publications and native-post imports/refusals.
MSAA recovery publishes all 3,600 deferred colours. Two bounded flat PNGs were
inspected and replace their same-purpose predecessors; not sequence/stereo/full-
game qualification. Thirty superseded diagnostics were removed, reclaiming
14,684,160 B measured; protected raw/failure evidence remains. Independent recipe/
ownership tests form the local checkpoint; renderer/Plume publication still needs
approval. Native framebuffer/pass construction, remaining getters/scaling, frame/
UI scheduling, broader scene ownership and full desktop gates remain. No Quest work.
Evidence: `research/20260906_0200_native-scene-source-allocation.md`.

Native single-sample scene ownership (2026-09-06, local renderer integration):
existing non-MSAA colour/depth images, views and descriptors now have a bounded
native owner with fence retirement; surface headers are binding adapters. Native
post receives the source images directly without initial colour publication or
getter imports. Matching depth getters borrow native backing; retired adapters
cannot enter the old surface pool and overwrite an image retained by a reader.
Host build, 31 CPU tests and 53 source guards pass. Normal non-MSAA flat/XR record
3,600/10,200 native depth handoffs and zero compatibility depth publications;
normal native post has zero imports/original scopes/refusals. Non-MSAA post-off
recovery publishes all 3,600 deferred colours; default-MSAA regression also passes.
One non-MSAA flat PNG was inspected, not stereo/sequence/full-game qualification.
Thirteen superseded small diagnostics were removed (884,736 B measured reclaimed).
Initial source allocation still uses the temporary surface allocator; native
allocation/pass construction, scaled/getter cases, UI/frame scheduling and the
full desktop gate remain open. Independent ownership contracts/tests form the local
checkpoint; renderer/Plume publication still awaits approval. No Quest work.
Evidence: `research/20260906_0138_native-single-sample-ownership.md`.

Native depth-image lease (2026-09-06, local renderer integration): matching
MSAA depth getters now borrow the native resolved image/view/descriptor instead
of copying it or creating a resolve link. Native and remaining adapter accesses
share one live layout record; final post outputs use the same retained-image
boundary. Host build, 31 CPU tests and 52 source guards pass. Normal flat/XR
record 3,600/9,600 native depth handoffs and zero compatibility depth publications;
normal native post has zero imports/original scopes/refusals. Post-disabled
recovery publishes all 3,600 pending colours while still borrowing native depth.
Non-MSAA post also passes but its 3,600 depth handoffs remain compatibility work;
scaled/other-format cases and complete scene/UI/frame ownership remain open.
One normal-flat PNG was inspected, not sequence/stereo/full-game qualification.
Twenty superseded diagnostics, including two replaced normal-flat PNGs, were
removed (11,141,120 B measured reclaimed), preserving raw/failure evidence.
Independent lease/layout contracts and tests are the local checkpoint; renderer
integration and Plume gitlink publication still await approval. No Quest work.
Evidence: `research/20260906_0110_native-depth-image-lease.md`.

Native post-image ownership (2026-09-06, local renderer integration): post
outputs now have their own bounded FP16 image/view/framebuffer pool, not
`PostColor` resource-header allocations. Native write leases exclude live
readers; the final UI/getter borrows the completed image and descriptor without
a copy or resolve link. Old backing/framebuffers retire behind a fence. The
host build, 31 CPU tests and 50 source guards pass. Normal flat, optical XR and
non-MSAA flat diagnostics record 3,601/7,501/3,601 native scopes and zero imports,
original scopes or refusals; residency settles at two post images. One existing
normal-flat PNG was inspected without obvious full-frame corruption, not a new
sequence/stereo qualification. The independent pool/test is the local checkpoint;
GPU creation/publication wiring and the required Plume gitlink remain uncommitted
pending dependency publication approval. Initial depth publication, UI scheduling,
remaining guest frame/game gates and Quest qualification remain open. Evidence:
`research/20260906_0014_native-post-image-ownership.md`.

Native post output/optical contract (2026-09-06): the rendering core now accepts
native HDR attachments and sampled optical images; it no longer reads output,
flare, heat or grain resource headers. Native inter-root reads carry the completed
image directly. The independent output contract/test is locally checkpointed;
renderer wiring remains uncommitted alongside the unpublished scene integration.
The host build, 31 CPU tests and 48 source guards pass. Capture-disabled XR with
synthetic flare/heat/animated grain and normal non-MSAA flat runs record
8,401/3,601 native post scopes with zero imports/original scopes/refusals.
Both profiles were restored. This removes core header dependencies, not the
temporary output allocator, final UI/depth publications or remaining frame/game
gates; no new pixel qualification or Quest work. Thirteen superseded small
diagnostics were removed (888,832 B measured reclaimed). Evidence:
`research/20260905_2351_native-post-resource-contract.md`.

Native Vulkan multiview state ordering (2026-09-05, local Plume `81bdca8`):
lazy native pass begin now reestablishes current native bindings, including
descriptor offsets and incremental push values, with layout-disturbance and
static/dynamic-state lifetime handling. The previously hidden first stereo draw
failure was reproduced before GPU submission. The expanded 8x8 GPU readback suite
passes mono/two-eye, indirect, pass-restart and compute/graphics transitions with
zero API errors/warnings. Desktop host build, 30 CPU tests and 46 source guards
pass. Capture-disabled XR/non-MSAA flat diagnostics record 8,701/3,601 native post
scopes, zero imports/original scopes/refusals, with profiles restored. No new
game pixels, full-game qualification or Quest work; publishing remains blocked
pending approval. Evidence: `research/20260905_2305_native-multiview-state.md`.

Native scene resolve integration (2026-09-05, **local uncommitted renderer work**):
scene MSAA now writes separately owned native colour/depth resolve images with
generation-safe keys, bounded residency and fence-gated descriptor/image lifetime.
Normal native post skips the initial colour copy; empty/refused/disabled post and
unconsumed normal views explicitly recover the required getter publication.
Initial depth and final UI/getter publications remain. The desktop host build,
30 CPU tests and 46 source guards pass. Normal/recovery diagnostics verify
3,600 deferred colours with zero/all recovered respectively; one bounded window
PNG was inspected, not a new sequence/VR qualification. The multiview ordering
fix above is GPU-tested; full image/game gates and remaining ownership conversion
are still open.
Publishing the required Plume dependency still needs owner approval; neither the
integration code nor its parent gitlink is committed. No raw captures or Quest
work. Evidence: `research/20260905_2206_native-scene-resolve-ownership.md`.

Native sampled-image inputs (2026-09-05): scene completion, native atlas,
composite and directional-bloom scene/depth reads now carry native texture and
descriptor identities with the owner's live layout record, without GuestTexture
headers in the input contract. Boundary adapters prepare sampling views; native
preflight rejects unresolved MSAA, invalid dimensions/descriptors/exposure,
eye-count mismatches and physical-image feedback before GPU work. The desktop
host build, 30 native texture/post CTests and 43 post/scene source guards pass.
Two capture-disabled flat diagnostics (default MSAA and no MSAA) each record
3,601 native post scopes, zero scene-image imports/original scopes/refusals;
the latter exercises direct source images. No new pixel or VR qualification.
At that checkpoint, initial scene publication copies, output/optical-image
adapters, native resolve producer wiring and full-frame/game gates remained.
No captures, downloads or Quest work. Evidence:
`research/20260905_2129_native-scene-attachment-images.md`.

Native attachment-resolve prerequisite (2026-09-05, before scene integration):
local Plume commit `a8b3c15` adds layered colour/depth MSAA resolve attachments,
mode/capability preflight and clear/discard handling. Test commit `465c2ad` also
covers actual FP16 HDR/D32_FLOAT_S8_UINT scene formats, depth MIN/SAMPLE_ZERO
with stencil NONE, resumed LOAD and held clears. Its 8x8 real-GPU test
passes mono/two-eye sample averaging, depth MIN versus SAMPLE_ZERO, LOAD,
DISCARD, pending/held zero-draw clears and eighteen-attachment readbacks with
core/synchronization validation (zero API errors/warnings). The existing 30 CPU
tests also passed without rebuilding them. That prerequisite did not remove
scene publication copies: native producer wiring, exposure/alpha/extent
semantics and full-frame pixel verification remained. The dependency push was
blocked by auto-review pending explicit owner approval; the parent gitlink is
not committed. No main-game build, capture or Quest run. Exact evidence and
small-tool storage accounting: `research/20260905_2047_native-attachment-resolves.md`.

Scoped native scene-result checkpoint (2026-09-05): scene end supplies exact
sampled colour/depth/exposure through a per-view, frame-bounded, single-use
result. Native target pins and temporary output references preserve its lifetime;
normal post no longer imports scene image getters or traverses resolve links.
All 30 CTests and 45 source guards pass. Capture-disabled diagnostics exercise
both MSAA materialization and direct source images. Normal flat/VR record
3,001/8,401 completed native inputs and zero imports, original scopes or refusals.
Both 120-frame sequences have 0/119 large changes and no cyan hits; first/last
VR depth is correctly crossed and all full eye/flat endpoints were inspected.
Initial scene MSAA/scale copies and output adapters, final UI publication, engine
producers and parent scheduling remain. Multi-root/HDR/nested-view GPU cases,
existing VR blur and full-game gates remain unqualified; no Quest. Six superseded
normal raw sets and their automatic copies/links were removed, preserving 16
PNGs/reports and all protected evidence. The new pair is the baseline; net volume
usage fell 7.38 GiB, ending with 60.97 GiB free. The historical archive remains
over budget; no new raw allowance remains. Exact source, runtime and retention:
`research/20260905_1958_native-scene-image-result.md`.

Explicit native post-image checkpoint (2026-09-05): native atlas/composite and
optical/noise consumers now use actual sampled colour/depth images and exposure.
The sequence imports the scene boundary once, passes completed images directly
between roots, applies incoming exposure once and publishes only the final result
for remaining UI/getter consumers. No intermediate resolve publication or new
cross-frame resource cache. All 30 CTests and 42 source guards pass. Normal flat/
VR record 3,001/7,801 native sequences/imports/final publications, zero original
scopes or refusals. Both 120-frame sets have 0/119 large changes and no cyan hits;
first/last VR depth is correctly crossed, all full endpoints inspected. Only one
root is GPU exercised; multi-root/HDR image flow remains unqualified on GPU.
Initial scene resolve/getter import, final UI publication, engine producers and
parent frame scheduling remain. Existing VR blur, authored events and full-game
gates remain open; no Quest. Four superseded normal raw sets removed, preserving
16 PNGs/reports and all distinct startup/preview/failure evidence; the new pair
is the baseline. Net volume usage fell 2.94 GiB, ending with 53.59 GiB free.
Exact evidence, consumed budget and retention review:
`research/20260905_1929_native-post-image-flow.md`.

Native scene-post handoff checkpoint (2026-09-05): the main scene caller now
passes its explicit colour/depth images directly to native effect scheduling,
skipping both guest temporary-container constructors, wrapper invocation and
complete destructors. Camera/focus updates and saved effect flags remain intact.
All 30 CTests and 39 source guards pass; only the affected guest partition
regenerated/rebuilt. Normal flat/VR record 3,001/7,801 direct handoffs and zero
original container/wrapper/post scopes or refusals. Both 120-frame sequences
have 0/119 large changes and no cyan hits; first/last eye depth is correctly
crossed and all full endpoint images inspected. Scene-output getters, resolve
links/exposure, engine producers, parent scheduling, UI and full-game gates
remain; distant VR blur is not fixed. No Quest. Removed superseded readiness
normal raws, kept all eight PNGs and protected startup/failure evidence; equal
replacement raw bytes leave the archive unchanged. Net volume usage increased
18.11 MiB, ending with 50.64 GiB free. Exact evidence and retention:
`research/20260905_1912_native-scene-post-handoff.md`.

Native effect-sequence checkpoint (2026-09-05): the supported complete list
wrapper now schedules post roots on the host with explicit depth, without
the guest global-depth copy, virtual dispatch or container cleanup. Full
callback/plan preflight, bounded alternating outputs and ordered focus
publication preserve the sequence contract. All 30 CTests and 36 source guards
pass. Normal flat/VR record 2,701/8,101 native sequences, zero original wrappers
or post scopes/refusals; only one root per sequence is GPU exercised. Both
120-frame captures have 0/119 large changes and no cyan; first/last VR depth
is correctly crossed and both full eyes were inspected. Multi-root/unknown
callback GPU coverage, resolve links/exposure, engine producers, UI and complete
frame/game gates remain. Its scene temporary containers are removed by the
newer handoff checkpoint above. No Quest.
Two superseded normal raw sets removed with eight PNGs/reports retained;
replacement raw bytes exactly match removal, so the raw archive did not grow.
Net volume usage increased 49.91 MiB, ending with 50.66 GiB free. Exact evidence:
`research/20260905_1842_native-effect-sequence.md`.

Native post-input readiness checkpoint (2026-09-05): the three recurring
startup/transition refusals were fresh depth images lacking sampling descriptors.
Native whole-post and direct DoF preparation now create/refresh explicit input
views under the host mutex, without a guest texture-binding warm-up frame.
Normal flat/VR runs have 3001/8101 sampled native post scopes, zero original
scopes and zero input refusals. All 29 CTests and 34 source guards pass;
120-frame normal flat/VR sequences have 0/119 large changes, no cyan and
correctly crossed first/last VR depth. Both eyes inspected. Early-startup
probes show the existing blank title background and fade, not qualified artwork
or stereo depth. Image/property/UI adapters, authored effect-event coverage,
late-scene failures and complete frame/game gates remain. No Quest.
Three superseded raw sets removed with reports/images retained; net volume
usage decreased 1.20 GiB after verification, with 50.71 GiB free. The archive
is still over budget and frozen against growth. Exact evidence and retention:
`research/20260905_1807_native-post-input-readiness.md`.

Native directional-bloom checkpoint (2026-09-05): mode 1 now imports authored
sigma/gain/count into native parameters and produces independent horizontal
and vertical masks in at most two private quarter-pair atlases. One layered
render pass per iteration and a 32-byte kernel replace guest mask caches,
blur submission and emulated resolves. Bright preparation precedes heat;
composition keeps both masks unwarped. All 29 CTests and 33 source guards
pass. Strong flat/VR previews have 0/31 large changes and no cyan, but washed
out near geometry makes preview VR depth inconclusive. Normal flat/VR have
0/119, no cyan and crossed first/last depth; both eyes inspected. Authored
mode-1 events/kernel comparisons, combined heat/bloom, other bloom-kernel
approximations, three input refusals, image/property/UI adapters and complete
frame/game gates remain. No Quest. Three superseded normal raw sets removed
to fund bounded verification, reclaiming 3.89 GiB while retaining small and
protected evidence. Full evidence, retention and net storage accounting:
`research/20260905_1726_native-directional-bloom.md`.

Native heat-shimmer checkpoint (2026-09-05): flag-16 filtering now selects
depth-aware scene/DoF coordinates inside the native composite, with four
noise samples and unwarped bloom. Existing cooked images, shared-eye host
animation and an explicit 224-byte composite layout replace the producer,
mutable guest phase array, submission and another intermediate/resolve.
All 28 CTests and 30 source guards pass. Strong synthetic flat/VR previews
show coherent animated distortion but have 31/31 large changes, not normal
stability passes. Normal flat/VR have 0/119, no cyan and correctly crossed
first/last VR depth; both eyes inspected. Authored heat events/comparisons,
VR comfort, dual-mask bloom, image/property/UI adapters and full-frame/game
gates remain. Three input refusals per run remain. Runs stopped and profile
restored; no Quest. Three superseded normal raw sets removed, retaining
reports/images, recovered 3.89 GiB; after new verification the net saving
is 87.27 MiB with 49.49 GiB free. The historical archive remains over budget
under a frozen inventory and explicit review triggers. Exact evidence:
`research/20260905_1644_native-heat-shimmer.md`.

Native colour-grading checkpoint (2026-09-05): discolor, animated grain and
gamma/saturation/gain/bias/target correction now use native parameters,
explicit layered images and an 80-byte shader layout. The supported path
removes the packed producer/submission, texture-list binding, gameplay RNG,
engine intermediate and emulated resolve. Three post stages share at most
two scratch images. Existing cooked grain assets are reused, not copied.
All 27 CTests and 28 source guards pass; 860 original grading/activation and
scanline-strength comparisons match. Corrected grain/grade flat and combined
VR previews have 0/31 large changes; normal flat/VR have 0/119 and no cyan,
with correctly crossed first/last VR depth. Both eyes inspected. Startup
native/control first/last images match exactly, including their existing
white background; title artwork is not qualified. Normal VR now has 9,598
native scopes and three input refusals instead of thousands of packed-effect
fallbacks. A longer interrupted preview reached 77 input refusals; its late
window was not captured or qualified. Intervening/dual-mask effects, image/
property/UI adapters, blur, full-frame and full-game gates remain. No Quest.
Ten superseded normal raw sets were removed, retaining reports/images and
protecting baseline/previews/failures; 11.68 GiB actually recovered. The
historical archive still exceeds the 10 GiB target and needs further review;
the former "active" totals were not total storage. Profile restored, runs
stopped. See `research/20260905_1603_native-post-grading.md` for exact binary,
coverage, deliberate animation changes and cleanup/retention accounting.

Native scanline checkpoint (2026-09-05): the supported post scope now owns its
final four-tap filter and animation on the host. Native image dimensions,
frame-index noise shared by both eyes, private scratch and explicit output
replace the guest producer/wrapper, state-308 calls, texture setters and
emulated resolve. The existing noise-off default remains. All 26 CTests and
26 source guards pass. The noise-disabled flat preview has 0/31 large changes;
the combined animated optical/scanline VR preview has 1/31 (6.33%), inspected
as changing horizontal bands, not counted as a normal stability pass. Normal
flat and 1440x1584-per-eye VR have 0/119 large changes, no cyan patches and
correctly crossed first/last VR depth. Both eyes inspected, profile restored,
all runs stopped; 42.34 GiB free with 8.60 GiB active unique raw evidence.
Authored activation/parameter comparisons and event coverage remain unqualified.
Packed/intervening filters, dual masks, image/property/UI adapters, existing
blur, late-scene failures and full-frame/full-game gates remain. No Quest run.
See `research/20260905_1527_native-scanline.md` for source, GPU coverage and
the deliberate separation of effect animation from gameplay RNG.

Native optical-adjustment checkpoint (2026-09-05): fisheye and colour inversion
now execute in one native layered pass, with explicit parameters and native
attachment records. Composite/flare render directly into private native input
scratch when needed; these filters no longer call their guest producers,
texture/depth setters, intermediate target allocation or emulated resolve.
The shared optical curve uses output aspect ratio instead of a fixed console
canvas. All 26 CTests and 24 source guards pass. Negative flat and positive
both-eye previews show coherent distortion/inversion with 0/31 large jumps.
Normal flat and 1440x1584-per-eye VR sequences have 0/119 large jumps and no
cyan patches; first/last normal VR depth is correctly crossed. Both eyes were
inspected, original profile restored, no app jobs remain, and no Quest run
occurred. Previews are synthetic: authored activation/parameter comparison and
VR comfort are not qualified. NTSC scanline/noise, intervening/packed filters,
dual-mask mode, image/property/output adapters and the full-frame/game gate
remain. Earlier late-scene failures are not superseded. See
`research/20260905_1504_native-post-adjustments.md` for exact evidence and
storage accounting (46.15 GiB free; no assets duplicated or outputs deleted).

Native lens-flare checkpoint (2026-09-05): the complete supported flare
producer and sprite submission now execute as native recipes and a single
instanced draw into the owned post output. No lens wrapper, per-sprite
constant flush, texture setter, UP vertices, target allocation or emulated
resolve executes on that path. Shared C++/HLSL quadrant folding preserves the
authored optical assets without copying them. The initial visible preview
exposed a wrong globally-linear UV assumption; corrected first/last images
in both eyes show smooth glows/rings instead of quarter-image rectangles.
All 26 CTests and 22 source guards pass; a diagnostic recorded 3615 matching
original sprite parameter checks and 5142 matching bloom checks. The supported
normal post scope now has zero old tail-effect/state-308 calls, superseding
the one-effect/three-state-call boundary in the prior checkpoint below.
Final normal flat and 1440x1584-per-eye VR 120-frame sequences have 0/119
large jumps and no cyan patches; first/last VR depth is correctly crossed.
Both eyes were inspected; flat Shu/windmill shadows remain. Original profile
restored, all agent-started app runs stopped, and no Quest run occurred.
Packed effects, other trailing adjustments, dual-mask mode, non-flare input
refusals, authored light/visibility producers, image/getter adapters and per-eye
optics remain. Full-frame, late-scene and full-game qualification are not done;
the synthetic preview does not qualify authored visibility or VR comfort.
See `research/20260905_1422_native-lens-flare.md` for the exact conversion,
failed preview, correction and bounded normal flat/VR checks.

Native post scheduling checkpoint (2026-09-05): supported DoF/bloom dispatch
now runs directly from authored native parameters into an explicit persistent
post output. It bypasses the bloom texture caches, blur loops, ms_tex input
array, shader-hash composite trigger and that scope's EDRAM allocation/resolve.
All 26 CTests and nineteen source guards pass; 3642 flat diagnostic parameter
checks match. Normal flat and 1440x1584 final-eye 120-frame sequences have
0/119 large changes and no cyan patches, with correctly crossed first/last
stereo depth. Flat Shu and windmill shadows remain. At that checkpoint one
trailing effect and three state-308 calls per tested field frame still executed
(replaced by the lens-flare checkpoint above); startup packed
effect combinations and two image/preflight refusals per normal run retain
the counted original scope. Other filter combinations, mode-1 dual masks,
image/getter/UI adapters and complete frame ownership remain. VR blur,
character-shadow visibility, late-scene and full-game qualification remain
unfinished. Original profile restored; no Quest run. See
`research/20260905_1344_native-post-scheduling.md`.

Native DoF producer checkpoint (2026-09-05): complete preparation and matching
quad-submission bodies are replaced by native parameters, explicit scene/depth
images and the host atlas. The supported path no longer executes the five-level
allocation/blur loops, DoF texture-binding loop, intermediate target or resolve,
and does not read PS c27. All 26 CTests and sixteen source guards pass; a flat
diagnostic has 6901 matching original parameter publications. Normal flat and
1440x1584 final-eye runs have 0/31 large changes in their 32-frame captures;
both first/last eye checks show correctly crossed depth. Flat Shu/windmill
shadows remain. Each normal run still records three startup/transition DoF
fallbacks, whose exact failed preflight condition remains to be traced.
Engine properties, authored camera/focus sources, image/getter adapters,
bloom execution/register inputs and the outer post scheduler remain. VR blur,
character-shadow and full-game coverage remain unqualified; field focus logs
above 1.0 are a new investigation lead, not a complete blur diagnosis. Disk
space limited the new sequences to 32 frames; they do not replace longer
qualification. Native sun remains opt-in, the original profile is restored,
and no Quest run occurred. See
`research/20260905_1256_native-dof-producer.md`.

Tracked camera ownership checkpoint (2026-09-05): a native per-view scope
selects the submitted scene camera and reuses one composed result across
its consumers. Arbitrary matrix setters no longer treat light cameras and
2D/post resets as headset cameras. This also stops unrelated view writes
from advancing anchor smoothing. All 25 CTests and thirteen source guards
pass. Normal 1440x1584 final-eye VR has 0/119 large changes and correctly
crossed first/last depth (far -1, near -9, spread 8 pixels); both eyes were
inspected. The original flat control is also 0/119, with Shu's cast silhouette.
These results supersede the earlier short-view foreground/depth findings,
not the late-scene failure or full-game gate. Distant blur and VR character
shadow qualification remain. Native sun stays opt-in; XR scale stays 0.65.
The original view scheduler, descriptor/camera sources, interpolation,
reflection derivation and post-focus producer remain conversion boundaries.
See `research/20260905_1216_scoped-native-xr-camera.md`.

Native eye geometry checkpoint (2026-09-05): scene and final layered output
now share the full native runtime extent instead of fitting the whole 3D
frame to the authored 16:9 HUD canvas. UI scaling remains separate; native
projection presentation uses the complete viewport without console alignment
rounding. All 24 CTests and thirteen source guards pass. The scale-1.0 desktop
OpenXR run verifies 1440x1584 scene/final eyes with image content across all
1584 rows in both inspected eyes, superseding earlier 1440x808/letterbox
findings for this path. The foreground passage still flags 10/119 large
changes; blur and inconclusive depth remain. Normal flat output has 0/119
large jumps and Shu's cast silhouette. UI/cinema/movie GPU coverage, readable
near/far framing, scene/output getter adapters, other frame producers and
broader desktop qualification remain. Native sun camera stays opt-in and
the default XR scale stays 0.65. See
`research/20260905_1026_native-full-eye-geometry.md`.

Native sun camera experiment (2026-09-05): current-view orthographic fitting,
scene snapshot and an explicit sun-scope culling volume are implemented, but
`bd_native_sun_camera` is **off by default**. GPU controls and exact character
submission source isolated a second, obsolete light-eye-distance cutoff after
the sphere cull. A scoped instruction adapter now skips that cutoff only for
the owned native sun pass; complete character submission is not converted.
The normal comparison-off short flat sequence restores Shu's cast silhouette,
with 0/119 large jumps, no cyan patches, 28173 matching attachment checks and
zero original snapshot/light-fit/cull-comparison calls. The diagnostic sequence
flagged two transitions around one changed foliage/shadow frame; the normal
run does not establish their cause or longer-term absence. All 23 CTests and
thirteen source guards pass. Normal 1440x1584 final-eye output has 10/119 large
changes during an inspected foreground-object passage, with 103019 matching
shadow ownership checks and no camera fallback. That passage is not yet
attributed to the camera change. That run's eyes were blurred/letterboxed with
1440x808 scene content; the native eye geometry checkpoint above supersedes
its sizing/letterbox finding, but depth and character-shadow visibility remain
unqualified in that framing. The default still executes the counted engine
snapshot/light fitter; the hooked binary's default-off short flat control has
0/119 large jumps, Shu's cast silhouette present and zero cutoff bypasses.
Final-eye and broader camera qualification remain;
no Quest result is claimed. See
`research/20260905_0956_native-sun-character-visibility.md`, which supersedes
the missing-caster conclusion in the earlier experimental-camera note, and
`research/20260905_1004_native-sun-final-eye-check.md` for the subsequent run.

Sun-shadow lifecycle checkpoint (2026-09-05): the complete begin/end bodies
now own an explicit persistent depth attachment and retained output association.
Native entry/exit and explicit-source publication replace engine allocation,
pass wrappers and resolve-source guessing. Empty caster passes publish their
owned far-depth clear; shadow output no longer publishes a post/UI tile chain.
All 22 CTests and nine source guards pass. The normal short desktop sequence
has 0/119 large jumps or cyan patches, 38674 matching ownership checks and no
lifecycle fallback. The same binary's normal 1440x1584 final-eye sequence also
has no large jumps/cyan patches, with 102251 matching ownership checks and no
fallback. VR remains blurred/letterboxed and depth-inconclusive; scene content
is still 1440x808. Later scenery/text and other shadow modes are not qualified.
Engine scene-camera snapshot and light-fitting execution remain
counted, and secondary shadows, caster scheduling and sampling/resource/getter
adapters remain. This is not a complete shadow system or fully native frame.
See `research/20260905_0756_native-shadow-pass-lifecycle.md`.

View-cache checkpoint (2026-09-05): complete camera/frustum-cache execution
and native cached-shape ownership now use host inverse/unprojection,
roll-free orientation and native transform values. Normal production does
not import engine planes or clip points. All 22 CTests and seven source
guards pass; the corrected desktop comparison records 15341 matching full
publications, zero fallback/matrix imports/cache bootstraps and a stable
120-frame sequence with no large jumps or cyan patches. Float cofactor and
trigonometric ordering corrections are documented, not hidden by a wider
tolerance. Normal comparison-off flat and full-size final-eye sequences also
have no large jumps or cyan patches, with 18054/41174 native view updates,
no fallback/imports and no missing native culling volume. Final eyes remain
blurred/letterboxed and depth-inconclusive; the scene stays 1440x808 during
the final-eye sequence, despite 1440x1584 output layers.
Engine camera sources, invalidation/settings, getter publications and broader
scene/frame ownership remain. Cache-hit/alternate-selection GPU coverage and
later scenery/text are not qualified. See
`research/20260905_0717_native-view-cache.md`.

Scene lifecycle checkpoint (2026-09-05): both scene begin/end bodies now run
on the host with explicit persistent colour/depth roles, typed native pass
entry/exit and explicit-source output publication. The supported path no
longer uses the engine's 16-slot allocation list, surface constructors,
tiling branch or resolve-source guessing. All 21 CTests and five source
guards pass; the final corrected-build desktop run has 53375 matching ownership checks,
no begin/end fallback and 0/119 large capture jumps or cyan patches. Initial
VR validation exposed one-layer depth outputs; creation now preserves both
eyes. The corrected full-size final-eye sequence also has 0/119 large jumps,
no cyan patches and 124147 matching ownership checks without fallback.
The camera/cache execution boundary is converted by the checkpoint above;
state 308, engine camera sources/descriptors/getters,
shared MSAA/scale copies and downstream post/UI tile-chain adapters remain.
This is not a completed scene producer set or fully native frame. VR remains
blurred/letterboxed and depth-inconclusive; later scenery/text are not
requalified. Both normal flat and final-eye checks use the corrected build. See
`research/20260905_0641_native-scene-pass-lifecycle.md`.

Frustum producer checkpoint (2026-09-05): complete six-plane construction
executes on the host, and the default-view host walk consumes a native
current-frame volume instead of importing engine planes. All 20 CTests and
three source-boundary guards pass. The final byte-safe build records 18341
matching original publications, 436841 matching consumer-shadow checks, zero culling
disagreements/fallbacks/missing native volumes and 0/119 large capture jumps.
Exceptional startup values are handled natively too. Normal comparison-off
flat and 1440x1584 final-eye sequences also have 0/119 large jumps and no
frustum fallbacks or missing native volumes. VR remains blurred/letterboxed
and depth-inconclusive; later scenery/text were not requalified.
Engine view/projection/cache producers,
other-view tables and getter publications remain. Scene lifecycle conversion
is tracked above. This
cluster is view-frustum construction, not the previously inferred fog helper.
See `research/20260905_0559_native-frustum-producer.md`.

Parameter producer checkpoint (2026-09-05): two complete pass-projection
builders, matrix transposition, parameter flushing and both float setters
execute on the host for supported inputs. All 19 material/texture/state CTests
and three source-boundary guards pass. The final guarded-build comparison
records 1891328 matching publications, zero refusals/fallbacks, and 0/119 large
jumps in its short desktop sequence. Normal comparison-off flat and full-size
final-eye sequences also have 0/119 large jumps, no cyan patches and no parameter
fallbacks. VR remains blurred/letterboxed and depth-inconclusive; later scenery
and text were not requalified. Engine inputs/parameter descriptors, inline
writers, draw-time shader-register import and full scene-begin ownership remain.
See `research/20260905_0513_host-parameter-producers.md`.

Sampler producer checkpoint (2026-09-05): complete scene defaults, seven direct
sampler setters and supported changed engine setter calls execute on the host.
All 18 material/texture/state CTests and three source-boundary guards pass.
The live original-publication comparison has 146571 matches and zero fallbacks;
normal short flat and full-size final-eye sequences also have 0/119 large jumps
and no cyan patches, with no sampler fallbacks. VR remains blurred/letterboxed
and inconclusive for depth; later scenery/text were not requalified. Inline
material writers and per-draw fetch import remain: this is producer execution
conversion, not complete live native sampler or frame ownership. See
`research/20260905_0436_host-sampler-producers.md`.

Nested pass checkpoint (2026-09-05): supported pass push/pop execution now
uses a host-owned attachment/extent stack and shared host attachment binders.
The engine's saved handles remain checked getter/lifetime adapters; native
pop does not recover its targets from them. All 17 material/texture/state/pass
CTests and three source-boundary guards pass. Normal short/late/final-eye runs
exercise native colour/depth/null scopes with no compatibility calls or shadow
mismatches (peak nesting 1). Short desktop and full-size 1440x1584 final eyes
have 0/119 large jumps, but later rock-wall popping remains (110/119 large
changes), and VR still has blurred/letterboxed content and inconclusive depth.
These are getter-shadow checks, not an original-producer comparison. Engine
traversal, scene-begin producers, allocation/resolve adapters and full frame
scheduling remain. See
`research/20260905_0355_native-pass-scopes.md`.

Scene-image replay checkpoint (2026-09-05): explicit per-draw current/next
roles now replace retained image bindings for converted scene callbacks.
Ordinary writes clear role ownership; null remains a no-op. Replay resolves
today's inputs and preflights the whole node outside the video/store locks.
All 16 material/texture/state CTests and three source-boundary guards pass.
The sampled run records 3414 matching scene-input checks and 11613 composed
scene-role draws; the general replay comparator still fails other inputs.
The source guard also distinguishes equal images selected by different table
paths. Normal execution records 34 matching source checks and 13133 dispatched
scene-role draws, with no refusal/compatibility calls. Later scenery still fails:
113/119 large changes and inspected rock-wall popping. Final-eye multiview at
explicit XR scale 1.0 produces 1440x1584 eyes with 0/119 large changes, but content
is still 1440x808 letterboxed/blurred and depth is inconclusive. That VR view
exercises no scene-role draws. Native scene associations, intra-node pass
sequences, other retained inputs and full visual qualification remain. See
`research/20260905_0318_native-scene-input-recipes.md`.

Scene-image producer checkpoint (2026-09-05): current/next scene-table selection
and the complete scene-image binding callback now execute on the host, with
explicit native image handles, live dynamic adapters and counted null no-ops.
Both bindings are preflighted outside the video lock before publication.
All 15 material/texture/state CTests pass. Desktop producer comparison records
39485 matching selections and 14 matching binding publications, with no
compatibility/refusal calls. All 28 bound inputs use native handles; dynamic
and null publication cases were not exercised. Its early field/title-transition
capture does not qualify the normal comparison-off path, later scenes or VR.
Scene-table production, persistent scene associations, remaining replay recipes,
the wrapper's blend/constants and full visual qualification remain. See
`research/20260905_0301_native-scene-textures.md`.

Reflection binding checkpoint (2026-09-05): supported direct phase-0 draws now
decode explicit selection and enable recipes, resolve current pass/table inputs
before submission, and discard the retained slot-5 image. All sub-draw bindings
are preflighted before dispatch. Null-selection inheritance remains an explicit
compatibility refusal, not an invented unbind; ordinary/animated overrides,
deferred/nonzero-phase recipes and persistent native scene associations also
remain. See `research/20260905_0144_native-reflection-selection.md` for source,
the initial diagnostic failure, corrected integration and verification scope.
This is not a completed reflection pass or fully native frame.
The corrected sampled transition has 490655 matching source checks, 179
unsupported scene-target callback draws and no slot-5 differences in the
bounded replay log. Supported GPU coverage here is disabled, pass-default
reflection selection; table-selected/enabled/dynamic cases remain unqualified.
The subsequent normal late run deadlocked between capture's texture lookup and
an IO upload, before producing captures. Source validation now snapshots at
draw time and resolves outside the video lock before template publication;
14 CTests and two source-boundary guards pass. The corrected normal run advances
through loading, with 1214021 matching source checks (including 6701 enabled
pass-default draws), but later rock-wall popping and damaged text remain:
108/119 large frame changes. Table/dynamic bindings remain unqualified. Normal
final-eye multiview has 0/119 large jumps but blurred/letterboxed, below-target
936x1030 eyes and inconclusive depth. See
`research/20260905_0235_reflection-validation-lock-order.md`.

Lighting checkpoint (2026-09-05): the complete lighting setup producer and its
reset/dimension helper execution now run on the host. Address-free records hold
ambient/camera/colour and shadow sampling inputs; supported direct phase-0
replays use the explicit shadow sampling record instead of retained constants.
The corrected short run has 13538 matching full publications and 200650 matching
direct-node input checks, with 0/119 large frame jumps. All 13 standalone
upload/state/verification/lighting tests pass. Engine scene/light descriptors,
texture associations, material staging/flush, other draw recipes and full-game
verification remain. The normal late run has 43580 host publications with zero
compatibility/reset calls and 700323 matching direct-node checks, but still
loses scenery and damages text (107/119 large frame changes). Normal multiview
has 0/119 large jumps but blurred/letterboxed eyes, inconclusive depth and the
same below-target 936x1030 eyes. See
`research/20260905_0121_native-lighting-pass.md`.

Verification follow-up (2026-09-05): replay diagnostics now retain bounded
examples in later scenes, report declared shader-input differences separately,
compare buffer fields without padding noise and flag incomplete compared-draw
counts. All 12 standalone upload/state/verification tests pass. Recurring reports
identify camera and animated-UV input mismatches; the sampled late baseline
still loses background surfaces and has damaged text. This is a diagnostic
checkpoint, not a new native producer or a fix for those pixels. See
`research/20260905_0053_recurring-draw-verification.md` for exact scope and runs.

Latest skin checkpoint (2026-09-05): explicit per-draw joint bindings now come
from model commands or deferred-entry indices. The host gathers each draw's
current palette before submission; matrix-value identity guessing and the
single final-node bone table are removed. Independent tests cover equal poses
that diverge, different per-draw bindings, capacity and transactional failures.
The normal late run records 787878 source checks with zero mismatches and
481158 replayed palettes. Inspected character stretching is gone, superseding
that specific failure in the earlier packet checkpoint below. Background
surfaces and text still fail: 110/119 frame pairs exceed the 6% jump threshold.
Normal final-eye multiview has 0/119 large jumps but inconclusive stereo depth.
See `research/20260905_0025_native-skin-bindings.md` for both runs and the
replay-off control. Skeleton/animation evaluation, pose sources, persistent
skin scene assets, discovery/list adapters and the shader-register ABI remain
explicit conversion boundaries; this is not a fully native skinned frame.

Packet checkpoint (2026-09-05): host draw packets now retain authoritative shader,
declaration and raster/blend/alpha intent throughout dispatch. Engine bind/setter
history no longer overwrites replay packets, and shared vertex decoding uses the
packet declaration. The new SDK-independent ownership regression test passes
alongside the other ten upload/state tests, and the desktop renderer linked
without rebuilding guest objects.

With replay enabled, the latest short flat and final-eye multiview sequences each
have 0/119 jumps over 6% and no cyan patches. Inspected eyes no longer have broad
horizontal banding. This supersedes the short-field flicker findings in earlier
checkpoints below; it does not qualify other scenes. Stereo depth remains
INCONCLUSIVE, blur/letterboxing remains, and actual eyes are 936x1030 instead of
the 1440x1584 target. At that checkpoint, a longer run using the prior late-scene
capture settings failed with deformed characters, disappearing scenery and
damaged text. See `research/20260905_0010_native-draw-late-scene.md`; zero allocation
failures and zero cyan patches do not qualify those pixels.
See `research/20260904_2348_native-draw-intent.md` for the replay-off control,
consumer overwrite trace, normal-path captures and remaining producer boundaries.
Earlier raster/blend/alpha draw-application counters included replay flushes;
their zero-mismatch setter checks did not establish packet ownership. The replay
comparator also did not dispatch its expected packet, so a zero pipeline-state
mismatch count could not detect this consumer bug.

The subsystem checkpoints below retain their historical verification outcomes.
Their remaining conversion boundaries still apply unless explicitly superseded.

`gpu/scene/native_mesh*` starts the native geometry asset boundary: loaded
model indices become triangle lists, GPU-ready vertex streams are persisted,
and native assets upload into shared host geometry arenas. Existing generated
LOD lists feed that same importer. The format contains no guest addresses.

This does **not** yet remove the draw-template interpreter dependency. The
importer currently retains packed vertex layouts understood by the existing
shaders, and its discovery is attached to replayed node draws. Complete native
material/layout definitions, asset-level loading, dynamic geometry, cache
streaming/eviction and replacement of the guest frame and draw producers are
still work to do. `bd_native_meshes` is on by default after the desktop checks
recorded in `research/20260904_1713_native-mesh-assets-and-capture-ownership.md`.
The counters cover indexed replays, not every draw in the game.

`gpu/scene/native_material*` now decodes named diffuse, specular/shininess and
reflection-colour properties from model commands into host-owned records. The
supported direct-tree phase-0 draws compose these with the object's colour,
without reading a sibling draw or the shared material staging globals.
`bd_native_materials` is on by default. Unsupported/ambiguous cases retain the
tracked compatibility path; this is not a complete native material system.
Materials are now shared immutable assets with stable content IDs, a checked
little-endian `.bdmat` format, independent cooking/loading and bounded residency.
The lighting-model slot includes a reserved Cel value; no native cel shader is
claimed. See [the format and cooker](NATIVE_MATERIAL_FORMAT.md). Complete texture
associations, mesh/material scene-asset loading, list-entry/phase-1 recipes, complete
lighting/shader definitions and shader-ABI replacement remain required. See
`research/20260904_1748_native-material-properties.md` for exact coverage and
correctness-only comparisons, and
`research/20260904_1806_persistent-native-material-assets.md` for independent
loading, cold/warm desktop captures and the persistent-asset tests.

`gpu/scene/native_shadow*` now composes a named receiver-shadow policy from
current node visibility and decoded model controls, instead of retaining that
decision in an old draw template. `bd_native_shadow_inputs` is on by default
for supported direct-tree phase-0 draws. The pure policy and stamp checks have
standalone tests; sampled and normal desktop checks found no input-composition
mismatches. This is still an import boundary: the pass enable, visibility stamps
and frame counters remain guest-produced, and the result still enters the old
shader ABI. List/phase-1 recipes and persistent shadow policy in native material
assets remain unconverted. It does not fix the recurring multiview defect.
See `research/20260904_2041_native-shadow-receiver-inputs.md` for exact coverage,
flat captures and the failed/inconclusive VR checks.

Deferred depth ordering and bounded allocation/batch planning now execute on
the host (`gpu/scene/deferred_work.h` and the temporary `deferred_list.cpp`
bridge). Replayed batches refresh world/palette and relocate their material
self-reference instead of retaining the original pooled pointer. The native
core has standalone capacity, ordering and relocation tests. This still
publishes big-endian entry images; remaining entry fields, material/pass records
and engine storage require conversion. The consumer replacement below now owns
the consuming loop. See `research/20260904_2055_host-deferred-work.md` for the
earlier allocation/sort checkpoint.
Its short flat check passes, but multiview still shows 10/119 jumps and the
later scene 79/119 with missing scenery/damaged text. Neither allocation/sort
conversion nor pointer relocation resolves those visual failures.

`gpu/scene/deferred_depth.h` now produces initial and replay depth on the host
from explicit bounds/far-extent or fixed policies. Replayed keys use current
world/view inputs, not old numeric depth. Whole-batch preflight includes depth
validation, and every entry must agree with its recipe's matrix source.
`bd_native_deferred_depth` defaults on. The input comparison recorded 20483
checks with zero mismatches; the final normal-path flat sequence has 0/119
jumps over 6% and no cyan patches. Multiview still has 10/119 jumps at the
64-frame cadence, with an inconclusive stereo-depth check. See
`research/20260904_2122_live-native-deferred-depth.md`.
The object/view transforms remain engine-produced; bounds/policy discovery is
not yet native scene-asset loading. Engine storage and other entry fields remain
tracked boundaries. The prior later-scene failure has
not been requalified by this short-run checkpoint.

`gpu/scene/deferred_consumer.cpp` now owns deferred-list iteration, visual-switch
scheduling, CPU bone gathering, material constant publication, ordinary/fur/
stencil surface expansion, direct draw issuance and list cleanup. Its valid-input
path replaces the original `sub_8227F360` loop. `bd_native_deferred_consumer`
defaults on; the explicit compatibility switch/import fallback is counted.
Standalone surface-policy and shader-ABI packing tests pass. The final flat
sequence has 0/119 jumps and no cyan patches; final multiview still has 10/119 jumps
at a 64-frame cadence and an inconclusive stereo-depth result. See
`research/20260904_2154_host-deferred-consumer.md` for verification and limits.

This is not a fully native frame: visual/material/shader callbacks and
state/resource adapters remain, with separate bridge counters. Some resource
adapters already route to host hooks, so these are boundary-call counts, not a
precise guest-instruction census. Engine entry storage, resource/declaration
associations, shader-register packing and replay's retained-state assumptions
still need replacement. Fur/stencil policies have standalone coverage but the
captured field does not exercise those GPU paths. The known later-scene failure
and full-game/both-eye acceptance gates remain open.

Object/pass transform publication is now host-produced too
(`gpu/scene/native_transform*`). The normal `bdBuildViewMatrix` path replaces
the guest producer, its default callback, transpose/multiply helpers and
constant setter. Camera interpolation/XR view composition feeds it directly
from native memory. `bd_native_transforms` defaults on; comparisons and
compatibility calls are counted independently. The final comparison recorded
826215 checks with no cache/constant/mask mismatches or compatibility calls,
including 203 nonfinite loading updates previously refused. Native assets
still use strict finite-value validation. See
`research/20260904_2216_native-render-transforms.md`.
The final normal flat sequence has 0/119 jumps and no cyan patches. Normal
desktop multiview produced 2387514 native transform updates with zero
comparison/compatibility calls, but still has 10/119 jumps at the 64-frame
cadence, blurred/banded eyes and an inconclusive stereo-depth result.
Engine object/camera/projection sources, inherited matrix cache and the
shader-register publication ABI remain temporary boundaries. This does not
replace native scene/pass scheduling or fix/qualify the previously documented
multiview and later-scene failures.

Raster/depth/stencil intent now lives in named host state (`native_raster*`).
The normal path replaces 15 `bdSetRenderState` setters and copies live native
fields at draw time, removing the per-draw engine raster-cache read/conversion.
`bd_native_raster` defaults on; diagnostic comparison defaults off. The live
comparison recorded 1491692 setter checks and 3070903 ordinary draw-state
checks, with zero publication mismatches/cache drift and one bootstrap import.
The normal flat sequence has 0/119 jumps and no cyan patches. Normal desktop
multiview still has 10/119 jumps at the 64-frame cadence, blurred/banded eyes
and an inconclusive stereo-depth result; it does not qualify VR correctness.
See `research/20260904_2238_native-raster-intent.md` for tests and captures.
Getter/cache/register shadows remain explicit engine adapters. Sampler,
other-state and material/pass producers, CCW stencil behavior, replay recipes and native
scene/pass assets remain unconverted. Field captures do not exercise stencil
operation/mask setters. This is not full frame or both-eye qualification.

Blend intent now also lives in named host state (`native_blend*`), with eight
host setters and no normal per-draw blend-register read/conversion.
`bd_native_blend` defaults on; comparison defaults off. The comparison recorded
4002268 setter checks and 3073105 ordinary draw checks without publication
mismatches, untracked blend writes or compatibility calls, using one bootstrap
import. Its short flat sequence has 0/119 jumps and no cyan patches.
The normal flat path also has 0/119 jumps and no cyan patches, with 4007188
native blend updates and no blend comparison/compatibility calls.
Normal desktop multiview records 8979675 host blend updates without blend
comparison/compatibility calls, but still reproduces 10/119 jumps at the 64-frame
cadence, banded/blurred eyes and inconclusive stereo depth. This is not a VR pass.
The source trace did not substantiate the earlier claim of inline device blend
writers outside the SDK setters; unrelated matching object offsets are not D3D
device writes. Verification still explicitly checks for untracked writers.
See `research/20260904_2302_native-blend-intent.md`. This is not a complete native
material/pass producer: getter/cache shadows, blend constants,
other-state execution and retained replay recipes remain. Separate-alpha and
operation setters have standalone coverage but no field GPU exercise so far.

Alpha cutout/coverage intent is now host-owned too (`native_alpha*`), with four
host setters and live ordinary-draw composition instead of retained pipeline
intent. `bd_native_alpha` defaults on; verification defaults off. The shared
C++/HLSL predicate supports all eight compare modes through specialization, and
the reference uses the SDK's exact 1/255 scale, correcting the former 1/256 hook.
CPU tests and regenerated SPIR-V verify the comparison contract, including
explicit ordered-NaN behavior. Publication comparison recorded 7196829 setter
checks and 7108657 draw-intent checks with zero mismatches/drift/compatibility
calls. The final normal flat path records 2274942 native updates without alpha
comparison/compatibility calls, 0/119 frame jumps and no cyan patches.
Normal desktop multiview records 13662279 native alpha updates without
comparison/compatibility calls, but the final-eye sequence still has 5/119 jumps
in one flicker cluster, blurred/banded eyes and inconclusive stereo depth. This
window does not establish recurrence or improvement over earlier two-cluster
captures. Actual eyes are 936x1030, not the requested 1440x1584 target.
See `research/20260904_2327_native-alpha-policy.md`. Engine getter/cache shadows,
native material/pass producers, replay recipes and the shader-register ABI remain.
The field exercises only GE and no alpha-to-coverage requests; other comparison
GPU paths, multisample coverage output and custom coverage offsets are not
qualified. This does not establish full frame or both-eye correctness.

Static textures now cross a persistent native boundary too: `.bdtex` files
preserve BC/RGBA data, mips, cube faces and volume slices with address-free
content IDs. The SDK-independent mip cooker persists missing chains; subsequent
loads use a versioned recipe cache without regenerating them. `bd_native_textures`
is on by default. See [the native texture contract](NATIVE_TEXTURE_FORMAT.md).
CPU assets are shared and budgeted. A device-owned native GPU store now shares
images, views and descriptors by content ID, with native handles and fence-gated
retirement independent of guest wrappers. The remaining resource bridge only
borrows those bindings. Explicit immutable material slots now hold native image
handles directly, including cube/volume companions. Stable sampler recipes use
native descriptors without per-replay fetch decoding. Inherited/dynamic inputs,
asset-level scene loading and guest draw/pass replacement remain required.
Cold/warm desktop and independent-loader evidence is recorded in
`research/20260904_1833_native-textures-and-persistent-mips.md`.
Shared GPU lifetime tests, runtime reuse/retirement and flat/multiview captures
are recorded in `research/20260904_1854_shared-native-texture-gpu-ownership.md`.
The native binding checkpoint, compound-recipe lifetime fixes and 120-frame
flat capture are recorded in
`research/20260904_1946_native-material-texture-bindings.md`. Its capture has no
jumps above 6% or cyan patches, but the longer run later exhausted a 32 MiB
constant-buffer slot. That checkpoint was not a clean long-session qualification;
the wrapping hazard is addressed by the upload separation below.

Resource staging now uses bounded, fence-reclaimed **host upload pages**,
independent of the shader-register buffer. Native textures and the native UI
use the host API directly; compatibility bulk adapters share it. Shader storage
no longer wraps on exhaustion, and transient vertex streams cannot be cached
as immutable cross-frame geometry. See [the upload contract](HOST_UPLOAD_ARENA.md)
and `research/20260904_1959_host-upload-pages.md`. A longer loading run no longer
reported overflow, but its later scene still had severe dark/missing-geometry
frames. This remains a correctness failure, not full transition qualification.
The longer baseline was rerun after the final transient-stream lifetime fixes:
77 of 119 frame pairs exceeded the 6% jump threshold and inspected frames still
showed broken geometry/text. Upload separation did not solve that scene.

The earlier diorama control exposed a 64-frame lighting flash in the existing
template path, present with native meshes or native materials disabled too.
The upload-page checkpoint's multiview check reproduced that cadence: 10 jumps in
119 frame pairs, with no upload or constant-storage errors. The presented eyes
in that distant view still do not establish a stereo-depth verdict.
The native packet-ownership fix above removes those jumps and broad banding in
its short capture, but still does not establish stereo depth or full-game
correctness. The host transition is not complete.

Shared working instructions live in [AGENTS.md](../AGENTS.md). The former
CLAUDE.md is preserved as a [historical snapshot](archive/CLAUDE_2026-09-04.md),
not current guidance. Use current code, dated evidence and this scope when
deciding what remains. Never claim that a desktop timing proves a Quest
performance result.
