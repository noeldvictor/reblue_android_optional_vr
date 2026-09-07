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

## Continuing the same storage ledger: water update, 07:11

Preflight 2026-09-06 07:11:57 EDT: 64,467,468,288 B free; no renderer/compiler/
CMake/Ninja/linker/CTest active. The 86 build logs total 138,214 B. The original
baseline remains 65,462,788,096 B (995,319,808 B cumulative growth); last retained
diagnostic accounting is 63,847,547 B. All original caps and the zero-incoming-raw
gate remain unchanged.

Reuse the post-output CPU fixture and desktop tree. Plan <=256 MiB temporary
build/link overlap and <=6 MiB new diagnostics across retries: focused CPU
publication/alias tests, the existing CPU suite, host link and bounded flat
original-publication comparison plus normal native execution with one <=4 MiB
window image. No new binary/tree/tools, raw capture, guest/shader rebuild or
persistent profile change. The comparison uses a fixed 32-word in-memory write
overlay, never a dump of the material/arena. Retire the preceding normal flat
log 867/perf-065659/material-window PNG and build 24/CPU 21/focused 13 logs only
after equivalent replacements pass. Keep distinct XR/non-MSAA/failure/raw data.
The existing live wrappers retain the cumulative free-space/log/diagnostic guards.

Focused build 14 and CPU 22 pass (31/31, 3.59 s). Host build 25 exits 0 with both
whole callbacks linked and no guest/shader rebuild. All 88 source guards pass.
The original-comparison run PID 22500/session 31230 stopped successfully at its
first sample above 256 updates: 301 checked, zero wrong/refused/compatibility,
288 tick advances and 6,923 parameter words. Log 868 is 111,333 B; no performance
CSV, PNG or raw output was produced. This distinct first exact water-publication
comparison is retained until an equivalent comparison supersedes it, not repeated
as a permanent per-commit log. Profile restored byte-for-byte; producer terminal.
Normal native execution now uses PID 26432/session 50037, started 07:22:58,
75-second bound, one <=4 MiB PNG, captures off and guaranteed profile restoration.
Its outputs and all retries stay within the same <=6 MiB step plan and original
checkpoint limits. Do not restart this producer merely because a poll is quiet.

Closeout 07:26: normal PID 26432/session 50037 terminal, wrapper exit 0 and
profile restored byte-for-byte. Last sample: 2,701 native updates, 1,719 tick
advances, 62,123 parameter words, zero fallback/refusal/reference execution.
The new flat PNG was inspected; temporal/stereo/refraction events remain open.
Both logs have zero runtime/config error matches. Evidence and exact hashes:
`20260906_0726_native-water-update.md`. No further game/XR/capture run queued.

Gross new diagnostics 4,353,076 B: build/test 6,333; comparison log 868 111,333;
normal log 869 273,140; perf-072301 pair 606,320; PNG 3,355,950. Zero new raw,
cache/dump, tool download, guest/shader rebuild or configured build tree.
After replacement/path/length/ignore/reparse/process validation, removed ten
files: reblue_24, cpu_21, host_post_output_test_13 stdout/stderr pairs, flat log
867, perf-065659 pair and native_refraction_material_window.png. Logical bytes
4,236,095; immediate free 64,460,967,936 ->64,465,207,296 B:
**4,239,360 B measured reclaimed**, counted once. Reports/hashes remain and
diagnostics can be regenerated; all protected data/distinct evidence preserved.

Retained payload growth 116,981 B includes the first exact-publication comparison
log (111,333 B, replace after equivalent comparison); remaining growth is minor
replacement size drift. Reserved accounting 63,961,991 B: 86 build logs/138,207 B,
12 runtime logs/3,504,145 B, 20 perf files/8,963,168 B, eight GPU-fixture files/
8,364,855 B plus unchanged 41 MiB tool/inspection reservation. Two PNGs/6,690,106 B
fit within that reservation. Final free 64,465,207,296 B (60.04 GiB); net from
07:11 preflight +2,260,992 B used; cumulative +997,580,800 B from the original
baseline. Build/Git/metadata and unrelated volume activity remain charged;
later documentation/Git writes still count. All producers terminal and profiles
restored. Full renderer/desktop gate remains open; no unpublished gitlink staged.

## Continuing the same storage ledger: native material disk guard, 07:45

Preflight 2026-09-06 07:45:04 EDT: 64,439,558,144 B free, no renderer/compiler/
CMake/Ninja/linker/CTest active. Original baseline stays 65,462,788,096 B:
1,023,229,952 B cumulative volume growth. The 86 build logs remain 138,207 B;
last reserved diagnostic accounting is 63,961,991 B. Actual native material
cache is 30 files / 2,040 logical B; it is not a cleanup target.

Source inspection found the material library's residency limit did not limit
its persistent files. Close that prerequisite before expanding native water
assets: aggregate disk byte/file caps, a free-space reserve, non-waiting writer
lease, bounded inventory and safe refusal without losing usable native data.
This does not convert the remaining water parameter buffers or shader ABI.

Reuse `out/native_material_test` and the desktop build tree. Step plan, including
all retries: <=256 MiB temporary build/link overlap, <=1 MiB temporary private
test fixtures and <=1 MiB new diagnostics. Existing material fixture/cooker,
source-free read-only validation of the 30 actual assets, then a host link;
no game runs, captures, asset conversion, downloads, shaders or guest rebuild.
The existing wrapper's cumulative free-space and log guards remain enforced.
After a successful replacement host link, retire only the previous reblue_25
stdout/stderr pair; retain all distinct runtime/CPU/GPU/pixel evidence. Other
new small logs establish this first disk-budget test and source-free reload;
replace them at the next equivalent verification, not per commit forever.

Closeout: focused material build 02/PID 26792 and CPU 02/PID 22152 pass (1/1,
0.11 s), including eight actual two-library contention trials. Cooker build
01/PID 25140 passes; read-only validation loads all 30 real native assets
(2,040 logical B unchanged; composable fields 30/30/7). Host build 26/PID 27524/
session 74032 terminates successfully, no guest objects or shaders rebuilt.
All producers terminal, private test scratch removed and profile unchanged.
Exact source findings, limits and hashes: `20260906_0757_native-material-storage.md`.

Gross new logs 5,934 B across all attempts. After replacement/hash/path/length/
ignore/reparse/process validation, removed six obsolete logs: reblue_25,
native_material_test_01 and material_cpu_01 stdout/stderr pairs. Logical bytes
4,184; immediate free 64,434,925,568 ->64,434,933,760 B, **8,192 B measured
increase**, counted once. Logs are regenerable, results retained in the report;
no protected evidence, actual material assets or build trees were removed.

Retained log growth 1,750 B establishes new material disk-budget coverage:
92 build logs / 139,957 B. Reserved checkpoint diagnostics 63,963,741 B; all
runtime/perf/PNG/GPU/tool reservations unchanged. No new capture, cache file,
asset conversion or download. Replace these small material logs after the next
equivalent verification. Cleanup-end free 64,434,933,760 B (60.01 GiB), drive-wide
use +4,624,384 B from 07:45 and +1,027,854,336 B from the original baseline.
Volume changes include build/Git/metadata and unrelated activity; do not equate
them with attributable artifacts. Later docs/Git writes remain charged.
Native water storage, the full frame/desktop gate and publication remain open.

## Continuing the same storage ledger: native parameter owner, 11:59

Preflight 2026-09-06 11:59:32 EDT: 64,435,494,912 B free, no tracked renderer/
build/test producers. Original baseline remains 65,462,788,096 B; cumulative
growth 1,027,293,184 B. Retained inventory unchanged at 63,963,741 B reserved
diagnostics (92 build logs / 139,957 B; 12 runtime logs / 3,504,145 B; 20 perf
files / 8,963,168 B; eight GPU-fixture files / 8,364,855 B plus 41 MiB tools/
inspection reservation). Two PNGs / 6,690,106 B fit within that reservation.

Shared parameter producer/upload ownership, not another storage-only change.
Reuse the desktop and CPU trees; <=256 MiB build/link overlap and <=1 MiB new
logs across retries, plus one <=4 MiB flat PNG within the existing aggregate
10 MiB PNG reservation (only 3,795,654 B overlap room before replacement).
The guarded runs disable raw capture; comparison runs also disable perf CSV,
stop on errors and require 10,000 checked blocks plus 256 field water updates
within 45 seconds. Normal flat sanity is bounded at 75 seconds; no broad game
or stereo-image qualification is claimed. The same checkpoint disk/log guards
remain active. No new raw allowance, downloads, guest/shader rebuild or tree.

First CPU build 01/PID 22588 and suite 23/PID 14308 passed 31/31. Host build
27/PID 25736/session 61031 linked successfully; comparison log 870/PID 17760
stopped on an unnotified c53 writer. Builds 28/PID 984 and 29/PID 21172 passed;
logs 871/PID 22052 and 872/PID 26768 exposed c57 then c50. Each stopped and
restored the profile. Exact sources identified visual setup, foliage's bool31
boundary and Toon/fur inline rows; these are explicit remaining imports.

After replacement/length/ignore/reparse/process validation, removed four logs:
reblue_26 and cpu_22 stdout/stderr pairs, 6,340 logical B. Immediate free
64,434,569,216 ->64,434,577,408 B: **8,192 B measured reclaimed**, counted once.
These logs are regenerable; results remain recorded. No protected evidence,
data, assets or build trees were removed.

Build 30/PID 26444 and parameter fixture 02/PID 21232 pass; CPU 24/PID 26468
passes 31/31 and all 94 source guards pass. Comparison log 873/PID 27016/session
10611 terminates successfully: last sample 34,591 checked blocks, zero wrong,
301 native water updates before bounded stop, all six profile settings audited.
No new raw/perf outputs in any comparison. The owner profile is restored.
Normal flat log 874/PID 25784/session 55678 terminates at its 75-second bound:
855,492 native blocks, 11,144,896 imported words, 157,468 legacy blocks; verifier
off. Water 2,701 updates / 1,964 preparations, no fallback/refusal/fault. One
3,362,832 B / 1920x1080 flat PNG was inspected; sane standing-field pixels,
not sequence/both-eye/full-game qualification. Perf pair totals 602,224 B.
After cleanup, the existing desktop XR simulator was reused for parameter-only
comparison log 875/PID 22748/session 14864: 501,224 checked native blocks, zero
mismatch, 301 water updates, all 17 settings audited and view composition active.
No XR perf/image/raw outputs. All jobs terminal; owner profile restored.

After replacement hashes/results/pixels, exact ignored paths, sizes, reparse-free
ancestors and absent producers were verified, removed 17 obsolete files:
reblue_27/28/29, host_parameter_test_01, cpu_23 stdout/stderr pairs; normal log
869, perf-072301 pair, water-update PNG; resolved parameter failures 870/871/872.
Logical 4,430,195 B; immediate free 64,430,690,304 ->64,435,142,656 B,
**4,452,352 B measured increase**. With the four logs above, this step reclaimed
**4,460,544 B measured**, 21 files / 4,436,535 logical B. No double credit;
results/causes/hashes remain in `20260906_1215_native-parameter-storage.md`.
No protected data, assets, distinct raw/eye/failure evidence or trees removed.

Gross new diagnostic payload 4,865,296 B across retries: logs 900,240 B,
normal perf 602,224 B, PNG 3,362,832 B. Net retained growth 428,761 B chiefly
establishes first flat/XR independent parameter-storage comparison coverage;
replace after equivalent checks, not permanent per-commit retention. Reserved
accounting 64,385,620 B: 94 build logs/138,514 B, 14 runtime logs/3,931,563 B,
20 perf files/8,959,072 B, eight GPU fixtures/8,364,855 B plus unchanged 41 MiB
tool/inspection reservation. Two PNGs/6,696,988 B fit within that reservation.
12:21:09 free 64,434,106,368 B (60.01 GiB), step volume use +1,388,544 B,
cumulative +1,028,681,728 B. Volume changes include unrelated activity and
source/build/Git/metadata, not solely diagnostics; later docs/Git writes count.
Full native frame/desktop gate and root/dependency publication remain open.

## Continuing the same storage ledger: immediate UI, 12:41

Preflight 2026-09-06 12:41:10 EDT: 65,302,700,032 B free (60.82 GiB), no
tracked build/test/game producers. The original 65,462,788,096 B baseline and
all checkpoint limits remain unchanged. The drive gained space since the last
status check without agent cleanup; do not credit that gain to this task.
Existing diagnostic reservation remains 64,385,620 B, including 94 build logs /
138,514 B and two PNGs / 6,696,988 B inside the 10 MiB inspection allowance.

Reuse the desktop/CPU trees for the immediate UI producer replacement; <=256
MiB build/link overlap, <=1 MiB new logs across retries, <=4 MiB replacement
flat PNG constrained by the actual 3,788,772 B aggregate overlap room, and
<=1 MiB normal perf pair. No raw frames, downloads, cooked assets, new build
tree or guest/shader rebuild. Verify original preparation/vertex submission,
then normal native rendering and parameter ownership. Replace equivalent
build/CPU/normal-flat evidence after validating it; retain a new original-UI
comparison only until equivalent subsequent coverage. Jobs continue to enforce
the original free-space, cumulative diagnostics/log and raw-capture gates.

Immediate-UI follow-up complete: CPU fixture 03/PID 22236 and suite 25/PID
25656 pass 31/31, with 99 source guards. Host 31/PID 24864/session 69277 and
32/PID 19944 pass without guest/shader rebuild. Original comparison log 876/
PID 27056/session 13646 has 16,664 matching UI submissions and 203,105 matching
native parameter blocks. Normal flat log 877/PID 24072/session 94346 has 84,502
native submissions without fallback/refusal/upload failure and one inspected
3,352,997 B field sanity image. The final lazy-buffer binary is checked in
desktop XR log 878/PID 26828/session 4754: 35,350 native UI submissions,
264,067 matching parameter blocks, no failures. All jobs terminal; profile
restored. No new build/game run after the owner's status interruption.

Validated equivalent evidence before removing 14 exact ignored, reparse-free
obsolete diagnostics: build 30/31, fixture 02 and CPU 24 stdout/stderr pairs;
runtime 873/874/875, perf-121336 pair and parameter-storage PNG. A first ancestor
check refused before deleting anything. Corrected FileInfo/DirectoryInfo checks
then completed two removals: 4,362,240 +323,584 B measured = **4,685,824 B**,
4,673,420 logical B total, counted once. Results/binary/image hashes remain in
`20260906_1252_native-immediate-ui.md`. No protected outputs/data/trees removed.

Gross diagnostic payload 4,677,313 B; net retained payload +3,893 B, combining
new original-UI coverage with replacement parameter evidence and size drift.
Replace on equivalent future verification. Reserved inventory 64,399,348 B:
94 build logs/138,209 B, 14 runtime logs/3,937,404 B, 20 perf files/8,967,264 B,
eight fixtures/8,364,855 B and fixed 41 MiB tools/inspection reservation. Two
PNGs/6,687,153 B fit inside that reservation; no new cache/raw/cooked outputs.
Closing pre-documentation/Git free 65,273,823,232 B (60.79 GiB), +28,876,800 B
drive-wide use since 12:41 and +188,964,864 B from the original baseline;
unrelated volume changes are not claimed as cleanup. Later docs/Git writes count.
Sorted scheduling, full native frame/game/both-eye gates and publication remain
open. A renewed normal Plume push was rejected for missing repository-specific
upload authorization; no workaround or parent gitlink staging.

## Continuing the same storage ledger: sorted visual schedule, 13:12

Preflight 2026-09-06 13:12:49 EDT: 65,279,000,576 B free, no tracked
build/test/game producers. Original 65,462,788,096 B baseline and all prior
limits remain unchanged. Retained reservation 64,399,348 B; 94 build logs /
138,209 B. Two retained PNGs / 6,687,153 B leave 3,798,607 B temporary overlap
inside the 10 MiB inspection cap. No unrelated volume gains credited as cleanup.

Reuse the existing CPU and host trees for the whole sorted scheduler. Bound
build/link overlap at 256 MiB, new aggregate logs at 1 MiB, one normal perf
pair at 1 MiB and a replacement flat PNG at the actual overlap allowance above.
No raw capture, new tool/tree, asset rewrite or guest/shader rebuild. CPU
linked-order oracle and callback-sensitive dispatch tests precede incremental
host build; independent parameter and normal flat/XR checks expose inline
writers formerly hidden by the enclosing legacy scope. Validate replacement
evidence before retiring equivalent outputs; preserve original UI comparison,
distinct non-MSAA and unresolved-failure evidence. Producers retain the original
cumulative free-space/log/diagnostic stop guards across retries.

Sorted-scheduler follow-up verified: host builds 33/PID 19548/session 39460
and 34/PID 784, CPU fixtures 04/PID 22612 and 05/PID 14720, suites 26/PID
27496 and 27/PID 26928 all pass. Final 31/31 CPU and 103 source guards.
Flat comparison log 879/PID 23784/session 64438 has 12,806 matching original
UI outputs and 239,393 matching native blocks. Final normal flat log 880/PID
26048/session 50316 records 2,121 host schedules / 78,861 primitives, no
fallback/faults, and the inspected 3,359,345 B field image. Final XR log 881/
PID 26508/session 52179 has 425 schedules / 36,784 primitives, 952,257 matching
parameter blocks and no faults. Normal flat/XR full legacy blocks are zero;
queued models/deferred effects were not observed and retain CPU-only coverage.
All jobs terminal and owner profile restored. No new run after the status/
README request. Exact binaries, coverage and timing limitations are recorded
in `20260906_1323_native-visual-schedule.md`.

After equivalent replacements, removed 18 exact ignored/reparse-free obsolete
files: build 32/33, fixture 03/04, CPU 25/26 stdout/stderr pairs; runtime
876/877/878; perf-124446 pair and immediate-UI PNG. Logical 4,680,814 B;
immediate free 65,261,813,760 ->65,266,507,776 B at 13:23:35, measured reclaimed
**4,694,016 B**, counted once. No protected data, assets, trees or distinct
failure/non-MSAA evidence removed. Results and hashes remain in reports.

Reserved inventory 64,407,085 B: 94 build logs/138,226 B, 14 runtime logs/
3,953,316 B, 20 perf files/8,959,072 B, eight fixtures/8,364,855 B and fixed
41 MiB tool/inspection reservation. Two PNGs/6,693,501 B fit within it; no new
cache/raw/cooked files. Gross diagnostic log/perf/PNG payload 4,694,899 B;
net retained replacement payload +14,085 B from run-size drift, replace on
equivalent coverage. Reused CPU fixture +418,304 logical B for new independent
order/model tests, within the tools reservation; final host binary +20,480 B.
Post-cleanup drive-wide use +12,492,800 B since 13:12, +196,280,320 B from the
original baseline, including unrelated volume activity/metadata. Later docs/
Git writes count. No push retry or unpublished dependency gitlink staging.

## Integration review and publication, 13:42

The owner has since explicitly authorized normal commits/pushes to the two
named GitHub forks. Root `d834754` and Plume `3094b35` are published, with the
parent gitlink already naming that dependency. Earlier rejection notes above
remain historical evidence, not a current publication blocker.

Reviewed the remaining 29 source/test integration files as one connected
checkpoint: device-owned scene/post stores; native single-sample and attachment
resolve framebuffers; scene commands/clears and precision getter publication;
snapshot leases; shared descriptor/layout publication; post shader alpha/exposure
handling; bindless, surface-pool and fence retirement adapters; source and CPU
tests. No implementation change was needed during review. In particular,
`VulkanDevice::createFramebuffer` already returns null unless both framebuffer
and render-pass handles are valid. Allocation failure cannot publish the
null-backed wrapper suspected during initial inspection. Descriptor allocation
precedes publication, and framebuffers/resolve owners retain their source images.

Reused evidence rather than rebuilding for a commit hash: host build 34 remains
47,759,360 B, SHA256
`08d36713f496081309fe3d537ef85a821f98f110c939b229eed59c34bbcb47a7`;
CPU suite 27 is 31/31 in 3.98 s. The 103 source guards were rerun successfully
with `python -B`, and the diff whitespace check passes. Flat log 880 records
3,600 completed/consumed native scene results, 3,600 native clears and depth
publications, zero compatibility clears/depth publications, and 3,601 final
post publications with zero scene-image imports. Existing XR log 881 and the
inspected flat field image are documented in `20260906_1323_native-visual-schedule.md`.
Strict GPU snapshot/resolve fixtures remain passing evidence from
`20260906_0629_native-scene-snapshots.md`; no new GPU/game run was made here.
These counters are periodic samples, not whole-frame ownership or broad
desktop/both-eye qualification. Authored snapshot and deferred execution still
need runtime coverage. Preserve the existing images, logs and failure evidence.

Same cumulative storage budget, no reset. Review preflight free 65,264,644,096 B;
no active build/test/game producer, and the owner's five-line profile is intact.
Retained diagnostics remain 64,407,085 B, including 94 build logs / 138,226 B.
No new binary, cache, capture, tool, asset or diagnostic output; no cleanup was
needed for this evidence-reuse checkpoint and no reclaimed bytes are claimed.
Only source/documentation and normal Git metadata are added; the final inline
handoff records ending free space and drive-wide change after commit/push.

## Continuing the same storage ledger: deferred visuals, 13:53

Previous review published root `b2910d4`, with 65,237,557,248 B ending free:
27,086,848 B drive-wide growth, only 256,939 B newly written Git objects and
small documentation growth identified. No new diagnostics or active producers;
the later scoped logs/verification check found no new outputs explaining that
drive-wide change. It is not charged as a known rendering artifact or credited
as cleanup. Original checkpoint baseline/limits remain unchanged.

Deferred-pass build preflight: 65,237,831,680 B free, no active build/game job.
Reuse CPU/host trees; <=256 MiB build/link overlap and <=1 MiB new aggregate
logs across attempts. Existing eight-mode strict snapshot GPU fixture covers
the unchanged copy/resolve commands; new CPU tests cover the live deferred
scheduler. A bounded normal flat regression may replace equivalent field/perf
evidence, with <=1 MiB perf and a <=3,792,259 B PNG (actual remaining overlap
inside the 10 MiB inspection allowance). No raw frames, asset/cache conversion,
download, new tree or guest/shader rebuild. Preserve distinct XR/original UI,
non-MSAA and failure evidence. Retire superseded build/CPU/flat outputs only
after validating replacements; no budget reset across continuations/retries.

