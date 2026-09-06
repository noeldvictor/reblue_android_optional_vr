# Native post-image ownership and boundary publication

2026-09-06; native pool implemented and diagnostic checks completed locally,
not sequence/stereo/full-frame qualification.

Previous goal turn: progress, native post contracts/wiring, host build, 31 CPU
tests, 48 guards and bounded XR/flat diagnostics. Root `e230e57` commits the
independent API/test; renderer integration and Plume publication remain pending.
This continuation does not grant the requested repository-publication approval.

## Evidence and intended change

Remove `PostColor`/`PostColorAlternate` resource-header allocation from actual
post scheduling, using native images/views/framebuffers and bounded residency.
The existing final getter will borrow a native image handle: no added copy and
no FP16-to-RGBA8 clamp. Preserve its existing identity for remaining UI consumers.

Inspected `resources.h`, `bindless.cpp`, `graveyard.cpp`, `resolve.cpp`,
`draw_framebuffer.cpp`, `draw_bindings.cpp`, `constant_buffers.cpp`, `present.cpp`,
native texture adoption, scene resolve ownership/cache, frame retirement, native
post scheduling and Plume barriers/framebuffers. Devloop/guest-source read fully.
The UI's tile-alias path already refuses host-owned post targets; retain that
refusal for native borrowed outputs. A native post image must not become a tile.
The backend tracks the actual old image layout; a reused native write lease
invalidates only its client-side layout cache to force a barrier after boundary
readers. Published handles prevent write-lease reuse while readers remain.

Descriptor release must distinguish borrowed native slots. Replacing getter
backing must park old framebuffers/views/images behind a fence; the getter's
resource header and declared format remain a compatibility boundary, not a new
post allocation. Native images are single-sample FP16, mono/two-eye, with a
256 MiB payload / 64-entry residency cap, counting pending retirements.

## Cumulative storage preflight

Original checkpoint starts 2026-09-05 20:47 at 65,462,788,096 B free; its 2 GiB
peak additional space, 100 MiB diagnostics and 10 MiB build/test log caps continue
unchanged. At 00:14 free was 64,712,122,368 B (~60.27 GiB), cumulative net volume
growth 750,665,728 B. Plan <=512 MiB additional overlap for host-only incremental
compilation/linking and small CPU checks in the existing trees. No new raw
captures, tools, downloads or build trees. Reuse the existing wrappers and their
original cumulative free-space floors, owned-process timeouts and cleanup.
Current diagnostic totals/retention are recorded in the 23:51 worklog; retries and
this continuation add to that ledger, not a new allowance. Preserve protected
raw/PNG evidence and inspect live processes before builds.

## Verification and remaining work

Implemented the native pool, removed both post target classes, moved final
publication to a borrowed native getter backing, and added native descriptor
ownership plus post-fence framebuffer retirement. A getter retains its image
even if a later scene publication temporarily links another source; direct
legacy copies can still target that retained backing. No native writer reuses
an image while the getter holds its handle. Tile aliasing continues to refuse
native output images, matching the previous host-owned-post-target restriction.

35 post + 15 scene source guards pass. CPU build `host_post_output_test_02`,
PID 26456, exit 0; `cpu_06`, PID 26124, exit 0, 31/31 tests in 3.33 s. Tests add
read/write lease exclusion, reuse, density-map key distinction, payload/entry
budget behavior, pending retirements, wrong-fence retention, cancellation,
single/two-eye metadata, null creation and descriptor->framebuffer->view->image
destruction ordering with CPU doubles. No GPU allocation in that test.

Host build `reblue_09`, PID 22744, session 66431, terminal exit 0. Host header
changes rebuilt host/common objects and linked the desktop executable (90/93
displayed final step); no guest objects rebuilt. Existing CRT warnings remain;
no native post compile error. Free after build 64,711,884,800 B. Exact hash and
runtime results follow after verification.

For this final-image ownership change, counters alone are insufficient. Plan one
bounded normal-flat window PNG, not a raw sequence: reuse the existing PrintWindow
helper with the explicit new name `native_post_image_ownership_window.png`.
Its PNG is encoded in memory and rejected before writing if >4 MiB or if all
retained `native*_window.png` images would exceed the existing 10 MiB aggregate
reservation. The protected older PNG is 3,196,857 B, so the planned incoming cap
fits. The original cumulative free-space floor is also checked before writing.
No existing image can be overwritten. This inspection is a sanity check, not a
replacement flat/VR sequence or an archive-budget exception.

