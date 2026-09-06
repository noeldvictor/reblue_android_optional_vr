# Complete parent view scheduling on the host

2026-09-06. Root 5294ea8 plus preserved local renderer integration; Plume remains
unpublished without owner approval. Previous goal turn made progress with shared
pass lifecycle execution. This follow-up uses guest-source/devloop and the same
storage ledger in `20260906_0333_native-scene-state-bridge.md`; the full desktop
renderer goal remains active and no Quest work is authorized before its gate.

## Scope and source review

The complete parent `bdRenderViewSubmit` (0x82184E90, size 0x19AC, generated file 16)
was read in the preceding dispatcher work, not substituted with the misleading
object insertion function named AllPasses. Rechecked current hook maps, wrapper,
dispatcher and source dependencies. Additional complete bodies read here include
scene preparation sub_821824A0 (file 99), ray construction sub_8213F240 (file 20),
sphere/frustum query sub_821CE028 (file 60), projection-frustum construction
sub_822873E0 (file 18), cached view construction sub_82186840 (file 24), the authored
trigonometric threshold sub_826BF5B8 (file 63), and post-container constructor
sub_8221C9A0 (file 57). Existing native_view/native_frustum code implements the
same view-shape/plane construction; the 10% vertical guard is in sub_822873E0,
so the parent uses that same convention, not an invented narrower reflection
volume. Generated source and hook addresses remain unchanged.

## Implementation and remaining imported boundaries

The native schedule preserves live indexed-view count/eligibility, sun and
six-face shadow modes, auxiliary and shadow-volume views, primary plus ten
secondary reflection candidates, six environment faces, additional/main scene,
motion blur, conditional post and saved effect-state restoration. Conditions are
sampled at their original stage boundaries instead of freezing every request
before callbacks. Parent-inlined starts now enter the shared native dispatcher.

Camera-ray, focus and reflection sphere/frustum arithmetic operate on native
values, with authored near-zero thresholds and exact tangent/NaN query rules.
The existing native camera/scene-result scope still surrounds the entire view.
Projection callbacks retain their FOV/aspect hook. No shader-GPR allocation calls,
tiling, console image creation or console resolves are introduced in this parent.

Temporary adapters remain for authored data, effect toggles, scene preparation,
pass descriptors, object lists, mirror projection, trig threshold, motion blur,
state-200 publication and unconverted participant callbacks. Descriptor getters
are released in original +36 then +28 order after pass completion; cube faces
reuse one descriptor through all six iterations. Reflection mode is set once
for the reflection group, not reset after every callback. A normal post handoff
uses native completed images; refusal preserves the full isolated compatibility
sequence and both temporary containers, never replaying the rendered parent.
The obsolete debug texture-export flag is an explicit pre-effect compatibility
case, not silently dropped game rendering or new permission to write captures.

Only preflight refusal may return to the original parent. Later invalid imports
fail explicitly. Inherited caller stack restoration is RAII; this is not a new
natural-shutdown guarantee. Parent wiring overlaps the existing dirty renderer
integration; inspect its API dependencies before staging, and never drag the
unpublished Plume dependency into a parent gitlink.

Tests cover all 256 schedule-request combinations, live indexed bounds and later
request mutation, empty/negative counts, failure without parent replay, ray/focus
normalisation boundaries, transformed spheres, tangent/NaN visibility and all six
planes. Five new source guards check full branch coverage, the pre-effect fallback,
native geometry/order boundaries, post fallback cleanup and outer camera scopes.
Actual test/build/runtime outcomes are recorded below, without equating a
short field check to the required full-game/both-eye qualification.

## Verification: linked implementation

Focused `host_scene_pass_test_03` (PID 24900) exited 0 in two build steps;
stdout/stderr are 139/0 B. Existing `cpu_16` (PID 25496) passed 31/31 in 3.33 s,
including the expanded scheduler cases; stdout/stderr are 3,626/0 B. The five
new view-schedule, 24 scene and 36 post source guards pass (65 total), rechecked
after the documentation interruption. These guards are not GPU evidence.

Host `reblue_19` (PID 19804, session 96640) is terminal, exit 0, displayed link
14/17. Expected glob reconfiguration added the new source to reblue_common's
OBJECT library. Codegen reported its module up to date; no guest objects rebuilt.
Build stdout/stderr are 2,730/36 B. Binary length 47,660,544 B, linked 04:43:25,
SHA256 `4381dd845c5c9d2dd7dc7523a301f7c80533f0cde259420e463de6b159165326`.
It records root 5294ea8 plus local integration, Plume 81bdca8. The later
AGENTS-only commit 4d9840e does not justify rebuilding that unchanged renderer.

Resume preflight found no game/build/compiler/linker producer, the expected
owner profile, and 64,578,666,496 B free. This is the same cumulative storage
budget, not a new allowance. Bounded flat MSAA run PID 20672 / session 48452,
04:50:53--04:52:09, wrapper exit 0; log 858 is 251,162 B. All five settings took
effect. Native view scheduling reached 3,601 views, zero compatibility/refused/
faults, 9,942 pass/list adapters, 3,601 preparation adapters, 36,010 reflection
candidate tests, zero rejected, legacy-post scopes or debug-export refusals.
The shared dispatcher now also owns previously inlined starts: 9,940 begins /
9,939 ends at its last sampled boundary, zero fallback/refused/faults. Counters
sample at different points during a view; their one-pass difference is not a
shutdown-balance assertion. Scene clears/results and native post handoffs remain
native, with zero scene ownership errors, compatibility clears/depth publications,
state-308 calls or post imports/refusals. Short autoplay coverage does not exercise
every authored special-view branch merely because the parent contains it.