Deferred verification complete: fixture 06/PID 22496, CPU suite 28/PID 15132
(31/31, 6.81 s), host 35/PID 20472/session 50592 all exit 0; 107 source guards
pass. No guest/shader rebuild. Flat log 882/PID 25340/session 49485 terminal
13:56:17, profile restored: 5,363 empty native deferred calls, zero nonempty
effects/snapshots or fallback/fault. Inspected 3,334,419 B field PNG; no XR or
raw run. Exact hashes/settings/remaining runtime gates are recorded in
`20260906_1358_native-deferred-visuals.md`; existing strict GPU copy tests reused.

After validated replacements, removed ten exact ignored/reparse-free obsolete
files: build 34, CPU fixture 05 and suite 27 stdout/stderr pairs; log 880,
perf-131943 pair and sorted-visual PNG. Logical 4,249,671 B; immediate free
65,228,255,232 ->65,232,515,072 B: **4,259,840 B reclaimed**, counted once.
No protected data/trees or distinct original-UI/XR/non-MSAA/failure evidence
removed. Gross new log/perf/PNG payload 4,237,027 B, net retained **-12,644 B**.
CPU fixture +19,968 B for exhaustive live deferred scheduling coverage inside
the tools reservation; host +10,752 B. No new cache/raw/cooked output/tool/tree.
Reserved diagnostics 64,419,367 B: 94 build logs/139,942 B, 14 runtime logs/
3,955,690 B, 20 perf files/8,967,264 B, eight GPU-fixture files/8,364,855 B,
fixed 41 MiB tools/inspection; two PNGs/6,668,575 B within that reservation.
Post-cleanup drive-wide use +5,316,608 B since 13:53, including unrelated volume
activity/metadata. Later source/docs/Git writes count; final handoff measures
ending free space. Replace equivalent evidence at the next qualified checkpoint.

## Continuing the same storage ledger: material passes, 14:11

Root `4dc37cf` published the deferred checkpoint; final free 65,232,334,848 B,
drive-wide use +5,496,832 B for that step, no further cleanup credit. Current
preflight free 65,236,197,376 B, no active producer, original profile intact.
The volume gain is not agent cleanup. Same original baseline, 2 GiB cumulative
peak, 100 MiB diagnostics, 10 MiB build logs and raw-capture gate remain.
Retained reservation 64,419,367 B; 94 build logs /139,942 B. Two PNGs /
6,668,575 B leave 3,817,185 B replacement overlap within the 10 MiB allowance.

Reuse existing CPU/host trees: <=256 MiB build/link overlap and <=1 MiB new
aggregate build/CPU logs across attempts. Five material lifecycle/binding bodies
need CPU callback/alias checks, host link and bounded normal-flat plus XR
parameter regression, since shared declaration/shader binding affects both.
Limit new runtime logs to an estimated <=1 MiB, normal perf <=1 MiB and one
flat PNG to the actual overlap above. Existing supervisors enforce original
cumulative free-space/log/diagnostic and zero-new-raw guards. No downloads,
assets/caches, new tree or guest/shader rebuild. Replace equivalent CPU/build,
flat/perf/image and XR-parameter evidence only after qualification; preserve
original UI, non-MSAA and unresolved-failure evidence. All retries share this
ledger; no additional artifact allowance is created by this continuation.

Standing-approval/status follow-up: the existing host build 36 / PID 19580 /
session 24991 has completed with exit 0. It linked the local material-pass edits;
no guest objects or shaders rebuilt (codegen reported the module up to date).
The executable is 47,778,304 B, SHA-256
`3e85baa70f1f1cc03ae692e0259f945c5f150629234dd0e195a362deb1a21fb0`.
CPU fixture 07 / PID 25344 and suite 29 / PID 27592 also completed with exit 0;
31/31 tests pass in 7.10 s. Fixture 572,928 B, SHA-256
`4f3c6c4e8454319b763f78c7dd1d4ee3841a81cbf4688f4336968c6bf5b784c6`.
The 111 source guards passed before interruption. No new build/test/game/capture
was launched for this follow-up. Flat-image and desktop XR parameter checks
remain pending; renderer source/test edits remain local, not published.

No active project build/test/game producers found; the owner's original
116-byte, five-line profile is intact. Follow-up free space before docs/Git
writes: 65,234,227,200 B, drive-wide use +1,970,176 B from material preflight.
Six new build/test logs total 6,503 B; retained reservation is 64,425,870 B,
including 100 build logs /146,445 B. Fixture grew 22,528 B within the existing
tools reservation; host grew 8,192 B. No new runtime/perf/image/raw artifacts.
Keep these build/CPU outputs for unfinished material-pass verification and
retain the published baseline evidence until that replacement is qualified;
then retire equivalent superseded outputs. Nothing removed, zero reclaimed
bytes claimed. Final handoff measures ending free space after docs/Git writes.

Material-pass verification resumed after published status `eccddc0`: previous
turn was progress (build completion established and status published). Current
preflight 65,233,252,352 B free; all prior producers terminal, owner profile
intact, no source changes since host build 36. Reuse that executable, CPU 29
and the same supervisors/budget; no rebuild. Two capture-disabled runs (normal
flat with one bounded window image, then desktop XR parameter comparison) may
add <=1 MiB runtime logs, <=1 MiB perf and <=3,817,185 B image overlap. Retire
equivalent flat/XR/build/CPU evidence only after checking replacements. Existing
raw archive remains over budget and has **zero new raw-frame allowance**.

Flat 883 / PID 27228 / session 16292 passed, one inspected 3,364,482 B PNG.
Ten superseded flat/build/CPU files removed after replacement validation:
4,237,027 B logical, immediate free 65,208,442,880 ->65,212,686,336 B,
4,243,456 B measured reclaimed. XR 884 / PID 26416 / session 99036 completed
with 303,353 matching parameters and 301 water updates, but the last parameter
sample predates the qualified field marker and the last camera sample remains
startup. Do not retire previous field-XR log 881 yet. Tighten the existing
verifier to require a nonzero field-camera marker and subsequent fresh matching
parameters; one capture/perf-disabled retry, 45 s /400 KiB runtime log limit.
Total new runtime logs remain budgeted within the existing <=1 MiB estimate,
including both attempts. No new image/raw/build allowance or new checkpoint.

The drive lost 19,922,944 B between flat completion and the cleanup precheck;
scoped verification/runtime/perf/cache/HLSL inventories find only the named small
outputs, zero new cache/HLSL files or raw captures. This unexplained volume
activity is not attributed to renderer output or credited as cleanup. Original
cumulative reserve/growth guards remain active. XR 884 ending free 65,212,301,312 B.

Stronger XR 885 / PID 18080 / session 13685 terminal 14:29:46, profile restored:
field-camera sample 14:29:40 precedes 2,083,519 matching parameter blocks at
14:29:45; zero full legacy imports or material-pass fallback/refusal/faults.
601 water updates sampled. Actual verifier rejects stale 884 and accepts 885.
All 17 settings audited; no error/config matches, raw or XR perf outputs.
Exact source mapping, flat/XR counters, hashes and remaining qualification are
recorded in `20260906_1429_native-material-passes.md`.

After validating replacement 885 and exact ignored/reparse-free paths with no
active producers, removed old field-XR 881 and superseded early-stop 884:
641,593 B logical; immediate free 65,211,326,464 ->65,211,969,536 B, **643,072 B
reclaimed**. Material checkpoint cleanup totals 12 files /4,878,620 B logical /
**4,886,528 B measured reclaimed**, counted once. No protected evidence or trees
removed. Gross new log/perf/PNG payload 4,943,337 B; net retained +64,717 B for
new binding counters, stronger field-XR sampling and replacement PNG size.
Retire equivalent current evidence after future qualification; no copied sets.
CPU fixture +22,528 B and small bounded-verifier edits fit the tools reservation;
host +8,192 B. Raw/cache/HLSL/cooked output allowance is unchanged.

Reserved diagnostics 64,454,021 B: 94 build logs /140,120 B, 14 runtime logs /
3,994,262 B, 20 perf files /8,963,168 B, eight GPU fixture files /8,364,855 B,
fixed 41 MiB tools/inspection. Two PNGs /6,698,638 B within that reservation.
All three runtime attempts total 966,032 B within the <=1 MiB estimate.
Post-cleanup free 65,211,969,536 B, drive-wide use +24,227,840 B from material
preflight (+21,282,816 B this continuation), including the unexplained volume
drop above. Later source/docs/Git writes count; final handoff measures again.

## Continuing the same storage ledger: Toon material callbacks

Previous turn was progress: published `0954377`, final free 65,211,736,064 B;
material checkpoint drive-wide use +24,461,312 B, no extra cleanup credit.
Current Toon preflight 65,212,903,424 B, no active producer, original profile
intact. The small volume gain is not cleanup. Existing diagnostic reservation
64,454,021 B and original cumulative limits remain unchanged. Two current PNGs
total 6,698,638 B, leaving 3,787,122 B temporary replacement overlap.

Three whole callbacks (Toon update/begin/end) now have native implementations;
pass dispatch routes recognized callbacks directly and counts remaining guest
bodies separately. CPU cases cover signed frame selection/wrap, all 6,804 normal
counter states, live image/next-counter changes and edge-word conversion.
115 source guards pass. Reuse CPU/host trees: <=256 MiB build/link overlap and
<=1 MiB new aggregate build/CPU logs. After builds, bounded normal flat plus
fresh-field desktop XR checks may add <=1 MiB runtime logs, <=1 MiB perf and
one PNG <=3,787,122 B. XR may compare edge words against the original leaf,
without double GPU execution. No shader/guest rebuild, new tree, download,
asset/cache conversion or raw frames. Retire equivalent old flat/XR/build/CPU
outputs only after validated replacements; preserve distinct non-MSAA/original
UI/failure evidence. All attempts and continuations share this ledger/budget.

### Toon qualification and cleanup, 15:05 continuation

User strategy interruption paused runtime verification. Prior CPU fixture 08
(PID 26400), host 37 (PID 21020/session 66487) and CPU suite 30 (PID 26032) were
confirmed terminal, exit 0; six logs total 6,523 B. Source checks remain 115/115;
CPU suite 31/31, 6.94 s. No duplicate build was launched. Continuation preflight
free 65,207,963,648 B, no active producer and original 116-byte profile intact.

Flat 886/PID 27108/session 83110 terminal 15:03:52, 282,570 B log,
598,128 B perf/metadata, 3,333,941 B inspected PNG. XR 887/PID 28072/session 83298
terminal 15:04:53, 365,625 B log, no perf/raw. Fresh scene/camera checks match
1,510 Toon publications and 2,627,009 native parameter blocks; zero mismatches
or Toon fallbacks/refusals/faults. Both profiles restored byte-for-byte. No new
raw/cache/HLSL/cooked outputs observed. All runtime attempts total 648,195 B.
Gross new log/perf/PNG payload 4,586,787 B; details, hashes, limits and remaining
qualification in `20260906_1505_native-toon-materials.md`.

Replaced equivalents qualified before cleanup. Removed 11 exact ignored files:
host 36, fixture 07 and CPU 29 stdout/stderr; flat 883, XR 885, perf-142258
CSV/metadata and material-pass PNG. No active producer/reparse ancestor; no
protected evidence, original data or build tree removed. Logical 4,625,184 B;
immediate free 65,201,819,648 ->65,206,452,224 B, **4,632,576 B reclaimed**, once.
Net retained diagnostic payload falls 38,397 B. Host +9,216 B, CPU fixture
+12,800 B and small supervisor edits remain within the tools/build reservation.
Ending post-cleanup volume use +6,451,200 B from Toon preflight, including
unattributed drive-wide movement; not all volume change is task output. Final
source/docs/Git writes are measured at handoff. No budget reset or new exception.

### Static-object dependency index, source-only continuation

Previous turn made progress: Toon `f956abd` and workflow/docs `99e6757` were
published, worktree clean, all producers terminal and profile restored. Final
free was 65,178,120,192 B (+34,783,232 B drive-wide use from Toon preflight).
Scoped verification/log/cache/HLSL checks found no new outputs after 15:05;
26 new Git objects totaled 128,445 B. The remaining drive movement is not
attributed to the renderer or claimed as cleanup. Earlier savings count once.

Current read-only preflight: 65,176,121,344 B free, no relevant active producer;
94 retained build logs total 140,140 B. The existing call graph is 2,375,990 B
and lacks recursion, source locations, indirect and instruction-hook metadata.
The new parser was verified in memory against all 18,777 generated bodies;
eight tiny fixture tests pass, with no surviving temporary files. No host/guest
build, game run, raw image, asset conversion, dump or new diagnostic log is needed.

Reserve at most 8 MiB additional overlap for one explicit bounded replacement
of `out/callgraph.json`, plus <=200 KiB total temporary test fixtures. The old
cache remains until atomic replacement succeeds; oversized/low-space/error
cases retain it. New retained growth, if any, is for source/indirect/hook
coverage and reusable indexing, not a per-run archive. Keep one current index;
replace it on source/schema changes, never retain a series. Tools/source edits
fit the existing reservation and cumulative limits; final sizes/free measured.

Index processes 52853, 73911 and 45602 are terminal (memory-only, schema 2,
then corrected schema 3). Ten fixture tests pass; real source excludes trailing
hook prototypes, retains recursion and recognizes Toon macro declarations.
Final index 8,249,479 B versus original 2,375,990 B: **+5,873,489 B retained**
for source/indirect/hook coverage and fast reuse. Schema 2's 8,260,958 B file
was atomically superseded; no partial or index generation archive remains.
Corrected retry peak: index+temporary <=16 MiB total; actual payload pair
16,510,437 B, or 14,134,447 B additional over the original cache, rather than
the initial 8 MiB additional estimate. Actual peak free space was not sampled;
the cumulative 2 GiB/100 MiB limits remain sufficient. No more index writes
are needed this checkpoint. Temporary fixture allowance remains <=200 KiB.

Pre-publication free 65,139,838,976 B, +36,282,368 B drive-wide use from this
preflight; the amount beyond index/source edits is unattributed, not renderer
output or cleanup savings. No game/build/capture/cache-asset outputs were
produced, no profile changes. Source map and exact evidence are in
`20260906_1531_static-model-ownership-frontier.md`. Final Git writes measured
at handoff; no cleanup credit beyond the already recorded prior checkpoint.

### Load-owned model material records, same checkpoint

Previous goal turn made progress through source-index publication `1482778`.
The intervening read-only strategy review confirmed the 150 s autoplay walking
threshold versus 75 s flat smoke duration; that is not movement/transition
qualification. Current worktree was clean and no build/game producer was live.
Preflight free: 65,140,396,032 B. The existing original checkpoint limits and
raw-capture prohibition remain unchanged; this is not a budget reset.

Implement load-owned primitive/material records and replace draw-time command
discovery. Reuse the native material test and desktop host build trees. Reserve
<=256 MiB incremental compile/link overlap and <=1 MiB new aggregate build/test
logs, within the existing 2 GiB peak /100 MiB diagnostics/10 MiB log ceilings.
CPU ownership fixtures add no persistent test data. No new shader/guest build,
raw captures, downloads or asset bulk conversion. Game/pixel verification needs
its own preflight after the focused checks, reusing bounded supervisors and
qualifying replacements before any predecessor cleanup.

Material fixture build 03 / CPU 03 pass (0.13 s suite); core owner/tests published
as `d723d70`. Host 38 failed on two missing original-entry declarations (no
guest object rebuild); fixed in host 39. Host 40 includes the final checked node
span, terminal exit 0, original codegen reports up-to-date/no files written.
The shared lease core stays source/GPU/disk independent; full static-object
geometry/instance/direct submission remains pending.

Live verification preflight: free 65,115,238,400 B; no active producer. Updated
bounded tool reservation from 41 to 48 MiB to include the source index growth
and <=1 MiB new fixture allowance. Actual supervisor reservation now 71,797,163 B
versus 75 MiB stop /100 MiB hard diagnostic ceiling. Permit one <=45 s flat
model/material comparison with PSO precaching disabled, <=400 KiB log, no perf
or raw output, and one readiness-triggered window PNG <=3,817,663 B remaining
overlap. Reuse the currently absent `native_material_pass_window.png` path;
the old file was already retired in the Toon checkpoint. Existing material
files remain reusable; newly reached materials obey their independent 1 MiB /
4096-file/reserve limits, with 25 MiB supervisor headroom including small-file
allocation. No bulk geometry or texture cooking is requested. The actual
supervisor comparison gate passes seven in-memory fresh/stale/missing/later-
failure fixtures. Restore the exact owner profile in guaranteed cleanup.

Run 888 terminal (PID 27768/session 60001, 16:02:26-16:02:55), all seven
settings audited including PSO precache off; 114 published models /738,768 B,
14,208 lookup hits, zero missing/load/input/budget failures or unsupported
meshes. Diffuse 3,951 and specular 3,467 original comparisons match; reflection
has zero checks. **The inspected image is the opening cinematic, not interactive
field qualification.** Water activity is an insufficient scene marker. This
finding corrects the attempted readiness inference; it does not requalify the
older field-camera/both-eye claims or resolve older scenery/text failures.

Retained log 110,488 B and PNG 1,794,992 B; six newly reached material files
total 408 B (36 files /2,448 B now). No raw/perf output, profile restored. Host
41 adds diagnostic-only context from existing Game/Field/Cutscene/Movie readers.
The updated supervisor requires FieldActive, field state 4, named stage, live
player, no event/movie, and following matching model/material samples; nine
in-memory fixtures reject missing/stale/loading/event/later-failure contexts.
Reserve a second <=75 s, <=400 KiB, capture/perf-disabled comparison (no image),
sharing the original budget. Do not relaunch the earlier raw/VR matrix. The
opening-cinematic image and unchanged Toon normal-field/VR evidence remain
distinct; no passing-field image replacement or cleanup is claimed yet.

Run 889 terminal, PID 24436/session 10626, 16:07:31-16:08:47, 291,441 B log:
**field gate failed** (Loading / state 0, `bg41_01`, event clears; do not infer
interactive readiness). Cumulative diffuse 126,735 and specular 120,369 checks
match, reflection zero checks; 114 publications/one retirement, 488,116 hits,
zero missing/load/input/budget failures. Profile restored; no new raw/perf/image.
120 source guards pass. No further producer is running or automatically queued.

All current-turn attempts produced 11,719 B build/test logs +401,929 B runtime
logs +1,794,992 B PNG =2,208,640 B gross diagnostics. Ten exact obsolete logs
were safely removed after CPU 03/host 41 replacements passed: old material
build/CPU 02 and host 38/39/40 stdout/stderr, logical 10,405 B. Immediate free
65,095,716,864 ->65,095,737,344 B, **20,480 B reclaimed**, counted once.
Net diagnostic retained growth 2,198,235 B for new cinematic material/pixel
coverage and the failed semantic field gate, with replacement/diagnosis as the
cleanup trigger. Material assets +408 B; protected normal flat/XR/failure/raw
evidence unchanged. Ending post-cleanup drive-wide use +44,658,688 B from this
continuation's preflight; source/build/Git/other-process use is not inferred to
equal diagnostic payload. Exact identities and limitations are recorded in
`20260906_1610_load-owned-model-materials.md`. Final Git writes still count.

### Corrected field observations, same checkpoint

The preceding read-only strategy review yielded actionable source evidence
(hidden icon handle and 150 s walking delay), not a renderer conversion. This
continuation corrected icon/strip visibility and the loader's 128-slot scan;
the generated fade-state writer/update confirm idle state 0, not 4. Native model
geometry/instance/submission ownership remains next; full desktop/both-eye gates
are unchanged. See `20260906_1638_field-state-observations.md` for source evidence.

Preflight free **65,061,658,624 B**. No live producer; original profile intact.
Reused host/native_texture build trees, <=256 MiB compile/link overlap and <=1 MiB
new build/test logs reserved, shared with all prior attempts and existing limits.
Fixture 01 cmake PID 29180/session 77239 stalled after regeneration; its exact
verified process tree was terminated, exit 1, before retry. Fixture 02 PID 19804
passed; CPU 01 PID 30176 passed (0.07 s). Host 42 PID 30204/session 9296 passed
(roughly 16 s log span), no guest/shader rebuild. All handles are terminal.
120 source guards and 15 in-memory actual-runner gate cases pass. No test data
files or source-index writes. New loader exe/PDB/object **718,242 B**, retain one
reusable fixture and replace it on change. Runner reservation increased from
48 to 49 MiB to account for this addition; 75 MiB stop/100 MiB hard diagnostic
and 2 GiB original checkpoint limits remain unchanged.

Runtime preflight free **65,059,090,432 B**, no live build/game producer. Existing
49 MiB reservation plus retained diagnostics is 73,257,385 B before run/cleanup,
leaving 5,385,815 B below the 75 MiB stop. Allowed one <=75 s PSO-off material
diagnostic, <=400 KiB log, no perf/raw/image capture. Run 890 PID 29612/session
98449 terminal at 16:37:38 after about 51 s; original 116-byte profile restored.
The opening event is observed before two idle `bg41_01` contexts at frames
1768/2068, five seconds apart. Their delta is 15,124 diffuse /14,514 specular
matches and 58,572 model hits; zero wrong/missing/load/unsupported/input/budget
failures. Reflection unexercised; no movement/input or new pixel qualification.

New build/runtime log payload totals **195,304 B** including the stalled attempt.
After replacement passed, removed five exact inactive non-reparse files: run
889, host 41 stdout/stderr, loader fixture 01 stdout/stderr. Reader failure is
diagnosed and retained in reports; no unresolved rendering evidence was deleted.
Logical removal **292,376 B**. Immediate free **65,049,759,744 ->65,050,058,752 B**:
**299,008 B measured reclaimed**, once. Net log retention **-97,072 B**; new
fixture yields **+621,170 B** diagnostic retention for new behavioral coverage.
All raw/PNG/game/active-build/normal-Toon flat-XR evidence remains unchanged.
Post-cleanup drive-wide use **11,599,872 B** from this preflight, not equated with
task payload. Final source/docs/Git writes still count; no producer left running.

