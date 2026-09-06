# Native single-sample scene image ownership

2026-09-06; unfinished renderer transition, not full-frame or Quest qualification.

Previous turn made progress: storage-first AGENTS checkpoint `b5a0e08`. This
turn resumes renderer implementation after reading the current worktree, goal
requirements, ownership evidence and guest-source/devloop/vrsim skills. The
earlier GitHub publication denial remains; no push or Plume gitlink commit.

## Change and remaining boundaries

The verified no-MSAA baseline (log 833) still used 3,600 compatibility depth
publications. Move the existing single-sample scene images, sampling views and
descriptor slots out of their surface adapters into a bounded native owner.
No duplicate GPU image, descriptor rewrite or shader copy at ownership transfer.
Source image generations key residency; fence retirement invalidates descriptors
before destroying views/images. Getter leases retain old backing across source
recreation without holding a HostTargetPin across frames. Short-lived completed
scene pins still serialize source writes through post/recovery.

Native post consumes direct single-sample colour/depth, with no getter import
or early colour publication. Finish queued writes/zero-draw clears before reads.
Depth getters borrow native backing; deferred colour still has explicit recovery
for disabled/refused/unconsumed post. Preserve non-MSAA source alpha; native MSAA
resolution's opaque-alpha contract is separate. Initial source creation still
uses the temporary surface allocator; moving allocation and pass construction
fully native, scaled/getter cases, UI/scheduling and full game gates remain work.

## Original cumulative storage budget

Reuse the original checkpoint (2026-09-05 20:47, 65,462,788,096 B free): 2 GiB
peak growth, 100 MiB new retained diagnostics including tools, 10 MiB build/test
logs, 20 GiB reserve. At 01:38 free was 64,687,906,816 B. Prior closing ledger:
runtime 2,419,436 B, perf 6,558,592 B, build/test 152,689 B plus the unchanged
41 MiB tool/inspection reservation. Original net growth is 774,881,280 B.

Reuse configured CPU/host trees and guarded wrappers. Plan <=512 MiB incremental
build/link overlap plus <=16 MiB diagnostics; focused CPU/source checks first.
No guest rebuild, new tools, downloads or build trees. No new raw allowance.
Keep current raw/failure/VR evidence and the existing normal-MSAA window image.
A necessary non-MSAA window sanity image must fit the existing 4 MiB individual /
10 MiB aggregate PNG reservation, with an explicit new name and no overwrite.
Retire only safely verified matching superseded successful diagnostics.

CPU build `host_post_output_test_04`, PID 23272, exit 0; `cpu_08`, PID 3968,
exit 0, 31/31 in 3.32 s. All 53 source guards pass (17 scene, 36 post).
The first host build `reblue_11`, PID 27420/session 66726, exited 1: the new
owner header needed the complete Plume interface include for unique_ptr moves.
Fixed that include and put the header first in its CPU test to cover isolation.
No guest objects rebuilt. Free after failed build: 64,690,176,000 B.

Lifetime review found a source-recreation hazard: the retired binding adapter
could enter SurfacePool even though another getter retained its native image.
Reject native leases at pool return so ordinary destruction releases the adapter
and the native store preserves/fences its backing; add a source regression guard.
Retry stays in the same configured tree and original cumulative budget.

The existing window helper now admits only the explicit non-MSAA filename
`native_single_sample_ownership_window.png` for `-NoMSAA -InspectWindow`; MSAA
filenames cannot be used for that configuration. Per-image/aggregate limits and
no-overwrite checks are unchanged.

## Built binary and runtime evidence

CPU rebuild `host_post_output_test_05`, PID 23664, exit 0. Host retry `reblue_12`,
PID 27460/session 28725, exit 0. Its final displayed step 18/19 links the desktop
exe. Codegen checked its already-up-to-date module (zero written/deleted); no
guest objects rebuilt. Free after build: 64,688,562,176 B. Binary linked 01:46:51,
47,615,488 B, SHA256
`2dc31e37b8b1adbdcf862f3a0eb6c3efa1e482d5e9b99c8234274c3614a1614c`.
Root base `b5a0e08` plus dirty renderer integration, local Plume `81bdca8`.

Flat non-MSAA: PID 22592/session 14044, 01:47:23--01:48:39, wrapper exit 0,
six settings audited, zero raws and exact profile restoration. `reblue_834.log`
241,946 B; 3,600 native depth publications / 0 compatibility publications,
3,600 deferred colours / 0 recoveries; native post 3,601, imports/original scopes/
refusals 0. Completed scene inputs are the actual sources: materialized colour
and depth both 0. Native target store 6 created / 7,194 reused / 4 retired /
2 resident, 33,177,600 payload B; post pool 6 / 3,594 / 4 / 2, same bytes.
Both refused/failed 0. This run includes source extent changes and retirement,
not just fixed-size steady state. Perf `perf-20260906-014726.csv` 606,208 B,
metadata 112 B. Ending free 64,683,581,440 B.

Inspected `native_single_sample_ownership_window.png`: 1920x1080, 3,353,484 B,
SHA256 `4dc8f9eaa06fed65df8d337a3ed47e78c9630754949bad6510f54583edd8a71c`.
Shu, terrain/foliage, shadows and distant DoF are visible without obvious whole-
frame corruption. This unaligned single PNG cannot establish temporal stability,
stereo, authored-event or full-game qualification. Keep the separate prior MSAA
PNG; the two configurations serve different purposes. Total window PNG payload
is 6,726,948 B within the unchanged 10 MiB reservation. No more PNGs planned.
These bounded game diagnostics are not VVL runs; wrappers stop their owned
processes at timeout, not a claimed natural game exit.