The existing 75 s wrapper still disables automatic captures and restores the
owner's profile in finally; XR/normal non-MSAA diagnostics use no images. The
vrsim skill and both existing wrapper scripts were read before use. All runtime
attempts and the PNG count against the original checkpoint budgets.

Final UI/frame ownership, scene depth publication, other guest rendering and
all desktop game/pixel gates remain open; no Quest run.

## Resumed verification (00:52)

The intervening instruction-only turn committed `AGENTS.md` as `721e3a8`, without
rebuilding or running the renderer. Previous goal work made concrete progress;
the ownership verification below resumes that work, not a new storage budget.
All three repository skills were reread. No renderer/compiler/build process was
live at the elevated CIM preflight. The five-key owner profile is restored.

Free space is 64,688,381,952 B, 774,406,144 B below the original 20:47 baseline.
Retained checkpoint runtime logs total 2,410,303 B; perf outputs 6,599,552 B;
build/test logs 162,298 B. Reserve the existing 41 MiB diagnostic/tool/inspection
allowance, with no new images/raws/downloads or rebuild. Two 75-second
capture-disabled runs reuse the 00:40 binary: optical XR and normal non-MSAA
flat. Plan <=16 MiB combined additional diagnostics/temporary growth, guarded by
the existing cumulative wrapper limits. Replace only matching superseded small
runtime/build diagnostics after verification; preserve protected raw evidence
and both window images. The volume includes unrelated writers; do not credit
its fluctuations as cleanup savings.

The existing 00:44:51 normal-flat PNG (1920x1080, 3,354,334 B) was inspected:
character, terrain, foliage and shadows are visible, with distant depth-of-field
blur and no obvious full-frame corruption. This is an unaligned single-frame
sanity check, not animation, stereo, authored-effects or full-game qualification.

## Exact binary and runtime evidence

The tested `out/build/win-amd64-release/reblue_vk.exe` was linked at 00:40:53,
47,598,080 B, SHA256
`369677f528799822dd76d270019763c7a3cf01fd498b476cfd63416788e54f87`.
It contains root `e230e57` plus the dirty renderer integration and local Plume
`81bdca8`. The later `721e3a8` changes only instructions, not this executable.
Build `reblue_09` records CMake regeneration and host compilation; no guest
object compilation. Its retained stdout/stderr are 29,551 / 36 B. CPU output
build `02` is 140 / 0 B; all-CPU `06` is 3,626 / 0 B. The 35 post + 15 scene
source guards and `git diff --check` were rerun successfully during resumption.

All three bounded runs use the same binary and existing game data; the wrapper
terminates its owned renderer at the timeout, not a claimed natural game exit.
Every wrapper finished with exit 0 and verified zero raw frames. Profile bytes
were restored in each finally path and the original five-key profile was
inspected again after the last run. No producer remained at the final CIM check.

- Normal flat/default MSAA: PID 22800, session 74789, 00:43:50--00:45:07;
  `reblue_827.log` 239,806 B, all five settings audited. Last native post scope,
  completed input and final publication counts are 3,601; scene completions and
  native attachment resolves 3,600, initial colour deferred 3,600/recovered 0.
  Pool: 6 created / 3,594 reused / 4 retired / 2 resident, 33,177,600 payload B,
  refused/failed 0. Perf `perf-20260906-004353.csv` 614,400 B, metadata 112 B.
  Ending free 64,706,244,608 B.
- Optical XR: PID 24796, session 25731, 00:53:09--00:54:25;
  `reblue_828.log` 444,022 B, all 19 settings audited. Existing xrsim uses
  1440x1584 per eye, multiview, render scale 1, zero diorama/head height and no
  mirror; synthetic flare/heat/grain are explicitly enabled. Camera log shows
  game `(20.1,151.4,23.1)` versus eye `(16.9,151.4,23.1)`, confirming active
  tracked view input. Last native post/input/publication count 7,501; flare/heat/
  grain 7,501 each, 112,515 instanced flare sprites. Scene attachment resolves
  and deferred colours 7,500, recovered 0. Last pool report (7,200 leases):
  6 created / 7,194 reused / 4 retired / 2 resident, 72,990,720 payload B,
  refused/failed 0. Perf `perf-20260906-005312.csv` 1,277,952 B, metadata 112 B.
  Ending free 64,682,106,880 B. Synthetic activity is not authored-effect proof.