### Bounded native mesh storage, same checkpoint

Previous goal turn progressed through `69de326`, correcting field observations.
The next geometry prerequisite was the unbounded native mesh disk writer, now
replaced in the active importer. Limits: 256 MiB logical /16,384 files, 20 GiB
reserve plus whole incoming payload/64 KiB metadata headroom; non-waiting lease,
restart accounting, no eviction/reparse traversal/conflicting valid overwrite.
Existing keys/formats/uploads remain unchanged. Geometry still imports through
guest wrappers during replay; native model/instance/direct submission remains.
Full source/test details: `20260906_1701_native-mesh-storage.md`.

Initial inventory: **65,047,642,112 B** free, cache **3,510 files /36,510,144 B**.
Build preflight **65,019,232,256 B**; permission-enabled launch free **65,005,232,128 B**.
Intervening source-work decrease is unattributed; scoped recent outputs and
authoritative process checks found no renderer/build producer or large new
verification/runtime output. Reserve <=256 MiB compile/link and <=4 MiB test/log
growth, no raw/game-run/asset-cook allowance or new build tree. Original ceilings
and protected evidence remain unchanged.

Mesh fixture 01 PID 30744, CPU 01 PID 27972 and read-only cache 01 PID 29548
(session 26779) passed. Host 43 PID 27644/session 14558 passed, roughly 13 s,
no guest objects/shaders rebuilt. Reconfigured only the existing mesh fixture
as Release: configure 01 PID 30920, build 02 PID 28984, CPU 02 PID 27972 and
cache 02 PID 24412 passed. Final exception-cleanup build 03 PID 30212, CPU 03
PID 27704 (0.12 s), cache 03 PID 29512 passed. PID reuse is chronological, not
one surviving job. All handles are terminal; all 124 source guards pass.
Read-only validation loaded every existing mesh unchanged, without source/GPU
or writes. No game/VR/capture/perf/new asset output; owner profile unchanged.

Fixture before: 3,402,863 B; expanded Debug peak 7,038,207 B; final Release
exe/objects **781,738 B**, **-2,621,125 B** versus the original. New aggregate
logs **7,971 B** across every attempt. After final verification, deleted the
unused Debug PDB and sixteen exact superseded logs (mesh build/CPU/cache 01/02,
configure 01 and host 42 pairs), **3,158,859 B logical**. Immediate free
**64,970,772,480 ->64,973,938,688 B**, **3,166,208 B actually reclaimed**, once.
No active tree/game/required evidence was removed. Final tool+log retained
payload shrinks **2,634,477 B**; keep one current mesh fixture, not debug/release
duplicates. No increased runtime diagnostic reservation needed.

Post-cleanup drive-wide use **73,703,424 B** from the initial inventory, including
unattributed changes outside the measured shrinking task payload. Existing raw/
PNG/normal field-XR/failure evidence unchanged. Final source/docs/Git writes follow;
no new raw allowance, budget reset or full native-frame claim.

### Load-owned geometry, same checkpoint

Storage prerequisite published as `cf95217`; actual geometry/association producer
and four consumers now advance at model loading. Host 47, Debug/Release material
fixtures, 130 source guards and post-event field run 893 pass; 2,973 load-owned
primitive geometries, fresh matching geometry/material deltas and one inspected
bounded JPEG. Runs 891/892 failed the freshness/image-helper gates respectively,
were diagnosed and replaced, not reported as complete qualification. All owned
producers are terminal and every temporary profile was restored. Full source,
binary hashes, PIDs, settings, limits and failures are recorded once in
`20260906_1743_load-owned-model-geometry.md`.

Initial free 64,970,539,008 B; <=256 MiB build overlap and <=4 MiB new fixture/log
growth reserved. No raw/perf/cooked-cache/dump output or new build tree. Existing
mesh cache unchanged at 3,510 /36,510,144 B; runtime comparison suppresses writes.
The fixture shrinks from 15,101,171 to 2,613,079 B. Retained log 893 /179,253 B
and JPEG /373,929 B replace prior scoped counter evidence and add geometry pixels;
old normal flat/XR and unresolved failure evidence stay protected.

Two validated exact cleanup batches removed 25 files, 4,906,121 logical B:
obsolete Debug symbols/object and superseded build/test/runtime logs. Immediate
free-space measurements establish **4,931,584 B reclaimed**, once. Fixture plus
runtime/image/build-log diagnostic categories shrink by at least 12,108,024 B;
no increased tool reservation. Keep final material build 06 /CPU 05 /host 47
logs, not every attempt. New JPEG shares the existing 10 MiB image reservation.
Post-cleanup free 64,961,671,168 B; drive-wide use 8,867,840 B from this inventory,
distinct from the smaller retained task payload. Final docs/Git writes remain
charged. Native instances/layouts/direct draws and full desktop gate remain.

### Native instance render poses, same checkpoint

Continued the unpublished instance draft on `ad5aa25`, following a read-only
planning turn. Final handoff publication includes late pose writers and the
actual derived callback's dirty gate. Host 53, Release CPU 09, 137 source guards,
11 scenario cases and flat run 900/image pass. Fresh delta 118,851 matching pose
reads, no pose misses/refusals/drift; 239 live /368,896 B. Full evidence and failed
attempts 894..899: `20260906_1850_native-instance-render-poses.md`. No full native
pose calculation, movement/reload/both-eye/complete-frame qualification claimed.

Draft start free 64,957,763,584 B; original <=256 MiB overlap /<=4 MiB fixture/log
plan and cumulative ceilings remain. No raw/perf/cache/shader dumps or new trees;
mesh cache unchanged at 3,510 /36,510,144 B. Final fixture payload 2,768,014 B.
After validating run 900 and its 399,365-byte image, deleted 36 superseded exact
logs/old geometry JPEG, 1,739,573 logical B. Free 64,882,774,016 ->64,884,563,968 B:
**1,789,952 B measured reclaimed**, once. Fixture/log/image/build-log retention
grows **204,331 B** for new boundary coverage; one current representation retained.
Keep final build 53 /material 10 /CPU 09 and run 900/image, not every attempt.
Prior protected raw/normal flat-XR/unresolved broad failures and game data remain.
Post-cleanup drive-wide use 73,199,616 B from draft start, not all attributable to
the task. Source/docs/Git writes follow; no new raw allowance or budget reset.

### Load-owned texture tables, same checkpoint

Continued the unpublished table draft on `e85b290` after a read-only planning
review. Completed synchronous/asynchronous table publication, separate teardown,
immutable image leases and atomic mirror-to-table publication now feed normal
host lookup. Source-selected return ABI, resource/dynamic overrides, direct
object submission and the full desktop gate remain. Host 59, binding CPU 05,
material CPU 10, 146 source guards and 18 scenario cases pass. Comparison run 905
adds 22,326 matching lookups /22,006 image checks; normal-table run 906 adds 21,745
lookups with zero original comparison/fallback/refusals and an inspected image.
5,736 live tables /2,172,192 B. Every owned session is terminal, every temporary
profile restored. Full evidence/failures: `20260906_1955_native-texture-tables.md`.

Draft preflight **64,571,400,192 B**; this continuation pre-build **64,515,645,440 B**.
Same <=256 MiB build overlap /<=4 MiB fixture-log plan and original cumulative
ceilings. Zero new raw/perf/cache/dump files, no new tree or asset cooking; mesh
cache unchanged at 3,510 /36,510,144 B. New binding exe/object **2,186,660 B**,
953,901 B growth; concurrent snapshot/retirement/8,192-list coverage retained in
one fixture. The table RAM byte budget stays 16 MiB; the record limit corrects
2,048 to16,384 after source/runtime proof of thousands of distinct inline lists.

After validating replacements, removed **39 exact superseded logs/JPEGs**,
**1,478,619 logical B**. Immediate free-space measurements reclaim **1,527,808 B**,
counted once. Keep comparison/normal logs 905/906, normal table JPEG and final
host59/binding05/material11-CPU10 logs. Prior normal flat/XR, broad failures,
raw archive and game data remain protected. Comparable retained fixture/log/image
set grows **1,163,567 B** for new lifecycle/concurrency and comparison/normal
coverage; replace by verification purpose, not per attempt.

Post-cleanup free **64,424,288,256 B**; drive-wide use **147,111,936 B** from this
draft's preflight, not all attributable to the smaller retained diagnostic growth.
Scoped producer/output checks leave the intervening drive-wide difference
unattributed; do not call it cleanup. Final source/docs/Git writes remain charged.
No new raw allowance, budget reset, Quest run or full native-frame claim.

### Native vertex-input ownership, same checkpoint

Preflight 2026-09-06 20:18 EDT: **64,168,857,600 B** free, no renderer/build
producer. Original checkpoint ceilings/floor and zero new raw allowance remain.
Plan <=256 MiB build/link overlap, <=4 MiB fixture/log growth, reuse existing
host/mesh/texture trees. No guest/shader rebuild or cache/cooked-asset writes.
One <=1 MiB normal-field JPEG replacement may overlap the current table image
until validated; existing total-image cap remains 10 MiB. Keep one latest
passing image by purpose and retire only superseded agent logs after validation.
The runtime input owner removes declaration dereferences in pipeline creation,
shared decoding and pulling; packed vertex bytes, shader ABI and templates remain.
Host60 (44.663 s, no guest/shader rebuild), mesh CPU04, draw-intent fixture,
152 guards and 23 scenario cases pass. PSO-off run907 and precache-on run908
pass fresh field checks; 908 adds 170,020 native pulled records, with instancing/
indirect calls active and inspected 1920x1080 pixels. Every producer is terminal
and the exact owner profile restored. This is not direct static drawing or full
desktop/both-eye/movement qualification. Details: `20260906_2040_native-vertex-inputs.md`.

No new raw/perf/cache/dump files; mesh cache unchanged at 3,510 /36,510,144 B.
Mesh exe+five objects now 865,196 B. Known exe/main/new-input-object growth
83,458 B; other modified fixture/build metadata lacks a complete before baseline.
Comparable runtime-log/build-log/image set grows 233,145 B for new input and
precache-off/on coverage. Aggregate build logs 175,358 B, below 10 MiB.
After validated replacements, removed nine exact superseded logs/JPEGs,
992,997 logical B; measured reclaim 614,400 +393,216 = **1,007,616 B**, once.
Keep logs907/908, current pulled-input JPEG and final host60/mesh04/intent01
logs. Older texture comparison905 and protected flat/XR/broad failures remain.
Post-cleanup free **64,135,020,544 B**: 33,837,056 B drive-wide use from this
preflight, not all attributable to retained outputs. Source/docs/Git writes
follow. Original checkpoint ceilings, zero new raw allowance and full goal remain.

### Readiness-driven desktop walking, same checkpoint

Parent b24e545. Reused the loader CPU fixture and current host tree; no renderer
ownership is claimed. Native policy replaces the fixed 150-second walk delay
with stable field/input readiness, and verifies observed displacement separately
from stick input. Host63, loader04/CPU03, 152 guards and 28 scenario cases pass.
Runs909/910 diagnosed selected Mindows panel versus actual overlay visibility;
run911 /PID3732 passes fresh motion/native-component checks and three inspected
1920x1080 images. All sessions terminal and profiles restored. Full evidence and
limits: `20260906_2120_readiness-driven-autoplay.md`.

Preflight **64,093,036,544 B**; planned <=128 MiB host/link overlap, <=2 MiB
fixture/log growth and <=1.5 MiB JPEG overlap, within original cumulative ceilings.
Before the host build, drive free fell to63,789,682,688 B; scoped inventories
found only18 recent files /1,841,248 B and no matching large producer. The larger
drive-wide change remains unattributed, not a new allowance. Existing stop floor
63,583,739,904 B was enforced by every producer. No new raw/perf/cache/dump files,
no downloads/new tree, mesh cache unchanged at3,510 files /36,510,144 B.

Reusable loader/autoplay fixture exe/PDB/two objects **1,483,285 B**, growth
**765,043 B** over the recorded718,242 B predecessor. Three new motion JPEGs
**1,255,213 B**; keep for actual movement/pixel coverage, replace when equivalent
motion evidence passes. The standing vertex-input image remains the comparison
reference for unqualified cliff-edge artifacts/blur; do not silently delete it.
Aggregate window images **10,106,400 B**, close to the unchanged10 MiB cap.

After validating replacements, deleted **16 exact superseded logs**: host60..62
stdout/stderr, loader builds02/03 stdout/stderr, loader CPU01/02 stdout/stderr,
and failed runs909/910. Their source diagnosis/hashes remain in research; exact
old logs are gone, equivalent diagnostics can be regenerated. Logical removal
**687,078 B**; immediate free63,739,285,504 ->**63,739,985,920 B** reclaims
**700,416 B**, counted once. No game/build/cache/raw/protected image data removed.
Final retained build logs146,016 B (29,342 B less than the175,358 B preflight).
Run911 log229,708 B is retained. Comparable fixture/log/image set grows
**2,220,622 B** for new movement/lifecycle evidence; small helper/build-metadata
and source/Git changes are not fully baselined, so this is not total task growth.

Post-cleanup drive-wide use **353,050,624 B** from preflight, mostly not explained
by the scoped retained outputs; do not label the difference task artifact bytes.
Source/docs/Git writes follow and remain charged. Broader normal flat/XR and
unresolved failure evidence stays protected. No budget reset or Quest work.

### Canonical rigid mesh values, same checkpoint

Parent9ab1bbd. BDMESH v2 and source-free geometry loading advance the static
object data contract; full link/pixels remain pending, not qualified by CPU tests.
Evidence: `20260906_2158_canonical-rigid-mesh.md`. Original cumulative ceilings,
supervisor floor63,583,739,904 B and zero new raw allowance are unchanged.
Preflight free **63,733,166,080 B**; initial <=4 MiB fixture/log growth and
<=128 MiB host/link overlap plan. The actual exe/PDB pair totals154,512,384 B;
a future complete host build should allow <=192 MiB overlap including changed
objects, rather than assume the original128 MiB estimate covers both files.

Only the existing mesh fixture was built. Build05 macro failure and CPU05 bad
half-float fixture were corrected; final build08/CPU07 pass (0.11 s CPU), as do
155 guards, four no-output host syntax checks and cache verify04 (3,510 v1 files,
36,510,144 B, read-only). No host/game/Quest run, image, raw/perf/cache output or
profile change. Host63/run911 remain the previous executable/runtime evidence.
All started producer handles are terminal; no duplicate jobs remain.

Drive free dropped beyond what the scoped outputs explain. At21:48 it was
63,642,419,200 B, about56 MiB above the operational floor; the full link did not
fit and was not launched. Requested approval to increase the cumulative cap to
3 GiB, still preserving20 GiB free and no new raw allowance; pending. Continued
small CPU/read-only checks under the unchanged floor, not a budget reset.

After replacement validation, removed **16 exact superseded fixture logs**:
mesh builds04..07, CPU04..06 and cache verify03 stdout/stderr. Logical7,919 B;
immediate volume free63,626,665,984 ->**63,626,686,464 B**, recovering **20,480 B**
once. Keep final build08/CPU07/cache04 instead. Old logs are no longer retained;
their diagnosed failures and reproducible checks are recorded. No protected
game data, builds, images or raw evidence removed.

Complete reusable mesh fixture tree **1,412,368 B**, versus measured1,151,798 B
before this implementation: **260,570 B growth** for canonical data/input/disk
coverage. Aggregate build logs145,872 B, down144 B from146,016 B preflight.
Comparable retained fixture/log growth is **260,426 B**; source/docs/Git and small
external metadata are not fully baselined. No new images; aggregate remains
10,106,400 B. Replace the same fixture and final logs at the next equivalent check.
Post-cleanup drive-wide use **106,479,616 B** (~101.5 MiB) from this turn's
preflight remains mostly unattributed; do not call it task output or reclaim.
Source/docs/Git writes follow and remain charged to the original checkpoint.
Before publication the volume fell further to **63,581,585,408 B**, below the
operational stop floor, despite zero new raw/perf/cache/dump files and no active
producer. Only low-storage documentation/Git work continues; no host link,
fixture retry or runtime job may launch under that floor. The 2 GiB hard ceiling
has not been increased and the owner budget question remains pending.

### Native pulling default-lane correction, same checkpoint

Parented2e884. Start free **63,426,617,344 B**, still below the unchanged
63,583,739,904 B operational floor; no build/run/capture launched. Source review
found native synthetic entries lose their format and reach BD_PullF's w=1
default, inconsistent with zero secondary-position blend weights. Native input/
staging now retains the format and uses the existing64-byte zero buffer with
STORAGE usage and zero stride. Shader payloads and game data are unchanged.
Evidence: `20260906_2211_native-pull-defaults.md`.

Three host and two fixture syntax checks pass, including compile-time encoding
assertions;156 source guards pass. Updated C++ runtime tests, full linking and
GPU/pixel qualification remain pending; do not relabel the preceding fixture
binary or host63/run911 as verification of this changed source.

Read-only process counters identify another project's Python data workflow
writing10,170,513 B/s at one sample. Its script is outside this workspace; no
foreign files were read/changed and no process was stopped. This is a likely
contributor to drive-wide changes, not an exact attribution permitting a budget
subtraction/reset. Pagefile allocation2,048 MiB was sampled once, with no prior
allocation baseline. Owner approval for the proposed3 GiB cap is still pending.

At22:11 free **63,516,618,752 B**, drive-wide gain **90,001,408 B** from this
turn's preflight, unrelated to cleanup here. **Zero bytes reclaimed**; no new
build/test logs, binary, image, cache, dump or raw output. Source/docs/Git writes
are small but not fully baselined and remain charged. Existing fixture and game
executables/profile are unchanged; protected evidence remains. No goal completion
or new resource allowance is claimed.

### Owner-approved verification resumption, same checkpoint

On 2026-09-06 the owner approved increasing **this existing cumulative peak cap
to3 GiB**, retaining the20 GiB free-space reserve and raw-capture ban. This is
not a change to AGENTS' default or a new checkpoint. Original starting free
**65,462,788,096 B** remains the ledger anchor. The three active producer
wrappers now stop at **62,509,998,080 B**, allowing2.75 GiB growth with256 MiB
headroom before3 GiB; their21 GiB reserve is stronger than the20 GiB minimum.
Diagnostics100 MiB, build/test logs10 MiB, window images10 MiB aggregate and
**zero new raw allowance** are unchanged. No prior cleanup is credited again.

Resumption preflight free **63,222,808,576 B**; no renderer/compiler/test process
observed. Reuse the mesh fixture and configured host tree. Plan <=192 MiB host
compile/link overlap and <=4 MiB fixture/log growth. Rebuild changed CPU tests
first, then host64, with no guest/shader rebuild or asset-library recook.
One new canonical-mesh sanity JPEG may use <=300 KiB within the existing image
cap (10,106,400 B retained,379,360 B available). Preserve the standing and
three-motion-image baselines; a single new image cannot qualify a sequence.
Runtime diagnostics must require fresh canonical draw activity, keep raw/perf/
cache writes disabled and restore the exact owner profile on every exit.
Validate replacements before retiring only superseded agent logs. Account for
actual results/free space below before another producer or publication.

Resumption results: mesh09 /PID31092, CPU08 /PID30632 and host64 /PID27696
all pass; no guest object/shader rebuild.156 boundary checks and34 scenario
cases pass. Flat run912 /PID27288 (23:12:47..23:13:58) adds fresh canonical
draw/pulling, matching geometry/material/pose and actual movement observations.
One210,325-byte1920x1080 JPEG was inspected; not a new sequence or full native
object qualification. Source-free GPU loads remain zero. All producers terminal,
owner profile restored exactly. See `20260906_2316_canonical-rigid-desktop.md`.

No new raw/perf/cache/dump files. The existing mesh cache is unchanged at3,510
files /36,510,144 B. Current window images total **10,316,725 B**, leaving
169,035 B under the unchanged10 MiB aggregate cap. Retain the new JPEG and
run912 log202,300 B for canonical-component coverage; prior standing/failure
and three-image movement baseline are not superseded by this single image.
Replace only after equivalent coverage is validated; no new raw allowance.

Cleanup after validation: removed mesh08/CPU07 stdout/stderr (1,931 logical B),
immediate free63,183,777,792 ->63,183,781,888 B, reclaim **4,096 B**. Removed
host63 stdout/stderr (744 logical B), immediate free63,246,290,944 ->
**63,246,295,040 B**, reclaim **4,096 B**. Total this resumption **8,192 B**,
counted once; six exact obsolete logs, reproducible from source. Final logs
mesh09/CPU08/host64 and cache04 remain; no protected images/data/builds removed.

Reusable mesh fixture now **1,419,621 B**, growth7,253 B from resumption baseline.
Aggregate build logs112 files /148,356 B, growth2,484 B. Comparable retained
fixture/log/image set including run912 grows **422,362 B** for new canonical
runtime/default-lane coverage. Game exe/PDB grow **176,128 B** combined; changed
objects, build metadata, helper/source/docs/Git bytes lack complete before
baselines and are not falsely reported as zero. No duplicate build tree.

Post-cleanup free **63,246,295,040 B** (~58.90 GiB), a drive-wide gain of
**23,486,464 B** from resumption preflight. This gain exceeds our8,192 B cleanup
and is not attributed to task reclamation; other processes continue changing
the volume. The lowest explicitly returned sample was63,110,840,320 B after
run912; producer supervision kept the operational floor enforced. Source/docs/
Git publication writes still count; preserve the original ledger and approved
3 GiB cap without resetting allowances at the next continuation.

### Load-owned shadow policy, same checkpoint

Parent ae04d1e. Source-only investigation followed the completed graph builder's
asset control table and E000 shadow policy consumer. Native programs now own
per-primitive Receive/Disabled/Unknown values; the draw adapter selects those
values without reading control words. Pass/instance visibility remains live.
Focused fixtures cover source destruction, missing data, conflicts, retirement
and reload. This is not complete native static-object submission.

