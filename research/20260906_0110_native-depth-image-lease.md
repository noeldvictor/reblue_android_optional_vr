# Native depth image handoff and shared layout ownership

2026-09-06; work in progress, no completed-frame or Quest qualification.

Previous goal turn made progress: independent native post pool/test checkpoint
`7c349c8`, desktop/CPU/source checks, flat image inspection and safe diagnostic
cleanup. Root still has pending renderer integration; Plume is locally `81bdca8`,
with the recorded parent gitlink `eb7b03c`. The prior GitHub publication denial
still requires explicit approval; no push or parent gitlink commit is attempted.

## Evidence and change

Read AGENTS, devloop/guest-source/vrsim skills, transition requirements and prior
ownership evidence. Inspected scene begin/end, result lifetime, resolve ownership,
getter publication, bindless retirement, texture layouts and post imports. Read
the existing `render_tweaks.toml` handoff contract and generated caller's focus
publication/native-hook/old depth-container sequence; no generated/hook edits.

Scene end still copied its MSAA depth through `PublishSceneOutput` even though
native post already receives the resolved native depth image. Replace that copy
with a retained native image/descriptor lease for matching depth getters.
Generalize the previous post-only lease instead of introducing another image
owner per effect. A shared layout record makes legacy reads/writes and native
passes observe the same state. Unbind before releasing the owning handle.

Complete native attachment resolves before publishing depth. Matching MSAA
depth publications do not allocate/copy, create a resolve link or publish tile
content. Non-MSAA/scaled/unsupported getter cases remain explicitly counted
compatibility boundaries, not silently dropped depth. The final post/UI colour
boundary reuses this lease while retaining its existing scheduling publication.

## Cumulative storage plan

Original checkpoint: 2026-09-05 20:47, 65,462,788,096 B free. The original 2 GiB
peak additional use, 100 MiB diagnostics, 10 MiB aggregate build/test logs and
20 GiB minimum reserve still apply. At 01:05 free was 64,687,411,200 B; prior
closing retained diagnostics were 2,365,356 B runtime, 6,443,904 B perf, 152,707 B
build/test, plus the existing 41 MiB tool/inspection reservation. No new raw
allowance; protected historical archive and both existing PNGs remain.

Reuse configured host/CPU trees and existing guarded wrappers. Plan <=512 MiB
additional build/link overlap plus <=16 MiB diagnostics, with cumulative original
free-space floor and 250 ms polling/early-stop headroom. Run focused source/CPU
checks before the host target. No guest rebuild, tools, downloads or new build
trees. Retire only matching superseded small diagnostics after replacements pass.
Any bounded image inspection must fit the remaining existing 10 MiB PNG
reservation; it is not a raw-sequence exception or full image qualification.

Pre-build CIM found no live renderer/build process. At 01:10 free was
64,690,167,808 B. Focused `host_post_output_test_03`, PID 12332, exit 0; then
`cpu_07`, PID 27216, exit 0, 31/31 in 3.70 s. CPU tests cover implicit transition
references, native-to-adapter and adapter-to-native layout updates, value-only
copy/assignment, detach-before-owner-release, type-erased reader lifetime and
shape/sample/descriptor refusals. The initial old post source guard failed on its
post-specific field-name assertion after generalization; updated it and added
guards for shared layout lifetime, depth resolve completion order and counted
compatibility publication, rather than removing the no-copy requirement.

Host build `reblue_10`, owned PID 25308/session 85396, terminated with exit 0.
Header changes rebuilt host/common objects; final displayed step 90/93 links the
desktop exe. No guest objects rebuilt. Existing CRT warnings remain. Free after
build: 64,688,713,728 B. 36 post + 16 scene source guards pass (52 total).

For a normal-flat sanity inspection, allow the explicit new helper output
`native_scene_depth_lease_window.png`. Existing two PNGs total 6,551,191 B,
leaving at most **3,934,569 B** in the unchanged 10 MiB aggregate reservation.
The helper still encodes in memory and checks both its 4 MiB per-image limit and
the tighter remaining aggregate space before writing; no existing file can be
overwritten. No new raw capture allowance. XR and post-disabled recovery checks
remain capture-disabled; profile restoration and producer shutdown stay in finally.

