# Native scene state boundary

2026-09-06. Desktop host-renderer conversion is incomplete; no Quest qualification.
Previous turn made progress with command contracts/tests (`ee57134`), completed
runtime evidence and verified cleanup. Rechecked the current dirty renderer tree,
AGENTS, guest-source/devloop/vrsim skills, transition scope and preceding evidence.
Publication still requires owner approval; no dependency push or gitlink commit.

## Cumulative storage plan

Continue the original 2026-09-05 20:47 checkpoint, starting free 65,462,788,096 B:
2 GiB peak growth, 100 MiB retained diagnostics including tools, 10 MiB aggregate
build/test logs and 20 GiB reserve. Previous closing inventory: 55,313,083 B of
diagnostics including the unchanged 41 MiB tool/inspection reservation. Current
03:32 free 64,619,159,552 B; no renderer/cmake/ninja process is live. Previous
volume deltas remain charged; this is not a fresh per-turn allowance.

Reuse the configured host/CPU trees and guarded wrappers. Plan at most 512 MiB
additional build/link overlap and 12 MiB diagnostics for callback identification,
focused tests and native-state verification. All runs have explicit 75 s timeout,
capture-off profile overrides, bounded log/cache/free-space monitoring and exact
profile restoration. One bounded flat PNG replacement may use the unchanged
4 MiB individual / 10 MiB aggregate reservation; inspect before retiring its
same-purpose predecessor. No raw captures, downloads, guest rebuild or new trees.
Retire only exact, superseded agent diagnostics after equivalent verification;
retain current raw baselines and unresolved-failure evidence.

## Identify the actual state before changing execution

The existing first-use remaining-state log now includes its checked device-table
callback address (one line per existing bounded state slot, not per draw). It
reads no new game assets, changes no state, and adds no dump/output directory.
This distinguishes the actual callback at device + 56 + 308 from an unsupported
guess based on the numeric offset. The scene's two calls remain in this diagnostic
build. Source inputs: bdSetRenderState in generated/reblue_recomp.24.cpp, scene
clear bracket in reblue_recomp.53.cpp, native raster hook/bridge, and the scene-post
hook map in config/hooks/render_tweaks.toml. Generated source stays unchanged.

Diagnostic build `reblue_16`, PID 23648/session 55925, exited 0 (12/15 final
displayed link, no guest objects rebuilt, codegen module up to date). Binary
47,639,040 B, linked 03:34:51, SHA256
`92f90218e53b543eb99ee6ade75415224df4828040372627773b40de7a3bbda8`.
The wrapper's new trace-only mode stops after observing the callback, with a 30 s
maximum and unchanged capture/storage/profile guards. PID 24864 ran only
03:36:03--03:36:06, exit 0; log 852 is 16,160 B, all five settings effective,
zero raw frames and exact profile restoration. Perf CSV 033605 is empty, metadata
112 B. Free 64,622,190,592 B. This is callback evidence, not a field qualification.

Actual state 308 callback: **0x82472540**, `D3DDevice_SetRenderState_HighPrecisionBlendEnable`.
The same bounded log identifies 328/332 as primitive-reset enable/index; those
other remaining producers are not changed here. A presumed MSAA implementation
would have targeted the wrong state.

## Remove the console precision toggle from native scene execution

Read the full setter in generated/reblue_recomp.28.cpp and getter
`sub_824725E8` in reblue_recomp.51.cpp. The setter always stores the requested value
at device +11756 when dispatched. For certain console colour-format nibbles it
also changes the bound surface's +28 word, device packet +10244 and dirty bit 37
at +24. The getter reads +11756. `bdSetRenderState` separately compares/publishes
the engine cache at 0x82DBE2DC. The device hook copies exact callback identities
from the XEX state table; this is not an inferred PC D3D enum.

Native scene allocation already specifies R16G16B16A16_FLOAT for mono/stereo and
every supported MSAA count. `pass_bindings.cpp` supplies that explicit format to
the pipeline, and `pipeline_cache.cpp` builds the native descriptor from it.
Native clears operate on those images, not on an Xbox storage mode. The native
resource allocator zeroes its temporary header, whose description hook uses host
fields; console format rewriting is not a native storage requirement.

Removed both scene `SetState(..., 308, ...)` calls and the off/on bracket. The
boundary now publishes only final requested/cached getter values of 1, once per
scene, via a tiny explicitly named import adapter. Device/cache ranges and exact
callback identity are checked before attachment allocation or scene publication.
No surface-format, GPU packet or dirty-mask writes are copied into that adapter.
There is no new GPU allocation, shader change or native precision toggle. Other
state producers/engine clients remain; the two getter words are still counted
compatibility data, not presented as fully native frame ownership.