Continue the original approved3 GiB ledger/floor62,509,998,080 B, diagnostics
100 MiB/logs10 MiB, images10 MiB and raw0. No reset or cleanup credit. Initial
read-only free63,129,739,264 B; later source-work sample63,424,847,872 B varies
with other drive writers. Plan material12/CPU11 in the existing fixture tree
(<=4 MiB growth), then host65 in the existing tree (<=192 MiB compile/link
overlap). Existing bounded wrapper supervises both. No guest/shader rebuild,
cache conversion or raw output. Validate replacement logs before retiring only
superseded successful agent logs; keep prior runtime/image evidence until its
equivalent is qualified. Runtime/pixels remain pending at this preflight.

Material12 /PID30504 stalled before any compiler or log output in the restricted
process environment. Read-only inspection confirmed only its idle Ninja child;
the exact owned tree was stopped, exit1. Retry13 /PID31504 outside that process
restriction passed in2.94 s wrapper wall; CPU11 /PID31416 passes (test0.10 s,
ctest0.13 s). Host65 /PID26664 passes; codegen module up to date, no guest object
or shader rebuild.157 source guards and39 scenario cases pass. Both attempts
count; the empty failed logs are not successful build evidence.

At23:40:51 free **63,420,157,952 B**. Material fixture6,962,620 B vs6,917,235 B
preflight (+45,385 B); aggregate build logs152,789 B vs148,356 B (+4,433 B).
Host exe/PDB grow35,840 B combined. No owned producer remains. Runtime913 plan:
<=75 s flat normal-MSAA/precache-on canonical/movement/material/pose/table check,
plus fresh owned-shadow lookup/receiver checks; <=400 KiB log, no raw/perf/cache/
dump output. New purpose-specific JPEG is1920x1080, quality60, <=160 KiB checked
in memory before writing;169,035 B remain in the unchanged10 MiB image cap.
Original profile hash is unchanged. Existing runtime wrapper enforces75 MiB
diagnostic stop plus25 MiB safety and the original cumulative free-space floor;
shutdown and byte-exact profile restoration are in finally blocks.

Run913 /PID26452 (23:41:21..23:42:19) passes the full requested field gate:
2,973 owned policies,15,019 fresh matching receiver checks,43,037 composed
replays, fresh canonical/geometry/material/pose/table/movement observations.
One1920x1080 JPEG134,082 B inspected; not a sequence/both-eye qualification.
All11 settings effective, original116-byte profile restored exactly, all owned
producers terminal. No new raw/perf/cache/dump files. Details and remaining
unsupported lookups: `20260906_2344_load-owned-shadow-policy.md`.

After validating equivalent replacement coverage, deleted the old canonical
sanity JPEG/run912 log, material11/CPU10/host64 stdout/stderr and empty stalled
material12 logs. Ten exact files417,565 logical B; immediate free63,419,695,104
->**63,420,121,088 B**, measured reclamation **425,984 B**, counted once.
Historical reports/hashes remain; exact old captures/logs no longer retained.
Current run913/image and material13/CPU11/host65 logs replace them. Run911's three
motion images, standing/failure evidence and protected raw archive unchanged.

Comparable material fixture/log/image set shrinks **7,771 B** after replacement:
fixture+45,385 B, run log+23,594 B, image-76,243 B, aggregate build logs-507 B.
The latter now147,849 B; images10,240,482 B,245,278 B remaining under10 MiB.
Exe/PDB grow35,840 B. Source/docs/Git/helpers, objects and build metadata lack
complete byte baselines; not falsely zero. End free at cleanup~59.06 GiB,
drive-wide gain290,381,824 B from the initial read-only preflight; only425,984 B
attributed to our cleanup. Keep the same original ledger/approved3 GiB cap.

### Object material texture publication, same checkpoint (2026-09-07)

Parent2aaf549. The previous goal turn made verified source/runtime progress and
pushed it. Current turn first measured free63,411,986,432 B, clean tree. Source
inspection found that null image selection keeps the prior binding and early
image overrides skip late overrides. A last-selector-only recipe is insufficient.
The new ordered program, checked object-level override importer, native table
leases and object-scope consumer preserve these rules; direct submission remains
the full next outcome, not replaced by a template-only completion claim.

Before outputs free **63,398,559,744 B**, material fixture6,962,620 B, aggregate
build logs147,849 B. Reuse material fixture14/CPU12 and configured host66. Plan
<=8 MiB fixture/log growth, <=256 MiB host compile/link overlap. Same original
starting free65,462,788,096 B, approved3 GiB cap, operational floor62,509,998,080 B,
diagnostics100 MiB/logs10 MiB/images10 MiB, raw0. Use the existing bounded wrapper
outside the restricted process context that stalled Ninja last time; no duplicate
job or guest/shader rebuild. GPU/pixel verification remains pending until focused
tests/build pass and its producer budget is reconciled. Keep prior run913/field
image and baseline/failure evidence until equivalent replacements are validated.

Material14/CPU12 and host66 pass (owned build PID30292 terminal, exit0).
Codegen reports its module up to date; no guest objects or shaders rebuilt.
162 boundary guards and44 scenario tests pass. Material fixture7,129,853 B
(+167,233 B), build logs155,971 B (+8,122 B); exe48,250,368 B/PDB106,885,120 B
(combined +410,936 B). The scoped cache/dump inventory has no new files and
there are no remaining renderer/compiler/linker/Ninja processes. Drive free
63,063,465,984 B is lower than the build's63,381,757,952 B terminal sample;
identified task outputs do not account for that drive-wide change. Other-volume
writes/unbaselined build objects are not falsely credited to cleanup or zero.
The same operational floor remains enforced, with553,467,904 B headroom.

Runtime914 plan: one <=75 s normal flat/MSAA/precache-on run, requiring fresh
post-event canonical/geometry/material/pose/table/shadow/movement gates and new
object publication/image/UV consumers. At most400 KiB run log and one1920x1080
quality60 JPEG <=160 KiB, within245,278 B remaining image overlap. No raw/perf/
cache/dump growth; original profile SHA remains2f1bc38d...f38e23b0. The existing
wrapper bounds aggregate diagnostics and cumulative free space, terminates only
its renderer and restores exact profile bytes in finally. Keep run913/image
until equivalent replacement coverage and actual pixels are validated.

Run914 /PID25128 (00:13:59..00:14:58) passes every requested post-event field
gate:33,600 fresh object publications,58,641 matching material-image/UV checks,
59,927 UV-composed replays,62,690 native image slots,0 mismatches/refusals;
canonical/material/geometry/pose/table/shadow/movement checks also pass. One
1920x1080 quality60 JPEG134,083 B inspected; background artifacts/blur remain,
not sequence/reload/both-eye qualification. All12 settings effective, exact
116-byte profile restored, no owned producer left, no new raw/perf/cache/dumps.
Full evidence: `20260907_0016_object-material-textures.md`.

After equivalent replacement validation, removed eight exact superseded outputs:
run913 log/shadow sanity image and material13/CPU11/host65 stdout/stderr.
364,409 logical B; immediate free63,459,553,280 ->63,459,926,016 B, measured
reclaimed372,736 B, counted once. Prior reports/hashes remain; exact old runtime
image/log files no longer retained. Protected raw and baseline/failure/motion
images unchanged. Build/test outputs can be regenerated.

Comparable retained material fixture/log/image growth171,999 B: fixture+167,233,
aggregate build logs+3,689 (now151,538 B), replacement run+1,076/image+1 B.
This retains new source-free texture/UV assignment and live consumer coverage;
replace evidence by purpose at the next equivalent qualification. Exe/PDB grow
410,936 B; objects/metadata/source/docs/helpers/Git lack complete byte baselines,
not zero. Images10,240,483 B,245,277 B headroom under the same10 MiB cap.
End free at cleanup63,459,926,016 B (~59.10 GiB), drive-wide gain47,939,584 B from
this turn's first read-only sample; only372,736 B attributed to cleanup. Retain
the original ledger and3 GiB exception, without resetting/recrediting them.

### Native primitive participation, same checkpoint (2026-09-07)

Parentcf923aa; preceding turn was verified progress, pushed and clean. Current
first free63,383,052,288 B. Source-only tracing follows the exact1000/2000/3000
winding,0900 alpha/pass rules, pass helper8227FDC8 and volume-dependent dual
direct/deferred path. Load-owned programs plus object-pass inputs now compose
native cull/participation records. Fresh native cull consumers and whole-node
compound invalidation are wired; volume-effect production and callbacks remain
explicitly unconverted, not treated as opaque or silently dropped.

Before verification outputs free63,355,101,184 B; existing material fixture
7,129,853 B, aggregate build logs151,538 B. Reuse material15/CPU13 and host67.
Plan <=4 MiB fixture/log growth, <=256 MiB host compile/link overlap; no guest/
shader rebuild or asset cook. Same original ledger/start65,462,788,096 B,
owner-approved3 GiB cap/floor62,509,998,080 B, diagnostics100 MiB/logs10 MiB/
images10 MiB/raw0 unchanged. Existing wrapper enforces floor/log/timeout limits.
Keep run914/material sanity image and all protected baseline/failure evidence
until an equivalent new runtime/pixel replacement qualifies. No runtime started
at this preflight; reconcile actual output bytes before that next producer.

Material15/CPU13 and host67 passed; owned PIDs22788/12928/576 terminal,
exit0. Fixture6.273 s build,0.10 s behavior/0.12 s CTest; host32 actual edges,
no guest objects or shader rebuild.167 boundary guards pass. Fixture now
7,268,861 B (+139,008), aggregate logs159,120 B (+7,582); exe48,266,240 B /
PDB107,040,768 B (combined +171,520). No remaining renderer/compiler/linker/Ninja
processes. Current free63,353,491,456 B; host terminal63,358,009,344 B. Unbaselined
objects/metadata and concurrent volume writes are not falsely attributed.

Run915 plan: reuse the previous field gates plus fresh primitive plans/known
routing/checks/cull consumers, <=75 s flat normal-MSAA/precache-on run. Log
<=400 KiB and one1920x1080 quality60 JPEG <=160 KiB, within245,277 B existing
image overlap headroom. No new raw/perf/cache/dumps; explicit native policy
setting, fail immediately on mismatch, original profile restored in finally.
Same original cumulative floor/budgets remain enforced. Keep run914 and its
material image until equivalent checks and actual replacement pixels pass.

Run915 /PID31620 passed at00:45:14; runner exit0, all13 settings effective,
exact116-byte profile restored, no owned producer or new raw/perf/cache/dumps.
Fresh frame2040/2340 field windows add45,313 known plans,15,493 matching policy
checks and59,292 native-cull replays. All prior gates pass, including movement.
One133,740 B1920x1080 JPEG inspected, known artifacts/blur remain. Live compound
refresh/cull changes0: not runtime-qualified. Full evidence and limits in
`20260907_0047_native-primitive-participation.md`.

Retired eight exact verified superseded outputs: run914 log/material image,
material14/CPU12/host66 stdout/stderr.369,175 logical B; immediate free
63,181,049,856 ->63,181,299,712 B, observed recovery249,856 B (244 KiB), once only.
Protected raw/baseline/failure/motion evidence unchanged. Old reports/hashes
remain; exact old runtime files no longer retained, build/test logs reproducible.

Comparable fixture/log/image growth127,452 B: fixture+139,008, aggregate logs-540
(now150,998 B), runtime log-10,673, image-343. New policy fixture/live-consumer
coverage justifies this growth, replaced by purpose at next equivalent check.
Exe/PDB grow171,520 B; other outputs lack complete byte baselines, not zero.
Images10,240,140 B,245,620 B headroom. End free at cleanup63,181,299,712 B,
drive-wide use201,752,576 B from current first63,383,052,288 B; identified outputs
do not explain all volume activity. Same original3 GiB cap/floor and raw0 remain.

### Named lit shader arithmetic, same checkpoint (2026-09-07)

Parent e712978; preceding turn was verified progress, pushed and clean. First
free63,016,787,968 B, no owned producers. Source tracing found that the normal
lit material still repeated three light and two fog register-machine blocks.
The live shader now calls named, shared C++/HLSL arithmetic, removing that
specific blocker for a direct rigid shader. Its texture/shadow front-end and
binding ABI remain; this is not a replacement definition of direct submission.

Pre-output free63,357,468,672 B, material fixture7,268,861 B, aggregate logs150,998 B,
images10,240,140 B. Reuse material16/CPU14 and host68. Plan <=4 MiB fixture/log/
single host-shader header growth, <=256 MiB compile/link overlap; no guest or
translated shader regeneration, no new cache/cook/tree. The new shared header
is an explicit dependency of only the normal host material shader, not every
host shader. Same original65,462,788,096 B starting ledger, owner-approved3 GiB
cap/floor62,509,998,080 B, diagnostics100 MiB/logs10 MiB/images10 MiB/raw0 remain.
Keep run915 and its sanity image until an equivalent live replacement qualifies.

Material16/CPU14 passed (0.09 s behavior/0.11 s CTest), including1,200 independent
light samples and fog matrices. Host68/PID22152 terminal failure at the single
normal host shader: HLSL rejects struct-valued ternaries. Replaced that expression
with an equivalent explicit assignment/branch; no runtime launched. Codegen
module remained up to date, no guest/translated shader objects rebuilt. Fixture
7,314,236 B (+45,375), shader header previous314,931 B. Retry material17/CPU15/
host69 shares the same original budget, existing tree and <=4 MiB/256 MiB plan.

Material17/CPU15 and host69 passed (owned PIDs26804/23168/14368 terminal), CTest
0.13 s, host retry6.002 s.170 boundary guards/52 scenario tests pass. Emitted
SPIR-V has fragment/multiview entry, finite math and image gather operations;
header298,505 B (-16,426). Fixture7,315,534 B (+46,673), aggregate build logs159,962 B
(+8,964); exe48,264,704 B/PDB107,036,672 B (combined -5,632). Free63,355,977,728 B,
no producer remains. Run916 budget: <=75 s normal flat native MSAA/precache-on,
same previous gates plus fresh normal-lit queued use, <=400 KiB log and one
1920x1080 quality60 JPEG <=160 KiB within245,620 B image overlap. Explicit
bd_host_materials=true; no raw/perf/cache/dump growth, exact profile restoration.
This is shader-use/pixel sanity, not numerical GPU parity or direct-object proof.

Run916 /PID780 passed,01:04:52..01:06:02, all14 settings effective. Fresh field
windows2038/2338 add56,835 updated normal-lit queued draws; all previous geometry/
material/image/UV/pose/table/shadow/policy/movement gates pass. One1920x1080
quality60 JPEG136,607 B inspected; known cliff artifacts/blur remain, not GPU
numerical parity/sequence/reload/both-eye proof. Exact profile restored, no new
raw/perf/cache/dumps or remaining owned producer. Evidence:
`20260907_0108_named-lit-shading.md`.

Removed14 exact verified superseded files: run915 log/primitive image and logs
from material15/CPU13/host67, material16/CPU14/resolved failed host68. Logical
363,770 B; immediate free63,352,668,160 ->63,353,044,992 B, measured376,832 B
(368 KiB) reclaimed once. Protected evidence untouched; small prior reports/
hashes remain, old runtime files gone, build logs reproducible.

Comparable fixture/log/image growth59,550 B (new numerical/live shader coverage):
fixture+46,673, aggregate logs-4,769 (now146,229), runtime log+14,779, image+2,867.
Normal shader header-16,426 B, exe/PDB-5,632 B. Other outputs lack complete byte
baselines, not zero. Images10,243,007 B,242,753 B overlap headroom. Cleanup ending
free63,353,044,992 B, drive-wide gain336,257,024 B from first63,016,787,968 B;
only376,832 B attributed to cleanup. Same original cap/floor and raw0 remain.

### Explicit queued descriptor bindings, same checkpoint (2026-09-07)

Parent1bf9811. Previous goal turn was verified dev-loop progress, pushed and
clean. First measured free63,354,544,128 B; no owned renderer/build producer.
The queue emitter fetched the global constant set and assumed three offsets.
This change supplies explicit layout/set/offset snapshots to the existing queue,
including grouping and caller restoration. Translated instance-record gathering,
engine shader selection and direct-object production are still separate gaps.

Pre-build free63,336,759,296 B. Reuse host_draw_intent_test in out/native_texture_test
(existing exe25,600 B), add the pure binding fixture to that target; no new tree.
Plan <=2 MiB fixture/log growth, <=512 MiB host compile/link overlap. Existing
exe48,264,704 B/PDB107,036,672 B; aggregate build logs146,229 B before this work.
Host dry-run requested CMake regeneration for added source/header files; inspect
the bounded real build and stop on guest compilation. No shader/translator edits.
Attempts: fixture18, draw_intent_cpu16, host70. Retain previous qualified evidence
until replacement passes; preserve the material17/CPU15 math evidence because
this fixture does not replace it. Same original65,462,788,096 B start, approved
3 GiB ceiling/floor62,509,998,080 B, diagnostics100 MiB/logs10 MiB/images10 MiB,
and zero new raw allowance. Runtime/image plan follows only after build passes.

Fixture18/CPU16 pass (0.03 s behavior/0.05 s CTest); host70/PID27928 is terminal
success,97 scheduled CMake/host edges including link, no guest/shader rebuild.
The shared queue header required a wider host-only rebuild, not a guest rebuild.
228 Python boundary/scenario checks and8 runner cases pass. Alternative native
layouts and exact restore have CPU command-emission coverage, not GPU family
qualification. Run917 reuses the normal flat/native-MSAA/precache-on complete
run916 gate plus fresh explicit binding emission. <=75 s, <=400 KiB runtime log,
one1920x1080 quality60 JPEG <=160 KiB fitting242,753 B image overlap headroom.
No raw/perf/cache/dump writes; exact owner profile restoration and owned shutdown.
Retire run916 log/image after the equivalent new regression gate/pixels pass;
retain its material/math fixture logs, which this binding fixture does not replace.

Run917/PID31508 terminal success,01:47:03..01:48:01 EDT, all14 settings effective.
Fresh post-event field binding windows2049/2349 add196,347 emitted draw commands,
128,465 descriptor binds and900 layout binds. Prior geometry/material/image/UV/
pose/table/shadow/policy/lit/movement gates pass. Alternate layouts have CPU
coverage only; these live draws still use the engine producer. One1920x1080
quality60 JPEG128,595 B inspected, known cliff marks/blur remain. Runtime log
218,794 B; no new raw/perf/cache/dump files, exact116 B profile restored. No
sequence/reload/both-eye or direct native-object qualification. Evidence:
`20260907_0157_explicit-draw-bindings.md`.

Removed six exact validated superseded outputs: run916 log/named-lit JPEG,
host69 and old draw-intent01 stdout/stderr.368,777 logical B. Immediate free
63,214,342,144 ->63,214,718,976 B, observed recovery376,832 B (368 KiB), once only.
Protected raw/baseline/failure/motion evidence unchanged. Old reports/hashes
remain; old runtime files gone, build logs reproducible. Material17/CPU15 stay
because their numerical coverage is distinct from the new binding fixture.

After run917, capped the unsupported-record-ABI diagnostic at four messages.
Host71/PID8956 terminal success,4.270 s: one host TU/link, codegen up-to-date probe
wrote0 files; no guest objects rebuilt. No further runtime needed for this
failure-log-only change. Run917 is tied to host70's fingerprint, not host71's;
both hashes are recorded in the report. Final Python228/runner8 checks pass.

Known comparable fixture/log/image growth374,799 B: new fixture object322,943,
fixture exe+40,960, aggregate build logs+31,190 (now177,419 B), runtime log-12,282,
JPEG-8,012. New native-layout command coverage justifies retained growth; replace
by purpose at the next equivalent qualification. Other object/metadata/helper/
source deltas lack complete baselines, not zero. Final exe48,271,872 B and
PDB107,040,768 B, combined+11,264. Images10,234,995 B,250,765 B overlap headroom.
Final measured free at01:57 is63,181,537,280 B (58.84 GiB), drive-wide use
173,006,848 B (165 MiB) from current first63,354,544,128 B, not all attributable
to this task. No owned producer remains. Same original ledger start/cap/floor
and raw0 gate remain; no reset and no duplicate cleanup credit.

### Native pipeline programs, same checkpoint (2026-09-07)

Parent9d3bab0. Previous goal turn was verified progress, committed/pushed clean.
Previous post-push free63,025,065,984 B; current first63,012,909,056 B with no
owned producers. At pre-build measurement free63,340,675,072 B; the drive-wide
increase is not agent cleanup. Original ledger65,462,788,096 B, approved3 GiB
exception/operational floor62,509,998,080 B and raw0 remain; no allowance reset.

Remove the shared pipeline builder's mandatory GuestShader/main-layout contract
for explicit native programs. Program-owned shader/layout/input/spec values are
pinned across async work and cache lifetime; mixed translated inputs are refused.
Native caps:8 specialization entries,256 pending compiles,2048 retained variants.
No actual game object/shader producer uses this yet. CPU descriptor/lifetime
coverage is not a native GPU shader or object qualification.

Reuse out/native_texture_test: native_texture_binding_test.exe195,072 B and
host_draw_intent_test.exe66,560 B; add the program fixture to the former, avoiding
the latter's intentionally opaque binding-test backend types. Plan <=3 MiB
fixture/log growth and <=512 MiB host compile/link overlap. Host exe48,271,872 B/
PDB107,040,768 B, logs177,419 B before work. Shared PipelineState header appends
one runtime-only pointer; CSV/generated designated initializers retain their
schema, native records are excluded. Build one target at a time, no guest/shader
rebuild, no new tree/cook/cache/raw/perf/dump. Existing bounded wrapper enforces
300 s/log10 MiB/free floor and stops unexpected guest compilation. Preserve
run917 and its image until any equivalent regression replacement qualifies.

Binding fixture19/CPU17 pass (0.05 s behavior/0.07 s CTest), intent20/CPU18 pass
(0.03/0.04 s). Host72/PID28040 terminal success:97 scheduled CMake/host edges,
no guest objects or shaders rebuilt.231 Python checks and8 runner tests pass.
Fixture binding exe256,000 B (+60,928), intent exe unchanged66,560 B, new program
fixture object809,228 B; other object/metadata deltas lack full baselines.
Exe48,287,232 B/PDB107,188,224 B, combined+162,816. Logs210,302 B (+32,883).
Host end free63,337,897,984 B. Run918 plan: same run917 complete flat native-MSAA/
precache-on regression gates, <=75 s, <=400 KiB log and one full-size quality60
JPEG <=160 KiB within250,765 B aggregate image overlap headroom. No new native
program producer is present: this run qualifies existing engine rendering only.
Exact original profile restoration, raw/perf/cache/dumps off, automatic shutdown.

