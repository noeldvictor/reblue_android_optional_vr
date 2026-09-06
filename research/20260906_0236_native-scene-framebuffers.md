# Native single-sample scene framebuffers

2026-09-06; desktop renderer transition, not completed frame or Quest qualification.

Previous goal work made progress: native source allocation, verified bounded runs
and cleanup; the subsequent instruction-only turn committed storage limits as
`a3617cd` without builds/captures. Current status and source rechecked. Finished
the already reviewed independent source-recipe/test checkpoint as `21d1c99`.
The Plume-dependent integration stays uncommitted; prior publication denial still
requires explicit owner approval. No push or parent gitlink commit is attempted.

Read guest-source/devloop/vrsim skills in full; current AGENTS desktop/storage
rules supersede their historical recipes. Inspected the scene begin translation,
scene-post hook map, native owners, framebuffer binding, foveation and fence drain.

## Change

Single-sample scene begin now constructs its framebuffer from retained native
colour/depth handles before entering the pass. A bounded native store owns the
framebuffer, independently of GuestTexture's framebuffer map. Native mono/stereo
recipes validate source formats, matching extent/layers and single-sample inputs;
no copies, guest-format conversion, surface-pool lookup or resolve attachments.
The existing native MSAA framebuffer/resolve path remains unchanged.

Cache identity uses retained source owners plus the device-lifetime density-map
identity, never extent alone or a resource header. Weak lookup keys do not keep
images alive. Reuse cancels pending retirement; entry limits count pending work.
Framebuffers retire only after a proven fence and retain source images until after
framebuffer destruction. Their store drains before the source image store. Source
payload bytes remain accounted by that image store; the framebuffer store bounds
object count and host metadata, not driver-private allocation sizes.

The binding bridge selects an exact already-owned native framebuffer for either
sample mode before considering the compatibility cache. Guest scene traversal,
draw-state binding/clear adapters, remaining getters/scaling, frame/UI scheduling
and full desktop game/pixel qualification remain open. No Quest work.

## Cumulative storage plan

Original checkpoint remains 2026-09-05 20:47 with 65,462,788,096 B free, 2 GiB
peak additional growth, 100 MiB retained diagnostics including tools, 10 MiB
aggregate build/test logs and 20 GiB free reserve. This turn's 02:36 measurement:
64,654,118,912 B free. No raw-capture allowance; protected historical/failure sets
remain unchanged. Latest prior retained diagnostics total 55,326,085 B, including
the unchanged 41 MiB tool/inspection reservation; two current flat PNGs 6,482,560 B.

Reuse configured CPU/host build trees and existing guarded wrappers. Plan <=512
MiB incremental build/link overlap and <=12 MiB diagnostics, including at most
one <=4 MiB non-MSAA window image within the existing 10 MiB PNG reservation.
No downloads, new build trees, guest rebuild or raw capture. Inspect a same-purpose
replacement before deleting its predecessor; superseded small diagnostics may be
removed only after equivalent verification and exact target/producer checks.
All retries share the original cumulative floor and enforced producer limits.

## Verification

21 scene + 36 post source guards pass (57 total); text/diff checks pass. Native
framebuffer CPU coverage is added to the existing post-output target.
Focused build `host_post_output_test_07`, PID 22520, exited 0 (two steps).
`cpu_11`, PID 25448, exited 0: 31/31 in 3.34 s. These exercise real native cache/
recipe/ownership types with CPU interface doubles, not actual GPU submission.
Free after CPU tests 64,654,024,704 B. No active producer was found by elevated
CIM before these builds. Host build `reblue_14`, PID 21524/session 87162, exited 0
under the existing cumulative guards. Final displayed step 87/90 links the
renderer; no guest objects rebuilt. Existing CRT warnings and the expected new-
file CMake glob recheck; stdout 29,248 B / stderr 36 B. Free after build
64,653,373,440 B. Binary 47,631,360 B, linked 02:43:01, SHA256
`7ded13d8a04fd7011bae269b5dc80d6ec0cef8e05eb9ffb9e1a6f582381aa0a3`.
Root `21d1c99` plus dirty renderer integration, Plume `81bdca8`.