The actual getter adapter's CPU test covers zero/one/noncanonical/all-bit initial
values, repeated publication, exact addresses/write order and preservation of all
other words in a 0x5000-byte device image. Source guards require native FP16
allocation, preflight before effects, no scene state-308 dispatch and only the two
getter writes. All 24 scene +36 post guards pass (60 total).
Focused existing `host_scene_pass_test_01`, PID 15276, exited 0 in two steps.
`cpu_14`, PID 8748, exited 0: 31/31 in 3.63 s, free 64,620,949,504 B.
No new test tree/target or generated source was needed.

For final runtime verification, plan normal MSAA flat/XR and non-MSAA flat with
the same resulting binary, plus one normal-flat PNG within the existing
reservation. The explicit new image name is admitted by both guarded wrappers;
no overwrite or broader capture permission was added. These checks do not
replace full sequences, both-eye inspection, authored event or full-game gates.

## Final desktop verification

Host `reblue_17`, PID 7088/session 19931, exited 0 (12/15 final displayed link,
expected new-header glob recheck, no guest objects rebuilt, codegen up to date).
Binary 47,639,040 B, linked 03:41:27, SHA256
`b95b1b31991e11132f6ea1be147ca0bfcebd8a69874b5ede55ba3e780f5ae7a3`;
root `ee57134` plus the current local changes, local Plume `81bdca8`. Build logs
2,529 B stdout /18 B stderr. Ending build free 64,620,945,408 B. A link-time
reading was 64,573,259,776 B; this fitted the planned overlap and cumulative cap.

All three normal checks use that same binary with temporary capture-off profiles,
zero new raws and exact profile restoration. Bounded wrapper termination is not
a natural-shutdown, game Vulkan-validation or full-game correctness claim.

| Diagnostic | Owned PID / session | Time | Log / bytes | Native clears / precision getters | Native post scopes |
| --- | --- | --- | --- | --- | --- |
| MSAA flat | 26524 /54501 | 03:43:03--03:44:19 | 853 /246,444 | 3,600 /3,601 | 3,601 |
| MSAA XR | 26908 /79530 | 03:45:34--03:46:51 | 854 /555,494 | 9,600 /9,601 | 9,601 |
| Non-MSAA flat | 27044 /84937 | 03:47:27--03:48:43 | 855 /245,208 | 3,600 /3,601 | 3,601 |

All report zero scene state-308 calls, compatibility begins/ends/refusals, wrong
ownership, compatibility clears/depth publications, post imports/original scopes/
refusals and recovered colours. Native depth/deferred colours match the clear
counts. Periodic begin/getter/post reports are one ahead of completed scene counts,
as in prior checks. The MSAA sampled-image materialization counters remain present;
they are not post getter imports and also occurred in the prior command binary.
Non-MSAA uses its single-sample source images directly.

All 5 /16 /6 settings took effect respectively. XR uses the existing absolute
xrsim manifest, 1440x1584 per eye, multiview, zero-height pose, scale 1 and mirrors/
previews off. No new stereo pixels were collected. Flat/XR/non-MSAA final free
readings: 64,616,300,544 /64,594,276,352 /64,593,309,696 B. Perf files:
`perf-20260906-034306.csv` 610,304 B, `034537.csv` 1,626,112 B and `034729.csv`
606,208 B; each metadata file is 112 B. Non-MSAA source/post stores each settle at
two resident 33,177,600-B payloads, with one resident native framebuffer and zero
allocation/refusal failures. No performance/Quest comparison is inferred.

Inspected `out/verification/native_scene_precision_window.png`: 1920x1080,
3,350,297 B, captured from owned PID 26524 at 03:44:04. SHA256
`cd50db70966e5375098238f8764f01dc55efed736a642ca972cf70c57baad48c`.
Shu, terrain, foliage, structures, cast shadows and distant DoF are visible without
obvious full-frame corruption. It is unaligned flat sanity evidence, not a sequence,
authored event or stereo/full-game qualification.

## Completed cleanup and cumulative storage reconciliation

After equivalent verification, checked each exact path, expected length, regular
file status, no reparse ancestors, workspace containment and Git ignore status.
Elevated CIM confirmed no live renderer/build/compiler/linker producer before
each cleanup. Old/replacement PNG hashes were verified after inspecting the new
image. Removed only these known agent-created disposable diagnostics:

- First batch: stdout/stderr pairs `attachment_resolve_reblue_15`, `reblue_16`
  and `cpu_13`; runtime log 847 and its `perf-20260906-030842` CSV/metadata pair;
  the replaced `native_scene_commands_msaa_window.png`. Ten files, 4,167,462
  logical B. Immediate free 64,592,670,720 ->64,596,848,640 B:
  **4,177,920 B measured reclaimed**.
- Second batch: runtime logs 848/849 and their `perf-20260906-031045`/`031242`
  CSV/metadata pairs, after normal non-MSAA/XR replacement checks. Also removed
  the empty callback-trace `perf-20260906-033605.csv` and its unused metadata;
  the actual callback evidence remains in log 852. Eight files, 3,017,560 logical
  B. Immediate free 64,592,433,152 ->64,595,451,904 B:
  **3,018,752 B measured reclaimed**.

Total this follow-up: **18 files /7,185,022 logical B /7,196,672 B measured reclaimed**,
counted once, not added to past checkpoints' savings. The deleted files are no
longer available verbatim; old reports/hashes remain and equivalent diagnostics
can be regenerated. No game data, saves, profiles, source, dependencies or build
trees were removed. Current raw baselines and all unresolved-failure evidence
remain protected. Keep the existing non-MSAA PNG/XR and post-disabled/optical
diagnostics because this follow-up does not replace their particular coverage.

A roughly 22 MiB decline between the flat check and first cleanup remains
unattributed. Scoped build-root files, native-test tree, cache/dump/profile/log and
verification outputs showed only expected diagnostics/profile changes, no new raw
or cache dump explaining the full decline. It is charged to the original volume
budget, not credited away or treated as a new allowance.

Closing inventory: 76 build logs /125,806 B; 11 runtime logs /3,296,897 B; 20 perf
files /8,950,880 B; no new cache/dump outputs. Including the unchanged 41 MiB tool/
inspection reservation, retained diagnostics total **55,365,199 B**. The two current
flat PNGs total 6,684,453 B inside that reservation. Zero automatic raw files have
been created since 20:47; the over-budget historical archive has no incoming raw
allowance. No active producer or temporary profile override remains.

Post-cleanup free 64,595,451,904 B: cumulative volume growth 867,336,192 B from the
original checkpoint, and 23,707,648 B from this follow-up's 03:32 reading. The later
closing audit read 64,595,132,416 B; subsequent metadata/volume movements remain
charged too. These are volume deltas, not a precise attribution of all bytes to
these jobs. Current small evidence retires after equivalent verified replacements;
protected raw/failure sets retain their existing review triggers.

## Remaining work / publication

The independent precision-getter adapter/tests, bounded callback identification
and updated current documentation can be locally checkpointed without the Plume
dependency. Scene integration and its source guard remain local/uncommitted with
the earlier renderer work; no unapproved push or parent gitlink commit is made.
There is still engine cache/device getter publication and substantial rendering
execution outside the converted scope. Source mapping locates the larger parent
`bdRenderViewSubmit` at 0x82184E90 (generated file 16) and its all-pass wrapper at
0x8213C160; mapping an entry point is not conversion or a claim about its internals.
Complete scene/material/animation/shadow/UI/frame ownership, modern GPU execution
throughout, asset conversion and the full desktop both-eye/game gates remain
required before Quest 2 qualification. This goal remains active.

## Continuing the same storage ledger: pass dispatcher, 04:12

This is not a new checkpoint allowance. At the new producer preflight, measured
free space is 64,596,459,520 B; elevated CIM finds no game/build/compiler/linker
producer. Reuse both configured build trees and the guarded build/run wrappers.
Plan at most 512 MiB additional build/link overlap and 8 MiB new diagnostics,
including one bounded flat PNG replacement inside the existing 10 MiB aggregate
inspection reservation. No raw capture, new tool, download, build tree, shader
regeneration or guest-object rebuild is planned. Start with the focused existing
scene CPU target, then the CPU suite and incremental host link. Flat/XR checks
must use capture-off bounded runs; inspect the flat replacement before retiring
equivalent old small evidence. Protected raw/failure evidence stays untouched.
The technical findings and actual follow-up results are recorded separately in
`research/20260906_0412_native-pass-dispatch.md`; reconcile completed bytes here.

All three build/test producers and both desktop runs are terminal, successful
within their bounds. No new raw, cache or shader-dump output was produced. The
two runs ended with free space 64,589,574,144 B and 64,586,895,360 B respectively.
The new flat image was inspected before retiring its predecessor.