Run918/PID24168 terminal success,02:13:14..02:14:13 EDT; all14 settings effective,
original116 B profile restored exactly. Fresh post-event contexts2040/2340 pass
all existing field gates:200,673 explicit draw commands,134,342 descriptor binds,
900 layout binds;111,862 matching poses,142,063 canonical draws,47,576 load-owned
draws,61,434 cull replays and57,533 normal-lit uses. Movement observes30 fresh
samples/+37.625316 units. One135,961 B1920x1080 JPEG inspected; known cliff marks/
blur remain. No new cache/dump/perf/raw files. Native program creation exists only
in CPU fixtures; the field run is not native-program GPU qualification.
Full evidence: `20260907_0217_native-pipeline-programs.md`.

Retired14 exact validated superseded outputs: run917 log/binding JPEG, binding05/
CPU05, intent18/CPU16 and host70/71 stdout/stderr.380,999 logical B; immediate
free63,334,731,776 ->63,335,129,088 B, measured397,312 B (388 KiB) reclaimed once.
Protected raw/baseline/failure/motion evidence unchanged; old reports/hashes
remain, old runtime files gone, build logs reproducible. Distinct numerical
material17/CPU15 evidence stays. No owned renderer/build process remains.

Known comparable retained fixture/log/image growth890,087 B: new native program
fixture object809,228, binding exe+60,928, intent exe unchanged, aggregate logs
-727 (now176,692), runtime log+13,292, image+7,366. This adds program/lifetime
coverage; replace by purpose at the next equivalent qualification. Other object/
metadata/helper/source deltas lack full baselines, not zero. Host exe/PDB grows
162,816 B. Image aggregate10,242,361 B leaves243,399 B overlap headroom. Cleanup
end free63,335,129,088 B (58.99 GiB), drive-wide gain322,220,032 B from current
first63,012,909,056 B; only397,312 B is attributed to cleanup. Same original
ledger/cap/floor and raw0 gate remain, without reset or double-credit.

### Production native rigid shaders, same checkpoint (2026-09-07)

Parentd98b980, previous goal turn verified progress, committed/pushed clean.
Current first free63,333,982,208 B, no owned producers. Pre-build63,334,043,648 B.
Original ledger/cap/floor/raw0 are unchanged; no reset. Implement actual rigid
scene/shadow shaders with named object/pass/texture inputs, no translated shader
common include, shared light/fog math and explicit two-eye matrices. Reuse the
existing tiny real-Vulkan scene snapshot fixture with a --rigid selection, not
a new build tree/game boot. No game object producer is connected yet.

Budget <=8 MiB added fixture/shader/log artifacts, <=128 MiB compile/link overlap,
same diagnostics100 MiB/log10 MiB/free floor62,509,998,080 B. Existing GPU fixture
8,364,855 B (exe705,024/PDB7,053,312), material fixture7,315,534 B, logs176,692 B.
GPU test uses8x8 mono shadow and two-eye colour/depth images, bounded CPU/GPU
buffers,5 s fence and30 s CTest; no raw/image/export/cache/perf/profile output.
Only three new host shaders compile, not translated shaders or guest objects.
Preserve run918/image and distinct prior numerical/snapshot coverage until
equivalent replacements pass. Retained new artifacts must add shader coverage,
then be replaced by purpose rather than accumulated across retries.

Material18/CPU16 pass (0.10 s behavior/0.12 s CTest). Fixture21 failed before GPU
work on generated-header ordering and Windows max macro; both corrected.
Fixture22 passes, only3 new host shaders compiled. Rigid01 reveals an exact
shadow-frustum-edge oracle instability and unused depth attributes; move samples
strictly inside and use position-only shadow input. Fixture23/PID30020 passes.
Rigid02/PID30332 passes4 two-eye cases/all512 color pixels plus eye/caster depths,
max color error0.0000404567, validation0/0,1.21 s. Snapshot19/PID31048 passes all8
layer/MSAA combinations, validation0/0,1.04 s. No raw/image/cache/profile outputs.
Host73/PID28680 passes9.882 s, host-only compilation/link, codegen up to date;
no guest objects/translated shaders.234 Python checks and8 runner tests pass.
No game producer calls the native factory; run918/image remain tied to host72.
Evidence: `20260907_0245_native-rigid-shaders.md`.

Reused GPU fixture now10,395,399 B (+2,030,544), material7,338,480 B (+22,946),
three new shader headers248,925 B. Host exe48,325,632/PDB107,266,048, combined
+116,224; new host factory object550,320 B. These add actual native shader
coverage, not another build tree. Retire equivalent logs on replacement; keep
current binaries/objects needed for incremental development. Other object/
metadata/source/helper deltas lack full baselines, not zero.

Validated replacements, then removed14 exact superseded agent logs: material17/
CPU15, snapshot03, snapshot builds01/04/21 and rigid01 stdout/stderr.20,826 logical
B; immediate free63,285,940,224 ->63,285,972,992 B, measured32,768 B reclaimed
once. Earlier distinct failure logs and protected baseline/raw/motion/run918
evidence are preserved; game data/profile untouched. Removed logs reproducible;
failed attempt hashes/causes retained in the dated report. No active producer.
Aggregate logs175,763 B (-929). Known retained fixture/shader/log growth2,301,486
B, plus666,544 host exe/PDB/new object. Cleanup-end free58.94 GiB; drive-wide use
+48,009,216 B from this turn's first free, not all attributed to the task. Same
original cap/floor/raw0 and retention triggers remain; no budget reset.

### Native model-node association, same checkpoint (2026-09-07)

Parent44cb12d; previous turn verified progress and pushed clean. First current
free63,282,425,856 B; pre-build63,282,368,512 B, no active producers. Same original
3 GiB cap/floor62,509,998,080 B/raw0 gate. Load-owned node/primitive associations
and mesh spheres now travel with native model leases into instance poses; host
walk consumes those bounds without source mesh reads outside verification.
This removes the missing model-to-primitive association for direct submission;
live draw bindings/light/fog and interpreter-free acceptance remain unfinished.
Four field-ready rigid candidate identities maximum, no per-draw dump/cache.

Reuse material tree7,338,480 B, exe364,032 B; plan <=4 MiB fixture/log growth and
<=256 MiB host compile/link overlap, existing log10 MiB/diagnostic100 MiB ceilings.
One target at a time under the existing supervisor; no guest or shader rebuild,
new tree, cook, raw, perf or dumps. Preserve run918/image and native shader GPU
evidence until an equivalent replacement qualifies. Runtime/image preflight is
separate after CPU/build checks; budget overlap before launching, not afterward.

Material19/PID15800 andCPU17/PID28244 pass (0.10 s behavior/0.11 s CTest).
Host74/PID28140 terminal success, codegen up-to-date, host objects/link only,
no guest or shader compilation. Host exe48,338,432/PDB107,388,928 B; material
tree7,461,125 B (+122,645). Logs181,101 B (+5,338).238 Python checks pass.
Runtime preflight free63,235,993,600 B, image aggregate10,242,361 B leaving
243,399 B. Plan one<=75 s flat/native-MSAA/precache-on run, all run918 gates
plus fresh native node/bounds comparisons; <=400 KiB log, one quality60 full
window JPEG<=160 KiB. Original profile hash verified unchanged. Same raw0/perf/
dump/cache restrictions; keep run918/image until replacement pixels qualify.

Run919/PID8932 terminal03:04:11, all14 settings effective and exact profile
restored. Fresh native node reads/checks+1,761,600, unavailable0/mismatch0, existing
field gates pass. One128,634 B1920x1080 JPEG inspected: coherent scene/player/
shadows, existing cliff marks/blur remain. No new runtime cache/perf/dump/raw.
Candidate filter yielded zero IDs, not native-object acceptance. Host75/PID28804
passes4.298 s after changing ONLY candidate diagnostics to expose actual technique,
primitive count and unknown skin state. Rendering/ownership behavior is unchanged;
run919 pixels stay tied to host74, not silently restamped as host75.

Retired8 exact superseded outputs after replacement passed: run918 log/image,
material18/CPU16 and host73 stdout/stderr.371,420 logical B; immediate free
62,992,408,576 ->62,992,789,504 B, measured380,928 B reclaimed once. Old reports/
hashes remain; old field outputs gone, build logs reproducible. Distinct rigid
shader/snapshot GPU evidence and protected raw/baseline/motion data untouched.

Volume use since first free is now289,636,352 B, beyond the initial compile-only
overlap estimate; scoped runtime cache/perf/dump inventory since02:49 has0 new
files. Known host/fixture output growth is much smaller; remaining attribution
is unknown, not zero. Producers are stopped. Original floor leaves482,791,424 B.
Next text-only candidate run reuses the same full field gate, no image, <=75 s/
400 KiB log, with an additional per-run192 MiB drive-growth stop plus the original
floor. No cumulative budget reset. Keep run919 image/evidence; new candidate
evidence has a distinct purpose and is replaced at direct-object qualification.

Run920/PID15876 terminal03:09:47, all14 settings effective/profile exact. Full
field gate passes again with+1,761,600 native bounds reads/checks,0missing/0wrong;
31 movement samples/+39.002439 units. Four content-keyed candidates recorded.
Choose geometry258694267A8DBAEE/material63B8D67932573E51, sole primitive of node64
in bg41_01. Material file68 B inspected: modulation off/specular black/power0;
canonical geometry is memory-only, no matching v2 cache file. No new runtime
cache/perf/dump/raw. Direct shader route and source-free load remain pending.
Current log235,282 B; current image remains run919's128,634 B, tied to host74.
Source/verification details: `20260907_0312_native-model-node-associations.md`.

Retired run919 full text log only after920 replaced all its field gates; its
image and actual host74 hash/observations remain.233,579 logical B; immediate
free63,315,308,544 ->63,315,546,112 B, measured237,568 B reclaimed. Total THIS
turn cleanup9files604,999 logical B/618,496 B (604 KiB) measured once. Do not
re-credit prior shader checkpoint cleanup. Final aggregate build logs178,481 B
(+2,718), images10,235,034 B (-7,327); one latest field log replaces the old one
(+3,196). Material tree7,461,125 B (+122,645): known comparable retained growth
121,232 B. Host exe/PDB+136,192; other object/metadata/helper deltas unknown.
Retention adds node association/lifetime coverage; replace by purpose at the
next equivalent/direct-object qualification. No owned producer remains.

Cleanup-end free63,315,546,112 B (58.97 GiB), drive-wide gain33,120,256 B from
current first free; fluctuations are not attributed to task cleanup. Original
ledger3 GiB exception/floor62,509,998,080 B and raw0 gate remain unchanged.

Final host76/PID29896 passes3.201 s, one host source/link: guard the candidate
call so normal verification-off rendering does not evaluate its source lookup
arguments. Verified-on behavior unchanged; no further game run/image. Exe/PDB
sizes unchanged; actual latest binary hash in dated report.238 guards/scenarios
pass again. Logs179,228 B (+747 from prior tally), comparable retained growth
121,979 B plus136,192 host exe/PDB. Build-end free63,313,158,144 B; no additional
cleanup savings or reset. Run919 pixels/920 text remain tied to host74/75.

### Selected native rigid asset, same checkpoint (2026-09-07)

Parent a4b4555; previous turn verified progress, committed/pushed clean. First
free63,298,228,224 B; no active renderer/build processes. Same original3 GiB
exception/floor62,509,998,080 B/raw0 gate; no budget reset. Reuse mesh fixture
tree1,419,621 B and desktop tree. Plan <=4 MiB fixture/log retained growth and
<=256 MiB per-build overlap. Selected geometry258694267A8DBAEE may add only one
<=2 MiB file to the existing36,510,144 B native cache, under its aggregate writer
limits. No broad recook, guest/shader rebuild, new tree or raw/image capture.
Keep run920 text/run919 image and distinct GPU fixture evidence until equivalent
replacement checks pass. Native shader input ownership is added at upload/load;
game draws are not rerouted by this change. Actual schema inspection is needed
before the selected object's direct consumer can be declared eligible.

Mesh06 failed on Windows min/max macros; fixed locally. Mesh10 built, CPU09
failed because the new fixture's nested directory interfered with an older
inventory assertion, not a production storage failure. Isolated scratch state;
mesh11/PID25416 and CPU10/PID30208 pass (0.12 s behavior/0.13 s CTest). Host77/
PID28004 passes, up-to-date codegen and host objects/link only. No producer left.
240 Python checks pass. Host exe48,346,624/PDB107,438,080 B; mesh tree1,490,087 B
(+70,466), logs187,058 B (+7,830). Same source/shader consumers, no game reroute.

Runtime preflight03:33:45 free63,006,949,376 B. Scoped cache/hlsl/perf files
modified since03:20: zero; current modified host object/metadata payload24,587,804 B
is not its net growth (prior per-object baseline unavailable). Drive-wide loss
291,278,848 B exceeds known artifacts; attribution remains unknown. Inspected
scope before continuing; enforce original floor AND per-run192 MiB growth stop.
One<=75 s full field regression with explicit selected cook, <=400 KiB log,
one<=2 MiB mesh file, no image/raw/perf/dump. Owner116 B profile hash unchanged.

Run921/PID27672 terminal03:35:38, all15 settings effective/exact profile restored.
Full field text gates pass; bounds+1,761,600 matching reads, poses+113,082,
movement30 samples/+37.772134 units. Exactly one17,572 B mesh file added; cache
3511files/36,527,716 B, one write/zero refusals. Read-only fixture verifies native
content/schema and retained input after CPU-data destruction. UV0 still16383..
16895; exact live material-family UV/flags remain unresolved. No direct game
route/source-free GPU load. No new raw/image/perf/dump; inspected run919 image
remains tied to host74. Run921 log236,311 B replaces run920 text. New source and
artifact hashes in `20260907_0340_selected-native-rigid-asset.md`.

After equivalent checks passed, removed13 exact superseded log files: mesh
builds09/06/10, CPU08/09 and host76 stdout/stderr, plus run920 full text. Causes
of the two test-side failures remain in the report; build/test logs reproducible,
old field text gone but recorded observations/hashes remain. Current mesh11/
CPU10/host77 logs, run921 and run919 image, distinct GPU/baseline/motion/raw
evidence preserved.241,190 logical B; immediate free63,316,140,032 ->63,316,393,984 B,
**253,952 B (248 KiB) actually reclaimed**, credited once. Earlier604 KiB cleanup
is not part of this turn's savings. No renderer/build producer remains.

Final measured mesh tree+70,466 B, build logs+1,922 B, field-log replacement+1,029 B,
selected asset+17,572 B, host exe/PDB+56,832 B: known retained growth147,821 B for
the new asset/input coverage; other objects/metadata/source/Git deltas unknown.
Cleanup-end free58.968 GiB, drive-wide gain18,165,760 B from this turn's first;
independent volume gains are not credited as cleanup. Same original cap/floor/
raw0 gate; retain the selected native asset for direct consumption and replace
the field/build logs by purpose, not by every revision.

### Owned object primitive packets, same checkpoint (2026-09-07)

Parent91d86ce, prior turn verified progress and pushed. First current storage
preflight free63,286,571,008 B; material tree7,461,125 B, logs181,150 B, images
10,235,034 B. One transient cmake/PID31364 appeared during inspection, but its
handle/process was authoritatively missing on recheck; never restarted/stopped
it. No owned producer active. Same original3 GiB exception/floor62,509,998,080 B,
per-build256 MiB/per-run192 MiB stops, raw0. Plan <=4 MiB fixture/log growth,
<=256 MiB build overlap. Reuse material/desktop trees and selected17,572 B mesh;
no guest/shader rebuild, new asset/capture/cache tree or bulk conversion. Runtime
and one replacement <=160 KiB image preflight follow CPU/build checks; keep
run921 text/run919 image until replacements qualify under the same10 MiB cap.

Material20/PID31720 passes (14 build edges,6.010 s); CPU18/PID27944 passes
0.10 s behavior/0.11 s CTest.245 Python checks pass. Host78/PID31256 passes:
codegen up-to-date,21 host edges/link, no guest/shader rebuild. All terminal.
Runtime preflight free63,269,978,112 B; no renderer/build producer. Material tree
7,532,110 B (+70,985), build logs187,675 B (+6,525), images10,235,034 B unchanged.
Host exe48,359,936/PDB107,626,496 B; owner116 B profile hash unchanged. One
<=75 s complete post-event field check with owned-color comparisons, <=400 KiB
log and one <=160 KiB JPEG; no selected cook, raw, perf or shader/cache output.
Image overlap fits10 MiB. Keep previous evidence until comparisons and actual
new pixels pass; retain only the replacement passing flat image/text afterward.

Run922/PID29112 terminal04:06:01; all14 settings effective, original profile
restored exactly. Complete post-event field gates pass:79,716 new owned color
reads/comparisons, zero mismatch/unavailable;33,600 publications and4 bounded
owned packet observations. Bounds+1,761,600/poses+113,514 matching; movement31
samples/+37.499395 units. Selected packet owns image mask0001, UV offsets0,
diffuse1, material mask3, direct opaque policy. Shader/UV family interpretation
and live light/fog remain unresolved; no native rigid game shader route yet.
Run922223,889 B and inspected137,491 B1920x1080 JPEG replace prior flat evidence.
Known cliff marks/blur remain; one image is not sequence/both-eye qualification.
No new raw/perf/cache/dump. Binary/evidence hashes and source audit are in
`20260907_0406_owned-object-primitive-inputs.md`.

After replacement validation, removed8 exact superseded files: material19/
CPU17/host77 stdout/stderr, run921 text and run919's model-node sanity image.
Historical reports/hashes and selected mesh preserved, along with distinct
baseline/GPU/failure/motion/raw evidence.370,066 logical B; immediate free
63,265,992,704 ->63,266,369,536 B, **376,832 B (368 KiB) actually reclaimed**.
Credited once; earlier cleanup is not part of these savings. Build logs are
reproducible; exact retired runtime files are no longer retained. No producer.

Known retained growth270,552 B: material tree+70,985, logs+1,404, flat image
replacement+8,857, field text replacement−12,422, host exe/PDB+201,728. This adds
owned-packet lifetime/color-consumer coverage; replace equivalent outputs next
checkpoint. Other objects/metadata/source/Git deltas unknown. Cleanup-end free
58.92 GiB, drive-wide use+20,201,472 B (19.27 MiB) from current first preflight;
not all attributed to task outputs. Original exception/floor/raw0 gate unchanged.

### Owned selected-light publication, same checkpoint (2026-09-07)

Parent ada121d, prior turn verified progress and pushed. First free63,263,776,768 B;
04:29 preflight62,948,524,032 B. No renderer/build producer. Scoped cache/HLSL/perf
inventory has no modified files since the prior run; material tree7,532,110 B,
build logs182,554 B and prior field/image unchanged. Drive-wide loss315,252,736 B
is not explained by these source-only edits; attribution remains unknown. Same
original3 GiB exception/floor62,509,998,080 B, no budget reset. Reuse material and
desktop build trees; plan <=4 MiB retained fixture/log growth and <=256 MiB build
overlap, enforce the tighter per-build256 MiB/free-floor stops. No guest/shader
rebuild, new tree, asset cook or raw output. CPU/source checks precede runtime;
one <=75 s field comparison and <=160 KiB replacement image will be preflighted
separately. Keep run922/image and current passing logs until replacements pass.

Material21/PID29552 and CPU19/PID28084 pass (0.12 s behavior/0.13 s CTest).
250 Python checks pass. Host79 stopped on an incorrect diagnostic GuestShader
field name; corrected to shaderCacheEntry->hash. Host80/PID28944 passes, only
host objects/link, codegen up-to-date. Build-end free62,924,156,928 B. No guest
objects/shader regeneration. Next one <=75 s full post-event field comparison
with selected-light publication AND actual normal-lit input checks, <=400 KiB
log, one <=160 KiB replacement JPEG, raw/perf/cache/dump/cook off. Existing image
10,243,891 B plus incoming163,840 B fits10 MiB. Enforce original floor and192 MiB
per-run stop; exact owner profile restoration in finally. Retire prior passing
image/text only after fresh matching comparisons and actual pixel inspection.

Run923/PID14416 terminal04:34:21, original profile hash restored. Its317,845 B
log records matching host publications but zero normal-lit draw checks; no image
was produced and the readiness gate correctly failed. The selected object uses
later per-node light overrides. Keep this failure until a corrected consumer
passes. Add exact-node publication at the authored producer boundary; immutable
returned packet values do not borrow another node's lights. Diagnostic packet
observation moves after the node callback. Material22/PID30824, CPU20/PID23656
(0.11 s behavior) and host81/PID18740 pass;250 Python checks pass. Host-only,
no guest/shader build. Build-end free63,291,977,728 B; intervening drive-wide gain
is not cleanup savings. Retry shares all original limits and retains the prior
failure/current flat evidence until fresh comparisons and a replacement image pass.

Run924/PID29684 terminal04:40:04; all15 settings effective and owner profile
restored exactly. Fresh post-event contexts2051/2351: light publications/checks
+13,870 each, changed slots+900, snapshots+1,986, normal-lit input checks+1,322;
zero mismatch/fallback/unavailable. The selected rigid packet now owns light
kinds1/0/0. All existing field/shadow/colour/node/pose gates and player movement
pass. Inspected136,226 B1920x1080 JPEG: coherent running Shu/bell/fence/terrain
and shadows, known cliff marks/blur remain; not sequence/both-eye qualification.
Run924239,652 B replaces prior flat text. No new raw/cache/perf/dump/asset file.
Final host82/PID27524 passes with defensive invalidation of unbound/refused
publications;250 guards pass again. That ownership guard was not rerun in-game;
runtime/pixels remain tied to host81. Hashes/limits in the dated selected-light report.

After replacement validation, removed17 exact superseded agent files: material
20/21, CPU18/19, host78/79/80 stdout/stderr, run922/923 text and prior object-input
sanity image. The compilation typo and per-node timing failure are resolved;
their causes/hashes remain recorded. Current material22/CPU20 and tested/current
host81/82 logs, run924/image, selected mesh and distinct baseline/GPU/motion/
unresolved-failure/raw evidence preserved.694,095 logical B removed; immediate
free63,234,830,336 ->63,235,543,040 B, **712,704 B (696 KiB) actually reclaimed**,
credited once. Build logs are reproducible; exact retired runtime text/image
files are no longer retained. No build/renderer producer remains.