Inspected owned flat window `out/verification/native_view_schedule_window.png`,
04:51:53, 1920x1080, 3,361,048 B, SHA256
`18bcf636d88066cda0fd0b845c5b45e6ba25f9974e9e1549b41db32926364233`.
Shu, terrain, foliage, structures, cast shadows and distant depth of field are
visible without obvious full-frame corruption. This is one unaligned flat sanity
image, not sequence, event, stereo or full-game qualification. Perf CSV 045055 is
606,208 B plus 112-B metadata. No new raw capture; ending free 64,573,485,056 B.
The wrapper stopped only its own game process and restored the profile bytes.
This is not a natural-shutdown or game Vulkan-validation result.

Desktop XR MSAA PID 356 / session 40012, 04:52:29--04:53:45, wrapper exit 0;
log 859 is 565,674 B. All 16 settings took effect, using the existing absolute
xrsim manifest, 1440x1584 per eye, multiview, height 0, scale 1 and no mirror or
preview. Native scheduler 9,601 views, 23,275 pass/list adapters, 9,601 preparation
adapters, 96,010 reflection candidates, zero compatibility/refused/faults, legacy
post scopes or debug-export refusals. Dispatcher 23,273 begins /23,272 ends at
its last sampled boundary, zero compatibility/refused/faults. Scene clears/results
9,600 and native post handoffs 9,601, with zero ownership errors, compatibility
clears/depth publications, state-308 calls or post imports/refusals. Perf CSV
045232 is 1,617,920 B plus 112-B metadata; ending free 64,569,561,088 B. No raw
capture or new stereo pixels. Same owned-stop/profile-restoration limits apply;
this does not establish Quest performance or full both-eye correctness.

Flat MSAA with native post disabled: PID 26468 / session 10387,
04:54:17--04:55:33, wrapper exit 0; log 860 is 259,974 B. All six settings took
effect. Parent views 3,601, legacy-post scopes 3,601, parent compatibility/refused/
faults zero. Dispatcher 9,942 begins /9,941 ends, zero fallback/refused/faults.
All 3,600 sampled deferred scene colours recovered; compatibility clears/depth
publications and scene ownership errors stayed zero. Native-post settings refusals
and original scopes are expected here (3,600 at its earlier sampled boundary),
not parent fallback; memory/effect/input refusals remain zero. This executes both
temporary container construction/destruction paths after rendering once. Perf
045419 is 618,496 B plus 112-B metadata; ending free 64,567,980,032 B.

Flat non-MSAA: PID 21420 / session 38338, 04:56:17--04:57:33, wrapper exit 0;
log 861 is 249,440 B. All six settings took effect. Parent views 3,601, pass/list
adapters 9,943, zero compatibility/refused/faults or legacy-post scopes. Dispatcher
9,941 begins /9,940 ends with zero fallback/refused/faults. Scene native clears
and completed inputs 3,600, zero ownership errors or compatibility clears/depth
publications. No MSAA result/materialisation occurred; normal native post had
3,601 direct handoffs with zero imported scopes/refusals. Perf 045620 is 602,112 B
plus 112-B metadata; ending free 64,566,403,072 B. Neither recovery nor non-MSAA
run produced raw captures or extra screenshots. All four runs are terminal and
the owner profile is restored; no game validation layers were enabled.

## Checkpoint and next conversion boundary

Staging review checked the committed APIs, not just the dirty worktree: parent
`NativeSceneResultScope::Clear`, `bdNativeScenePostHook`, resource release, native
view math and the source glob already exist in HEAD. The scheduler and its parent
wrapper can therefore be checkpointed together without changing the Plume gitlink
or staging unrelated native image/framebuffer integration. The explicit result
clear stays before camera-scope teardown. Tests used the recorded dirty renderer
binary, not a separately built committed-only tree; do not imply otherwise.
After runtime verification only comments/documentation changed; no redundant
renderer rebuild was needed. No push was retried under the existing upload denial.

The next ownership boundary is authored scene/effect/participant production and
the remaining pass callbacks, not another guest parent scheduler. Rechecked the
complete sub_821824A0 body: it owns a lazy 104-B scene node, readiness/mask checks,
update/alternate-update, tree submission and conditional node draw. The effect
selector sub_82173DF8 is not merely a byte setter: its inspected selector-1 path
also changes registry membership through sub_8221D678/sub_8221D9A8. Do not replace
these with flag writes or erase their work. Their full ownership conversion,
native authored data/materials/animation/UI, asset formats and the desktop
field/battle/cutscene/menu/transition/reload/both-eye gate remain required. No Quest
qualification is claimed; the full goal remains active.

Storage accounting is in the existing continuing ledger. Nineteen superseded
small outputs were actually removed after equivalent verification, reclaiming
8,036,352 B measured. Protected raw/failure evidence and original game data remain.