Lifetime review also checked Khronos's destruction requirements: submitted work
referencing a view/framebuffer must finish before destruction. See
[vkDestroyImageView](https://docs.vulkan.org/refpages/latest/refpages/source/vkDestroyImageView.html)
and [vkDestroyFramebuffer](https://docs.vulkan.org/refpages/latest/refpages/source/vkDestroyFramebuffer.html).
The existing native store/fence drain still owns destruction; CPU handle release
alone is not treated as GPU completion. This source review is not Vulkan validation
of the game run or full-device teardown qualification.

## Current runtime evidence

Binary `reblue_vk.exe`: linked 01:13:38, 47,602,688 B, SHA256
`89adf4b27260665ccd308c3ea41efa8a8337d70d382157051c2005ff680ab418`.
Root base `7c349c8` plus the current dirty renderer changes, local Plume `81bdca8`.

Normal flat/default MSAA: PID 25164, session 78821, 01:14:30--01:15:46;
wrapper exit 0, all five settings audited, zero raw frames, original profile
restored byte-for-byte. `reblue_830.log` 241,295 B; last counts: 3,600 native
depth publications / 0 compatibility depth publications, 3,600 native resolves
and deferred colours / 0 recovered colours. Native post/input/final publication
3,601, imports/original scopes/refusals 0. Native scene pool 3 created / 3,597
reused / 2 retired / 1 resident, 33,177,600 payload B; post pool 6 / 3,594 / 4 / 2,
33,177,600 payload B. Both pools refused/failed 0. Perf
`perf-20260906-011433.csv` 606,208 B, metadata 112 B. Free after run:
64,683,671,552 B. The wrapper ended its owned process at the timeout; this is
not a claimed natural game exit or a Vulkan-validation run.

Inspected `native_scene_depth_lease_window.png`: 1920x1080, 3,373,464 B, SHA256
`3c6ceff9b52d3fc27527ff5350aa4ffd165ed92f5614492ddd4e934e2766f379`.
Character, terrain, foliage, shadows and distant DoF are visible; no obvious
full-frame corruption. The three window PNGs total 9,924,655 B, still within
the existing 10 MiB reservation. This unaligned single frame cannot qualify
animation, stereo, authored events or the full desktop gate. No more images
are planned; XR, post-disabled recovery and non-MSAA runs use no captures.
All four short runs together still fit the planned <=16 MiB diagnostics growth.

Normal XR: PID 8824/session 73793, 01:16:48--01:18:03, wrapper exit 0,
16 settings audited. `reblue_831.log` 537,154 B. Native depth publications,
attachment resolves and deferred colours 9,600 each; compatibility depth and
recovered colours 0. Native post/input/final publication 9,601, imports/original
scopes/refusals 0. Scene pool 3 created / 9,597 reused / 2 retired / 1 resident;
post pool 6 / 9,594 / 4 / 2; each 72,990,720 payload B, refused/failed 0.
Perf `perf-20260906-011650.csv` 1,622,016 B, metadata 112 B. Ending free
64,680,841,216 B. Same 1440x1584/eye, multiview, render-scale-1, zero-height
xrsim setup as the previous normal-XR diagnostic, with all previews off.

Post-disabled flat recovery: PID 25508/session 14348, 01:18:39--01:19:55,
wrapper exit 0, six settings audited. `reblue_832.log` 250,094 B. All 3,600
deferred colours recovered, 3,600 native depth publications / 0 compatibility
depth publications. As explicitly requested by this correctness diagnostic,
native post scopes 0 / original scopes 3,600 / settings refusals 3,600; input,
memory, effect and sequence refusals 0. This tests the remaining legacy post
readers of the borrowed native depth; it is not a normal-path fallback failure.
Perf `perf-20260906-011842.csv` 614,400 B, metadata 112 B. Ending free
64,679,972,864 B. Both XR and recovery runs retained zero raw frames and restored
the original profile bytes; no new pixel/VR qualification is claimed.

## Retention review

The new normal-flat image covers the same bounded window-sanity purpose as
`native_scene_resolve_color_window.png` and `native_post_image_ownership_window.png`:
same 1920x1080/default-MSAA configuration and field, with both earlier ownership
changes still active (native resolves and post images, zero imports/fallbacks).
It was inspected, not merely produced. This qualifies replacement for that
limited purpose only, not a sequence, stereo or failure regression baseline.
Retire those two successful normal-flat PNGs after final producer shutdown;
retain their hashes/findings in the old reports. Keeping one per implementation
milestone would violate retention by verification purpose. Preserve the actual
protected raw/flat/VR baselines and every unresolved failure image/sequence.

Small superseded candidates: stdout/stderr pairs `reblue_09`, `cpu_06` and
`host_post_output_test_02`; runtime log 827 with `perf-20260906-004353` (normal
flat), log 823 with `perf-20260905-233021` (normal XR), log 821 with
`perf-20260905-224540` (post-disabled recovery), and log 829 with
`perf-20260906-005454` (non-MSAA, only after its replacement passes). Each perf
pair means CSV plus metadata. Preserve the optical-XR diagnostic 828: a normal
XR run does not supersede its synthetic effects coverage. Current 10/07/03
build/CPU evidence replaces the listed previous successful build/test logs.

Non-MSAA flat: PID 26396/session 7885, 01:20:27--01:21:42, wrapper exit 0,
six settings audited. `reblue_833.log` 238,553 B. Native post/input/publication
3,601, imports/original scopes/refusals 0. Native depth publications 0 /
compatibility depth publications 3,600; native attachment resolves 0. This proves
the remaining non-MSAA adapter still works, not that its ownership is converted.
Perf `perf-20260906-012029.csv` 602,112 B, metadata 112 B. Ending free
64,679,059,456 B. Zero raws and exact profile restoration verified; final elevated
CIM found no live renderer/compiler/build process. Both older PNG hashes were
checked against their recorded identities before retirement.

Cleanup completed: the 20 exact candidates above (six build/test logs, four
runtime logs, eight perf CSV/metadata files and two superseded PNGs) were checked
as regular files with expected lengths and no reparse ancestors, with resolved
paths confined to this workspace. Removed 11,128,056 logical B. Immediate free
space 64,679,051,264 -> 64,690,192,384 B: **11,141,120 B measured reclaimed**.
No source, game data, profile, build tree or raw/failure baseline was removed.
The old successful logs/PNGs are no longer available verbatim; their reports
remain and equivalent diagnostics can be regenerated. This cleanup is separate
from the previous 2,781,184 B and 888,832 B savings, not counted twice.

The new lease/layout header and expanded CPU tests can be committed independently
of unpublished Plume changes. GPU getter publication, resource/bindless/layout
wiring, scene end integration and updated source guards remain uncommitted with
the earlier renderer work. The post-pool comment was clarified after testing;
it changes no executable code and does not justify another rebuild. No push is
attempted and no parent gitlink is committed. Full native scene attachment
ownership (including non-MSAA/scaled cases), final UI/frame scheduling and the
full desktop checklist remain required; no Quest work.

Closing cumulative ledger: runtime logs 2,419,436 B; perf 6,558,592 B;
build/test logs 152,689 B. No modified runtime caches/dumps or new raws. One
current window PNG remains, 3,373,464 B. With the unchanged conservative 41 MiB
tool/inspection reservation, diagnostics remain under 100 MiB and build/test
logs under 10 MiB. Free space 64,689,799,168 B (~60.25 GiB), **2,387,968 B less
net volume use** than the 01:05 preflight, including this turn's build/runs/image
and completed cleanup. Original cumulative net growth 772,988,928 B remains
within the original 2 GiB cap. Net volume change includes other writers; the
only separately credited cleanup is the measured 11,141,120 B above. Both source
guard suites and `git diff --check` were rerun successfully after documentation
and the comment-only change. All owned producers are terminal.