Known retained growth211,701 B: material tree+77,533, build logs-2,186, flat image
replacement-1,265, field text+15,763, host exe+15,360/PDB+106,496. Growth covers
owned light publication, node isolation and real consumer comparison; replace
these outputs by purpose at the next checkpoint. Other objects/metadata/source/
Git deltas unknown. Cleanup-end free58.89 GiB; drive-wide use+28,233,728 B
(26.93 MiB) from this turn's first preflight, not all attributable to task files.
Same original exception/floor/raw0 gate; no budget reset or new raw allowance.

### Owned fog continuation, 2026-09-07

Parent8e92e3f, prior checkpoint verified/pushed. First free63,232,987,136 B;
pre-build measured63,228,907,520 B; no active build/renderer. Four new fog files
and bounded owner/consumer/test integration;254 Python guards/scenarios pass.
Reuse material and desktop trees, <=4 MiB retained fixture/log growth estimate
and <=256 MiB temporary build overlap. Enforce original3 GiB exception and
floor62,509,998,080 B plus per-build256 MiB stop, aggregate10 MiB build logs.
No guest/shader rebuild, new tree, asset cook or raw allowance. Material23/CPU21
then host83 are planned; all retries share these limits. One bounded field/pixel
run will be separately preflighted. Preserve run924/image and passing logs until
replacement evidence passes; retire superseded artifacts at this checkpoint.

Material23/PID27528, CPU21/PID30512 (0.10 s behavior/0.11 s CTest) and host83/
PID28536 pass. Host-only objects/link, codegen up-to-date; no guest/shader build.
Build-end free63,227,383,808 B. Planned one <=75 s full post-event field run:
fog producer and normal-lit active-fog comparisons added to the existing gates,
<=400 KiB text, one <=160 KiB replacement JPEG; aggregate image10,242,626 B plus
incoming163,840 B fits10 MiB. Raw/perf/dump/cache persistence/cook off; original
floor plus192 MiB free-drop stop and guaranteed exact profile restoration.
Keep prior924/image until fresh matching checks and actual pixels qualify.

Run925/PID30644 terminal05:06:02;333,008 B text, no image/raw/cache. Owner profile
restored exactly. One layer is refused repeatedly, zero owned snapshots/draw
checks: gate correctly fails, prior924/image remain baseline. Log SHA256
D45702875FB3FD94FF113AD4B0857BCD60C922B086FEE42C7CA513637B5B60FB.
Review found an overly strict ascending-range assumption in both fog import and
existing native rigid pass validation: the shared/original shader permits signed
nonzero end-minus-start. Preserve descending endpoints; refuse only singular
active ranges. Add independent descending-falloff CPU coverage and bounded
refusal/packet endpoint diagnostics to establish whether this explains the live
layer. Measured free63,225,409,536 B. Material24/CPU22/host84 and one same-bounded
retry are planned; no budget reset, prior failed evidence retained until resolved.
Changing the C++ section of the rigid input header may regenerate its three
small native shaders; no translator or guest-source changes are requested.

Material24/PID31084, CPU22/PID28028 (0.10 s behavior/0.12 s CTest), host84/
PID25416 and254 Python checks pass. Run926/PID25840 terminal05:10:04, all15
settings audited, exact profile restored. Fresh contexts2005/2305 add2,400 fog
publications/comparisons,33,600 owned snapshots,1,335 draw checks/2,670 active
layers; zero fallback/unavailable/mismatch. The selected asset owns active ranges
0/1600 and100/-200, confirming the descending-range diagnosis. Existing gates,
light checks and movement pass. Inspected127,143 B1920x1080 image: coherent
running Shu/terrain/foliage/shadows, known cliff marks/blur remain; not sequence/
both-eye qualification. Retained228,077 B run926 text. No raw/cache/perf/dump/cook
outputs; hashes, exact binary and limitations in20260907_0510_owned-fog.md.

After validation, removed17 exact superseded agent files: material22/23,CPU20/21,
host81/82/83 stdout/stderr; run924/925 text and old selected-light window image.
The refused descending-range failure is resolved and its hash/cause preserved.
Kept material24/CPU22/host84 logs, run926/image, selected asset and all distinct
protected baseline/GPU/motion/failure/raw evidence.718,226 logical B removed;
immediate free63,188,860,928 ->63,189,598,208 B: **737,280 B (720 KiB) actually
reclaimed**, credited once. Build logs are reproducible; retired exact runtime
text/image files are no longer retained. No build/renderer producer remains.

Known retained growth121,983 B: material tree+50,331, build logs-874,
flat image-9,083, field text-11,575, host exe+15,360/PDB+77,824. Additional code/
fixture bytes cover owned fog and signed-endpoint validation; replace retained
verification by purpose at the next checkpoint. Other objects/metadata/source/Git
deltas unknown. Ending free58.85 GiB; drive-wide use+43,388,928 B (41.38 MiB)
from this continuation's first measurement, not all attributable to task files.
Original exception/floor/raw0 gate unchanged; no new storage allowance.

### Primitive shader-input continuation, 2026-09-07

Parent04490b1 verified/pushed. First measured free62,890,655,744 B; pre-build
05:29:41 free62,857,560,064 B. No active renderer/build. Scoped inventories of
material fixture, verification, native caches and shader dumps show no writes
since05:14; exe/PDB/run926/image sizes and times match the preceding checkpoint.
Build logs179,494 B, window images10,233,543 B, material tree7,659,974 B.
Drive-wide decrease since prior push is not attributed to these unchanged
outputs; other-process/source/Git effects remain unknown. Existing original
3 GiB exception and62,509,998,080 B operational floor remain, not a new budget.

Plan: reuse material tree (material25/CPU23) and host tree (host85), no guest
or shader regeneration expected. Estimate <=192 MiB transient additional build
space, enforce existing floor and256 MiB free-drop/log10 MiB stops. One <=75 s
field run afterward, <=400 KiB text and <=160 KiB replacement JPEG (overlap fits
10 MiB image cap); raw/perf/dumps/cook/cache persistence off,192 MiB runtime stop.
Keep926/image until replacement comparisons and actual pixels qualify, then
retire equivalent previous agent-created logs/image. No new tools or build tree.

Material25/PID28736 and CPU23/PID16220 pass (behavior0.10 s/CTest0.11 s).
Host85/PID25316 passes, only host objects/link; codegen up-to-date, no shader
regeneration. Build-end free62,855,606,272 B. All257 Python checks pass. Run927/
PID31504 terminal05:33:02, all15 temporary settings audited and exact profile
restored. Fresh post-event frames2015/2315 add1,332 matching shader-input checks
and42,166 owned-input draws; all prior field/light/fog/movement gates pass.
No raw/cache/perf/dump/cook outputs. Retain231,185 B text and135,419 B JPEG;
actual pixels inspected, known cliff marks/blur remain. Binary/evidence hashes
and source contract in20260907_0535_owned-primitive-shader-inputs.md.

After replacement qualification, removed8 exact superseded agent outputs:
material24/CPU22/host84 stdout/stderr, run926 text and native_fog_window.jpg.
358,685 logical B removed; immediate free62,806,982,656 ->62,807,351,296 B:
**368,640 B (360 KiB) actually reclaimed**, credited once. Build logs can be
regenerated; exact retired runtime text/image no longer retained. Keep current
material25/CPU23/host85 logs,927/image, selected cooked asset, and all distinct
protected GPU/baseline/motion/failure/raw evidence. No active owned producer.

Post-cleanup material tree7,683,155 B, build logs182,749 B, images10,241,819 B.
Known retained growth91,068 B: material+23,181, logs+3,255, image+8,276, field
text+3,108, exe+8,192/PDB+45,056. Code/fixture growth covers load-owned shader
inputs; replace equivalent verification by purpose at the next checkpoint.
Other objects/source/metadata/Git deltas unknown. Ending free58.49 GiB;
drive-wide use+83,304,448 B (79.45 MiB) since this continuation's first measure,
not all attributable to task files. Original cumulative limits remain unchanged.

### Owned lighting-pass continuation, 2026-09-07

Parentafe5491 verified/pushed. First free62,773,608,448 B, no active producers;
prior post-push free62,778,208,256 B differs by4,599,808 B, not attributed to
unchanged retained outputs. Original3 GiB exception and62,509,998,080 B floor
remain, including both attempts below. Planned <=192 MiB incremental link overlap,
existing build256 MiB/runtime192 MiB drop stops;400 KiB text/160 KiB JPEG,
aggregate logs10 MiB/images10 MiB. No raw/perf/cache/cook/dump allowance.

Lighting1/CPU1 and material26/CPU24 pass; host86 and corrected87 pass, only host
objects/link. A wrapper parse error started no producer. Run928/PID26808 stopped
with zero eligible lighting comparisons because its new view identity used a
lighting texture slot; failed coverage, no image. Corrected to the shared scene
render-view ID. Post928 free62,769,586,176 B; host87 end62,769,532,928 B.
Run929/PID25960 terminal05:59:34 passes fresh lighting, prior field and movement
gates; all15 settings audited and exact profile restored. End free62,768,562,176 B.
259 Python checks pass. New evidence232,568 B text/129,132 B inspected JPEG;
no new cache/dump/perf/raw files. Known cliff marks/blur remain; no native direct
game shader, reload/both-eye or speedup claim. Hashes and exact windows in
20260907_0602_owned-lighting-pass.md.

Replacement validated before cleanup: removed11 exact obsolete files, host85/86,
material25/CPU23 stdout/stderr,927/928 text and primitive-shader JPEG.
608,941 logical B removed; immediate free62,767,718,400→62,768,340,992 B:
**622,592 B (608 KiB) actually reclaimed**, credited once. Build logs reproducible;
exact retired runtime text/image no longer retained. Preserve current fixture and
host87 logs,929/image, selected cooked asset, all distinct protected evidence.
No owned renderer/build remains.

Retained material tree7,686,080 B; aggregate build logs179,992 B; window
images10,235,532 B. Known component growth10,112 B: material+2,925,
logs-2,757, image-6,287, runtime text+1,383, exe+2,560/PDB+12,288.
Lighting fixture, other objects/source/Git/metadata deltas not fully attributed.
Growth covers ownership/test code, not an extra retained verification set.
Cleanup-end free58.46 GiB; drive-wide use+5,267,456 B (5.02 MiB) from first measure.
Same original budgets/floor/raw0 gate; no new allowance. Review equivalent
verification for replacement at the next checkpoint, not accumulation.

### Ordered material-feature continuation, 2026-09-07

Parent892ef66 verified/pushed; first free62,964,940,800 B, no active producers.
The prior post-push free62,961,704,960 B and this first measure differ by3,235,840 B;
not credited to cleanup. Same original3 GiB exception,62,509,998,080 B floor,
raw0 allowance. Planned <=192 MiB link overlap; enforced build256 MiB/runtime
192 MiB free-drop stops,400 KiB log/160 KiB JPEG and10 MiB aggregate logs/images.

Material27/PID28560 and CPU25/PID26240 pass; host88/PID22220 passes with host
objects/link only, no guest objects or shader regeneration.261 Python checks
pass. One initial source-guard spelling error and one wrapper patch-order error
were corrected before runtime; no failed renderer attempt. Run930/PID30484,
06:22:52-06:23:53, passes fresh material-feature/lighting and existing field gates.
All15 settings audited, exact116 B profile restored; no owned producer remains.
The234,945 B text and inspected147,837 B image replace equivalent prior evidence.
Post-run cache/dump/perf/raw inventory finds zero new files. Known cliff marks
and distant blur remain; no native direct game shader, reload/both-eye or speedup
claim. Sources, exact windows and hashes:20260907_0625_owned-material-features.md.

After replacement qualification, removed eight exact obsolete agent-created files:
host87, material26/CPU24 stdout/stderr,929 text and lighting-pass JPEG.
364,432 logical B removed. Immediate free62,939,676,672 ->62,938,685,440 B fell
991,232 B while cleanup ran: actual reclaimed bytes cannot be isolated, and no
positive measured recovery is credited. Build logs reproducible; exact retired
runtime text/image gone, hashes and observations retained in the previous report.
Keep current fixture/host88 logs,930/image, selected cooked asset and all distinct
protected GPU/baseline/flat/VR/movement/unresolved-failure/raw evidence.

Retained material tree7,732,718 B; aggregate build logs183,980 B; window images
10,254,237 B. Known component growth109,084 B: material+46,638, logs+3,988,
image+18,705, runtime text+2,377, exe+4,608/PDB+32,768. Growth covers new ownership
code/tests and replacement evidence, not another verification set. Other objects,
source/Git/metadata deltas unknown. Cleanup-end58.62 GiB free; drive-wide use
+26,255,360 B (25.04 MiB) from first measure, not wholly attributed to task files.
Original floor/budgets and next-checkpoint replacement review remain unchanged.

### Owned material-sampler continuation, 2026-09-07

Parent35c0448 verified/pushed. First free62,765,817,856 B; no producer or scoped
new output explained the preceding491,520 B difference. Later prebuild free
63,015,092,224 B rose without task cleanup; no credit claimed. Same original
3 GiB exception,62,509,998,080 B floor, raw0 allowance; <=192 MiB planned link
overlap and existing build256 MiB/runtime192 MiB free-drop stops. Text400 KiB,
JPEG160 KiB, aggregate build logs10 MiB/images10 MiB; retries share these caps.

Sampler1/CPU1, material28/CPU26, binding20/21 and CPU18/19 pass. Host89/90 pass
with host objects/link only; no guest or shader regeneration.263 Python checks
pass. Run931/PID31668 terminal06:47:28 failed coverage with0 sampler checks/draws:
plain2D eligibility rejected the uploader's array2D views. No image, exact profile
restored. Added a boundary fixture for actual view types before the retry.
Run932/PID17888,06:54:27-06:55:25, passes fresh post-event sampler and all prior
field/movement/light/fog/shadow gates. All15 settings audited, exact profile
restored; no owned producer remains. No new raw/perf/cache/dump/cooked files.
Text234,233 B and inspectedJPEG135,574 B replace equivalent prior evidence.
Known cliff marks/blur remain; direct native shader, cold-load/reload, both-eye
and speedup unproven. Hashes and contracts:20260907_0655_owned-material-samplers.md.

After validation, removed19 exact superseded agent files: host88/89,
material27/CPU25, binding19/20 and CPU17/18 stdout/stderr;930/931 text and
material-feature JPEG.738,922 logical B removed; immediate free62,879,055,872
->62,879,813,632 B: **757,760 B actually reclaimed**, credited once. Reproducible
build logs and retired exact runtime text/images are gone; hashes/observations
remain in research. Preserve current fixture/host90/932 evidence, selected
asset and all distinct protected GPU/flat/VR/motion/failure/raw evidence.

Retained material tree7,761,059 B, texture fixture tree62,769,999 B, build
logs182,380 B and window images10,241,974 B. Known retained growth221,693 B:
material+28,341, texture fixture+148,023, logs-1,600, image-12,263, runtime
text-712, exe+6,656/PDB+53,248. Growth supports ownership/boundary regression
code and replacement evidence, not an extra retained verification set. Other
objects/source/Git/metadata deltas unknown. Cleanup-end58.56 GiB free;
drive-wide free increased113,995,776 B from first measure, mostly unattributed
concurrent activity, not cleanup credit. Original limits/floor remain unchanged;
review and replace equivalent evidence at the next checkpoint.

### Host light-selection continuation, 2026-09-07

Parent a2004cb verified/pushed. First source-only free62,872,088,576 B;
output preflight62,842,773,504 B, no active producers. The intervening drop had
no large task outputs; no cleanup credit. Same original3 GiB exception,
62,509,998,080 B floor/raw0 gate; <=192 MiB link overlap planned, build256 MiB
and runtime192 MiB free-drop stops.75 s/400 KiB log/160 KiB JPEG and10 MiB
aggregate build logs/images, all attempts counted together.

Material29/PID27160,CPU27/PID26168,host91/PID29640 pass. No guest objects or
shader regeneration; codegen0 written/up to date.267 all-boundary Python checks
pass. Run933/PID31508,07:26:37-07:27:38, passes fresh selection/rebuild/candidate
comparisons and all prior field/movement/light/fog/shadow/sampler gates. All15
settings audited,116 B profile restored exactly, no producer remains. No new
raw/perf/cache/dump/cooked outputs.249,997 B log and inspected144,660 B JPEG
replace equivalent prior evidence. Contracts/hashes:20260907_0727_native-light-selection.md.

Cleanup removed eight exact superseded agent-created files after replacement
qualification: host90,material28/CPU26 stdout/stderr,932 text and sampler JPEG.
373,861 logical B removed; immediate free62,939,316,224 ->62,939,705,344 B:
**389,120 B actually reclaimed**, credited once. Build logs reproducible; exact
retired runtime text/image no longer retained, hashes/findings remain in research.
Keep host91/material29/CPU27/933/image, selected asset and all distinct protected
GPU/baseline/flat/VR/movement/failure/raw evidence. No raw archive allowance added.

Retained material tree7,912,406 B, build logs183,089 B, window images10,251,060 B.
Known component growth277,258 B: material+151,347,logs+709,image+9,086,
runtime text+15,764,exe+14,336/PDB+86,016. This retains new ownership/fixture code
and replacement evidence, not another verification set. Other objects/source/
Git/metadata deltas unknown. Cleanup-end58.62 GiB free; drive-wide free increased
67,616,768 B (64.48 MiB) from first measure, mostly unattributed activity, not
cleanup credit. Original budgets/floor unchanged; next review replaces equivalent
verification rather than accumulating it.

### Native primary-shadow image continuation, 2026-09-07

Parent95cd24c verified/pushed. First source-only free62,739,849,216 B;
output preflight62,890,577,920 B. No active renderer/build producer or new large
task output explained the earlier drop from the previous closing measurement
or subsequent free-space rise; no cleanup credit. Same original3 GiB exception,
62,509,998,080 B floor/raw0 gate and prior <=192 MiB link overlap estimate.
Build256 MiB/runtime192 MiB free-drop stops;75 s/400 KiB log/160 KiB JPEG and
10 MiB aggregate logs/images unchanged. No new tree, shader generation or cook.

Output15/PID28844,post-output CPU1/PID26548 and host92/PID28092 pass. Codegen0
written/up to date, host objects only;271 Python source/scenario checks pass.
Run934/PID20808,07:56:07–07:57:05, passes300 fresh native shadow handoffs and all
prior field/movement/light/fog/sampler gates. Fifteen settings audited,116 B
profile restored exactly; no producer remains. No new raw/perf/cache/dump/cooked
outputs.239,424 B log and inspected135,867 B JPEG replace prior equivalent
evidence. Known cliff marks/blur remain; no direct native rigid draw, reload,
both-eye or speedup claim. Contracts/hashes:20260907_0757_native-shadow-images.md.

After qualification, removed six exact superseded agent-created files:
host91 and output14 stdout/stderr,933 text and light-selection JPEG.397,349
logical B; free62,888,697,856 ->62,889,103,360 B, **405,504 B reclaimed** once.
Build logs reproducible; exact retired runtime text/image gone, hashes/findings
retained in prior research. Preserve output15/CPU1,host92,934/image,material29/
CPU27 and all distinct protected GPU/flat/VR/movement/failure/raw evidence.

Texture fixture tree62,912,102 B; build logs187,513 B; window images10,242,267 B.
Known retained component growth158,905 B: texture fixture+142,103,logs+4,424,
image-8,793,runtime text-10,573,exe+7,168/PDB+24,576. New ownership/fixture code
and replacement evidence, not an additional runtime set. Other object/source/
Git deltas unknown. Cleanup-end58.57 GiB free;1,474,560 B (1.41 MiB) used from
output preflight. Drive-wide free rose149,254,144 B (142.34 MiB) from the first
source-only measure, mainly unrelated/unattributed activity, not cleanup credit.
Original budgets/floor unchanged; next review replaces equivalent evidence.

### Direct rigid-shadow continuation, 2026-09-07

Parent b9d3076. Same original3 GiB exception,62,509,998,080 B operational
floor/raw0 gate; no reset. Output preflight62,877,323,264 B, unchanged116 B
owner profile and no active producers. Existing build256 MiB/runtime192 MiB
free-drop stops,75 s/400 KiB log/160 KiB JPEG, aggregate logs/images10 MiB.
Reuse existing CPU/GPU/host trees and selected cooked asset; no new raw or cook.

Output16/PID27088 failed on a Windows max macro in the new overflow assertion;
fixed before output17/PID29516 and CPU2/PID24408 passed (0.34/0.36 s).
GPU build24/PID19644 and rigid03/PID31484 passed four8x8 two-eye cases with
zero validation errors/warnings,1.07/1.08 s. Host93/PID29384 passed host-only
dependent recompilation/link; codegen0 written/up to date, no guest/shader
objects rebuilt.275 all-boundary Python checks pass.

Run935/PID24424,08:43:40–08:44:42: first direct native rigid caster at frame787;
fresh post-event reports1987/2287 add300 submissions/300 fence retirements.
Prior field, movement and native shadow-image gates pass. Sixteen settings
audited, exact profile restored, no producer survives.242,052 B runtime text
and inspected148,199 B JPEG; no new raw/perf/cache/dump/cooked outputs.
Normal profile keeps the acceptance switch off. Scene/receiver drawing,
source-free cold-load/reload, native batching and full-game/both-eye gates remain.
Evidence/hashes:20260907_0846_direct-rigid-shadow.md.

After validation removed14 exact obsolete agent files: host92,output15/CPU1,
failed output16,GPU build23,rigid02 stdout/stderr,934 text and shadow-image JPEG.
386,909 logical B;62,832,992,256 ->62,833,393,664 B free: **401,408 B reclaimed**,
credited once. Exact old runtime image/text gone; hashes/findings remain.
Keep current935/image,output17/CPU2,GPU24/rigid03,host93,other distinct fixtures
and all required baseline/flat/VR/movement/failure/raw evidence and game data.

