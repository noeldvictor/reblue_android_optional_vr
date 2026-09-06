# Native scene source allocation

2026-09-06; unfinished desktop renderer transition, no Quest qualification.

Previous turn made progress: `3c2b189` independent target-ownership checkpoint,
host/CPU/source checks, four bounded diagnostics, inspected non-MSAA PNG and safe
cleanup. Current worktree rechecked; guest-source/devloop/vrsim skills read in full.
Root/Plume publishing is still blocked pending explicit owner approval after the
prior denial. No push or parent gitlink commit is attempted.

## Architectural change

Replace the previous adoption step with native source image creation. Scene begin
supplies explicit native FP16 colour / D32S8 depth formats, extent, layers and
sample count. Native source allocation/residency has no surface-pool, guest format,
header, tile or resource-address input. It supports mono/stereo with 1/2/4/8 samples,
checks device capabilities and uses the existing fenced budget. Single-sample post
receives the same native image directly; MSAA remains invalid as an ordinary
sampled-image lease and resolves through native attachments.

Host target slots allocate only the binding header after the image exists. Its
remaining GetDesc ABI response still needs the legacy format encoding; native
allocation does not consume it. Read `render_tweaks.toml`'s scene handoff and the
original scene begin allocation/binding sequence in generated/reblue_recomp.53.cpp,
plus the D3DSurface_GetDesc hook. No generated-code or hook-TOML edits.

Source attachments have a separate retained native handle because MSAA sampling
is not a valid ordinary NativeImageLease. Bindless release, old-pool rejection,
alias prevention and detach-before-owner-release handle both native source and
sampled-output owners. Native resolve framebuffers retain their actual source
owners; retaining a resolved getter therefore cannot outlive its framebuffer's
source backing. These dependencies die after the framebuffer/views, behind fences.
Remaining binding headers, framebuffer selection, getters/scaling, pass traversal,
UI/scheduling and the complete desktop game/pixel gate still require migration.

## Original cumulative storage plan

Original checkpoint remains 2026-09-05 20:47, 65,462,788,096 B free: 2 GiB peak
growth, 100 MiB new retained diagnostics including tools, 10 MiB build/test logs,
20 GiB minimum reserve. Current preflight at 02:00 free 64,652,877,824 B. Previous
ledger: runtime 3,478,448 B, perf 9,553,104 B, build/test 160,099 B, plus unchanged
41 MiB tool/inspection reservation. Two window PNGs total 6,726,948 B. No new raw
allowance; historical raw/current/failure evidence remains protected.

Reuse configured CPU/host trees and guarded wrappers: <=512 MiB additional build/
link overlap and <=16 MiB new diagnostics, within the original cumulative floor.
Focused CPU/source checks precede the host build; no guest rebuild, new toolchain,
downloads or build trees. Any needed window inspection must fit the existing
4 MiB individual / 10 MiB aggregate limit, use an explicit configuration-matched
new filename, and never overwrite. Retire only inspected same-purpose predecessor
images and matching superseded successful small diagnostics after validation.

Focused CPU builds `host_post_output_test_06` (PID 26180) and
`host_post_images_test_07` (PID 22948) both exit 0. `cpu_10` (PID 572), exit 0:
31/31 in 3.36 s. The native target tests check mono/stereo 1/2/4/8 samples,
overflow/invalid recipes, identity reuse without layout reset, separate entry and
byte limits, old-getter lifetime, pending-retirement accounting and sample-lease
rejection of MSAA sources. Resolve CPU doubles check source handles remain alive
through framebuffer destruction; this is dependency lifetime, not GPU validation.
19 scene + 36 post source guards pass (55 total). Pre-build CIM found no active
producer; free before focused builds 64,655,167,488 B, after 64,654,966,784 B.

Host build `reblue_13`, PID 20444/session 13225, exited 0 under the unchanged
300-second / cumulative-space / log / unexpected-guest-rebuild guards. Final
displayed step 91/94 links the desktop executable; no guest objects rebuilt.
Binary linked 02:14:04, 47,619,072 B, SHA256
`a6887682f80f9d20558daf6118632ffc4c9147659f9250b0f1583a215d5b5cd5`.
Root `3c2b189` plus current dirty integration, local Plume `81bdca8`. Free after
build 64,654,151,680 B. No runtime success is yet claimed.

