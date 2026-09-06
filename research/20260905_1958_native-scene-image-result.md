# Scoped native completed-scene images

2026-09-05 EDT, Windows Vulkan desktop. Base `5e98c97`.

Previous goal turn made progress: native post-image flow passed short normal
flat/both-eye verification, documentation was pushed, and net volume usage fell
2.94 GiB. No delegation or device work. Guest-source/devloop read completely;
the first combined read truncated, so the affected instructions were reread.

## Source decision

Read native scene begin/end, target ownership, pass push/pop, resource references,
explicit output publication and the scene-to-post callback. The main view caller
contains several pass types; a global last-scene pointer without scope would be
unsafe. Read its final phase-3 construction, flush, teardown and focus/post tail,
plus `bdShaderSystemFlush` (generated file 0), which invokes vtable +20 after
the render-list callbacks. Read scene end in generated file 9 and the existing
view wrapper in `engine/frame_interp.cpp`.

Using the existing owned-XEX helper in memory only, checked the PE section map:
final phase vtable 0x8206C884 has +12=0x82186BA0 and +20=0x82187010; +16 is
the no-op 0x820DFA50. Distinct earlier pass tables use other entry points.
No decrypted file, key output, decompiler installation or generated edit.
The native end is therefore the final scene-image producer before camera/focus
updates and the existing post hook at 0x821865B8.

Introduce a per-view RAII completion scope in the existing view wrapper. Native
scene end supplies the exact sampled image/exposure receipt from publication,
including materialized MSAA/size/format output; the normal post caller consumes
it once without resource-getter imports or resolve-link traversal. Native target
pins prevent reuse/recreation while a result is live; temporary destination
references remain explicitly counted adapters. Invalidate on a new scene begin,
reject foreign view/frame results, and release on consumption, scope exit or
exception. Keep the old importer only for compatibility scopes. Initial scene
publication/MSAA-scale copies, final UI publication and parent producers remain
migration work, not a completed host frame.

## Storage preflight

Actual free before work: 57,541,398,528 bytes (53.59 GiB). Reuse existing host
and CPU-test trees. Plan <=2 GiB peak host rebuild/link growth (shared device
header may rebuild host consumers), expected reserve >51 GiB; minimum 20 GiB.
No guest/hook/codegen input, shader, asset or dependency changes planned.
Low-storage checks precede runtime. New logs/perf/reports share a 100 MiB cap;
any diagnostic must explicitly disable captures and stop at 75 seconds.

Current baseline `native_post_image_flow_flat` / `_vr` stays protected. The raw
archive is still over budget: last verified 28,091 unique raws / 260,139,759,700
logical bytes. No new raw producer authorized by this preflight; reconcile and
review fresh eligible cleanup before any pixel captures. Do not reuse the prior
checkpoint's spent capture allowance or delete the current baseline prematurely.

## Implementation and low-storage verification

Scoped completion is implemented in the existing view wrapper and native scene
bridge, with no new guest hooks or generated source changes. Publication emits
a success-only `SceneImage` receipt: alias/source with incoming exposure, or
materialized destination with exposure 1. Native target reader pins are separate
from engine reference counts and reject reuse/recreation before slot mutation.
Move-only completed results retain the two remaining output adapters and release
pins/references on consume/clear/scope exit. The normal post root loop now takes
explicit inputs; only the compatibility helper imports getters. Disabled/empty
lists stay native no-ops even if compatibility image import is unavailable.

All 30 CTests and 45 source guards pass (29 post, 13 scene, 3 reflection).
Extended the existing scene-policy test with move-only lease accounting across
nested slots, once-only take, replacement, explicit invalidation, frame mismatch,
rollover and exception cleanup. Two guards initially failed on old API spellings;
updated the boundary assertions and added lifecycle/pin/receipt coverage, without
weakening original scope/ABI checks. These are not GPU/pixel qualification.

Host build session 18391 exited successfully. New headers triggered in-place
CMake glob refresh; codegen wrote zero files, module up to date. Shared device
header rebuilt host consumers only; no guest TU/shader/asset/dependency rebuild.
Focused CPU target `host_scene_pass_test` built two steps and all tests passed.
Post-build actual free 57,542,295,552 bytes, above the 20 GiB reserve and within
the <=2 GiB peak plan. No raw or other large producer started. Next is the
bounded 75-second, explicitly capture-disabled diagnostic; restore the original
five-setting profile afterward and inspect the actual completion/import counters.

Binary linked 20:15:05 EDT, 47,532,544 bytes, embedded `5e98c9720` plus these
local changes; SHA-256
`6843380c9cfa907e53daab26cfd05afaef8197baf5157d71e4954e35d817b72d`.
Diagnostic session 19165 / PID 17884 is terminal, log 813, 20:16:33-20:17:50.
All five settings audited, capture delay zero and zero raws. Last post sample
3,601 completed native inputs/scopes/final publications, zero image imports,
original scopes or refusals. Scene reporting occurs one sample earlier: 3,600
completed/consumed results, 3,600 materialized colour and depth images. Default
MSAA is four samples; this establishes the materialized receipt path, not the
direct source alias path. No pixel or multi-root GPU qualification from this run.

Next bounded correctness-only diagnostic: same binary/75-second stop/no captures,
temporarily `bd_msaa=0` to exercise direct source receipts/pins. Continue the same
100 MiB small-output budget, including automatic caches; actual free after the
first run 57,540,812,800 bytes. No additional build, asset conversion or capture
allowance. Restore the five original settings after this probe. Pixel/VR checks
remain pending a reconciled capture/retention plan.

## Resumption and source checkpoint

The previous goal work made progress: host build, 30 CPU tests, 45 source guards
and the materialized-input diagnostic passed. The intervening owner request
strengthened AGENTS storage enforcement and was pushed separately as `f6ff45d`.
No duplicate runtime was launched. Session 14823 / owned PID 23640 completed
at 20:20:46; log 814 covers 20:19:29-20:20:46. With MSAA zero, its last post
sample has 3,601 completed native inputs/final publications, zero imports,
original scopes or refusals. Scene reporting has 3,600 completed/consumed,
only one materialized colour and zero materialized depth: direct source pins
are exercised, as well as the prior four-sample materialized path.

Both diagnostics mounted 1673 archives / 119346 names and audited their five/
six settings. No checked error/critical/VK_ERROR markers, and neither produced
raws. The original five-setting profile is restored (delay 60, no MSAA override);
no renderer remains. Binary SHA-256 is unchanged. All 30 CTests and 45 guards
rerun successfully on resumption, without rebuilding. Lifetime/source review
confirms references are retained before original releases and pins block native
target reuse/recreation. This remains counter/CPU evidence, not pixel or full
native-frame qualification. Commit this implementation before further capture work.

Storage recheck at 20:27:18: 57,528,176,640 bytes free. Existing current baseline
is preserved. Scoped per-directory capture inventory was read without producing
copies; earlier layered-scene VR has distinct corrected-failure/framing evidence
and is not selected for cleanup. Review eligible normal controls before budgeting
any new captures; no new raw allowance has yet been established.