CPU texture fixture tree65,095,361 B; GPU fixture tree10,420,169 B (its growth
not separately measured); logs212,792 B; images10,254,599 B. Known component
growth2,443,146 B: CPU fixture+2,183,259,logs+25,279,image+12,332,
runtime+2,628,exe+23,040/PDB+196,608. New admission/camera fixtures and native
consumer code justify the retained growth; runtime evidence replaces its
predecessor, not a new archive. Other object/source/Git and drive activity
unattributed. Cleanup-end58.52 GiB free,41.89 MiB drive-wide use from output
preflight, not all task-owned bytes. Next checkpoint retains the same budget
and replaces equivalent evidence after qualification.

### Rigid sampled-array contract continuation, 2026-09-07

Parent ce56e24; same original3 GiB exception/62,509,998,080 B floor/raw0 gate.
Preflight62,715,219,968 B free, no active producer, exact116 B profile unchanged.
Existing GPU tree only; estimated overlap under16 MiB, wrapper256 MiB free-drop
stop/300 s timeout,30 s CTest,10 MiB aggregate build logs. No host build, game
run, captures, new tree or cook. Host93/run935 remains the live-game evidence.

GPU build25/PID4756 and rigid04/PID23632 pass: production native shader with
explicit single-layer array views and D32/S8 sun depth, four8x8 two-eye cases,
max error0.0000404567, zero validation errors/warnings,1.16 s CTest.276 Python
checks pass. Contracts/hashes:20260907_0906_rigid-array-view-contract.md.
Live scene/receiver routing, batching and cold-load/reload remain open.

Replaced and removed four superseded GPU build24/rigid03 stdout/stderr logs:
3,997 logical B, free62,715,523,072 ->62,715,531,264 B, **8,192 B reclaimed**
once. Old exact text is gone; findings remain and tests can be rerun. Keep
build25/rigid04 and all current host/runtime, distinct GPU and protected evidence.
GPU fixture tree10,433,599 B (+13,430); aggregate logs212,743 B (-49), known
component growth13,381 B. Source/generated PS/CMake/Git and unrelated volume
changes not fully attributed. Cleanup-end58.41 GiB free;311,296 B drive-wide
free gain from preflight, not all cleanup credit. Budget unchanged.

### Direct rigid scene continuation, 2026-09-07

Parent9759a75; same original3 GiB exception/62,509,998,080 B operational floor,
100 MiB diagnostics/10 MiB logs/images and raw0 gate. No budget reset. Previous
post-push free62,712,070,144 B; current output preflight63,036,755,968 B. That
initial free gain is unattributed, not cleanup. Original checkpoint starting
free65,462,788,096 B and protected historical raw252,177,116,500 B/27,131 files
remain the accounting baseline; no new raw allowance or historical deletion.

CPU output18/PID30216 passed, ending62,899,900,416 B free:136,855,552 B
(130.52 MiB) drive-wide transient use exceeded the initial under32 MiB fixture
estimate. Material30/PID972 passed, ending63,035,432,960 B, returning most of
that free space. Output attribution and true peak are unknown, not zero.
Remaining host/fixture overlap was corrected to under192 MiB; existing256 MiB
free-drop/300 s supervisor and original cumulative floor stayed enforced.
CPU3/PID25628 and materialCPU28/PID30776 passed. Host94/PID30444 failed on a
missing include; host95/PID28064 passed after the fix,63,033,151,488 B free.
No guest rebuild or shader change; GPU25/rigid04 evidence remains applicable.

Pre-runtime free63,160,893,440 B; the intervening free gain is unattributed.
Run936/PID28504,09:42:32-09:43:43, passes the fresh field/movement gates and
adds300 direct native scene draw emissions/300 fence retirements, plus matching
caster/image deltas. First observed native scene/caster draws at frame766.
17 settings audited, exact116 B owner profile restored, no producer survives.
New242,091 B runtime log and inspected141,437 B JPEG, no raw/perf/cache/cook/dump.
Runtime stops remain75 s/192 MiB free drop/400 KiB log/160 KiB JPEG and the
aggregate10 MiB image ceiling. Single mono image is not sequence/reload/both-eye
qualification. Findings, exact hashes and source limitations are in
20260907_0946_direct-rigid-scene.md.

A final test-only change proves the plan is the sole remaining geometry/albedo/
shadow owner after all source copies retire, then proves expiration. Output19/
PID30400 and CPU4/PID29012 pass; no additional host rebuild/game run required.
0.34 s assertions/0.36 s CTest; materialCPU28 is0.12/0.14 s. All280 Python
source/scenario checks pass. Reused existing fixture trees; no new outputs beyond
bounded replacement build/test logs and binaries/objects.

After validation removed18 exact superseded agent files: host93/failed94,
output17/18, CPU2/3, material29/CPU27 stdout/stderr and935's log/caster JPEG.
431,108 logical B; free63,143,288,832 ->63,143,739,392 B: **450,560 B reclaimed**
once. Exact old runtime text/image are gone, hashes/findings remain; fixture
logs can be regenerated. Keep936/image,host95,output19/CPU4,material30/CPU28,
GPU25/rigid04 and all distinct protected baseline/flat/VR/movement/failure/raw
evidence, source, game data and build trees. No active producer at cleanup.

Texture CPU tree65,863,979 B (+768,618), material CPU7,968,151 B (+55,745),
GPU tree unchanged10,433,599 B. Build logs187,086 B (-25,657), window images
10,247,837 B (-6,762), runtime replacement+39 B. Exe48,493,568 B (+36,352),
PDB108,462,080 B (+200,704). Known component growth**1,029,039 B**, retained for
new native consumer code and stronger source-to-consumer/lifetime tests; no
duplicate successful runtime archive. Other objects/source/Git/volume activity
remain unattributed. Cleanup-end63,143,739,392 B/58.807 GiB free,106,983,424 B
drive-wide free gain from output preflight; only450,560 B is measured cleanup.
Next work shares this same ledger and must replace equivalent evidence after
validation; new raw remains0.

### Native rigid batching continuation, 2026-09-07

Parent62a095f; previous turn made verified progress. Same original3 GiB exception,
62,509,998,080 B floor/100 MiB diagnostics/10 MiB logs/images/raw0 gate; no reset.
Previous post-push free63,143,137,280 B; current output preflight63,115,522,048 B.
The intervening volume use is not fully attributed. No active renderer/build
producer at preflight and the116-byte owner profile hash was unchanged.

Planned host/fixture overlap under192 MiB; reused existing256 MiB build free-drop/
300 s/10 MiB log supervisor. Output20/PID28180 and CPU5/PID28536 pass0.34/0.36 s;
GPU26/PID15436 and rigid05/PID30300 pass five8x8 two-eye cases including two
instances/indirect scene and caster commands.1.21/1.22 s, zero validation errors/
warnings, no raw/images. Host96/PID28072 passes, codegen0 written/up to date,
no guest objects rebuilt.284 Python checks pass. No failed build/run retry.
True peak not separately recorded; sampled frees stayed above the original floor.
Build-end63,116,124,160 B free; movement in drive free space is not cleanup credit.

Pre-runtime63,113,625,600 B free. Run937/PID19332,10:11:14-10:12:15, passes
fresh field/movement and native scene/shadow indirect emission/fence gates:
300 instances and300 calls each,0 merged instances. Multi-instance GPU fixture
coverage is distinct from this live singleton path.243,734 B runtime log and
inspected143,601 B JPEG, no new raw/perf/cache/cook/dump.17 settings audited;
exact profile restored and no owned producer survives. Runtime192 MiB free-drop/
75 s/400 KiB log/160 KiB JPEG and aggregate10 MiB image limits unchanged.
Hashes, source boundaries and pixel limitations:20260907_1016_native-rigid-batches.md.

After validation removed12 exact superseded agent files: output19/CPU4,
GPU25/rigid04, host95 stdout/stderr plus936 log/direct-scene JPEG.393,067 logical B;
free63,110,656,000 ->63,111,065,600 B: **409,600 B reclaimed** once. Exact old
runtime text/image gone; prior hashes/findings retained, tests reproducible.
Keep937/image,host96,output20/CPU5,GPU26/rigid05 and unchanged distinct/protected
baseline/flat/VR/movement/failure/raw evidence. No game data, source or build tree
removed. The historical raw archive and incoming raw0 allowance are unchanged.

Texture CPU tree66,119,132 B (+255,153), GPU tree10,487,791 B (+54,192),
logs212,666 B (+25,580), images10,250,001 B (+2,164), runtime replacement+1,643 B.
Exe48,519,680 B (+26,112), PDB108,617,728 B (+155,648). Known component growth
**520,492 B** for native batch implementation and expanded CPU/GPU fixtures;
replacement evidence is retained until its coverage is superseded. Other objects,
source/Git and unrelated volume activity are not fully attributed. Cleanup-end
63,111,065,600 B/58.777 GiB free,4,456,448 B drive-wide use from output preflight.
Keep the same cumulative ledger/budget for continued lifecycle work.

### Hard-off routing continuation, 2026-09-07

Parent096d732. Same original3 GiB exception and62,509,998,080 B floor;
100 MiB diagnostics/10 MiB logs/images, incoming raw0, no budget reset.
Output preflight63,084,503,040 B free; owner profile hash unchanged and no
renderer/build producer active. The20,017,152 B drive-wide decrease from the
previous post-push measurement is not attributed to source edits or cleanup.
Planned incremental host/CPU-fixture overlap under192 MiB, reusing the existing
256 MiB free-drop/300 s/10 MiB aggregate-log supervisor. No shader changes:
GPU26/rigid05 remains applicable to unchanged programs. Retain937/image until
equivalent replacement passes. New hard-off negative/lifetime cases require
the existing CPU fixture; no new tree, assets, cache or raw capture is planned.
Output21/PID27848 and CPU6/PID14616 passed, then expanded missing-geometry
identity rejection before runtime. Output22/PID24552 and CPU7/PID24236 pass
0.37/0.39 s. Host97/PID27176 and refined host98/PID29904 pass; the latter
rebuilds one host source. Codegen0 written/up to date, no guest objects. All288
Python checks pass. Final host build-end62,884,683,776 B free. Scoped CPU fixture
growth is2,777,301 B; executable/PDB growth40,448 B. The larger drive-wide fall
does not match these retained outputs and is not fully attributed. No producer
was left running; no extra tree or capture was created to investigate it.

Pre-runtime62,816,825,344 B free. The minimal new runtime output still fit the
original floor and the existing192 MiB per-run free-drop limit. Run938/PID31104,
10:42:22-10:43:25, passes cold-start hard-off admission before first native draw
at frame764 plus all existing fresh field/movement/scene/shadow gates.300 scene
and shadow admissions/emissions, merged instances0, no hard-off refusal.
Actual selected-asset teardown/reload remains pending. All18 settings audited;
exact116 B profile restored. New259,886 B log and inspected119,222 B JPEG;
raw/perf/cache/cook/dump outputs0. Runtime-end62,728,318,976 B free; no owned
producer survives. No shader/GPU rerun. Details and hashes are in
20260907_1045_native-rigid-hard-off.md.

Removed14 verified superseded agent files after938/image passed: output20/21,
CPU5/6,host96/97 stdout/stderr plus937 log/batch JPEG.422,147 logical B;
free62,644,748,288 ->62,645,186,560 B: **438,272 B actually reclaimed** once.
Old runtime text/image no longer retained; hashes/findings remain, tests are
regenerable. Keep938/image,host98,output22/CPU7, unchangedGPU26/rigid05 and all
distinct protected baseline/flat/VR/movement/failure/raw evidence. No source,
game data, profile or active build tree deleted.

CPU tree68,896,433 B (+2,777,301), GPU tree unchanged10,487,791 B,
build logs184,150 B (-28,516), images10,225,622 B (-24,379), runtime+16,152 B,
exe48,527,360 B (+7,680), PDB108,650,496 B (+32,768). Known component growth
**2,781,006 B** for the linked real model registry/lifetime fixture and native
guard, with one replacement runtime set. Other objects/source/Git and unrelated
volume activity remain unattributed. Cleanup-end58.343 GiB free;439,316,480 B
drive-wide use from preflight is not all task storage. Actual peak not separately
recorded; supervisors preserved the original floor. No more build/run queued.
The next producer must remeasure/reconcile this same budget before launching;
only135,188,480 B remained above its operational floor at cleanup, not a new
allowance. New raw stays0 and the protected historical archive is unchanged.

### Selected-asset reload continuation (2026-09-07)

Same original65,462,788,096 B start, owner3 GiB exception and
62,509,998,080 B operational floor; no budget reset. Before new outputs,
losslessly NTFS-compressed64 already-retained capture files in the existing
top-level logs/capture directory: frame_1788581218_0.raw through
frame_1788581219_63.raw,530,842,880 logical B. Exact regular-file names,
pre-September6 timestamps, parent paths and non-reparse status were checked.
All64 SHA-256 values matched before/after; no content/path/file-count change,
deletion, archive copy or new raw allowance. Trial file compressed to1,495,040 B;
its hash remains12A25FE9279BFDF5C8B384CE139BD3B5C8737A2727A6F1E286B73CE0E512626A.
Trial free63,113,867,264 ->63,120,060,416 B; remaining63 files
63,115,436,032 ->63,386,419,200 B. Combined observed gain277,176,320 B,
counted once. Batch session96070 completed63/63 in9.714 s; no producer remains.
Compression is reversible with compact /U given sufficient expansion room.
The separate roughly473 MiB earlier free-space increase is unattributed,
not credited as cleanup. Protected raw logical union remains252,177,116,500 B.

11:26 preflight63,374,901,248 B free, no renderer/build/compact processes.
Exact116 B owner profile hash still2F1BC38D763A1B7BDBA31F560684FD4AA7E42A714600B8D344F19DA7F38E23B0.
Existing aggregate build logs184,150 B. Planned minimum outputs: existing
post-output CPU target23/test8 and incremental host99, estimated160 MiB peak
overlap, enforced256 MiB per-producer free-drop and original floor;300 s timeout,
10 MiB aggregate logs. These connect actual source destruction and native
submission/emission/fence generations to a same-process title round trip.
289 Python source/scenario checks pass. No shaders, asset cooking, new build
trees or raw captures. Retain replacement build/test evidence after validation;
runtime/pixel acceptance and its exact bounded output accounting remain pending.

Output23/PID28788 failed on the Windows max macro; corrected locally before
output24/PID25660 passed. CPU8/PID29008 passes in0.40 s. Host99/PID30444
failed on a missing instance include, host100/PID27164 on an anonymous-namespace
autoplay cvar link; host101/PID30008 then passed. Host102/PID28268 adds explicit
output-window baseline records and passes one-source rebuild/link in3.36 s.
All codegen passes report0 written/up to date; no guest objects rebuilt.
292 Python checks pass. These attempts share the original budget and their
small failed logs remain until replacement runtime evidence is validated.

Pre-runtime free63,344,873,472 B. Extend the same existing runner for one
180 s mono title round trip:800 KiB total log, each field epoch still400 KiB,
one final <=160 KiB JPEG within unchanged10 MiB aggregate image limit,
192 MiB supervised free-drop and original floor. New raw/perf/cook/dump
allowance0. Two independently readiness-gated native output windows plus
source destruction and old GPU-fence retirement are required; failures do
not qualify reload. Exact profile restoration/owned process shutdown remain
in finally. No shader/GPU fixture rerun for unchanged shader programs.

939/PID24732 ended11:34:51 after a real title round trip but failed sampling
acceptance. Selected93 retired1379 draws per view, then206/instance384 rendered
again. The600-emission cold prefix lacked two fresh caster/admission windows;
later gameplay retired206 before the wrapper refused extra lifetime events.
Retained failed log520,428 B, no image/raw/perf/cook/dump; exact profile restored.
Source/fixture regression uses the observed guard-before-context ordering and
now requires900 emitted instances per consumer per field epoch, without weaker
freshness.293 Python checks pass; output25/PID27012 and CPU9/PID27176 pass in
0.40 s CTest. Host103/PID26000 passes3.32 s incremental one-source/link,
codegen0 written/up to date; exe48,547,840/PDB108,756,992 B. No owned producers.

After replacements passed, removed18 old reproducible build/test logs:
host99/100/101/102,output22/23/24,CPU7/8 stdout/stderr.14,264 logical B;
free63,330,750,464 ->63,330,783,232 B,32,768 measured B reclaimed once.
The earlier277,176,320 B compression gain is separate and not counted again.
Retain host103/output25/CPU9,938 passing pixels/log and failed939 until its
corrected runtime passes. No game/source/profile/build tree or raw deleted.
The next retry uses the same180 s/800 KiB/160 KiB JPEG/192 MiB drop/original
floor, not another budget. See20260907_1140_native-rigid-reload.md for evidence.

Corrected run940/PID28128,11:44:43-11:46:46, on unchanged host103 binary now
published as86f00d4: **PASS**, all19 settings audited, no owned producer remains,
exact profile restored. Generation93/instance144 ->207/389 through real title
shutdown.901 fresh scene/shadow emissions per ready-field epoch; all1676 old
submissions retire before title. Each independently checked epoch also adds300
fresh native admission/emission/retirement samples and observed movement. Final
1920x1080/124,696 B JPEG inspected; coherent scene, known cliff marks/blur remain.
Log483,446 B; no raw/perf/dump or940 cache output. No shader/GPU rerun, full-game,
both-eye, sequence stability, repeated-instance batching or speedup claim.

940 preflight63,172,657,152 B free ->runtime-end63,098,085,376 B. Later audit
found one68 B native material entry created in939 at11:34:51, not940:
cache/native_materials/v1/561cb848b5104e0c.bdmat. Unexpected relative to the
zero-cook plan; retain its single versioned reusable cache representation and
include the bytes, rather than claiming zero. No new raw in either run.

Once940/pixels passed, deleted five replaced agent files:938/939 runtime logs,
938 hard-off JPEG,host98 stdout/stderr.900,296 logical B; actual free
62,970,437,632 ->62,971,351,040 B,913,408 B recovered once. Old exact diagnostics
are gone; their findings/hashes remain, including939's regression-covered
sampling failure. Keep940/image,host103/output25/CPU9,unchangedGPU26/rigid05 and
all distinct protected baseline/flat/VR/movement/failure/raw evidence.

Totals for this continuation:277,176,320 compression +32,768 build-log cleanup
+913,408 replacement cleanup =278,122,496 measured B reclaimed, no double credit.
Known retained growth471,903 B: CPU fixture115,818 (tree69,012,251),
exe20,480/PDB106,496,build logs7 (aggregate184,157),JPEG5,474 (aggregate10,231,096),
runtime223,560,cache68; GPU tree unchanged10,487,791. The longer two-epoch log
and lifecycle fixture are new coverage; replace on equivalent qualification.
Other objects/source/Git and unrelated volume activity are not fully attributed.
Output-preflight63,374,901,248 ->cleanup-end62,971,351,040 B (58.647 GiB):
403,550,208 B drive-wide use. Actual peak not separately sampled; existing
supervisors enforced the floor/drop/log/image limits. No new raw allowance,
no active producer or next run queued; remeasure this same budget next time.

### 2026-09-07 receiver callback/owned packet continuation — output preflight

Same original65,462,788,096 B baseline, owner3 GiB exception and62,509,998,080 B
operational floor; no reset. Previous cleanup/compression already counted once.
Read-only preflight63,202,775,040 B free; owner116 B profile SHA unchanged,
no renderer/build/test process found. The increase since aa00b09's post-push
62,889,668,608 B is unattributed volume activity, not cleanup savings.

New requirement: qualify replacement of original sub_82176708 by host receiver
setup and one retained image/camera/late-colour packet at the direct scene
consumer.298 artifact-free Python guards/scenario tests pass; runtime pending.
Reuse the output CPU fixture/tree, host tree, existing bounded reload runner,
scenario checker and window inspector. Estimate160 MiB peak new overlap for
incremental fixture/host compilation and link; supervised256 MiB free-drop
limit plus the original floor. Existing300 s build/10 MiB aggregate log caps.
One180 s reload, <=800 KiB log (<=400 KiB independently per field), one<=160 KiB
JPEG,192 MiB runtime free-drop; aggregate images currently10,231,096 B fit the
10 MiB cap with replacement overlap. Raw/perf/dumps zero; no asset cook planned,
inspect any automatic cache output and count it. Retained raw archive remains
252,177,116,500 B/27,131 files with zero incoming allowance.

Keep940/image and host103/output25/CPU9 until the corresponding replacements
pass, then retire only those superseded agent diagnostics. Unchanged GPU26/
rigid05 and distinct baseline/VR/movement/failure evidence remain protected.
No shader changes; no GPU-fixture rerun solely to stamp this source revision.
Builds/runs/actual pixels, sizes, cleanup and terminal process state pending.

Output26/PID31460 built; CPU10/PID31112 failed an existing fixture's scene-plan
assertion and hit the30 s timeout. NativeFogLayers in that test was not value
initialized, leaving disabled-layer payloads indeterminate; changed to fog{}.
No production admission relaxation. Output27/PID29384 and CPU11/PID31436 pass
(0.38 s CTest); ordering/finite/stamp/lifetime assertions run. Host104/PID29348
also passes,13 host objects plus link, no guest objects; codegen0 written/up to
date. Binary48,558,592 B SHA51E715A16727ACEC27AF74D00D3A8E193F105EB9F19B9DE7D53D8472690F5E5F,
PDB108,822,528 B, watermark aa00b09 dirty. All producers terminal.
Post-test free63,200,669,696 B. Live receiver qualification still pending;
no raw/perf/cook/capture output produced in these fixture/build commands.

After fixture acceptance, removed eight superseded output25/26 and CPU9/10
stdout/stderr logs (2,870 logical B):63,200,071,680 ->63,200,079,872 B,
8,192 B recovered once. Runtime941 preflight63,047,393,280 B; the intervening
volume change was unattributed. PID23416/session68217 ran12:22:17–12:24:10,
terminal with exact owner profile restored. No image/raw/perf/dump accepted.
Native receiver original/refused/missing remained zero; cold title closed all
1688 old submissions per consumer, but the second epoch failed the existing
light-selection comparison:237FA614 view0 output237FA618 actualFFFFFFFF vs
expectedFFFFFFFE. Keep941's430,499 B log as unresolved failure evidence and940/
image as last accepted live evidence. Failure SHA
F177A5A1AF65403CBAE585E18474BBD86997BA74614E376A12832947751B4C36.