- Non-MSAA normal flat: PID 21864, session 59957, 00:54:52--00:56:09;
  `reblue_829.log` 236,308 B, all six settings audited, including `bd_msaa=0`.
  Last native post/input/publication count 3,601; scene completions 3,600,
  materialized colour 1/depth 0, native attachment resolves/deferred colours 0.
  Pool: 6 created / 3,594 reused / 4 retired / 2 resident, 33,177,600 payload B,
  refused/failed 0. Perf `perf-20260906-005454.csv` 606,208 B, metadata 112 B.
  Ending free 64,681,181,184 B.

Each run reports zero scene image imports, original post/container/sequence
scopes and post input/sequence refusals. Only one post root is exercised. The
pool's bytes are logical GPU payload, not disk accounting or physical driver
allocation measurements. These game runs did not request Vulkan validation;
zero wrapper error matches do not qualify API/synchronization correctness.

The inspected PNG is
`out/verification/native_post_image_ownership_window.png`, SHA256
`5da0eb6b2854f41f7d1c6c4d07e4900fe52aa36437df467b9f68436d087b5fa8`.
It and the prior `native_scene_resolve_color_window.png` total 6,551,191 B,
within the existing 10 MiB image reservation. Retain both distinct ownership
milestone sanity images until a qualified replacement covers their purpose.
Historical raw sets remain protected/over budget; no new raw allowance exists.

## Checkpoint scope and retention

Checkpoint the independent `native_post_images.h` pool and expanded CPU output
test, plus active documentation/evidence. These use already-existing Plume
texture/view/framebuffer interfaces, not the unpublished resolve API. GPU
creation, borrowed getter publication, post/scene renderer wiring and their
source guards remain dirty; do not imply that this independent commit contains
the whole tested integration. Plume remains clean at `81bdca8`, three commits
ahead of its last verified remote. Root's recorded gitlink is still `eb7b03c`.
No push is retried without the previously requested publication approval.

Matching superseded cleanup candidates are the `reblue_08`, `cpu_05` and
`host_post_output_test_01` stdout/stderr pairs, plus runtime logs 825/826 and
their `perf-20260906-000022` / `perf-20260906-000310` CSV/metadata pairs. Current
09/06/02 build/test evidence and 828/829 optical-XR/non-MSAA runs replace those
specific purposes. Keep the older normal XR and post-disabled recovery evidence:
optical XR and normal post are not matching replacements. Keep the earlier
dated reports and all protected raw/failure/GPU-validation evidence. Exact old
historical logs cannot be recreated verbatim; new equivalent diagnostics can.

Cleanup completed after confirming all producers terminal. All 12 candidates
above were checked as exact regular files with expected lengths, no reparse
ancestors and resolved paths within this workspace, then removed without any
recursive directory operation. Removed 2,774,900 logical B; immediate free space
64,682,262,528 -> 64,685,043,712 B, **2,781,184 B measured reclaimed**. This is
separate from the prior turn's 888,832 B cleanup, not counted twice. A later
1,081,344 B free-space rise before deletion was not attributed to this cleanup.
Current validation/failure evidence and game/build/source data were untouched.

Closing retained cumulative diagnostic inventory: runtime logs 2,365,356 B,
perf outputs 6,443,904 B, build/test logs 152,707 B. No checkpoint-modified runtime
cache/dump files and no new raw frames were found. Adding the conservative
41 MiB tool/inspection reservation keeps the total below 100 MiB; build/test
logs remain below 10 MiB. Both PNGs are included in that existing reservation,
not an extra allowance. Earlier raw-archive protection and no-growth gate remain.

Closing free space 64,685,043,712 B (~60.24 GiB). Net volume use increased
27,078,656 B since the 00:14 ownership preflight, including its build/first PNG;
the resumed 00:52 interval increased it 3,338,240 B. Original cumulative net
growth is 777,744,384 B, still within the original 2 GiB checkpoint cap with
the original wrappers' early-stop headroom. Volume net includes other writers
and is not an exact attribution to these outputs. No new build, image, download,
capture or Quest run occurred during resumed verification.

Next renderer work remains removal of the initial depth/getter publication and
remaining UI/frame ownership boundaries, followed by the rest of the full
desktop checklist. This checkpoint does not qualify a fully native frame.
