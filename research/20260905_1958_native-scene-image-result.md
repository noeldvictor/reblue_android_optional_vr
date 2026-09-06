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

## Fresh retention review and bounded image plan

Read the complete sampler (0436), parameter (0513), frustum (0559) and view-cache
(0717) worklogs. Select only their four normal comparison-off flat field sets:
`host_sampler_flat`, `host_parameters_flat`, `native_frustum_guarded_flat`,
`native_view_flat`. Each contains 120 raws / 995,330,400 bytes and two retained
endpoint PNGs. They answer the same short normal field stability question as
the current qualified post-image-flow baseline; their complete sequences are
superseded, not needed for an unresolved comparison. Keep comparison controls,
all VR/framing/failure evidence and all eight PNGs (2,348,009 bytes) and reports.
The view/frustum failed or initial probes are not targeted. Older layered-scene
sets are also untouched. This is individual review, not blanket archive expiry.

Reuse the existing exact-path cleanup validator with the four fresh names and
worklog endpoints, rather than making another helper copy. It must validate
counts/sizes/endpoints, retained PNGs, no reparse ancestry, exact two-link NTFS
membership, all same-name references and no running renderer, first dry-run and
again at deletion. Planned removal 480 unique raws / 960 links, 3,981,321,600
logical bytes. Historical pixels will be unrecoverable; small evidence remains.

Only after measured reclamation: allow one 120-frame normal flat and one
120-frame final-eye VR set, `native_scene_result_flat` / `_vr`, together
3,185,054,400 raw bytes, no extra retry allowance. The existing <=100 MiB
cumulative small-output cap includes both prior diagnostics, analysis exports,
logs/perf, helpers and caches. Reuse the exact built executable/test trees;
no build, asset cook, download or new configuration tree. Expected peak reserve
exceeds 53 GiB after reclamation and replacement; enforce minimum 20 GiB plus
polling headroom. Check limits during each bounded run, stop owned processes
and restore temporary settings in guaranteed cleanup. Retain current baseline
until both new sets are actually inspected and qualified. This plan does not
permit the historical raw archive to grow or relax the full desktop gate.

Fresh global NTFS inventory matches the prior checkpoint exactly: 29,085 raw
paths / 28,091 unique payloads / 260,139,759,700 logical bytes, 230,379,540,272
allocated bytes. First cleanup dry run safely refused: these older sets are
independent copies, not the newer two-link isolation pattern. No files removed.
Direct NTFS identity checks across all 480 pairs confirm distinct identities and
one link each. Extend the same validator to accept only singleton paths with
matching pairwise SHA-256; retain the same ancestry/reference/process checks.
Thus planned removal is 960 unique payloads / 960 paths / 7,962,643,200 logical
bytes, not the initial 480-payload estimate. Measure actual recovery separately;
the incoming capture allowance stays 3,185,054,400 bytes and is not increased.

Cleanup dry-run session 98265 and deletion session 21120 both completed. All
480 copy pairs matched SHA-256 and singleton NTFS membership; reference/ancestry
and stopped-renderer checks passed again before removal. Removed exactly 960
paths / unique payloads. Actual free 57,528,311,808 -> 65,494,888,448 bytes:
7,966,576,640 bytes (7.42 GiB) recovered. All eight endpoint PNGs/reports remain;
removed historical pixels are unavailable. No other directories or evidence
were deleted. Raw allowance consumed zero, remaining 3,185,054,400 bytes.

Reused `run_post_image_flow.ps1` with the new exact set names and 19:58 cutoff.
It defaults to no captures, requires the original five-setting profile at entry,
restores its exact bytes in nested finally cleanup, and stops only its owned
process. Explicit image runs have 120-frame/byte limits and 110-second timeout;
two-second checks cover cumulative raw bytes, logs/perf/cache/dumps, with a
75 MiB stop threshold leaving 25 MiB for analysis/polling, and stop below 21 GiB
to protect the 20 GiB reserve. Cache/dump roots have no reparse descendants.
The existing xrsim manifest has an absolute path to the 31,232-byte DLL.
No global runtime/registry changes. Pixel/stereo analysis will reuse the existing
streaming helper and only export endpoints, not every frame.