Read the full selector/rebuild/insertion/classification source and the
sub_8218A998 invalidation writer. Source contains all-bits-set invalidations,
but941 lacks pre/post producer observations to establish an actual race or
attribute the failure to this receiver change. Do not silently retry/relax it.
Added a16-view dirty-output/missing-output regression using the production
comparison helper, a299th Python test preserving941's exact failure, and
bounded failure-only context (one header plus five planned/observed writes).
Next outputs are the existing material fixture31/material CPU29 and host105;
estimate<=64 MiB incremental overlap within the original160 MiB plan and
existing supervisors. Preflight63,176,953,856 B. No second game run/capture is
planned until the failed producer boundary is understood. These diagnostics
improve the next investigation; they are not a fix or a live pass.

Final checks: material31/PID31080 builds; material CPU29/PID28688 passes0.13 s
including the16-view strict dirty-output/missing-output regression. Host105/
PID17772 builds one host object/link, no guest objects. Binary48,564,736 B,
SHA E535D6E8E7DE126A5457E2F829AA648A53C1C73F6A3488C907E1D253151FDD80;
PDB108,843,008 B. Host105 not run; host104 remains941's failed live binary,
host103/run940 the last accepted live/pixel evidence.299 Python checks pass.
Source/tests/report committed and pushed6de0c2f, exact remote main verified.

Removed four replaced material30/CPU28 stdout/stderr logs (2,203 logical B):
63,175,114,752 ->63,175,118,848 B,4,096 measured B recovered. With prior8,192 B,
this continuation reclaimed12,288 B from12 superseded reproducible logs;
no historical raw, accepted live evidence, game data or build tree removed.
Keep output27/CPU11,material31/CPU29,host105 diagnostic build,host104/941 failure,
host103/940/image accepted baseline and unchangedGPU26/rigid05.941's unresolved
430,499 B log is new coverage with cleanup trigger: reproduce/explain/fix its
dirty-output mismatch and replace with accepted equivalent reload/pixel proof.

Known retained net growth661,444 B: texture CPU tree+116,739 (69,128,990),
material fixture+7,862 (7,976,013), exe+16,896/PDB+86,016, build logs+3,432
(187,589 aggregate),941 failure log430,499. Images unchanged10,231,096;
runtime audit confirms zero new raw/cache/HLSL dump/perf CSV. Other host
objects/source/Git and unrelated volume changes are not fully attributed.
Output-preflight63,202,775,040 ->cleanup-end63,175,118,848 B (58.836 GiB free):
27,656,192 B (26.375 MiB) drive-wide use, distinct from known retained bytes.
Actual peak not separately sampled; supervisors enforced the original floor,
drop/time/log caps. All owned jobs terminal; exact116 B owner profile restored
(SHA2F1BC38D763A1B7BDBA31F560684FD4AA7E42A714600B8D344F19DA7F38E23B0).
No next game run/capture queued. Live acceptance remains pending, not relabelled.

### 2026-09-07 light-selection producer continuation — diagnostic preflight

Previous turn made source/fixture progress (6de0c2f,bb0e73c), not live acceptance.
Worktree clean; reuse host105's already-built failure-only context, no restamp
build. Current free63,182,209,024 B; same original65,462,788,096 B baseline,
3 GiB exception,62,509,998,080 B operational floor and zero incoming raw gate.
One text-only180 s/800 KiB existing reload diagnostic is needed to distinguish
the dirty-mask mismatch using the added before/after context. No new image,
raw/perf CSV/dump/cook planned; <=1 MiB named output, existing192 MiB runtime
free-drop/aggregate diagnostics limits enforced. This is a bounded investigation,
not a retry campaign to obtain a pass. Keep941's failure and940/image's accepted
baseline; retain942 only for new diagnostic information or equivalent qualified
replacement after the cause is resolved. A successful isolated rerun cannot by
itself explain941. Exact owner-profile restoration remains guaranteed.

942/PID31128/session31599 ran12:40:55–12:41:05 and exited0 through the title-menu
Exit route before field loading; wrapper correctly failed missing acceptance.
Final log21,237 B; all20 settings applied, profile restored. No light-selection
observation or image. Actual immediate preflight63,179,436,032 B, closing
63,178,510,336 B; no duplicate/live producer. A terminal-diagnostic regression
now distinguishes this from merely pending field evidence (300 Python checks).

Source shows autoplay owns the pad but HoverTitleRows independently changes the
title cursor from host mouse position before the original title update. The
source path can combine hover of Exit with an autoplay confirm;942 does not log
pointer events, so exact user-vs-hover attribution is unknown. The existing
bd_mouse_menu=false setting now isolates menu hover in the temporary automated
diagnostic profile (21 settings); manual defaults/persistent profile unchanged.
One replacement text probe943 is justified to reach the original failed boundary
with this uncontrolled input path disabled, not to weaken any render test. Same
180 s/800 KiB/192 MiB-drop caps, original floor/aggregate budgets, no new raw or
image/cook. Reuse host105, no build. Reconcile942 before retry; keep its small
log until startup succeeds and its findings are recorded.

943/PID30948/session61243 is terminal,12:45:40-12:47:42. Host105 was reused,
all21 settings applied, owner116 B profile SHA unchanged. Text gates pass in
both independent epochs, including receiver original/refused/missing zero and
300 fresh consumer reads in the newer window. Old93/144 closes all1656 draws
per consumer before title; new207/385 then emits fresh native work. Each epoch
adds900 scene/shadow emissions after readiness. No941 mismatch occurred: this
does not identify its writer or fix it. No new pixels/sequence/both-eye claim.
943 log486,641 B SHA
E4807B454BDB1BCAAF3D811F131F35DC533EE4C411AF541F46A64477A37D3A08.
Preflight63,175,229,440 B; immediate closing63,174,180,864 B.

300 artifact-free Python checks pass in0.123 s; existing943 log independently
passes --rigid-reload --receiver-setup. No C++ changes/builds in this follow-up.
Scoped audit since12:40:55 finds zero new cache/HLSL/raw/perf-CSV/dump files.
No renderer/debugger producer remains and no next run is queued. Retain943 for
new two-epoch receiver text coverage, replaced on equivalent full acceptance;
keep941 for its unresolved failure and940/image for last accepted pixels. All
protected historical raw/baseline/VR/other failure evidence remains untouched.

942's startup-only failure is now superseded by the regression, documented
input isolation and943 reaching both fields. Its21,237 B log, SHA
F5E2A8B27DD4C7D280224C5392D8CF63AAC6E7CF815B1C6A084990F8470F950E,
is eligible for exact-path cleanup; original pointer events were never logged.
Remove only that agent diagnostic, not941/940 or any source/game/build data;
record actual recovery below after deletion. No repeated cleanup credit.

Completed exact-path942 cleanup after size/hash/reparse/process checks:
63,170,146,304 ->63,170,170,880 B free,24,576 B recovered once. One obsolete
agent log removed; exact diagnostic unavailable, findings/hash/regression kept.
No game data/profile/source/build tree/protected raw or failure evidence removed.
Current retained diagnostic growth is943's486,641 B; no new build/image/cache/raw
outputs. Other source/Git/volume activity is not fully attributed. From this
follow-up's63,182,209,024 B starting free to cleanup-end63,170,170,880 B:
12,038,144 B drive-wide use, ending58.832 GiB. These are not all renderer bytes.
Original cumulative floor/budgets remain unchanged; final post-push free-space
measurement belongs in the checkpoint handoff, not a new restamp build.

Owner's newest request is status/README/commit/push. No further renderer build,
game run, capture or debugger job is authorized by that documentation update;
leave941's cause and full receiver pixel acceptance explicitly pending.

### 2026-09-07 owned scene-lighting bundle

Goal continuation resumes implementation after the committed workflow revision.
Starting measured free62,937,022,464 B; pre-build62,932,135,936 B. Reuse the
original65,462,788,096 B baseline,3 GiB exception,62,509,998,080 B operational
floor,100 MiB diagnostics,10 MiB aggregate build logs/images and incoming raw0.
No live producer remains. Existing host105 and fixtures are reused incrementally;
planned material32/CPU30 and host106 need <=192 MiB peak overlap (host objects,
linker/PDB and fixture replacement), enforced by the existing300 s/256 MiB-drop
supervisor and original floor. No guest/shader rebuild, cook or new build tree.
Retain only current build/test logs until replacement passes, then retire their
exact superseded agent logs. Keep940/image,941 failure and943 text until proper
equivalent integration replacement. A targeted reload/window-image gate follows
CPU/build acceptance, with its existing limits/profile restoration; no raw output.

Material32/PID25736 and CPU30/PID29820 pass; CPU0.15 s,305 artifact-free Python
checks pass. Host106/PID28988/session18083 terminal:48,609,792 B exe,
SHA084A9883CC71C5E639F140B55D1779223B28B13438D64F6A46964C9025E857BC;
PDB108,961,792 B. No guest/shader rebuild; codegen0 written. After accepted
fixture replacement removed four material31/CPU29 logs (2,035 logical B):
62,930,989,056 ->62,930,993,152 B free,4,096 B recovered once.

Run944/PID28276/session30334 terminal13:46:36-13:48:56, host106 reused, all21
settings applied, exact owner-profile hash unchanged. Enforced180 s/800 KiB
text/192 MiB free-drop and original floor,160 KiB JPEG/10 MiB images,raw0.
Pre-run free62,926,897,152 B; immediate close62,920,568,832 B. Both independent
field epochs pass complete reload/receiver plus new scene-lighting checks. New
generation207/instance384 consumes fresh owned lights without source caches;
old93/144 fully fence-retired before title. No941 mismatch; its cause remains open.
944 log490,886 B SHA7630E837C613AE116611D4A38FD25DBD2FAB861648FD2786499B531E2710F8A1.
Inspected135,993 B JPEG SHA6424763E163BC784035E7BD21860B9B86D1A023C002B54C084C11713E3B6D2C0:
coherent terrain/trees/shadows, player partly obscured, known cliff marks. One
mono image only. Scoped post-run audit finds zero new raw/perf/cache/dump files.

After independent944 parser/pixel acceptance, removed exact943 text and two
host105 build logs (487,408 logical B):62,916,591,616 ->62,917,083,136 B free,
491,520 B recovered. Total495,616 B reclaimed once from seven superseded agent
outputs; no protected raw/baseline941/940/image, game/profile/build data removed.
943's exact text is gone; findings/hash remain in earlier reports, stronger944
text/pixels replace its receiver purpose. Keep940/image for the open941 failure.

Known retained net growth462,603 B: material fixture+153,473 (8,129,486 B/41files),
exe+45,056/PDB+118,784, build logs+5,052 (192,641 B), runtime replacement+4,245,
new JPEG135,993 (aggregate images10,367,089 B). Other objects/source/Git/volume
activity unallocated. First measured free62,937,022,464 ->cleanup-end62,917,083,136 B:
19,939,328 B drive-wide use,58.596 GiB free. This is not all renderer growth.
Same original floor/exception/retention obligations; no duplicate jobs or reset
budget. Details in20260907_1342_owned-scene-lighting.md. Final post-push read-only
measurement belongs in handoff; no rebuild merely to stamp the commit hash.

### 2026-09-07 opaque caster-family bundle

Same original65,462,788,096 B baseline,3 GiB exception,62,509,998,080 B floor,
100 MiB diagnostics,10 MiB aggregate build logs/images and incoming raw0.
First measured free63,162,793,984 B; pre-build63,157,366,784 B. Existing texture
fixture and host tree reused with <=192 MiB planned peak overlap,300 s/256 MiB
free-drop supervisor. No guest/shader rebuild, new build tree or asset conversion.

Output28/PID25096 stalled before compilation in sandbox. Read-only process
inspection confirmed its CMake/Ninja30156 tree; stopped the owned tree and
observed terminal session90984/exit1 before output29/PID29648. Output29/CPU12
passed. Final added suppression/capacity fixtures pass output30/PID31364 and
CPU13/PID31052 (0.38 s assertions/0.40 s CTest).310 artifact-free Python checks
pass. Host107/PID30172/session19411 terminal, codegen0 written/module current,
exe48,622,080 B SHA663715CBFD7E69C378A7F50C8F219149CAAE2899CAD128D9001ECA48A5F6757A;
PDB109,068,288 B. No new GPU fixture/shader claim; unchanged GPU26/rigid05 reused.

After fixture29/CPU12 acceptance, removed exact output27/CPU11/stopped-output28
stdout/stderr: six files1,239 logical B;63,136,337,920 ->63,136,342,016 B free,
4,096 B recovered once. Their build/test behavior is replaced and reproducible.

Run945/PID28908/session67557 terminal14:20:22-14:22:20,180 s/800 KiB text/
192 MiB free-drop and original floor. Raw/perf/dumps/cook off; all21 temporary
settings audited, exact owner profile restored. Both independent epochs pass
the complete prior gate plus new non-regression multi-primitive caster evidence.
The newer300-frame window adds12,532 native family emissions and retirements;
selected scene/caster reload counters remain isolated.941's failure stays open.
105,191 B renderer-owned1920x1080 JPEG was encoded in memory under110 KiB,
fitting replacement overlap without deleting protected images or lowering
resolution. Inspected coherent scene/shadows, known cliff marks; mono sanity
only. Aggregate scope/sequence/stereo and full-game requirements remain open.

945 log505,192 B SHABBBB860A54C78D6E802C66D0C2B08C254BA9337BC30B1E1C73370C13DAE1CD8E;
native_caster_family_window.jpg SHADB5B45846632841877805FC646165E9CEF207417E431BF204DD72BF92E0C408C.
Immediate run-end free63,028,518,912 B. Scoped audit: no new raw/perf/cache/HLSL/
dump files or >1 MiB install-tree file during the run; drive-wide use is not all
attributable to renderer outputs. No renderer/build/test process remains.

After independent945 parser/pixel and final fixture acceptance, removed eight
exact superseded outputs:944 runtime text/receiver JPEG, host106 logs and
output29/CPU12 logs.632,465 logical B;62,969,610,240 ->62,970,253,312 B free,
643,072 B actually recovered. Total this bundle647,168 B from fourteen files,
not prior cleanup credited again. Exact944 text/image gone; report/hashes kept,
945 replaces that current acceptance purpose. Keep940/image,941 and all other
protected raw/VR/movement/baseline/failure/game/profile/build data unchanged.
An already absent935 image was inspected as a possible replacement candidate;
it was not deleted and earns no new recovery credit.

Known retained net growth568,300 B: texture fixture+467,033 to69,596,023 B/129files;
exe+12,288/PDB+106,496; build logs-1,021 to191,620 B/136files;
runtime replacement+14,306; image replacement-30,802 to10,336,287 B aggregate.
Material fixture8,129,486 B/41files and GPU fixture10,487,791 B/10files unchanged.
First free63,162,793,984 ->cleanup-end62,970,253,312 B:192,540,672 B (183.62 MiB)
drive-wide use,58.646 GiB free. Other host objects/source/Git/volume activity is
unallocated, not claimed as renderer growth or cleanup savings. Revalidate this
ledger before the next producer. Details:20260907_1420_native-caster-families.md.

Pre-commit read-only recheck fell further to62,695,747,584 B free with no owned
producer running. A scoped all-out recent-large-file audit finds only the
existing fixture object/PDB just rebuilt above; their net growth is already
included. This additional drive-wide use remains unallocated, not cleanup credit
or attributed renderer growth. Only185,749,504 B (177.14 MiB) remains above the
original operational floor: the planned192 MiB next integration overlap does
not fit. Do not launch that producer without rechecking free space and resolving
the existing cumulative budget. Source work/Git handoff need no build or capture.

### 2026-09-07 layered rigid scene bundle

Same original65,462,788,096 B baseline,3 GiB exception,62,509,998,080 B floor,
100 MiB diagnostics,10 MiB build logs/images and incoming raw0. First measured
free62,550,061,056 B; pre-fixture62,628,921,344 B. No owned build, game or fixture
process remains from the preceding source investigation. Drive-wide fluctuation
is unallocated, not claimed cleanup or renderer growth. The192 MiB host integration
estimate does not fit; that producer remains paused. An asynchronous request to
increase the cumulative allowance to4 GiB is pending, not approval. Do not change
the floor or start a larger job on that request alone.

Connected edits now carry independent third-layer UV ownership through native
material packets, explicit four/five-attribute programs, three sampled layers,
whole-node scene admission/queue retention and extended CPU/GPU fixtures.
311 artifact-free Python source/scenario checks pass; no new binary/pixel claim.
Small sequential fixture replacements can fit the existing allowance after a
fresh preflight: material33/CPU31 and output31/CPU14 plan <=32 MiB peak per job;
GPU27/rigid06 plans <=64 MiB including CMake/shader headers and fixture replacement.
The existing supervisor now accepts a tighter per-job free-drop cap, still using
the unchanged original floor,300 s timeout and10 MiB aggregate log limit. No new
tree, guest rebuild, asset cook, raw/image output or owner-profile changes.
Retain current logs until replacement acceptance, then remove exact superseded
fixture logs; keep945 current runtime and940/941 baseline/failure evidence.

Material33/PID31468 and CPU31/PID23768 pass (0.12 s assertions/0.13 s CTest).
Output31/PID30772 and CPU14/PID7612 pass (0.34/0.36 s). GPU27/PID26616 builds
all four production native shaders; final GPU28/PID26900 rebuilds only the
fixture for production inactive-descriptor coverage. Rigid07/PID26432 passes
all12 two-eye8x8 cases in1.08 s, zero validation errors/warnings, max error
3.39895e-5. Raw/images0; one unrelated GOG loader message. Fixture exe SHA
ACF89F7C69B61D2A5A93F8FBDAB23E5D7A086B0011F824416ADC5FFEC94220E9.
Host107 exe/PDB and the owner's profile are byte-unchanged; no game launch/link.
Six real host consumer files pass exact-settings/PCH `-fsyntax-only`: PIDs25168,
23732,29284,31428,24140,31048. All producers terminal. The initial tighter-cap
parameter call failed before creating a producer/output; the ignored-wrapper
edit was corrected before actual material33 launch.

After CPU acceptance removed eight exact old material32/CPU30/output30/CPU13
logs,4,896 logical B:62,627,069,952 ->62,627,082,240 B free,12,288 B reclaimed.
After final GPU28/rigid07 acceptance removed20 exact old GPU26/rigid05,
intermediate GPU27/rigid06 and empty syntax01..06 logs,10,672 logical B. Their
cleanup command measured before/after, but PowerShell's mixed table formatting
hid the values; no second physical-recovery amount is claimed.28 files/15,568
logical B removed in total, with at least12,288 B measured recovery. Logs are
reproducible and replaced; no protected runtime/raw/game/profile/build deletion.

Known fixture/log retained net+806,314 B: material8,159,345 B (+29,859),
texture70,274,593 B (+678,570), GPU10,585,571 B (+97,780), build logs191,725 B
(+105;136files). The added shader header50,885 B and other shader/CMake/source/Git
changes are separate; all four current native shader headers total326,650 B.
First measured free62,550,061,056 ->post-cleanup62,608,248,832 B:58,187,776 B
drive-wide gain,58.31 GiB ending free, not attributed cleanup savings. Only
98,250,752 B remain above the original floor;192 MiB host integration still
does not fit.4 GiB request remains pending, not approval. Recheck before further
producers. New scene game/reload/pixel acceptance remains open;945 is unchanged
last live evidence and941 stays strict/unresolved. Details in
20260907_1501_layered-rigid-scene.md; final post-push free check belongs in handoff.

### 2026-09-07 ordered inherited scene lighting bundle

Same original65,462,788,096 B baseline,3 GiB exception,62,509,998,080 B floor,
100 MiB diagnostics,10 MiB aggregate build logs/images, incoming raw0. The4 GiB
request remains unanswered and is not used. First source-only free62,592,278,528 B;
later62,353,035,264 B paused producers, then65,844,150,272 B before fixture work.
This drive-wide fluctuation is unallocated, not cleanup or renderer savings.
No prior CMake/Ninja/compiler/game/fixture process remains. Existing build logs
191,725 B/136files; material fixture8,159,345 B and texture fixture70,274,593 B
are the replacement baselines. Existing shader/GPU fixture bytes are unchanged.

Connected changes own ordered Bind/Keep light values and speculative tickets,
handoff callback identity imports, direct-node commit and the outbound mirror
for retained consumers. Unknown writes invalidate rather than seed native data.
Devloop reuses material34/CPU32 and output32/CPU15 with <=32 MiB peak each,
300 s supervision, tighter32 MiB free-drop and the unchanged original floor.
Host108's <=192 MiB integration estimate now fits after free-space recovery;
recheck before starting it. No new tree, guest rebuild, shader/GPU rebuild, cook,
raw/image output or profile change is needed for these checks. Retain prior
fixture logs until replacements pass, then remove only exact superseded logs.
945 remains last live evidence;940/941 and protected baseline/VR/raw are unchanged.

Material34/PID28764,CPU32/PID29920 (0.56/0.62 s), output32/PID29284 and
CPU15/PID33180 (0.44/0.48 s) pass.312 Python checks pass. Host108/PID29072/
session2796 terminal/exit0, codegen0 written, no guest/shader compilation.
Exe48,659,456 B SHA4DBABAED0B02FEB65085B0B6D02F0EC68620CD817CCA8425BD8BE22D34375DD1;
PDB109,326,336 B; owner profile unchanged. Game acceptance remains pending.

After acceptance removed10 exact superseded material33/CPU31/output31/CPU14/
host107 stdout/stderr logs,8,124 logical B.65,604,374,528 ->65,604,390,912 B free,
16,384 B reclaimed once. Current logs191,504 B/136files. Known retained net
+380,179 B: material+68,065,texture+16,911,exe+37,376,PDB+258,048,logs-221.
Other objects/CMake/source/Git/volume changes unallocated. Firstfree62,592,278,528
->cleanup-end65,604,390,912 B is a3,012,112,384 B drive-wide gain, not attributed
cleanup savings. No runtime/raw/image/profile/game/build deletion or new capture.
Details and source provenance:20260907_1544_ordered-scene-lighting.md.