The guarded window helper now admits explicit `native_scene_source_allocation_window.png`
(MSAA) and `native_scene_source_single_window.png` (no MSAA), with unchanged
configuration matching, no-overwrite and byte limits. The first must fit the
remaining 3,758,812 B aggregate PNG space. Only after inspecting and safely
retiring its same-purpose MSAA predecessor may a second new image fit; then
review the corresponding non-MSAA predecessor. Plan five bounded runs: flat
MSAA/non-MSAA with those PNGs, capture-disabled XR MSAA/non-MSAA, and MSAA
post-disabled recovery to exercise sampling the new multisampled source owner.
All five runs plus two PNGs must stay within the <=16 MiB diagnostic plan; no
new raw allowance or sequence/stereo pixel qualification is implied.

## Runtime checks and retention

Normal MSAA flat: PID 26816/session 36928, 02:15:10--02:16:26, wrapper exit 0,
five settings audited, zero raws and exact profile restoration. `reblue_838.log`
243,904 B; native depth / attachment resolve / deferred colour results 3,600
each, compatibility depth/recovered colour 0. Native post 3,601 with imports,
original scopes and refusals all 0. Native source store 6 created / 7,194 reused /
4 retired / 2 resident, 132,710,400 payload B (four samples); resolve store 3 /
3,597 / 2 / 1, 33,177,600 B; post pool 6 / 3,594 / 4 / 2, 33,177,600 B. All
refused/failed 0. Native source generations handle 1920x1080 -> 1280x720 ->
1920x1080 recreation. Perf `perf-20260906-021513.csv` 606,208 B, metadata 112 B;
ending free 64,649,441,280 B. Bounded timeout termination, not natural exit/VVL.

Inspected `native_scene_source_allocation_window.png`: 1920x1080, 3,250,318 B,
SHA256 `1b1cb519c9770854fcd13277095ae03ec7738130b3bb24a3adfabac4b2bdeec5`.
Shu, field geometry/foliage, shadows and distant DoF are visible with no obvious
full-frame corruption. Single unaligned sanity image only, not sequence/event/
stereo qualification. It replaces the earlier normal-MSAA sanity PNG's purpose,
not the protected raw/failure baselines.

Removed `native_scene_depth_lease_window.png` after verifying the replacement,
the old recorded SHA256 and expected 3,373,464 B length, regular-file status,
resolved workspace containment and no reparse ancestors. Free immediately before/
after: 64,649,175,040 -> 64,652,550,144 B; **3,375,104 B measured reclaimed**.
The old PNG is no longer available verbatim; its report/hash remains and an
equivalent diagnostic is reproducible. No protected/raw/game/build data removed.
Remaining PNGs total 6,603,802 B, leaving 3,881,958 B under the same aggregate cap
for the planned no-MSAA inspection. This is completed cleanup, not a proposal.

Normal non-MSAA flat: PID 2236/session 74673, 02:17:17--02:18:33, wrapper exit 0,
six settings audited, zero raws and exact profile restoration. `reblue_839.log`
240,509 B; 3,600 native depth publications/deferred colours, compatibility depth/
recovered colour 0; native post 3,601, imports/original scopes/refusals 0. Native
source store 6 / 7,194 / 4 / 2, 33,177,600 payload B; post pool 6 / 3,594 / 4 / 2,
same bytes; both refused/failed 0. Perf `perf-20260906-021720.csv` 602,112 B,
metadata 112 B; ending free 64,648,015,872 B.

Inspected `native_scene_source_single_window.png`: 1920x1080, 3,232,242 B,
SHA256 `e90d643dca56bb9977fd8af34262ff48198e78c8ff5ac262a0d5f64618f4c5bb`.
Shu, terrain/foliage, shadows and DoF visible without obvious full-frame corruption.
Again only unaligned window sanity, not temporal/stereo/full-game qualification.
It replaces `native_single_sample_ownership_window.png` for the same no-MSAA
sanity purpose. After verifying the old SHA256, expected 3,353,484 B length,
regular-file status, exact workspace path and no reparse ancestors, removed that
old PNG. Immediate free 64,647,753,728 -> 64,651,108,352 B: **3,354,624 B measured
reclaimed**, separate from the earlier MSAA image cleanup. Reports/hashes remain;
the old PNG itself is no longer retained. No protected game/build/raw/failure
data removed. The two new PNGs total 6,482,560 B, below 10 MiB; no more planned.

Normal MSAA XR: PID 19508/session 6440, 02:20:02--02:21:18, wrapper exit 0,
16 settings audited. 1440x1584/eye, multiview, render scale 1, zero-height xrsim,
mirror and synthetic previews off. `reblue_840.log` 547,156 B; native post 9,601,
imports/original scopes/refusals 0. Zero raw captures and exact profile restoration;
ending free 64,648,404,992 B. This is no new stereo pixel qualification.
Final native depth / resolve / deferred colour counts are 9,600 each, compatibility
depth/recovered colour 0. Native source store 6 / 19,194 / 4 / 2, 291,962,880 B;
resolve store 3 / 9,597 / 2 / 1, 72,990,720 B; post pool 6 / 9,594 / 4 / 2, same
bytes; all refused/failed 0. Perf `perf-20260906-022004.csv` 1,622,016 B plus
112 B metadata.