Completed cleanup: checked exact expected lengths, regular-file status, workspace
containment, all ancestors for reparse points, Git ignores, old/replacement PNG
hashes and elevated CIM producer absence. Removed 13 superseded files:
stdout/stderr pairs reblue_17, cpu_14 and host_scene_pass_test_01; normal logs
853/854 and their perf-20260906-034306/034537 CSV/metadata pairs; and
native_scene_precision_window.png. They total 6,395,187 logical B. Immediate free
space rose 64,586,350,592 ->64,592,752,640 B: **6,402,048 B measured reclaimed**,
counted once. The exact old outputs are no longer retained; reports/hashes remain
and equivalent diagnostics can be regenerated. No protected raw/failure evidence,
non-MSAA/optical/recovery evidence, game/save/profile/source/dependency/build tree
was removed. Current diagnostic replacements retire after equivalent verification.

Closing inventory: 76 build logs /125,827 B; 11 runtime logs /3,298,040 B;
20 perf files /8,926,304 B; zero new raw/cache/dump files. With the unchanged
41 MiB tool/inspection reservation, retained diagnostics total 55,341,787 B.
The two remaining flat PNGs total 6,610,887 B inside that reservation.
Free 64,592,752,640 B (60.16 GiB); net growth from this follow-up's preflight
is 3,706,880 B and from the original checkpoint is 870,035,456 B. Build/link
overlap stayed within the guarded cumulative budget; no additional allowance
was taken. No live producer or profile override remains. Later metadata/volume
changes still belong to this same ledger, not a reset budget.

## Continuing the same storage ledger: parent view scheduler, 04:42

Measured preflight free space is 64,594,759,680 B; elevated CIM finds no live
renderer/build/compiler/linker. The original 65,462,788,096-B checkpoint baseline,
2 GiB cumulative peak cap, 100 MiB diagnostics, 10 MiB build logs and 20 GiB reserve
remain unchanged. Last closing diagnostics were 55,341,787 B including the fixed
41 MiB tool/inspection reservation. Current flat PNGs total 6,610,887 B.

Reuse both configured trees and guarded wrappers. Plan at most 512 MiB temporary
build/link overlap and 12 MiB new diagnostics across all attempts: focused CPU
test, existing CPU suite, source guards, incremental host link, bounded flat/XR
checks and isolated post-off/non-MSAA coverage as needed. No raw captures, tools,
downloads, new build trees, shader regeneration or guest-object rebuild. At most
one bounded normal-flat PNG replacement uses the existing aggregate reservation;
inspect it before retiring the prior same-purpose PNG. Current/protected raw and
unresolved-failure evidence stays untouched. Reconcile actual outputs here after
verification; source findings belong in the separate parent-scheduler report.

The documentation interruption completed without a build/capture. On resumption
the original test/build producers were terminal and their linked binary/logs were
verified; no producer was restarted. Resume free space was 64,578,666,496 B.
The difference from the 04:42 preflight stays charged to this cumulative volume
ledger; it is not credited as reclaimed space or used to reset the budget.
Scoped diagnostics/cache inventories found no unrecorded raw/cache/dump output.

All three test/build producers and four bounded desktop runs are terminal and
successful: normal flat/XR, post-disabled MSAA recovery and normal non-MSAA flat.
Source guards 65/65 and CPU tests 31/31 pass. Runtime results and the exact binary
are recorded in the parent-scheduler report. No new raw capture, tool download,
shader/guest-object regeneration or build tree. One 3,361,048-B flat sanity PNG
was inspected before retiring its same-purpose predecessor. Four runtime logs
total 1,326,250 B; their perf CSV/metadata total 3,445,184 B. New build/test logs
total 6,531 B. These outputs and the PNG total 8,139,013 B, within the 12 MiB
follow-up plan; the PNG is charged within the existing inspection reservation.

Completed cleanup: the first invocation's exact-parent list guard stopped before
deletion. Corrected the list, rechecked all targets and removed 19 superseded
regular ignored files after checking lengths, workspace containment, reparse-free
ancestors, old/replacement PNG hashes and absence of active producers. Removed
stdout/stderr pairs reblue_18, cpu_15, host_scene_pass_test_02; runtime logs
856/857/851/855 and their perf-041532/041715/032352/034729 CSV/metadata pairs; and
native_pass_dispatch_window.png. Logical bytes 8,027,983; immediate volume free
space 64,566,398,976 ->64,574,435,328 B: **8,036,352 B measured reclaimed**, counted
once. Exact old outputs are gone; their reports/hashes remain and equivalent
diagnostics can be regenerated. No protected raw/failure, distinct non-MSAA PNG,
optical/non-MSAA-recovery evidence, game/save/profile/source/dependency/build tree removed.

