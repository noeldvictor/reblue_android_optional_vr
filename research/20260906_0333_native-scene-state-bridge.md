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