Normal non-MSAA XR: PID 17128/session 68876, 02:21:36--02:22:51, wrapper exit 0,
17 settings audited; same stereo setup with only MSAA disabled. `reblue_841.log`
565,457 B; native depth/deferred colours 10,200, compatibility depth/recovery 0.
Native post 10,201 with imports/original scopes/refusals 0. Source store 6 /
20,394 / 4 / 2 and post pool 6 / 10,194 / 4 / 2, each 72,990,720 B; refused/
failed 0. Perf `perf-20260906-022138.csv` 1,753,088 B, metadata 112 B. Zero raws,
exact profile restoration, ending free 64,645,951,488 B. No stereo pixel claim.

MSAA post-disabled recovery: PID 3804/session 18543, 02:23:30--02:24:46, wrapper
exit 0, six settings audited. `reblue_842.log` 251,950 B; all 3,600 deferred
colours recovered, 3,600 native depth / 0 compatibility publications. Native post
intentionally 0 / original scopes 3,600 / settings refusals 3,600; other refusal
categories 0. Source store 6 / 7,194 / 4 / 2, 132,710,400 B; resolve store 3 /
3,597 / 2 / 1, 33,177,600 B; refused/failed 0. This tests the remaining recovery
shader's sampling of the new native multisampled source owner. Perf
`perf-20260906-022333.csv` 614,400 B, metadata 112 B. Zero raws, exact profile
restoration, ending free 64,644,583,424 B. All five runs use bounded owned-process
termination; none claims natural game exit, game VVL or full-frame qualification.

## Completed small-output cleanup

After elevated CIM confirmed no live renderer/build/test producer, removed 28
exact superseded diagnostics, validating expected lengths, regular files, resolved
workspace containment and no reparse ancestors:

- stdout/stderr pairs `reblue_11`, `reblue_12`, `cpu_09`,
  `host_post_output_test_05`, `host_post_images_test_06`. The explained include
  failure in build 11 is resolved, not unresolved renderer evidence; current
  build 13 and CPU/source tests replace these purposes.
- Runtime logs 830, 831, 832, 834, 835 and 837, replaced by matching normal-MSAA
  flat/XR, MSAA-recovery, and non-MSAA flat/XR runs 838--842. Two older default-
  MSAA flat diagnostics served the same purpose; both are superseded now.
- Their perf CSV/metadata pairs: `20260906-011433`, `011650`, `011842`,
  `014726`, `014856`, `015300` (all the same date prefix).

Removed 7,938,087 logical B. Immediate volume free space 64,644,579,328 ->
64,652,533,760 B: **7,954,432 B measured reclaimed**. Together with the two
separately measured PNG removals above, this turn removed 30 files / 14,665,035
logical B and reclaimed **14,684,160 B measured** (~14.00 MiB). Do not double-
credit earlier turns. Reports remain; old diagnostics/PNGs are no longer available
verbatim, but equivalent bounded checks are reproducible. Kept distinct non-MSAA
recovery log 836/perf and optical XR 828/perf, all current/tiny GPU fixture evidence,
protected raw/failure baselines, game data, profiles, source and active build trees.

Closing cumulative inventory: 10 runtime logs / 3,251,310 B, 20 perf files /
8,930,400 B, 74 build/test logs / 152,759 B; no modified project cache/dump files
or new raws. Two current flat PNGs total 6,482,560 B. The unchanged 41 MiB tool/
inspection reservation gives a conservative diagnostic total of 55,326,085 B,
below 100 MiB; logs remain below 10 MiB. Free 64,652,533,760 B (~60.21 GiB),
net volume growth only 344,064 B relative to this turn's 02:00 preflight, after
builds, all five runs, image replacement and cleanup. Original cumulative net
growth 810,254,336 B remains within the original 2 GiB cap. Volume changes include
other writers; only the three immediate measurements above are credited cleanup.

Local checkpoint scope: updated native recipe/ownership header, independent
post-output CPU tests, README, transition document and this evidence. The actual
native image factory, header binding bridge, MSAA source-retention wiring and
source guards remain with the earlier uncommitted renderer integration that needs
unpublished Plume changes. No parent gitlink or push. Next architectural work:
native framebuffer/pass construction instead of compatibility framebuffer caches,
then the remaining complete scene/frame/UI ownership and desktop game gates.