Non-MSAA XR: PID 25556/session 49186, 01:48:54--01:50:09, wrapper exit 0,
17 settings audited. Same 1440x1584/eye, multiview, zero-height xrsim and render
scale 1, mirror/previews off. `reblue_835.log` 564,898 B; native depth/deferred
colour 10,200, compatibility depth/recovered colour 0. Native post 10,201,
imports/original scopes/refusals 0. Native target store 6 created / 20,394 reused /
4 retired / 2 resident, 72,990,720 payload B, refused/failed 0; post pool 6 /
10,194 / 4 / 2, same bytes, refused/failed 0. Perf `perf-20260906-014856.csv`
1,757,184 B, metadata 112 B. Zero raws and exact profile restoration. No new
stereo pixel qualification is claimed.

Free after XR was 64,654,913,536 B, more volume growth than its diagnostics
explain. Investigated before the next producer: project cache/HLSL dump, install
root and verification root had no new large outputs; scoped NVIDIA DX/GL caches
had no modified files; D3DSCache had two modified files totaling 65,536 B. Free
remained stable (64,654,909,440 B at recheck). The remaining approximately 25 MiB
is not attributed to a specific writer or claimed as reclaimed: count the entire
measured volume change against the unchanged cumulative budget. No broad scan or
shared-cache deletion. There is still roughly 1 GiB headroom to the producer stop
floor and well over the 20 GiB reserve.

Non-MSAA post-disabled recovery: PID 14860/session 93700, 01:51:20--01:52:35,
wrapper exit 0, seven settings audited. `reblue_836.log` 249,994 B; all 3,600
deferred colours recovered; 3,600 native depth / 0 compatibility publications.
Native post intentionally 0 / original 3,600 / settings refusals 3,600; memory,
effect, input and sequence refusals 0. Native target store 6 / 7,194 / 4 / 2,
33,177,600 payload B, refused/failed 0. This explicitly tests compatibility post
readers, not an unexpected normal-path fallback. Perf `perf-20260906-015122.csv`
618,496 B, metadata 112 B. Zero raws, exact profile restoration, ending free
64,654,020,608 B.

Normal default-MSAA flat regression: PID 20868/session 27003, 01:52:58--01:54:14,
wrapper exit 0, five settings audited. `reblue_837.log` 240,727 B; native depth,
attachment resolves and deferred colours 3,600 each, compatibility depth/recovered
colour 0. Native post 3,601, imports/original scopes/refusals 0. Native resolve
store 3 created / 3,597 reused / 2 retired / 1 resident; post pool 6 / 3,594 / 4 /
2; both 33,177,600 payload B and refused/failed 0. Perf
`perf-20260906-015300.csv` 614,400 B, metadata 112 B. Zero raws, exact profile
restoration, ending free 64,653,156,352 B. No new MSAA image was produced.

Final `cpu_09`, PID 12936, exit 0: 31/31 in 3.34 s. The header is first in its
test translation unit; ownership tests use real native owner/cache types with
CPU interface doubles (no GPU execution). Checks cover preserving exact image,
view and descriptor identities, atomic transfer after cache insertion, invalid
shape/overflow/identity/descriptor/missing-object refusals, byte/entry budgets,
reuse without layout reset, shared adapter/getter state, old getter survival
after source recreation and descriptor/view/image destruction after the correct
fence. Both source suites rerun: 17 scene + 36 post = 53 passing guards. Elevated
CIM found no live renderer/compiler/build/test producer; original profile checked.

## Completed retention review and cumulative ledger

Removed 13 exact superseded successful outputs after replacements passed:
stdout/stderr pairs `reblue_10`, `cpu_07`, `cpu_08`,
`host_post_output_test_03`, `host_post_output_test_04`; non-MSAA runtime log 833
and `perf-20260906-012029.csv` plus metadata. Current build 12, CPU 09, target
build 05 and non-MSAA run 834 replace those limited diagnostic purposes.
Kept the small failed-build 11 log pair and its explained include failure;
kept both different-configuration PNGs, all protected raw/current VR/failure
evidence, and older different-configuration MSAA/optical diagnostics.

All targets were validated as regular files of expected lengths, resolved within
this workspace with no reparse ancestors. Removed 877,878 logical B. Immediate
volume free space 64,652,562,432 -> 64,653,447,168 B: **884,736 B measured reclaimed**.
No source, game data, profile, active build tree or raw/failed-renderer evidence
was removed. Superseded logs are no longer available verbatim; reports remain
and equivalent diagnostics are reproducible. This is separate from all earlier
cleanup measurements, not double-credited.

Closing inventory: 11 runtime logs / 3,478,448 B; 22 perf files / 9,553,104 B;
76 build/test logs / 160,099 B; no modified project cache/dump files or new raw
frames. Two bounded window PNGs total 6,726,948 B. With the unchanged 41 MiB
tool/inspection reservation, conservative cumulative diagnostics are 56,183,267 B
(below 100 MiB); aggregate build/test logs remain below 10 MiB.

Free space 64,653,443,072 B (~60.21 GiB). This turn's measured net volume use rose
34,463,744 B (~32.87 MiB), including new diagnostics/image, build changes, cleanup
and the unattributed volume growth above. Original cumulative net growth is
809,345,024 B, within the original 2 GiB cap. Do not credit unexplained changes
as cleanup. Retained large evidence keeps its prior baseline/failure purpose;
the new no-MSAA PNG is replaceable only by inspected same-purpose evidence.

Only the independent native ownership header, CPU test and active docs/worklog
are intended for a local commit. GPU adapter, scene/fence/pool integration and
source guards remain with the earlier uncommitted renderer work that requires
the unpublished Plume API. Plume is still clean at `81bdca8`; no parent gitlink
commit or push. The next architectural work is native source allocation/pass
construction, not another wrapper advertised as complete frame ownership.