Closing inventory: 76 build logs /126,025 B; 11 runtime logs /3,320,459 B;
20 perf files /8,930,400 B; zero new raw/cache/dump files. With the unchanged
41 MiB tool/inspection reservation, retained diagnostics total 55,368,500 B.
The remaining normal-flat and non-MSAA PNGs total 6,695,204 B within that
reservation; retire them only after equivalent purpose-specific verification.
Free 64,574,435,328 B (60.14 GiB). Net growth since the 04:42 preflight is
20,324,352 B; since runtime resumption, 4,231,168 B; since the original checkpoint,
888,352,768 B. These are volume-level measurements, not a claim that all growth
was diagnostic payload. Unattributed volume/metadata growth remains charged, not
discarded. No live producer or temporary profile override remains. Protected
historical raw sets retain their prior review/cleanup gates and zero incoming
raw allowance. Later source/Git metadata still counts in this same ledger.

## Continuing the same storage ledger: effect activation, 05:16

Preflight free space 64,576,348,160 B; no live renderer/build/compiler/linker.
The small increase from the prior closing volume is not claimed as cleanup.
Original baseline 65,462,788,096 B, 2 GiB peak cumulative growth, 100 MiB retained
diagnostics, 10 MiB aggregate build logs and 20 GiB reserve remain unchanged.
Retained diagnostics are 55,368,500 B including the 41 MiB tool/inspection
reservation; the two existing PNGs total 6,695,204 B. No raw incoming allowance.

Reuse the configured CPU and desktop trees, installed runtime and guarded wrappers.
Plan at most 512 MiB temporary build/link overlap and 8 MiB new diagnostics across
all attempts: focused scene CPU target, existing CPU suite, incremental host link,
bounded capture-off flat/XR checks and one bounded normal-flat PNG replacement.
This tests changed effect/registry execution, not merely a new commit label. Keep
the original budget floor and live producer bounds; do not rebuild guest objects
or shaders. Inspect the replacement before retiring same-purpose small outputs;
protected raw/failure evidence and distinct recovery/non-MSAA evidence stay intact.
Actual outcomes and cleanup must be reconciled here, not in a fresh allowance.

All three build/test producers and both desktop checks completed within their
bounds, with no new raw/cache/dump files. Focused CPU target and host link passed;
CPU suite 31/31, source guards 69/69. Two runtime logs total 843,788 B, perf CSVs
and metadata 2,224,352 B, build logs 6,535 B, and the inspected flat PNG 3,200,734 B:
6,275,409 B new diagnostics within the 8 MiB plan. The PNG is inside the unchanged
inspection reservation, not charged twice. No guest/shader rebuild or new tools.

Completed cleanup: validated expected lengths, regular-file status, explicit
workspace parents, reparse-free ancestors, ignores, both PNG hashes and absence
of active renderer/build producers. Removed 13 superseded outputs: reblue_19,
cpu_16 and host_scene_pass_test_03 stdout/stderr pairs; logs 858/859; perf-045055
and perf-045232 CSV/metadata pairs; native_view_schedule_window.png. Logical
6,408,767 B. Immediate free space 64,540,344,320 ->64,546,762,752 B:
**6,418,432 B measured reclaimed**, counted once. Reports and hashes remain;
the exact old outputs are gone and equivalent diagnostics can be regenerated.
Protected raw/failure sets, distinct non-MSAA/recovery/optical evidence and all
game/save/profile/source/dependency/build trees remain intact.

Closing inventory: 76 build logs /126,029 B; 11 runtime logs /3,347,411 B;
20 perf files /8,930,400 B; zero new raw/cache/dump files. With the fixed 41 MiB
tool/inspection reservation, retained diagnostics total 55,395,456 B. The two
remaining PNGs total 6,534,890 B. Free 64,546,762,752 B (60.11 GiB); net volume
growth since this follow-up's preflight 29,585,408 B and since the original
checkpoint 916,025,344 B. The volume fell an additional 27,463,680 B between the
last run and cleanup. Rechecked scoped verification/log/perf/cache/dump/profile
metadata: only the recorded outputs and restored 116-B owner profile changed.
No live renderer/build producer remains. That additional volume change is not
attributed to these diagnostic payloads or credited away; it remains charged to
the cumulative cap. No further large producer is launched. Required evidence
retains its existing cleanup conditions; later source/Git metadata still counts.

## Continuing the same storage ledger: effect lifecycle, 05:36