The guarded window wrappers admit one new configuration-matched name,
`native_scene_framebuffer_single_window.png`. Existing no-overwrite, 4 MiB per
image / 10 MiB aggregate PNG and zero raw allowances are unchanged. Plan normal
non-MSAA flat with that inspection, non-MSAA XR, non-MSAA post-disabled recovery,
and default-MSAA flat regression. Four runs and the PNG share the <=12 MiB
diagnostic plan; they cannot establish sequence/stereo/full-game correctness.

## Runtime and image check

Normal non-MSAA flat: PID 26924/session 16170, 02:43:49--02:45:05; wrapper exit 0,
six effective settings, zero new raws, exact profile restoration. Log 843:
243,214 B; native framebuffer store 3 created / 3,597 reused / 2 retired / 1
resident, refused/failed 0. Source store 6 / 7,194 / 4 / 2, 33,177,600 payload B;
post pool 6 / 3,594 / 4 / 2, same bytes, both refused/failed 0. Native depth /
deferred colour 3,600, compatibility depth/recovered colour 0. Native post 3,601,
imports/original scopes/refusals 0. Perf `perf-20260906-024351.csv` 610,304 B,
metadata 112 B; ending free 64,648,011,776 B. Owned timeout, not natural exit/VVL.

Inspected `native_scene_framebuffer_single_window.png`: 1920x1080, 3,356,023 B,
SHA256 `40a9d4d35b3e13711bf4086be80eeac71459c25de8b2552fbbeeee5d7865907d`.
Shu, terrain/rock/foliage, wood structure, shadows and distant DoF visible with no
obvious full-frame corruption. One unaligned flat image is not temporal/stereo/
event/full-game qualification. Current binary above, owned PID at 02:44:49.

After checking old/new hashes, expected old length, exact resolved workspace path,
regular-file status/no reparse ancestors, and elevated CIM confirming no producer,
removed the superseded `native_scene_source_single_window.png` (3,232,242 logical B).
Immediate free 64,647,962,624 -> 64,651,198,464 B: **3,235,840 B measured reclaimed**.
Old hash/report remain, old PNG is no longer available verbatim; an equivalent
diagnostic is reproducible. No raw/failure/build/game/save/profile/source data
removed. Two retained window PNGs now total 6,606,341 B within the unchanged cap.

The independent framebuffer header/test only uses the recorded Plume gitlink's
existing framebuffer fields (checked against `eb7b03c`). Replaced one test-only
assertion referencing unpublished resolve fields with existing view-field checks;
the host binary is unaffected. Focused rebuild `host_post_output_test_08`, PID
25308, exited 0. Final `cpu_12`, PID 4752, exited 0: 31/31 in 3.45 s. This CPU
work overlapped the correctness-only recovery run; no timing comparison is claimed.

Normal non-MSAA XR: PID 19448/session 37310, 02:45:45--02:47:02, wrapper exit 0,
17 effective settings, 1440x1584/eye, multiview, zero-height xrsim, render scale 1,
no mirrors/previews. Log 844: 578,743 B. Last framebuffer report at 10,200 requests:
3 / 10,197 / 2 / 1, refused/failed 0; last source report at 10,500 scenes:
6 / 20,994 / 4 / 2, 72,990,720 payload B, refused/failed 0. Native depth/deferred
colour 10,500, compatibility depth/recovered colour 0; post 10,501, imports/original
scopes/refusals 0. Perf `perf-20260906-024547.csv` 1,777,664 B, metadata 112 B.
Zero raws, exact profile restoration, free 64,647,733,248 B. No new stereo pixels.

Non-MSAA post-disabled recovery: PID 25008/session 22802, 02:47:24--02:48:40,
wrapper exit 0, seven effective settings. Log 845: 251,606 B; 3,600 deferred and
recovered colours, 3,600 native / 0 compatibility depth publications. Post remains
intentionally original: 3,600 settings refusals, other refusal categories 0. Native
framebuffers 3 / 3,597 / 2 / 1 and sources 6 / 7,194 / 4 / 2, 33,177,600 payload
B; both refused/failed 0. Perf `perf-20260906-024727.csv` 618,496 B, metadata 112 B.
Zero raws, exact profile restoration, ending free 64,628,318,208 B. All runs are
owned timeout termination, not natural exit or game Vulkan validation.