VR runner session 14380 / owned PID 23708 started at 20:34:39, log 815.
Output is the automatic capture root plus hard-linked `native_scene_result_vr`;
limit 120 x 18,247,700 bytes, timeout 110 seconds. The 16-setting audit passed.
The separate flat allowance remains 995,330,400 bytes; do not duplicate this run
if a poll is quiet. Capture receipt and actual pixel verdict follow on completion.

## Next boundary source evidence

Read the existing Plume interface and Vulkan resolve implementation in the
actual submodule root. `VulkanCommandList::resolveTextureRegion` hardcodes
base array layer zero / layerCount one and loops mips, not eye layers. It calls
`vkCmdResolveImage`; the searched Vulkan backend has no attachment-resolve
description. It is not a ready drop-in for a complete two-eye colour/depth
completion path. Current `CopySurfaceToTextureLocked` also handles exposure,
format retargeting and differing extents. Removing the initial compatibility
publication requires explicit native resolved-image ownership and suitable
layered colour/depth resolve support, while retaining ordered exposure and
separate resampling semantics. Do not merely rename the shared copy path or
substitute the single-layer helper and claim EDRAM removal. No dependency or
shader code changed during this review.

## Completed native-result VR qualification

Session 14380 / PID 23708 completed at 20:35:51, log 815. Profile restored
byte-for-byte in cleanup. Exactly 120 raws / 2,189,724,000 bytes, hard-linked
isolation, `frame_1788654942_0.raw` through `frame_1788654949_119.raw`, render
frames 8484..8603, stacked 1440x3168. Actual free after capture 63,302,299,648
bytes. Remaining raw allowance 995,330,400 bytes for flat only; no retry.

Streaming analysis session 6899 completed: 0/119 changes above 6%, maximum
0.312763047% (pair 63); cyan maximum/median zero and no hits. First/last
stereo both correctly crossed: far -1 px, near -9 px, spread 8. Inspected all
four full-resolution eye endpoints: consistent rocky village, stairs, ground,
orange sky and moving windmill shadows, without broad missing bands/cyan.
Existing distant blur remains; this framing does not qualify character-shadow
alignment. Six endpoint/overview PNGs only, no every-frame export.

Last post counters: 8,401 completed native scene inputs/scopes/sequences/roots/
final publications, zero imports, original scopes or refusals; max roots one,
no direct inter-root GPU coverage. Scene report samples one entry earlier:
8,400 completed/consumed, both images materialized 8,400 times at MSAA4. The
16 settings audited; checked error/critical/VK_ERROR/device-loss/fatal/assertion/
exception/upload-exhaustion markers absent. No renderer or analyzer remains.
This qualifies the normal short field stereo window, not full-game coverage.
Next use only the remaining 120-frame flat allowance with the original settings.

## Flat qualification and baseline retirement

Flat session 98427 / owned PID 27436, 20:37:37-20:38:44, log 816, completed;
the original profile was restored byte-for-byte. Five settings audited, full
1673 archives / 119346 names mounted. Last sample 3,001 completed native inputs,
post scopes/sequences/roots/final publications, zero getter imports, original
scopes or refusals. Scene samples 3,000 completed/consumed with both images
materialized; maximum post roots one. All four checkpoint logs have zero checked
error/critical/VK_ERROR/device-loss/fatal/assertion/exception/upload-exhaustion
markers. VR log confirms the actual runtime session and eye/game pose difference.
The executable SHA-256 remains unchanged; no rebuild for documentation hashes.

Flat captures: 120 x 1920x1080, `frame_1788655120_0.raw` through
`frame_1788655123_119.raw`, render frames 2845..2964, 995,330,400 bytes. Streaming
analysis completed: 0/119 changes above 6%, maximum 3.200279707% (pair 1), no
cyan hits, maximum .020978009%, median .011453511%. Both full endpoint images
inspected: recognizable Shu/cast silhouette, foliage, solid ground and moving
windmill shadows. Distant DoF remains. This does not qualify the late scene,
authored events, title artwork or the full desktop game-mode gate.