Preflight 2026-09-06 05:36:14 EDT: free 64,539,910,144 B; no live renderer,
compiler, linker, CMake, Ninja or CTest producer. Original baseline remains
65,462,788,096 B; cumulative volume growth is 922,877,952 B, including subsequent
source/Git and unattributed volume changes. Scoped latest runtime logs remain
862/863, and the owner's 116-B capture profile is unchanged. The 76 build logs
total 126,029 B. Previous retained diagnostics total 55,395,456 B, including the
fixed 41 MiB tool/inspection reservation; protected raw evidence has zero incoming
allowance. No prior cleanup is credited again.

Reuse the configured CPU/desktop trees and guarded wrappers. Plan no more than
512 MiB temporary build/link overlap and 8 MiB new diagnostics across retries:
focused lifecycle CPU cases, existing suite, host link, bounded capture-off flat
and XR runs, one bounded normal-flat PNG replacement. These verify new callback
ordering and lifecycle execution. Do not regenerate guest/shader objects, acquire
tools or create raw captures. After equivalent checks and pixel inspection,
retire eligible reblue_20/cpu_17/focused_04 logs, normal flat/XR 862/863 and their
perf pairs, and the effect-activation PNG. Preserve distinct non-MSAA, recovery,
optical and unresolved-failure evidence. The original live storage floor and all
aggregate caps remain enforced; record actual results and cleanup here.

All three build/test producers and flat/XR runs are terminal, exit 0. Host build,
31 CPU tests and 72 source guards pass; runtime evidence is in the 05:36 lifecycle
report. New build/test logs 6,319 B; runtime logs 883,378 B; perf files 2,253,024 B;
one inspected PNG 3,278,821 B: 6,421,542 B new diagnostics, below the 8 MiB plan.
The PNG stays inside the fixed inspection reservation, not counted twice.
No new raw/cache/dump files, guest/shader rebuild, tool download or build tree.

Completed cleanup: validated all 13 exact targets, lengths, ignored regular-file
status, approved workspace parents, reparse-free ancestors, old/replacement PNG
hashes and absence of active renderer/build processes before deletion. Removed
reblue_20, cpu_17 and host_scene_pass_test_04 stdout/stderr pairs; logs 862/863;
perf-051857 and perf-052041 CSV/metadata pairs; native_effect_activation_window.png.
Logical bytes 6,275,409; immediate free space 64,529,297,408 ->64,535,580,672 B:
**6,283,264 B measured reclaimed**, counted once. Exact old files are gone;
reports/hashes remain and equivalent checks can regenerate diagnostics. All
protected raw/failure, distinct non-MSAA/recovery/optical evidence, game data,
saves, profiles, source, dependencies and build trees are preserved.

Closing inventory: 76 build logs /125,813 B, 11 runtime logs /3,387,001 B,
20 perf files /8,959,072 B; zero new raw/cache/dump files. Retained diagnostics
including the unchanged 41 MiB tool/inspection reservation total 55,463,502 B.
The two retained PNGs total 6,612,977 B within that reservation. They replace
equivalent evidence; the modest payload change captures the new lifecycle
counters/current pixels and is eligible for retirement after equivalent future
checks, not a new permanent archive. Free 64,535,580,672 B (60.10 GiB). Net volume
growth from the 05:36 preflight is 4,329,472 B; cumulative from the original
baseline is 927,207,424 B. Unattributed volume/Git/metadata changes remain charged.
No live producer or profile override remains; later source/Git metadata still
counts. The protected historical raw gate and all original caps remain active.

## Continuing the same storage ledger: native scene snapshots, 06:01

Preflight 2026-09-06 06:01:13 EDT: 64,533,454,848 B free; no active game,
compiler, CMake/Ninja/linker or CTest. The 76 build logs total 125,813 B. Original
baseline remains 65,462,788,096 B, so cumulative volume growth is 929,333,248 B;
all earlier metadata/unattributed growth stays charged. Last retained diagnostics
55,463,502 B include the unchanged 41 MiB inspection/tool reservation. Protected
historical raws still have zero incoming allowance.

Reuse existing CPU/desktop trees and guarded wrappers. Plan at most 512 MiB
temporary build/link overlap and 10 MiB new diagnostics across attempts: focused
snapshot command fixture, CPU suite, host link, bounded normal flat/XR and
non-MSAA checks, one normal-flat PNG replacement. These test newly changed image
copy/barrier/lease execution, not a new commit stamp. No shader/guest rebuild,
tools, raw captures or new build tree. Validate replacements before removing
equivalent previous build/test logs, normal flat/XR 864/865 and their perf pairs,
the lifecycle PNG, and the old normal non-MSAA runtime/perf only after that
configuration passes. Keep distinct post-disabled/optical/failure evidence and
the non-MSAA sanity PNG. Original cumulative caps and live guards remain active.