Investigated the intervening roughly 19 MiB volume decrease before another run:
scoped native-test/install/cache/dump inventories show only the small rebuilt
CPU object/PDB/exe and logs/perf/profile writes, no new game caches, dumps or raws.
Those files do not explain the full change. Unattributed volume growth remains
charged to the original checkpoint, not credited away or treated as a new budget.
The unchanged floor still allows the final <=1 MiB default-MSAA regression run.

Normal default-MSAA flat regression: PID 23700/session 71074, 02:49:26--02:50:42,
wrapper exit 0, five effective settings. Log 846: 243,329 B. Native resolve /
deferred colour / native depth counts 3,600 each, recovered colour/compatibility
depth 0. Post 3,601, imports/original scopes/refusals 0. Native source store
6 / 7,194 / 4 / 2, 132,710,400 payload B; resolve store 3 / 3,597 / 2 / 1 and
post pool 6 / 3,594 / 4 / 2, each 33,177,600 B. Refused/failed 0 throughout.
Perf `perf-20260906-024928.csv` 610,304 B, metadata 112 B. Zero raws, exact
profile restoration, ending free 64,627,462,144 B. No additional PNG or XR/MSAA
pixels; all four runtimes are bounded diagnostics, not full-game qualification.

## Completed cleanup and closing ledger

With all four runtimes and both final CPU jobs terminal, elevated CIM confirmed
no live renderer/build/test producer. Validated exact workspace-contained paths,
regular files, no reparse ancestors, expected lengths and Git ignored status
before removing 22 superseded successful small diagnostics:

- stdout/stderr pairs `reblue_13`, `cpu_10`, `cpu_11`,
  `host_post_output_test_06`, `host_post_output_test_07`;
- runtime logs 836, 838, 839, 841, replaced respectively by non-MSAA recovery,
  normal MSAA flat, normal non-MSAA flat and normal non-MSAA XR checks above;
- perf CSV/metadata pairs `20260906-015122`, `021513`, `021720`, `022138`.

Removed 4,917,387 logical B; immediate free 64,627,462,144 -> 64,632,393,728 B:
**4,931,584 B measured reclaimed**. Together with the separately measured PNG
removal, this turn removed 23 files / 8,149,629 logical B and reclaimed
**8,167,424 B measured** (~7.79 MiB). Old files are no longer available verbatim;
their reports/hashes remain and equivalent diagnostics are reproducible. Kept
distinct MSAA-XR/recovery and optical evidence, current CPU/tiny GPU fixture
results, all protected raw/failure baselines, saves/profiles/game/source/build data.
Do not double-credit any earlier checkpoint's cleanup.

Closing cumulative inventory: 10 runtime logs / 3,268,338 B, 20 perf files /
8,967,264 B, 74 build/test logs / 152,404 B; zero modified game caches/dumps and
zero new raws. The two retained window PNGs total 6,606,341 B. With the unchanged
41 MiB tool/inspection reservation, conservative diagnostics total 55,379,622 B,
below 100 MiB; build/test logs remain below 10 MiB. Free 64,632,393,728 B
(~60.19 GiB); net volume growth 21,725,184 B (~20.72 MiB) from the 02:36 reading,
including the unattributed change discussed above. Original cumulative growth
830,394,368 B remains below 2 GiB. Only immediate deletion measurements count as
cleanup; other volume changes cannot be attributed solely to this work.

Checkpoint scope: independent native framebuffer header, post-output CPU tests,
README, transition document and this evidence. GPU factory/binding/fence wiring
and source guards remain with the earlier uncommitted Plume-dependent renderer
integration; no dependency gitlink is committed or published. Next: native scene
pass command/binding/clear execution, then remaining scene/UI/frame ownership and
full desktop fields/battles/cutscenes/menus/transitions/reloads/both-eye gates.