All planned raw bytes are now consumed: 3,185,054,400, no remaining allowance.
The new pair passed streaming, endpoint and both-eye review and becomes the
current baseline. Following the prior post-image-flow worklog's explicit
equivalent-replacement cleanup condition, its normal flat/VR pair is superseded.
Retire only `native_post_image_flow_flat` (`frame_1788652260_0.raw` through
`frame_1788652264_119.raw`) and `_vr` (`frame_1788651740_0.raw` through
`frame_1788651750_119.raw`), each 120 raws with their exact automatic hard links.
Keep all eight PNGs (9,780,919 bytes), logs/reports and distinct protected evidence.
Revalidate same-name references, ancestry, sizes, endpoints, NTFS link membership
and terminal processes using the existing validator, dry-run before deletion.
Planned further removal 240 unique raws / 480 paths / 3,185,054,400 logical bytes.
This is additional checkpoint cleanup, not an allowance to start another producer.

## Final storage ledger and handoff

The second dry run passed, then deletion revalidated all targets and completed:
240 unique raws / 480 paths / 3,185,054,400 logical bytes removed. Actual free
62,285,619,200 -> 65,471,127,552; measured recovery 3,185,508,352 bytes. All eight
old post-image PNGs remain. Combined with the four earlier retired flat sets,
six directories now contain zero raws and retain all 16 PNGs (12,128,928 bytes)
and reports. Historical deleted sequences are unrecoverable.

Current baseline `native_scene_result_flat` / `_vr` retains 240 unique raws /
3,185,054,400 bytes plus eight PNGs / 9,716,547 bytes. Four checkpoint logs total
1,255,865 bytes, eight perf files 3,195,328 bytes; no changed cache/dump files.
Runtime/analysis small outputs total 14,167,740 bytes. Reused helpers, CTest logs
and small documentation keep the checkpoint well below its 100 MiB small cap.
No new asset outputs, downloads, guest rebuild, new build tree or duplicate
capture payloads. All owned runtimes/analyzers/cleanup sessions are terminal,
and the original profile and unchanged binary hash were checked again.

Final scoped NTFS inventory: 28,125 raw paths / 27,131 unique payloads,
252,177,116,500 unique logical bytes; GetCompressedFileSizeW reports
222,416,897,072 bytes after identity deduplication. Relative to preflight,
960 fewer unique payloads and 7,962,643,200 fewer logical raw bytes. The archive
still exceeds the 10 GiB target: this is reduced cleanup debt, not compliance.
Readiness/grading startup controls, distinct authored/synthetic previews/shared
probes, comparison/VR/framing evidence and unresolved late-scene failures remain
protected pending their individual review/resolution. Keep the new baseline until
equivalent verified replacement; preserve small reports/endpoints afterward.

Ending actual free 65,470,930,944 bytes (60.97 GiB). From the original worklog
preflight of 57,541,398,528 bytes, net volume usage fell 7,929,532,416 bytes
(7.38 GiB). Both cleanup receipts are gross; replacement captures and all small
outputs are included in the measured net change, as is unrelated volume activity.
The entire 3,185,054,400-byte capture allowance is spent. No further large
producer is planned under this checkpoint; reconcile storage anew before one.

Implementation is pushed as `ca90d3f`; README/transition now distinguish scoped
completion from the remaining image publication, MSAA/scale copies and engine
producers. Guest-source anchored the ownership boundaries; devloop reused the
built executable and bounded/restored runs; vrsim qualified only desktop eyes.
This is progress, not completion. Next remove the initial compatibility image
publication with explicit native resolved images and suitable layered resolve
support, then final UI publication and remaining frame/data producers. Full
animation/material/light/scene ownership and representative fields, battles,
cutscenes, menus, transitions/reloads and both-eye authored effects remain
required before Quest 2 qualification. No device work or performance claim.