Resumption after the instruction-file update: reblue_22, focused output_11,
cpu_19 and flat PID 9876 are terminal, all exit 0. Flat log 866 is 267,480 B,
perf-060320 CSV/metadata total 602,224 B, and the inspected 1920x1080 snapshot
PNG is 3,362,161 B. Prior build/test logs total 9,206 B: 4,241,071 B new
diagnostics so far. No raw captures. The flat image is sane, but no snapshot
telemetry was emitted; authored water/refraction execution is unproven. The
original profile was restored byte-for-byte. No queued XR/non-MSAA run resumed.

Source inspection found the original snapshot constructor still creates a
fixed 1280x720 texture. Requiring exact destination dimensions would preserve
the compatibility resolve at higher native extents. Explicit native-source
extent adoption now replaces that restriction; other output publications remain
strict by default. Qualify the actual copy core with an 8x8 off-screen Vulkan
fixture instead of another unchanged field scene. Distinct layers/HDR and two
retained snapshots across resumed live writes are required; readbacks stay in
memory. First-call telemetry will expose short-lived authored use in later runs.

At 06:11 the read-only volume check showed 64,520,839,168 B free and no active
renderer/build process. Reuse existing Plume/validation/CPU/desktop trees. Revise
the remaining verification plan, not the checkpoint allowance: at most 512 MiB
temporary build/link overlap and 24 MiB aggregate new diagnostics INCLUDING the
4,241,071 B above and all retries. The extra retained test executable/object/PDB
budget is 16 MiB for previously missing real-GPU snapshot coverage; keep one
build representation and retire it when the fixture is superseded. No new tool,
shader generation, raw capture, game run or persistent profile change is needed
for this fixture. Existing overall 2 GiB/100 MiB/10 MiB-log caps and protected
archive gate remain unchanged. Reconcile measured outputs before further work.

Closeout 06:29: all producers terminal. The fixture reproduced an MSAA stale
resolve despite zero validation errors; Plume 3094b35 fixes completion of pending
sibling clears before resolve-output reads. Snapshot GPU CTest 03 passes all
eight mono/layered x 1/2/4/8-sample cases with zero validation errors/warnings;
the expanded existing resolve suite passes too. CPU 20: 31/31; source guards
77; host build reblue_23 exit 0, no guest/shader rebuild. Exact binaries, runs,
failure evidence and limitations are in `20260906_0629_native-scene-snapshots.md`.
No new game run followed the earlier flat check; its snapshot use is unproven.

Gross new measured diagnostic payloads before cleanup: build/test logs 58,119 B,
flat runtime log 267,480 B, perf pair 602,224 B, PNG 3,362,161 B and the reusable
GPU fixture directory 8,364,855 B: 12,654,839 B, within the revised 24 MiB plan.
The PNG remains inside the existing inspection reservation for retained totals.
No new tool download, raw capture, guest/shader rebuild or configured build tree.
The same original limits applied to all compilation/test failures and retries.

Completed cleanup, after validating all exact paths/lengths, ignored-file status,
approved parents, reparse-free ancestors, replacement hashes and absence of live
producers: 24 files /4,167,824 logical B. Removed reblue_21/22, cpu_18/19,
host_post_output_test_10/11, native_scene_snapshot_test_02/03,
native_attachment_resolve_test_15 and pixels_11 stdout/stderr pairs; flat log 864,
perf-054145 CSV/metadata, and native_effect_lifecycle_window.png. Immediate free
space 64,495,013,888 ->64,499,191,808 B: **4,177,920 B measured reclaimed**,
counted once. Files are gone; reports/hashes remain, and equivalent diagnostics
can be regenerated. Keep GPU failure logs 01/02, current flat log 866/PNG,
XR 865, distinct non-MSAA/recovery/optical evidence and protected raw archives.
No game/save/profile/source/dependency/build-tree data was removed.

Closing inventory: 86 build logs /164,482 B; 11 checkpoint runtime logs
/3,387,152 B; 20 perf files /8,959,072 B; 8 GPU-fixture files /8,364,855 B.
With the existing 41 MiB inspection/tool reservation, retained diagnostics total
63,867,177 B; two PNGs /6,696,317 B fit within that reservation. The new fixture
is retained for distinct actual-GPU snapshot/resolve coverage, one current build
representation; replace/retire it when that qualification is superseded.
Old historical runtime logs outside this checkpoint were inventoried but not
removed or mistaken for newly produced diagnostics. Protected raw growth: zero.

Ending free space 64,499,191,808 B (60.07 GiB). Net growth from the 06:01 snapshot
preflight is 34,263,040 B; from the resumption's 06:11 check, 21,647,360 B; from
the original checkpoint, 963,596,288 B. Source/Git/metadata and unexplained volume
activity remain charged. Later docs/Git writes still count. All producers are
terminal and the owner's profile is restored; no pending run is authorized by
this accounting entry. Dependency publication remains unapproved, with no push
retry or unpublished parent gitlink staged.

## Continuing the same storage ledger: water/refraction parents, 06:50

Preflight 2026-09-06 06:50:13 EDT: 64,500,072,448 B free and no live renderer,
compiler, CMake/Ninja/linker or CTest. The 86 build logs still total 164,482 B.
The original 65,462,788,096 B baseline and all cumulative caps remain unchanged;
cumulative volume growth is 962,715,648 B. Last retained diagnostic inventory
is 63,867,177 B; historical raw growth remains prohibited.

Reuse the existing post-output CPU executable and configured desktop tree to
qualify the new water/refraction setup core/imports and compile both whole hooks.
Plan at most 256 MiB temporary compilation/link overlap and 1 MiB additional
build/test logs across retries. Source checks run without bytecode/log exports.
No new test binary/tree, downloads, guest/shader regeneration, game captures or
profile changes are needed for this step. CPU policy/import tests do not prove
authored material execution or pixels; those remain pending. After successful
replacement, retire only reblue_23, cpu_20 and host_post_output_test_12 log pairs.
The guarded wrapper enforces the original free-space and aggregate-log limits;
all new retained growth and Git/metadata still count in this ledger.

Focused build 13, CPU 21 (31/31 in 3.32 s) and desktop build 24 all exit 0.
Both new hook symbols compiled/linked; codegen wrote nothing, no guest objects
or shaders rebuilt. Revise the same step's minimal-output plan to at most 5 MiB
new diagnostics including its build logs: one 75-second normal flat check and
one <=4 MiB window PNG, captures disabled. This checks newly replaced material
parents and actual field behavior, not a commit stamp. The prior snapshot run
never observed its child; first-call parent telemetry now makes that gap testable.
No queued XR/non-MSAA matrix is implied. The wrapper now includes the retained
8,364,855 B GPU fixture and aggregate build logs in its existing 75 MiB small-output
stop threshold (100 MiB ceiling); PNG overlap remains <=10 MiB. After validating
the replacement, retire only equivalent flat log 866/perf-060320/snapshot-window
PNG in addition to the three superseded build/test log pairs above. Retain all
distinct XR/non-MSAA/failure/raw evidence. All original checkpoint caps persist.

Closeout 07:00: build 24 and flat PID 23888/session 94172 are terminal, exit 0;
profile restored byte-for-byte. Water parent last reports 1,964 executions with
zero material fallback/refusal/faults, but no refraction or snapshot branch use.
The new normal-flat image was inspected; this is not full material/sequence/VR
qualification. CPU 31/31 and 83 source guards pass. Exact sources, binary hashes,
settings and limitations: `20260906_0700_native-refraction-materials.md`.

Gross new diagnostics 4,236,095 B: build/test logs 6,340 B; flat log 867
270,022 B; perf-065659 pair 606,320 B; PNG 3,353,413 B. Zero raw/cache/dump files,
downloads, guest/shader rebuilds or new build tree. After validating replacements,
exact ignored paths/lengths, reparse-free ancestors and no active producers,
removed ten files: reblue_23, cpu_20, host_post_output_test_12 stdout/stderr pairs,
flat log 866, perf-060320 CSV/metadata and native_scene_snapshot_window.png.
Logical bytes 4,264,473; immediate free 64,463,409,152 ->64,467,681,280 B:
**4,272,128 B measured reclaimed**, counted once. Reports/hashes remain and
diagnostics can be regenerated. All protected data/distinct evidence preserved.

Retained diagnostic payloads decreased 28,378 B. Reserved inventory 63,847,547 B:
86 build logs/138,214 B, 11 runtime logs/3,389,694 B, 20 perf files/8,963,168 B,
eight GPU-fixture files/8,364,855 B and unchanged 41 MiB tool/inspection reserve.
Two PNGs/6,687,569 B are within that reserve. Normal-flat evidence is eligible
for replacement after equivalent qualification, not a growing per-commit archive.
Free 64,467,681,280 B (60.04 GiB); net volume growth from 06:50 32,391,168 B,
cumulative original-baseline growth 995,106,816 B. Build/Git/metadata and
unattributed volume activity stay charged; scoped cache/dump inspection found
no new files. Later docs/Git writes still count. No live producer/profile override
remains; no further verification matrix is launched by this accounting entry.
