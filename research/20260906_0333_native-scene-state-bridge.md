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

ccb8dc2 committed/pushed and remote verified. One targeted host108 integration
now budgets <=192 MiB peak,180 s and800 KiB text with all existing rigid/reload/
receiver/scene-light/caster-family comparisons retained. Captures/perf/cook/dumps
off; no new raw/image. This first run asks which broader scene packet boundary
is actually reachable/refused after connecting ordered light ownership; text is
not pixel or representative inherited-draw qualification. The exact owner profile
must be restored in the existing supervisor's finally block. Git reported auto
packing at commit; no continuing Git/build/game process was found before the run.

Run946/PID30068/session11109 terminal15:51:18 after15:50:41 start. All21settings
applied, profile restored exactly, no capture requested. Atframe807 the selected
native scene node submits after its light commit; the next admitted node refuses
whole-node shader resources before field readiness. This is not inherited/family/
reload acceptance. Keep946 text76,605 B, SHA
E52260819C74EA4F318D6D9CB35055FE188BD96479195806D278AAE090FFC76B;945 and941 remain.
Follow-up source check identifies an independently overstrict material mask:
the shader consumes diffuse/specular bits, not an inactive reflection channel.
Add its causal fixture and precise failure-only packet reason/identity before
another probe. Output33/CPU16 <=32 MiB each and host109 <=192 MiB reuse the same
trees and original supervised floor; no new GPU/shader/raw/image work.

Output33/PID24532,CPU16/PID572 (0.44/0.47 s),host109/PID33228/session97407
pass. Run947/PID32972/session29432 terminal15:57:21 after15:56:58 start,
all21settings applied/profile restored. Atframe645 the next packet is instance129/
generation78/node1/primitive0, geometryF9F0B95507EC870C, material63B8D67932573E51,
mask1/layers1/image-mask1. It fails the still-unconditional specular requirement.
947 text74,436 B SHA5A68EB5ED2837607EBE527DE11E9517703F196CB96848DDACB65F49E44F022F1.
The shared material has the specular fields (selected object hasmask3); the object
producer disables shininess and ComposeNativeMaterialFeatures disables specular
with that same owned flag. Follow-up removes only this unused dependency and
canonically packs its inactive GPU fields; active specular still requiresbit2.
Output34/CPU17 <=32 MiB andhost110 <=192 MiB, same cumulative gates, no new
shader/GPU/raw/image output. Keep946/947 evidence until equivalent qualification
resolves/replaces its purpose, not just because a later probe exists.

Final output34/PID27960 and CPU17/PID26656 pass (0.44/0.46 s); host110/PID33656
passes with no guest/shader compilation. Exe48,662,016 B SHA
1E7C4CD87DFEC71D548C9A7080F70898A5F80E0CBEEF59B36F597C4682AE119F;
PDB109,334,528 B. Run948/PID19644/session83944 ends16:03:21 after16:01:09 start.
All21 settings applied, exact116 B profile restored, no capture/perf/raw output.
Fresh cold-field frames1899->2199 add1,265 wider-family scene submissions,
emissions and retirements. Selected scene/shadow close1664/1664/1664 at title;
the reloaded field then fails strict UV comparison visual23820098/channel16.
This is unresolved, not reload/pixel acceptance;945 remains last accepted live.
Retain948 text397,691 B SHA
BB9A0485A1939C7EDBC405D6E927583377FFFEB4F4E76C90780F566194841D9A, plus947.
Sampled layered/inherited draws0; no speed or complete native-frame claim.

After final fixture/build acceptance, removed12 superseded output32/CPU15/
output33/CPU16/host108/host109 logs and946's generic refusal, now replaced by947's
precise packet diagnosis and948 clearing that earlier refusal.13files/86,236
logical B removed; free64,831,266,816 ->64,831,401,984 B (135,168 B observed,
not isolated from unrelated volume activity). Total cleanup this turn23files/
94,360 logical B; earlier16,384 B measured recovery is not credited again.
Known retained net+887,586 B: material+68,065,texture+44,751,exe+39,936,
PDB+266,240,build logs-3,533,retained947/948+472,127. Logs188,192 B/136files;
GPU fixture/images/raw unchanged. Other objects/CMake/source/Git unallocated.
Firstfree62,592,278,528 ->cleanup-end64,831,401,984 B is a2,239,123,456 B
drive-wide gain, not a cleanup claim. No owned producer remains; a reused PID
was verified as an unrelated Android build and left untouched. No game/profile/
protected baseline/build-tree deletion. Original allowance/floor unchanged.

Continuation after28abb64: prior turn is progress (consumer fix and preserved
new UV failure). First measured free62,920,818,688 B, no renderer/build process
found. Source inspection confirms ordinary per-node UV initialization and ordered
override/reset rules;948 lacks actual/expected values, so it cannot distinguish
recipe import from a later staging/flush writer. One bounded failure-only
provenance observation will make that decision. Host111 <=192 MiB peak, same
300 s supervisor/original floor; no guest/shader rebuild. One subsequent <=180 s
capture-disabled reload probe, <=800 KiB text and192 MiB free-drop, captures/perf
off/profile restored. Preserve947/948 and945/940/941 until replacement evidence
qualifies; no new raw/image/cache/tree. Source checks stay artifact-free.
Host111/PID34264/session52412 passes, no guest/shader compilation (codegen0
written); native UV diagnostics only, exe SHA
295E4FB6B8265DCA01701D411BCCBB9EC4088B0D4DC7BEB143AF2C8F3EFDA655.
A Python wiring guard initially found diagnostic reads placed in its broad
consumer section. Relocated that failure-only routine to the source-boundary
section without changing the guard or diagnostic behavior;312 checks pass.
The build had already started before the orchestration surfaced the failed guard;
future producer chaining must check exit codes before launch. No logic restamp.
Run949/PID31872/session41826 terminal16:24:21 after16:21:21 start: cold teardown
1998/1998/1998 completed, reload loaded16:24:00, but180 s expired during the
reloaded opening event. No UV observation or acceptance. Exact profile restored.
Change the observation window, not comparisons: existing supervisor now exposes
180..300 s ReloadTimeoutSeconds, default180; the next diagnostic explicitly300 s
to reach reloaded movement. Same800 KiB log/192 MiB drop and original floor, no
new raw/image. Retain949 until its timing-purpose evidence is superseded.
Run950/PID33028/session69692 terminal16:27:00 after16:25:55 start, stopped by
the cumulative75 MiB small-output threshold before reload; profile restored.
No UV observation. Actual retained scoped runtime logs7,516,376 B/26files plus
historical perf8,954,976 B/20files, fixed49 MiB reservation, GPU fixture10,585,571 B
and build logs191,113 B nearly consume the stop threshold. Reconcile before retry.
Preserve the ten historical CSV+metadata pairs215905..150239 losslessly in one
ZIP under logs/perf, verify every decompressed entry's length/SHA-256 before
removing the redundant originals. Peak<=32 MiB including conservative overlap,
same original floor; source set exactly20files/8,954,976 B, output<=10 MiB.
No raw/game/profile/build/required-evidence deletion; archive remains retained
for the original reports' purposes. No next runtime until this budget reconciles.
Compression completed: all20 decompressed entries match original lengths/hashes.
Archive retained-20260905-215905_to_20260906-150239.zip is1,519,695 B, SHA
76100860DFBEB64D03986246F766A1903BE057AFB5D6B394CEA4194D753084AA.
Removed20 redundant originals8,954,976 logical B, fully recoverable from archive;
logical net reduction7,435,281 B. Free63,045,853,184 ->63,051,153,408 B,
5,300,224 B observed recovery (concurrent volume activity not isolated).
This restores over7 MiB of the diagnostics stop headroom. Resume the same pending
UV observation with the revised300 s bound, unchanged800 KiB/192 MiB/raw0 caps.
Run951/PID35436/session74854 terminal16:33:52 after16:31:07 start; free-space/
drop threshold stopped the cold-field run. No UV observation, no new acceptance.
All21settings applied; exact116 B profile restored. No fourth probe. Local
supervisor now preflights complete diagnostic text/image overlap against75 MiB,
plus192 MiB runtime peak above the original floor, before touching the profile.
PowerShell parser passes; guard not runtime-exercised.313 Python checks pass.
Host111 and unchanged C++/GPU behavior evidence retained; no new pixel/raw/cache.

Removed950's recorded/reconciled budget-stop log253,784 B and host110 stdout/
stderr1,034/0 B after111 passed;3files/254,818 logical B, free63,058,161,664 ->
63,058,419,712 B (258,048 B observed). Exact old logs gone; results/hashes in
20260907_1638_uv-boundary-provenance.md.949/951 retained664,637 B;945/940/941/
947/948 unchanged. Build logs190,079 B/136files. Known retained net-6,732,917 B:
compression-7,435,281,exe+7,168,PDB+28,672,logs+1,887,new949/951+664,637.
Objects/CMake/source/Git unallocated. Firstfree62,920,818,688 ->cleanup-end
63,058,419,712 B is137,601,024 B drive-wide gain, not cleanup attribution.
No owned producer remains. All original budgets/protected raw sets unchanged.

Continuation after b910ae9: the push-only turn was no renderer progress. Source
work now connects load-owned ordered alpha references and object/pass cutout
inputs to the native rigid scene shader and blended pipeline. Solid shadow
casting is unchanged; textured cutout casting and live acceptance remain pending.
First measured free63,759,519,744 B; before producers62,583,951,360 B. This
1,175,568,384 B drive-wide drop is not attributed to these small source edits;
existing fixture sizes remain material8,227,410 B, texture70,319,344 B and
GPU10,585,571 B. No agent producer has run during this drop. Original floor
62,509,998,080 B and3 GiB exception remain unchanged.314 artifact-free Python
checks pass. Reuse existing trees for sequential material35/CPU33, output35/
CPU18 and GPU29/rigid08, each <=32 MiB free-drop/peak,300 s supervisor and10 MiB
aggregate logs. Recheck peak overlap before every producer. New logs <=64 KiB;
no raw/images/perf/cache/cook outputs. Host112 needs <=192 MiB peak above the
floor and is paused until that reserve fits. Preserve all unresolved-failure
evidence and the last accepted945 image; no fixture can restamp game acceptance.

Material35/PID24032/session20560 and CPU33/PID34692 pass (0.12/0.15 s);
output35/PID21864 and CPU18/PID36272 pass (0.41/0.44 s). GPU29/PID36328/
session27134 compiles four production shaders and the existing fixture; rigid08/
PID36048 passes23 two-eye8x8 modes (2.87/2.89 s), including eleven new blended
cutout modes, zero Vulkan validation errors/warnings, one absent-GOG-overlay
loader message. Raw/image output0. All handles terminal. Host112 was not launched;
its192 MiB peak never fit. A later four-source syntax plan (10..13; alpha/blend
names added to the existing supervisor) failed32 MiB preflight before producer
or log creation. Free62,335,897,600 B was then below the unchanged floor; no
more build/game work. Profile and host111 exe hashes unchanged. Source guards314
pass; host compilation and live cutout/legacy-interoperation acceptance pending.

Removed12 superseded material34/CPU32/output34/CPU17/GPU28/rigid07 logs after
replacement CPU/GPU coverage passed,8,214 logical B. Free62,240,915,456 ->
62,240,931,840 B,16,384 B observed (external volume activity not isolated).
Exact old log text gone; prior reports preserve results and tests are reproducible.
All game/UV/light/baseline/pixel evidence remains. Counted retained net+197,469 B:
material+63,416,texture+119,466,GPU fixture+8,731,build logs+5,856. Logs195,935 B/
136files; no new game logs/cache/dumps since16:40. Root shader headers/CMake/
source/Git unallocated. Inventory ending free62,243,811,328 B is1,515,708,416 B
drive-wide loss from first measurement, not attributed to this small retained
growth. Continue low-storage source work; unchanged reserve gates all producers.

Continuation after8a1bf9a: the intervening push-only turn was no renderer
progress; worktree was clean and origin/main synchronized. Phase1 cutout shadow
source now connects assignment-time alpha/texture recipes, object alpha and
owned sampled images to native depth programs and the existing indirect/fence
queue.315 artifact-free Python checks pass; behavior and integration pending.
First measured free61,869,699,072 B was below the original62,509,998,080 B floor,
so no compiler/game producer ran. Later drive-wide recovery gives63,141,998,592 B;
this is not attributed to cleanup. Baseline fixture sizes unchanged: material
8,290,826 B/41files,texture70,438,810 B/129files,GPU10,594,302 B/10files;
build logs195,935 B/136files. No owned build/game process found; profile unchanged.
Reuse sequential material36/CPU34,output36/CPU19,GPU30/rigid09: <=32 MiB peak/drop
each,300 s supervisor/original floor,10 MiB aggregate logs,new text<=64 KiB.
No game/raw/image/perf/cook output. Only after passing fixtures and a fresh
reserve check may host112 use <=192 MiB peak in the existing tree. All unresolved
UV/light failures and945's last accepted pixels remain protected; no fixture
restamps that game evidence. These continuations share the original3 GiB cap.

Material36/PID24192 and CPU34/PID34112 pass0.13/0.15 s. Output36/PID32580 and
CPU19/PID31912 pass0.40/0.42 s. GPU30/PID33340 failed because new shaders were
missing from the existing explicit cross-directory header-order list; fixed
the list and added an all-entry guard. GPU31/PID35344 builds three new shaders /
factory; rigid09/PID23348 passes37 two-eye scene/mono-shadow8x8 modes1.86/1.87 s,
validation0 errors/0warnings,oneGOGloader message. All terminal, raw/image0.
Host112's192 MiB preflight then failed before compiler/log creation; host111
and the116 B normal profile remain unchanged.315 Python checks pass. No game
acceptance or UV/light fix claimed. Detailed contract/hashes in1742 report.

Removed14 superseded material35/CPU33/output35/CPU18/GPU29/rigid08/GPU30 logs,
17,207 logical B; free63,146,737,664 ->63,146,766,336 B,28,672 B observed recovery
with concurrent volume activity not isolated. No protected evidence/data deleted.
Retained growth406,432 B: material+38,306,texture+197,240,GPU+60,486,logs+3,104,
new shader headers107,296. Other objects/CMake/source/Git unallocated. Logs now
199,039 B/136files; ending inventory free61,580,406,784 B is289,292,288 B below
first measurement. Original allowance/reserve unchanged; no next producer until
its peak fits. A separate visible Android toolchain build is not ours to stop.

After a08e128 push, free73,721,511,936 B fit the unchanged192 MiB host peak.
Host112/PID32864/session17477 completed exit0,38 scheduled host steps,codegen0
written/module up to date,no guest object rebuild. Exe48,695,296 B SHA256
ED0B16D60DE438671B39ECE731ACB88B3BAF20203B2525A3ED4130D2451A6F76,
PDB109,453,312 B. Build endfree73,666,396,160 B. No game/capture/profile change;
live cutout reachability/interoperation/reload/pixels remain pending.
Removed superseded host111 stdout/stderr2files/2,921 logical B after replacement
passed. Free73,573,986,304 ->73,573,142,528 B (-843,776 B amid concurrent activity;
no isolated savings claim). Turn cleanup total16logs/20,128 logical B. Known
retained net524,436 B, adding exe26,112/PDB90,112/netlogs1,780 to prior406,432.
Logs200,819 B/136files. Other objects/CMake/source/Git unallocated. Lastfree
73,573,142,528 B =11,703,443,456 B drive-wide gain since first, not task cleanup.
Original floor/exception and all protected baseline/failure evidence unchanged.

## 2026-09-07 cutout integration continuation (same allowance)

First free78,924,582,912 B; original3 GiB exception/floor62,509,998,080 B unchanged.
Host113/PID25168/session6384 and114/PID30120/session97473 pass; run952/PID34196
stops on192 MiB free-drop guard before useful evidence. GPU32/PID32464/session48551
builds; rigid10/PID35752 stops on32 MiB guard, then11/PID31932 passes41 pixel modes
1.65/1.67 s, validation0/0 after concurrent builds are absent. Memory readback only.
Runs953/PID34072/session21773 and954/PID35800/session3840 identify node65's false
light-space cutoff refusal. Fixed from exact translated control flow; material37/
PID22788 and CPU35/PID22244 pass0.17/0.20 s.320 Python checks pass. Host115/
PID37360/session50752 links with no guest objects rebuilt. All producers terminal.

Run955/PID27696/session70140 clears the refusal and prior cold-field gates:
fresh1842..2142 textured scene cutout emissions+1,198/retired+1,196, zero-layer
shadow cutout emissions+8,778/retired+8,766. Textured shadow0 keeps the new gate
Pending. Old generation93 closes;192 MiB guard stops reloaded opening event.
No new raw/image/perf/cache/dump;116 B profile restored exactly. Keep945/948/941.
Detailed source correction, hashes and pending alpha/pixel gates in1838 report.

Retired16 superseded build/test logs25,787 logical B plus952/953 logs64,367 B:
18 files/90,154 B total, no protected data/evidence deletion. Immediate free deltas
were+7,376,896,+20,480,+16,384,+69,632 B across four deletions; concurrent volume
activity is not isolated or claimed as task savings. New retained954/955 logs
420,247 B preserve precise failure/current cold progress. Material tree8,331,276 B/
41files (+2,144), texture70,636,050 B/129 unchanged, GPU10,670,060 B/10 (+15,272),
logs199,175 B/136 (-1,644), exe/PDB+8,192/+28,672. Counted retained growth472,883 B;
other objects/CMake/source/Git unallocated. Inventory free71,608,135,680 B, down
7,316,447,232 B drive-wide, not attributed to the~462 KiB subtotal. No allowance
reset, no new raw exception. Pending reload/textured-shadow/pixels remain explicit.

## 2026-09-07 phase1 coverage correction (same allowance)

The intervening push-only turn was no renderer progress. First free78,655,266,816 B.
Source investigation now distinguishes direct shadownull from deferred shadowmap:
the latter uses fixed texture alpha0.6, no object/vertex alpha or cutoff state.
The ordinary deferred rigid family connects to the existing native depth queue;
zero-texture casters use the solid program.320 artifact-free Python checks pass.
Reuse material38/CPU36, output37/CPU20, GPU33/rigid12 sequentially, each32 MiB
peak/free-drop,300 s supervisor/original62,509,998,080 B floor,10 MiB aggregate
logs. New text <=64 KiB, no raw/image/perf/cook outputs. Current baseline trees:
material8,331,276 B,texture70,636,050 B,GPU10,670,060 B,logs199,175 B.
Only after fixture success and another preflight may host116 use192 MiB peak.
An unrelated Android build is visible; it is not ours to stop. All prior game
failure/pixel evidence remains protected. No host/game acceptance yet.

Commit checkpoint: material38/CPU36 pass (0.25 s test /0.29 s CTest), output37/
CPU20 pass (0.88/0.94 s). GPU33 stopped on32 MiB free-drop during regeneration;
GPU34 exposed Windows' min macro, corrected with `(std::min)`. GPU35 built.
Rigid12 passed modes0..43, then failed the overlap coverage assertion: the far
caster was level with the nearer receiver. Both pixel/depth oracles agreed;
validation0 errors/0warnings. Fixed the fixture's far translation to -.1875
(depth .3125, before receiver .375), preserving tolerances. GPU36 builds.
Rigid13/PID31548/session49661 stopped on the32 MiB free-drop guard; supervisor
reported its owned child tree terminated. No completed46-mode result, host116
build, run956 or new game pixels. Commit/push request does not restart producers.

Contract sources: generated/reblue_recomp.62.cpp sub_82198138 publishes modes0..3
as texture enable booleans; generated67 sub_821981E0 flushes them. Generated64
sub_82174270 chooses ordinary technique0 shadowmap for the list callback versus
technique24 shadownull VS for the direct callback; both table entries use
bd_shadowmap_ps (tools/shader_cache/shader_table_descriptors.csv). Retained
bd_shadowmap_ps.hlsl, hash0x3646F81DE849C63C, rejects sampled base alpha below
asfloat(0x3F19999A), with no scene float/colour inputs. Generated40's node phase1
branch forces the sorted route when visual+3068 is nonzero. The ordinary
deferred_consumer light-space path skips depth sorting/scene alpha enable and
retains depth writes. This scopes the native min-depth substitution; general
scene/translucent ordering is not removed. Earlier37/41-mode shadow acceptance
does not establish this corrected contract. Preserve the authored-texture gate.

No cleanup or new producer during this push-only handoff. Ending measured free
81,861,353,472 B is3,206,086,656 B above the continuation's first measurement;
that drive-wide gain is not attributed to cleanup. Existing attempt logs and
fixture outputs remain retained pending replacement verification; their final
growth inventory is still pending. Original allowance/floor and protected
failure/pixel evidence remain unchanged. Host115/profile are not restamped.

Continuation after808aa45: the preceding turn preserved and pushed the connected
source/test checkpoint, resolved rigid13's terminal storage-stop status and
corrected active acceptance claims; it did not advance host/game qualification.
First measured free81,861,492,736 B. No owned renderer/build/test process remains.
Actual trees: material8,333,551 B/41files,texture70,641,489 B/129,GPU10,658,612 B/
11, build logs236,678 B/156. Attributed retained net+33,769 B since the coverage
correction baseline (other objects/CMake/source/Git not included).
Reuse the compiled fixture for rigid14,32 MiB free-drop/300 s/10 MiB aggregate
logs; after passing, host116 may reuse the existing tree with192 MiB peak and
the original62,509,998,080 B floor. No guest rebuild, raw/perf/cook output.
New text remains bounded; a later changed-code run956 can reserve800 KiB log
and110 KiB mono inspection only after checking the existing75 MiB diagnostics
guard and full strict cutout/reload chain. No automatic game-image acceptance.

Rigid14/PID37760 passes46 GPU modes1.30/1.31 s, validation0/0. Host116/PID29224/
session48752 exits0 after host compile/link, codegen0 written/no guest objects.
320 Python checks pass. Run956/PID28364/session24363 exits0 after the full strict
cold/reload chain: each fresh300-frame epoch emits/retires300 textured shadows,
with scene cutouts+1,000/998 cold and+949/947 reloaded. Generation93/instance144
fully retires at title before207/435. New mono JPEG101,553 B was inspected:
tree-trunk gaps leave pixel acceptance open; no matched camera baseline yet.
Profile restored exactly; no producers or new raw/perf/cache/dump/cook output.
Keep945/948/941. Details/hashes/next observation in1927 report.

Removed24 superseded CPU/build/GPU logs54,186 B, old955 cold-only log364,742 B,
and unreferenced generated alpha-only shader header31,075 B after replacements.
Total26files/450,003 logical B, no protected data/evidence removed. Exact old
logs gone; reports/tests preserve results/reproducibility. Immediate free gains
8,192/57,344/368,640/32,768 B are not isolated from other volume activity.
Counted retained net+155,039 B, retaining956 log488,126 B and JPEG101,553 B for
the visual question: GPU tree+414, logs-35,653, exe/PDB-3,584, old955-364,742,
old shader header-31,075. Material/texture trees unchanged this continuation;
GPU10,659,026 B/10files, logs201,025 B/136. Other objects/CMake/source/Git not
allocated. Ending free81,856,401,408 B, drive-wide loss5,091,328 B since first;
not wholly task-attributed. Original3 GiB exception/floor unchanged.

## 2026-09-07 owned occlusion connection (same allowance)

The previous turn made progress:98a49f3 preserved the new source and restored the
legacy shader ABI before its requested push. It did not qualify runtime changes.
This continuation connects owned node observations -> native depth queries ->
fence history -> native rigid consumers, removing the old translated query block
and address-keyed draw filter. First measured free81,584,267,264 B. Reuse the
existing texture and GPU fixture trees; output38/CPU21 and GPU37/occlusion1 are
the next bounded attempts,32 MiB peak/free-drop each,300 s supervisor and10 MiB
aggregate build logs. New text <=64 KiB; no raw/perf/cache/dump/cook outputs.
After fixtures, host117 may use192 MiB peak with the unchanged62,509,998,080 B
floor. No guest rebuild or game launch in this preflight. Retain945 accepted
pixels and956 tree-gap image/log plus948/941 failures. Prior cleanup is not
credited again; replace only superseded attempt logs after verification.

Output38/CPU21 pass (0.40/0.43 s). GPU37/38 compile; the first query fixture
exposes a real Plume deferred-pass query begin/end mismatch and an invalid
combined depth/stencil fixture readback. Plume now starts the pass before an
occlusion query and fails visible on unavailable reads; the fixture copies only
depth. GPU39 and occlusion2 pass all8 cases (1.09/1.10 s), validation0/0, no raw.
325 Python checks pass. Host117/PID28944/session67898 terminates0:36 host build
steps, codegen0 written, no guest objects. Plume2d206ee is pushed before parent.
Next changed-code observation: run957, existing full strict cold/reload flags,
Count0/no images,300 s,800 KiB log,192 MiB free-drop,75 MiB diagnostic overlap
and unchanged original floor. It must show fresh native query emission/collection
and whether native draw skips advance; prior renderer gates alone do not prove
occlusion. No speedup or pixel acceptance from this text run. Preserve956/945.

Run957/PID3732/session47473 is terminal failure: access violation in descriptor
binding during loading. Its87,567 B log and host117 symbolization are preserved
in the2007 report. Profile restored exactly; no new raw/image output. The query
layout had escaped into the following descriptor consumer. The production
binding scope now restores an explicit resume snapshot; GPU40/occlusion3 adds
that real three-offset consumer and passes8 cases1.31/1.32 s, validation0/0.
Host118/PID22648 links0 without guest objects. Run958 is the next causal attempt,
with identical300 s/800 KiB/192 MiB/75 MiB/original-floor limits and no images.
Removed12 superseded output37/CPU20/GPU36..38/occlusion2 log files,6,198 logical B;
the failure log and current tests remain. No prior cleanup is credited again.

Run958/PID36336/session19501 terminates0 after the complete strict cold/reload
chain; generation93 fully retires before206. Latest native query sample4674 has
450,896 emitted,450,679 collected,101,924 zero results and0 native draw skips.
Queries are live; useful culling, performance and game pixels remain unqualified.
Both effective settings and exact profile restoration were checked. See2007
report for hashes, epoch counters and next eligibility/refusal decision.

A separately requested JPEG was not produced: the already-owned renderer had
terminated. The existing image inventory is10,437,840 B (only47,920 B free below
10 MiB), so the proposed110 KiB reservation cannot fit without superseded-image
cleanup. No image/raw archive growth or producer restart. Preserve945/956 images.

After verifying the native replacement and confirming no source/CMake/Ninja
consumer, removed the obsolete generated occ_proxy_vs SPIR-V header20,425 B.
Total cleanup this continuation:13 files/26,623 logical B. These superseded logs
and old generated header are reproducible from recorded commands/source history;
957 failure87,567 B and958 current query/reload501,130 B remain deliberately.
Immediate drive gains were12,288 B (logs) and20,480 B (header), not isolated from
other volume activity. Nothing from earlier cleanup is credited again.

Current texture fixture tree72,119,840 B/129 files (+1,478,351 B); GPU fixture
11,651,129 B/12 (+992,103); build logs217,514 B/146 (+16,489); exe/PDB+6,144/
188,416 B. Together with the588,697 B new run logs and removed20,425 B header,
the counted retained net is+3,249,775 B: new production/CPU/GPU coverage and
distinct current/failure evidence, not duplicate captures. Other host/Plume
objects, CMake, source and Git are not allocated in that subtotal. Last measured
free81,313,525,760 B is270,741,504 B below this continuation's first reading,
a drive-wide change not wholly attributable to the~3.10 MiB subtotal. No owned
producer remains, no fresh raw/perf/cache/dump/cook output was found, profile
restored exactly. Original3 GiB allowance/floor and all protected evidence remain.

### Native consumer admission continuation (2026-09-07)

Starting free81,299,386,368 B; same cumulative3 GiB allowance and62,509,998,080 B
floor. Reuse existing fixture/host trees,32 MiB fixture and192 MiB host/run free-drop
limits,10 MiB aggregate build logs, no raw/images/cooking. Output39/PID29224 and
CPU22/PID37148 terminate0 (0.37/0.39 s); GPU41/PID32580 and occlusion4/PID29660
terminate0. Eight query cases now exercise two real submissions/fences feeding
the production native-consumer decision: hidden-only culling, validation0/0,
unchanged colour/depth pixels,1.10/1.11 s.326 Python checks pass.

Host119/PID38816/session51328 terminates0,29 host steps/codegen0 written, no guest
objects. Current free81,278,443,520 B before run959. Run959 will use the existing
strict cold/reload chain, Count0, no image/raw,300 s,800 KiB log,192 MiB free-drop,
75 MiB diagnostic overlap and original floor. New observation: eligible-native
query count and exact cumulative refusal reasons; no inference of speedup or
game pixel acceptance. Preserve945/956 images and957/948/941 failures. Replacement
fixture logs will retire their superseded predecessors after live accounting.

Run959/PID2480/session20034 is terminal0 at20:45:00 after the complete strict
cold/reload chain. Profile restored byte-for-byte; no new raw/image/perf/cache/
dump/cook outputs. The2007 report records native-only query/refusal evidence,
first fresh multi-instance scene batches and all remaining pixel/culling gates.

Removed eight superseded agent fixture logs: output38/CPU21/GPU40/occlusion3
stdout/stderr,4,361 logical B. Replaced by output39/CPU22/GPU41/occlusion4;
reproducible from recorded commands. Immediate free-space gain8,192 B is not
isolated from concurrent volume activity. Preserve runtime958 as the prior
connection baseline,959 current admission/batching proof and all failure logs.
No prior cleanup is credited again; no protected image/raw data was removed.

Current texture fixture72,151,719 B/129 files (+31,879); GPU fixture12,207,529 B/12
(+556,400); aggregate build/test logs224,546 B/148 (+7,032 after cleanup);
exe/PDB+1,536/+12,288 and new runtime959511,958 B. Counted retained net +1,121,093 B
(~1.07 MiB), for expanded production/CPU/GPU coverage and current live evidence;
other host objects/CMake/source/Git are not allocated in that subtotal. Last free
81,282,560,000 B is16,826,368 B below this continuation's start, a drive-wide
change not wholly attributable to the subtotal. All limits and the original
floor remain intact; no owned producer remains.

### Indexed native bounds continuation (2026-09-07)

Starting free81,254,711,296 B; unchanged cumulative3 GiB allowance and
62,509,998,080 B floor. No native file migration/recook: derive indexed bounds
from the persisted v2 positions before GPU upload; retain only bounded per-mesh
metadata. Mesh tree preflight1,490,087 B/30 files. Fixture free-drop32 MiB, host
build192 MiB,300 s supervision and10 MiB aggregate build logs. No raw allowance.

Mesh12/PID34340 is a compile failure from Windows max macro in the new test;
corrected locally before retry, not a runtime failure. Mesh13/PID36308 and
mesh CPU11/PID27688 pass (0.12/0.14 s). Output40/PID29528 and CPU23/PID23180
pass (0.36/0.38 s). GPU42/PID22756 recompiles the96-byte box-query shader;
occlusion5/PID31916 passes all8 indexed-bounds/transform/query cases1.09/1.11 s,
validation0/0, no raw.327 Python checks pass. Host120 will link the connected
primitive consumer with the same bounded wrapper; no guest object rebuild.

Host120/PID33752/session92497 terminates0,38 host steps/codegen0 written, no
guest objects. Before the next image, inspected the old940 mono reload JPEG and
verified its hash against1140 evidence; its generic field/reload sanity purpose
is superseded by accepted945 (also inspected/hash checked), which includes the
selected reload regression. Retired only940's124,696 B JPEG; original pixels
are no longer on disk, findings/hash remain in1140. This updates the historical
"keep940" designation;945 remains the accepted baseline and956 the tree-gap
evidence. Immediate drive gain126,976 B is not isolated from concurrent activity.
Image archive is now10,313,144 B;110 KiB replacement fits below10 MiB. No raw
capture or prior cleanup credited. Normal profiles/game data/failure evidence
unchanged. Existing supervisor now routes an explicit OcclusionInspect JPEG
inside the qualified run, before shutdown, with the same cap/strict gates.

Run960 planned: full strict cold/reload chain, Count0, one110 KiB JPEG,300 s,
800 KiB runtime log,192 MiB free-drop,75 MiB diagnostic overlap/original floor.
Question: do exact per-primitive native boxes qualify for queries and suppress
hidden native draws, with fresh scene/reload and inspected mono pixels? Camera
history checks stay exact; failure to cull is not permission to loosen them.

Run960/PID33516/session76067 terminates1 at21:12:50 on the300-second timeout.
It reaches1,026 native primitive skips and completes cold source/GPU retirement,
but never qualifies the selected reloaded epoch. No image was produced; the
full strict gate remains open. The2107 report preserves the missing-readiness
evidence, exact counters, log hash and next observation. Profile restored exactly;
all owned producers are terminal. Preserve960's780,656 B failure log,959's prior
strict pass,945 accepted pixels,956 tree-gap evidence and957/948/941 failures.

After replacement fixtures passed, removed14 superseded stdout/stderr logs:
native_mesh_test11/12, mesh_cpu10, host_post_output_test39, post_output_cpu22,
native_scene_snapshot_test41 and occlusion_pixels4. These9,474 logical B include
the resolved Windows max compile failure, not an unresolved runtime failure.
The replacement is mesh13/CPU11, output40/CPU23 and GPU42/occlusion5, reproducible
from source and recorded commands. Immediate drive gain8,192 B is not isolated
from other volume activity. Together with the already recorded940 JPEG, cleanup
this continuation totals15 files/134,170 logical B; do not add the image twice
or credit any prior continuation's cleanup. The deleted940 pixels are no longer
on disk; their hash/findings remain in1140 and the reviewed945 successor remains.

Retained mesh fixture1,537,443 B/30 files (+47,356); texture fixture72,184,565 B/129
(+32,846); GPU fixture12,878,617 B/13 (+671,088); aggregate build/test logs231,549 B/
150 (+7,003 after cleanup). Exe/PDB+4,608/+20,480; runtime960+780,656 and retired
940 image-124,696. The later scene transition also produced three reusable native
material-cache records,68 B each, in `cache/native_materials/v1`:
`2d3212820e194cf6.bdmat`, `9736f2804261ce3b.bdmat`, `999e24d31205983a.bdmat`.
Retain those204 B under the existing versioned cache budget; no broad recook,
duplicate asset set, new raw/perf/dump or image output was found.

Counted retained net+1,439,545 B (~1.37 MiB), for expanded fixtures and distinct
current failure evidence. Other host objects/CMake/generated shader header/source/
Git are not allocated in that subtotal. Image archive10,313,144 B/9 files leaves
172,616 B below10 MiB. Last measured free81,040,945,152 B is213,766,144 B below
this continuation's first reading; drive-wide activity is not wholly attributed
to the counted files. There is no next producer approved by this accounting:
review the unexplained drive change before another output-producing attempt.
The cumulative3 GiB allowance, original62,509,998,080 B floor and protected
evidence obligations remain unchanged.

### Readiness and separate culling-image continuation (2026-09-07)

Previous goal turn made progress through indexed native bounds, actual primitive
culling and the preserved960 timeout. First free80,994,217,984 B. Scoped build-root
inventory since960 start finds only the already-counted780,656 B log, restored
116 B profile and204 B material records; no renderer/compiler process is live.
Later free80,991,305,728 B: the unallocated drive-wide change is not a new owned
artifact producer. No broad drive scan or protected cleanup is justified.

Reuse the same fixture/build trees and cumulative3 GiB allowance/original floor.
Next output41/CPU24 reserve32 MiB peak/free-drop and10 MiB aggregate text; then
host121 reserves192 MiB with300 s supervision, no guest rebuild. Changes only
expose exact readiness refusal/window resets and add a fresh-culling observation
trigger that is separate from full acceptance.330 Python checks pass before
build. Unchanged query shaders/GPU fixtures need no rebuild for these edits.
After the host link, at most one changed-observation run961 is planned, same
strict settings,300 s/800 KiB/192 MiB/75 MiB overlap and one110 KiB JPEG within
the existing172,616 B headroom. No raw/perf/dumps or asset conversion requested;
bounded existing material caches remain counted. Retire only superseded fixture
logs after replacement passes; preserve all prior failure and accepted pixels.

Output41/PID26688 and CPU24/PID37792 terminate0 (0.36/0.38 s); host121/PID38032/
session58654 terminates0,18 scheduled host steps, codegen0 written, no guest
objects. Source checkpoint9c4a6b7 is pushed before run961. The2107 report records
actual binary hashes/stamp and the separate-observation contract.

Run961/PID33484/session3602 terminates21:35:45 with supervisor exit1 for missing
requested image. Its full strict cold/reload text chain passes, with no readiness
reset observed; no cause/fix for960 is claimed. No new interactive-field skip
delta reaches the image trigger. The supervisor's early text-pass stop is now
corrected locally, syntax checked only; no further boot. Profile restored exactly,
no cache/perf/dump/raw/image growth found. Retain961's506,703 B current strict
native-bounds reload proof and960 failure;959 still owns distinct batching proof.

Removed six superseded output40/CPU23/host120 stdout/stderr logs,7,639 logical B,
after inspecting their replacement and old terminal success evidence. Reproducible
from source/commands; build hashes/findings remain2107. Immediate free gain12,288 B
is not isolated from other volume activity. No prior cleanup double-credited and
no protected runtime failure, game data, profile, raw or image file removed.

Texture fixture72,201,319 B/129 files (+16,754), aggregate build/test logs227,957 B/
150 (-3,592), exe/PDB+3,072/+16,384 and new runtime961+506,703. Counted retained
net+539,321 B for expanded CPU coverage and current strict reload evidence;
other host objects/CMake/source/Git are not allocated. Image archive unchanged
10,313,144 B/9 files,172,616 B headroom. Ending free80,978,022,400 B is16,195,584 B
below the continuation's first drive reading, not wholly attributed to counted
files. All producers terminal; original3 GiB allowance/floor unchanged.

### Current-depth visibility fixture continuation (2026-09-07 22:09)

Previous turn made progress: source prototype2637bba is pushed with uncompiled
GPU/runtime gates explicit;330 Python checks pass. First measured free this
continuation80,956,170,240 B. No renderer/compiler/fixture process found live.
Existing GPU fixture tree12,878,617 B/13 files and aggregate attachment logs
227,957 B/150 files are reused; no new build tree. The earlier unbuilt prototype
work first measured80,975,896,576 B, not a new allowance or attributed producer.

Next GPU43 and visibility1 use the existing bounded wrapper,300 s producer /
30 s CTest /5 s fence limits,32 MiB maximum free-drop/temporary overlap and10 MiB
aggregate attachment logs. Expected added fixture/shader objects under8 MiB,
text under64 KiB; no game boot, raw/image export, perf trace, recook or download.
The original3 GiB allowance and62,509,998,080 B floor remain. Preserve GPU42 and
occlusion5 until replacement tests pass, then retire only superseded build logs.
945 accepted pixels,956 tree gap,960 timeout,961 reload and959 batching evidence
remain protected. Current raw/image archives are unchanged by this fixture:
252,177,116,500 B raw and10,313,144 B images; no incoming raw allowance.

GPU43/PID35916 terminates0; four compute shaders and mask PS compile, native
fixture links with no guest build. Visibility1/PID32484 terminates0:40 cases
in1.43/1.45 s, RTX3060, validation0/0, one unrelated missing GOG overlay manifest
loader message. Free80,955,744,256 B. No raw/image files produced.

Queue audit identifies a causal integration prerequisite: existing emission
counters increment at CPU recording, so GPU-zeroed commands require separate
post-fence classification; generated commands alone cannot count as output.
GPU44/visibility2 reserve the same32 MiB guard (not a reset), under64 KiB new text
and unchanged cumulative limits. They add ordered same-image depth/camera
refreshes, shared byte/owner budgeting, actual-draw receipts and post-fence
readback before runtime queue/lifecycle integration. No game boot planned.

GPU44/PID37592 fails on a missing standard span include, corrected before
GPU45/PID35828 terminates0. Visibility2/PID34056 passes44 cases in1.20/1.21 s,
validation0/0. It verifies four same-image camera/depth changes per sample
count, no intermediate fence wait, exact-once real-draw receipts, shared buffer
budget refusal and source pinning through the actual fence. Last free
80,936,206,336 B. The intervening drive-wide drop is not attributed wholesale:
scoped fixture size14,022,121 B/15 files after43 (+1,143,504 B), plus10,828 B
attachment logs; root writes are Ninja metadata. No owned producer remains.

GPU46/visibility3 add perspective projection and deliberate malformed command
receipt rejection;334 Python guards/scenario checks pass. Same32 MiB producer
guard, under64 KiB text across attempts, original cumulative budget/floor and
protected archives. Run the existing rigid/query regressions on the same binary
afterward because the indexed-command ABI was extracted. No host/game rebuild.

GPU46/PID37480 terminates0; visibility3/PID5612 passes72 cases in1.27/1.28 s.
Rigid15/PID30452 passes46 cases in1.28/1.29 s; occlusion6/PID28096 passes8 cases
in1.04/1.05 s. All validation0/0, one known missing overlay loader message,
no exported raw/image bytes. GPU47/PID32088 then compile-checks noncopyable /
nonmovable budget reservations with no shader/behavior change; terminal0.
GPU46 remains the pixel-tested fixture;47 is compile-only.334 Python checks
pass. Actual executable/log hashes and the unconnected runtime accounting work
are recorded in2209 native-depth-visibility evidence. Host121 is not restamped.

Retired18 superseded stdout/stderr files,48,815 logical B: GPU42/43/44/45/46,
visibility1/2, rigid14 and occlusion5. Each replacement is inspected and passes;
44's resolved missing-span failure is preserved here, not an unresolved GPU/game
failure. Logs are reproducible from source/commands; no source, active build tree,
profile, game data,945 accepted/956 failure pixels or protected runtime evidence
was removed. The final257 B deletion produced no measured immediate free-space
gain; no isolated physical-reclaim credit is claimed for the text cleanup.

Final GPU fixture14,550,168 B/15 files (+1,671,551), five generated new visibility
shader headers149,285 B and aggregate attachment logs241,730 B/152 files
(+13,773 after cleanup). Counted retained net+1,834,609 B (~1.75 MiB), for
expanded executable/PDB/object coverage and the final distinct GPU proof. All
attempts produced62,588 B of attachment text before cleanup, below64 KiB and
the unchanged10 MiB aggregate cap. Other source/Git/CMake/Ninja bytes are not
allocated in that subtotal; retained diagnostics remain under the original cap.

Ending pre-commit free80,920,801,280 B is35,368,960 B below the continuation's
first reading; this is drive-wide change, not solely the counted retained bytes.
Scoped root writes are Ninja metadata and the fixture; NVIDIA DXCache has no
files modified since22:09. No renderer/compiler/fixture process remains live.
Raw/image archives are unchanged, no asset/profile/perf/dump output was produced,
and no prior cleanup is double-credited. Original3 GiB allowance and floor remain.

### Current-depth runtime connection continuation (2026-09-07 22:56)

Previous turn made progress: GPU contract9e7ee1e is pushed and runtime source
connection4c890ad is committed locally. Its push is security-blocked pending
payload-specific owner approval; do not retry without that authority. Source
work continues locally. Corrected the stale emission guard to check regression
identity and exclusion of culled commands;339 Python guards/scenarios now pass.
The old check failure remains recorded in4c890ad, not a runtime qualification.

First measured free80,008,998,912 B. Existing texture fixture72,201,319 B/129 files,
GPU fixture14,550,168 B/15 and attachment logs241,730 B/152 are unchanged since
the prior checkpoint. No compiler/game/fixture producer is live. The intervening
drive-wide decline is not assigned to these unchanged artifacts or credited as
task activity; scoped recent build/log/driver-cache writes are inspected before
launch. Original3 GiB allowance and62,509,998,080 B floor remain unchanged.

Reuse output fixture42/CPU25 next,32 MiB free-drop/temporary guard,300 s wrapper
and30 s CTest timeout. After it passes, incremental host122 uses192 MiB guard,
300 s timeout and the existing unexpected-guest-object stop. Estimated fixture
growth under2 MiB and host replacement/object/link overlap under192 MiB; text
under32 KiB for this build group, still within10 MiB aggregate attachment logs.
Keep prior passing output41/CPU24/host121 until replacements pass, then retire
only superseded logs. No new tree, guest build, raw/image, asset or profile output
for these jobs. Existing protected raw252,177,116,500 B and images10,313,144 B
remain, with172,616 B image headroom and no incoming raw allowance.

Output42/PID38292 and CPU25/PID34036 terminate0; the C++ output test passes in
0.37/0.40 s. Host122/PID21964 stops during configure: the backend-only source
list still named deleted occlusion_cull.cpp. No compile/game producer remains.
Free80,007,593,984 B. Remove that dead reference, classify the new Vulkan-aware
visibility implementation as backend-specific, and guard both in the source
test. Host123 is the bounded retry under the same192 MiB/300 s/log limits.
Scoped root writes before these jobs were Ninja/CMake metadata; host121 binary
and logs were unchanged, with zero recent NVIDIA DXCache writes. No isolated
cause for the earlier drive-wide decline is asserted.

Host123/PID23020/session42964 terminates0 after102 scheduled steps/99 emitted
steps (host only, codegen0 generated files changed). Exe48,731,648 B and
PDB109,654,016 B, stamped4c890ad dirty. Free80,004,616,192 B. GPU shaders are
unchanged from72-case visibility3 proof; this host connection still needs live
evidence. Runtime preflight reserves78,191,956 B before800 KiB text+110 KiB JPEG,
which exceeds75 MiB by480,596 B; no game launch/profile mutation occurred.

Losslessly compress retained940/949/951 logs before live work: exact inputs
483,446+410,254+254,383=1,148,083 B, archive overlap bounded under1.2 MiB. Keep
940's pre941 baseline and949/951 timing/storage failures byte-for-byte; verify
all archive entry sizes/SHA256 against originals before retiring the plain
copies. Archive stays under logs/retained-*.zip, explicitly counted in both
preflight and live cumulative checks. This is not deletion of failure evidence
or a budget reset. Current945/956/957/958/959/960/961 and941/947/948 stay plain.

Archive verification passed all three entry lengths/SHA256. ZIP196,094 B replaces
1,148,083 B plain text, net951,989 logical B reclaimed; measured free gain962,560 B
is not solely attributed. Entries remain fully recoverable; no historical raw or
image was removed.940 SHA2785A5001BC6DD0EF935269CB322A8E0A266439CB5A7163B4DA850DF574B536B;
949 D8372C7399ACF76F1C10C9C96C163A29F96FC5CE6FA84F63D41A98E8DDF40FF5;
951 3A953CFE06CBDA7961BE0D42C37A81DB175EF196839ECC34302DECE9556DFA8B.

Next run962 uses the inspected run_post_image_flow supervisor, not the historical
run_scene_handoff helper. Its current-depth observation trigger is updated and
PowerShell syntax checked. Same complete cold/reload/receiver/lighting/caster/
cutout chain,300 s,800 KiB text, no raw/perf/dump/cook, one at-most110 KiB mono
JPEG only after fresh current-depth generated/drawn/collected/visible/culled
deltas. Image observation cannot satisfy or bypass strict reload acceptance.
Profile SHA2F1BC38D763A1B7BDBA31F560684FD4AA7E42A714600B8D344F19DA7F38E23B0
is checked before temporary overrides and must restore exactly in finally. Peak
runtime guard192 MiB and75 MiB cumulative diagnostic stop remain, including the
new archive. No renderer/build producer live at preflight. Preserve961 until the
new strict chain passes and preserve956 tree-gap pixels regardless of counters.

Run962/PID27128/session52256 terminates23:07:28, supervisor exit0. All21 settings
took effect; exact116 B profile/hash restored. Both strict cold/reload epochs
pass, including actual900 visible-emission windows and cold1658/1658/1658 source/
GPU closure. Fresh reloaded4634..4934 adds3248 generated/draw-recorded commands,
3247 fence-collected receipts,3236 visible and11 culled instances. No safety gate
was reduced. This is native GPU consumption, not frame timing or full-host proof.

Manual inspection **rejects** the97,976 B JPEG: a title logo conflicts with the
logged active field/player movement. Preserve it as unresolved window/presentation
discrepancy, not accepted game pixels.945 remains accepted mono baseline,956 tree
gap and941/947/948/960 failures remain.962's521,322 B text is current strict
runtime proof;961 is also retained, not silently replaced by failed visual
evidence. No further game boot. Next use the existing renderer-owned screenshot
path with frame/fence provenance to distinguish capture from presentation.

Final measured texture fixture72,286,199 B/129 (+84,880), unchanged GPU fixture
14,550,168 B/15, attachment logs269,518 B/152 (+27,788 net), image archive
10,411,120 B/10 (+97,976),74,640 B headroom. Attachment text produced33,267 B across
output42/CPU25/host122/123,499 B above the32 KiB estimate but far below the enforced
10 MiB aggregate limit. Eight superseded build/test logs removed5,479 logical B;
the12,288 B instantaneous volume gain is not isolated physical-reclaim credit.
Combined actual logical cleanup:951,989 B lossless log compression +5,479 B
superseded text =957,468 B. All archive contents remain checksum-recoverable.

Counted retained net change is-269,175 B: fixture+84,880, net attachment text
+27,788, run962+521,322, JPEG+97,976, archive replacement-951,989 and host exe/PDB
net-49,152. Other object/PCH/source/Git/CMake bytes are unallocated; no raw/perf/
dump/cache/asset output found for the game run. Codegen reports0 written and one
module up to date; no guest objects rebuilt. End free79,974,555,648 B is34,443,264 B
below the first reading, a drive-wide change not wholly attributed to the counted
files. All owned producers terminal;339 Python checks and diff check pass.
Original cumulative exception/floor and all protected raw/data remain unchanged.

### Frame-identified screenshot ownership continuation (2026-09-07 23:32)

Previous turn made progress: f7ad261 records host123/run962's strict reload and
current-depth output proof, while retaining the title-logo image discrepancy.
4c890ad/f7ad261 remain local after security rejection; no new push authority.
First measured free79,956,594,688 B, texture fixture72,286,199 B/129, GPU fixture
14,550,168 B/15, attachment logs269,518 B; no owned producer live. Source edits
only so far, no profile/image/raw/cache/asset change. The intervening volume
delta is not assigned to the unchanged scoped artifacts.

Reuse the existing screenshot consumer and actual frame-slot fence; remove
present-counter retirement, preserve frame/request/source identity and add a
default-off one-shot bounded JPEG probe. Existing CPU/GPU fixtures exercise the
production readback and installed Windows encoder, with no disk images. Plan
output43/CPU26 and GPU48/screenshot1,32 MiB per-producer overlap/free-drop guard,
300 s wrapper/30 s CTest/5 s GPU fence; estimated added fixture objects/PDB under
4 MiB combined and text under64 KiB. Then host124,192 MiB/300 s guard and stop on
unexpected guest objects. The same original3 GiB exception,62,509,998,080 B floor,
100 MiB diagnostics and10 MiB attachment logs apply. Retire superseded logs only
after replacement proof; preserve962 text/image,945 baseline,956 gap and open
failures. No game boot until fixture/host and new image-overlap preflight pass;
74,640 B image headroom cannot cover another110 KiB image yet. No new raw allowance.

Checkpoint verification recovered before the owner's commit/push request:
output43 and CPU26 passed the request/size and real JPEG round-trip fixture;
344 Python checks passed. GPU48 built, and screenshot1 passed all eight exact
pixel/frame-identity cases on RTX3060 with zero validation errors/warnings and
zero disk images. Host124 failed compiling `src/gpu/screenshot.cpp:24`: the
new cvar references undeclared `kCvarGroup`. No replacement host executable or
new game run is qualified. This remains a WIP source/fixture checkpoint;
the live supervisor/request receipt integration and renderer-owned image gate
are pending, and run962's image discrepancy remains unresolved. No additional
build, game run or capture was launched for the commit/push request.

### Frame-probe integration follow-up

Previous turn made progress: 08d7525 and both preceding commits pushed to
origin/main. On resumption free79,953,272,832 B; texture fixture73,466,070 B/131,
GPU fixture14,961,203 B/17, attachment logs291,851 B/162. No matching renderer or
build producer is live. Host125 reuses the configured target after the missing
settings-header fix:192 MiB peak/free-drop guard,300 s,10 MiB aggregate build
logs and stop on guest objects. Existing screenshot1 GPU pixels remain valid;
no shader/readback behavior changed. Add bounded request/receipt integration to
the existing scenario/supervisor, not another game harness. New image remains
blocked on its own overlap preflight until protected evidence can be retained
within the same10 MiB ceiling. No raw capture or new asset allowance.

Host125/PID26916/session72215 terminal0:27 emitted build steps, codegen0 written
and1 module up to date, no guest objects. Actual stamp08d7525 dirty. EXE48,755,712 B
SHA256 B7BBBCA8917FB8A9307BBEC7E0905CB4F2A82999EDF03AD93F184D0965E41269;
PDB109,924,352 B SHA256 8BCB437ED1426E829D5C3193F8E86FF3222F25E28D3E155F9ECF9E372FAABBB1.
350 Python checks pass for source/scenario contracts, including request/frame/
source/dimensions/bytes/fence-order validation; the local supervisor now uses
that existing parser and a one-shot exclusive request, not PrintWindow, in its
FrameProbe mode. The full strict reload gate remains independent of the image.

Completed storage work before the new producer:
- `native_material_pass_window.png` lossless IDAT recompression:1,794,992 ->
  1,697,263 B,97,729 logical B reclaimed. All non-IDAT chunks, filtered scanline
  bytes and decoded1920x1080 RGBA pixels are identical. Old file SHA256
  a5f158a89958e1bc9674e557b8f81b2f6df47abf8e01d0086414eaa770b7cb90;
  new6ab19caaf33ab84953ca859f3e02c2a6bb2cfe6e53f8f0be57e4f48f2f8e582d;
  decoded pixel SHA2565e6185e301d117fda5c88fcb8ab54319362adcac67da236fad5e92fcac07c1f5.
  Exact filtered data/CRC/Pillow decode checked before atomic replacement;
  <=4 MiB temporary overlap, no resized/recoloured or AI-altered evidence.
- Retained960/961 text is now lossless
  `logs/retained-native-runtime-960-961.zip`,198,143 B instead of1,287,359 B.
  Both entry length/SHA256 and unchanged source were checked before removing
  plain copies.960 SHA256517D72619B74980777D0E2C1C737834DD0728B7CE5EF44228CEB4575F36C5DCF;
  961 SHA2567AA01668CDEA82D187C1204B3109D2C5FF7D057FCA74A6DF15A1F7BF4408675C.
  All failure/reload evidence remains checksum-recoverable;1,089,216 logical B
  reclaimed, <=2 MiB temporary overlap. Archives remain counted by the supervisor.
- Removed8 superseded output42/CPU25/GPU47/host123 stdout/stderr files,32,240 B,
  after output43/CPU26/GPU48/host125 replacement proof. No failure log removed.
Total logical cleanup1,219,185 B, credited once. Free79,952,560,128 B after cleanup;
drive-wide changes are not an isolated physical savings claim. Image archive now
10,313,391 B;172,369 B headroom covers one110 KiB JPEG. Diagnostic preflight before
cleanup was78,204,042 B and refused the run; archive/log cleanup now frees1,121,456 B
there. Planned run963 retains at most800 KiB log +110 KiB JPEG +64 B request,
300 s/192 MiB/free-floor limits, no raw/perf/dump/asset outputs, original profile
restored in guaranteed cleanup. The new observation is the actual post-gamma
frame/source/fence, needed to distinguish962's stale-window/presentation cases.

Run963/PID36428/session57817,23:53:46..23:55:56, terminal1 after the complete strict
cold/reload chain passed (old generation93/instance144 ->207/384). The bounded
request `1 4983 5103` produced frame5070/slot0 input0170E4397050 output017046440570,
descriptor99,1920x1080 at the actual fence. JPEG decode correctly refused the
65,536 B truncated output; no pixel acceptance or962 diagnosis. Retain the
535,227 B log (SHA256DF96F670913A3F5FEA1AD64332AD9BB4D9E136371A0930D902FF3BDD9B116780),
JPEG (SHA256F0402D31CD1A085747F697E27AA789E47D9092AEA38DEC30C9BBCECA88C12772)
and11 B request. Original116 B profile restored to SHA256
2F1BC38D763A1B7BDBA31F560684FD4AA7E42A714600B8D344F19DA7F38E23B0.

CPU-only causal follow-up, not another game run: output44/PID33360 builds the
1920x1080 patterned encoding case. CPU27/PID37920/session72353 fails its
>64 KiB output assertion under the old encoder (CTest30 s timeout terminates
the assert dialog; no fixture remains live). Existing tiny32x16 encoder test
had missed this boundary. Replace seek-cursor length with bounded write-extent
tracking, sticky overflow refusal and JPEG SOI/EOI checks; exercise full decoded
pixels as well as tail/length. Plan output45/CPU28 with the same32 MiB/300 s/30 s
bounds, no image exports; then host126 only if the fixture passes. No second
game producer until both build and an independently fitting image budget exist.

Correction to the initial regression interpretation: output45 builds the new
bounded stream, but CPU28 exits8 because the16-pixel-block pattern produces only
57,462 valid encoded bytes. Its >64 KiB expectation was incorrect; CPU27's
assertion alone therefore did not reproduce963's cause. Strengthen the pattern
to8-pixel blocks (do not lower the byte-boundary requirement), then repeat the
bounded CPU fixture as output46/CPU29. The live truncated JPEG is still the
authoritative failure. No game retry or pixel claim follows the failed fixture.

Output46/CPU29 reaches full-image decode but fails the unchanged12-channel-level
block-centre error limit; CTest terminates the assertion at30 s. Keep that
failure. Microsoft's WIC JPEG codec defaults to4:2:0 chroma subsampling; test an
explicit4:4:4 setting without changing the case, quality sequence, byte cap or
colour threshold. Output47/CPU30 retain the same32 MiB/30 s fixture bounds;
failure reporting now prints extent and maximum error then exits instead of
leaving a Windows assertion dialog. No additional game/image run.

Output47/PID29472 terminal0; CPU30/PID36840 terminal0 in0.58 s:153,370 B JPEG,
complete1920x1080 decode, maximum block-centre RGB error4 (unchanged limit12),
full tail and small-budget refusals pass. The fixture uses256 KiB for the detailed
pattern; live output remains110 KiB and its fit is still unproven.350 Python
checks pass. Host126/PID33008/session31734 terminal0, stampa6727b6 dirty,
codegen0 written/1 module current, no guest objects. EXE48,760,320 B SHA256
44C7F7719115FAF6512AE3AC4E7996072EA03BEA2DF66B4DA3D7CB986B966773;
PDB109,948,928 B SHA256DFFFD90D8349248BEA7DD33A762B36A9C7246A7F2DCC5033AAB28EA5551D37CD.

After replacement proof, removed12 superseded output43..46/CPU26/host125
stdout/stderr logs,17,196 logical B. Keep CPU27/28/29 failure text, current
output47/CPU30/host126 and the distinct screenshot/visibility/rigid GPU proofs.
Total logical cleanup this continuation1,236,381 B; originals960/961 remain
losslessly recoverable, PNG pixels identical, removed build/test logs reproducible.
Final measured free79,948,947,456 B, down4,325,376 B from this continuation's first
reading; not an isolated physical cleanup measurement. Texture fixture73,703,715 B
is237,645 B larger; GPU fixture unchanged14,961,203 B; attachment logs266,448 B
(162 files). New963 log/JPEG/request total600,774 B. Those selected retained
diagnostic scopes, including archive and PNG savings, net373,929 B smaller;
host objects, source/Git and unrelated volume activity are separate. All owned
producers terminal; exact owner profile restored. Image total10,378,927 B leaves
106,833 B, short of the next110 KiB reservation. No additional game run or raw
capture; preserve963 and preflight a distinct next export before launching.

### Corrected encoder live continuation (2026-09-08)

Previous turn made progress:0efb3d0 pushed, host126 and CPU30 pass;963's invalid
JPEG remains preserved. First free79,952,072,704 B; no owned renderer/build/test
live, host126 EXE hash and exact116 B owner-profile hash still match. Reuse the
actual a6727b6-dirty binary; no restamp build or unchanged fixture rerun.
Read current devloop/disk policy and the entire local run supervisor. Planned
run964 uses the same strict reload/field/culling observation,300 s,800 KiB text,
110 KiB JPEG,64 B request and192 MiB/free-floor guard; raw/perf/dump/cook off.
Before launch, losslessly recompress only the reviewed scene-commands PNG under
4 MiB temporary overlap, retaining exact chunks/scanlines/decoded pixels. Preserve
963's JPEG/request under explicit truncated-evidence names (rename only, still
counted, no space credited), then check the existing aggregate budgets. The
new observation is a valid renderer-owned post-gamma frame using the corrected
encoder, not another PrintWindow image or relaxed visibility gate.

Run964/PID36196/session59694 completed00:14:57..00:17:04, terminal1. The strict
cold/reload chain passed (generation93/instance144 ->207/384); fresh reloaded
frames4660..4960 added300 snapshots,3229 generated/recorded draws,3227 collected
instances,3214 visible and13 culled. Request `1 4960 5080` recorded frame5005,
slot1, input0185D1CFD150, output0185F3B958E8, descriptor104,1920x1080 awaiting
the submission fence. At00:17:03.680 the renderer reported
`encoding/size/exclusive-write refused`; no JPEG was written. The supervisor's
generic reload-error exit followed that probe error, not a failed reload gate.
This is not new pixel acceptance or a diagnosis of962's title-logo discrepancy.
Retain519,448 B `logs/reblue_964.log`, SHA256
5434E7DA19DA08652068375C3F4E25F482144B351B97FFC32205F67486FB6980,
and11 B `out/verification/native_frame_probe.request`. Exact116 B owner profile
restored to SHA256
2F1BC38D763A1B7BDBA31F560684FD4AA7E42A714600B8D344F19DA7F38E23B0.

Completed lossless scene-commands PNG recompression:3,334,156 ->3,258,340 B,
75,816 logical B reclaimed once, with identical chunks/scanlines/decoded pixels.
New SHA2561CD34950E217EA8A82AA6D26F3C91A327C2F8AE36532E1CFD0A17542E16CD7C3.
Preserved963's JPEG/request as `native_frame_probe_truncated_window.jpg` and
`native_frame_probe_truncated.request` in `out/verification`; hashes unchanged,
no bytes reclaimed by renaming. Image total10,303,111 B leaves182,649 B under
the10 MiB aggregate limit. New964 text/request retains519,459 B; subtracting
the PNG savings, these selected diagnostic scopes grew443,643 B. No new raw,
performance CSV, asset cook, build or fixture output in this continuation.
The proposed256 KiB per-image limit still needs owner approval and separate
aggregate-budget preflight; no further capture is authorized by this checkpoint.

### Immediate GPU geometry ownership (2026-09-08)

Previous turn made progress by committing/pushing964's completed evidence as
18eeb69. Current source audit confirms queued/native/instanced emissions change
physical PSO/VB/IB without changing the immediate producer's logical dirty flags.
Move immediate binding outside those flags, reusing prepared views, pipeline
cache and existing fence-owned resources. No guest import or queue warm-up is
added. This corrects an independent handoff contract, not a proven cause of962.
The existing C++ draw-intent fixture covers real Plume view ranges/gaps/offsets;
the existing rigid GPU fixture gains three stale-input controls and restorations
with the unchanged two-eye pixel/depth oracle.351 Python guards/scenario tests
pass; C++/GPU/host results and live acceptance are pending before these builds.

Preflight free79,936,110,592 B; no renderer/build/test producer live. Reuse original
3 GiB exception/floor62,509,998,080 B, not a new budget. Texture fixture73,703,715 B
(131 files), GPU fixture14,961,203 B (17), build/test logs266,448 B (162). Plan
draw-intent build21/CPU19 and GPU build49/rigid16 using the existing wrapper:
32 MiB maximum free drop per fixture producer,300 s supervisor/30 s CTest,
10 MiB aggregate log cap; estimated retained fixture growth below2 MiB and new
logs below128 KiB. Host127 follows passing fixtures only,192 MiB peak free-drop
guard covering the incremental object/link overlap. No new image/raw/cook/perf
output or game launch. Retire only superseded matching success logs after proof;
all unresolved runtime and distinct GPU evidence remain protected.

Build21/PID33696 and CPU19/PID27096 terminal0; GPU49/PID28652 and rigid16/PID29332
terminal0.52 two-eye cases pass in1.45 s, validation0/0, including separate
stale-PSO/VB/IB controls and production restorations. Host127/PID34664 terminal0,
18eeb6994 dirty; codegen0 written/1 module current, no guest objects compiled.
EXE48,761,344 B SHA256
A0792FE25CB53149000151DA5927141A4F38C0CA9DEAA5574FAE2E685AB2C038;
PDB109,965,312 B SHA256
DC2B16F2075F15DEAB2BFB05A435B7A5DDD09080454AF1880E4976FB353893C6.
No game producer or image; all jobs terminal and unchanged owner-profile SHA256
2F1BC38D763A1B7BDBA31F560684FD4AA7E42A714600B8D344F19DA7F38E23B0 confirmed.

After replacement proof, removed10 superseded draw-build20/CPU18/GPU48/rigid15/
host126 stdout/stderr files,21,071 logical B once; these success logs are
reproducible. Current binding/rigid/host logs and distinct screenshot1/visibility3/
occlusion6 proofs remain, as do every unresolved failure and protected image/raw
set. New build/test logs22,744 B, net1,673 B growth after cleanup, total268,121 B
(162 files). Texture fixture74,104,103 B (132 files), up400,388 B; GPU fixture
15,027,922 B (17 files), up66,719 B. Selected fixture/log scopes net468,780 B
larger for the new executable/object and causal test coverage. No raw/cook/perf/
image output. Measured free79,935,156,224 B after cleanup, down954,368 B from
preflight; host/link/source/Git and unrelated drive activity are separate from
the selected diagnostic scopes and logical cleanup. Live acceptance remains open.

### Deferred packet/light-order continuation (2026-09-08)

Previous turn made progress by pushing8d6f4e4 as an explicitly unconnected source
checkpoint. First measured free79,931,392,000 B; no owned build/test/game process
was live. Reused the original3 GiB exception/floor62,509,998,080 B and existing
fixture trees, not a new allowance. The local supervisor gained only explicit
deferred build/CTest target routing. Every fixture job used32 MiB free-drop,
300 s supervisor/30 s CTest and10 MiB cumulative build-log caps; estimated new
retained fixture data below2 MiB, logs below128 KiB. Host128 used192 MiB free-drop
and the same automatic unexpected-guest-object stop. No game/raw/image/cook/perf
producer was launched, and the per-image budget question remains unanswered.

All jobs completed0: deferred build1/CPU1, output build48/CPU31, material builds39
and40/CPU37 and38, GPU build50/rigid17, host128. Exact PIDs/contracts/hashes are in
`20260908_0109_deferred-packet-order.md`. Three CPU fixtures,351 Python checks and
55 two-eye GPU cases pass, validation0/0. Host128 is8d6f4e426 dirty, codegen0
written/1 module up to date and no guest object compilation. The producer/consumer
native deferred connection and live/pixel qualification remain pending.

After replacement validation, removed18 explicitly resolved superseded success
logs: output47/CPU30, material38/39 and CPU36/37, GPU49/rigid16 and host127,
stdout/stderr pairs.26,887 logical B removed once; logs are reproducible, replacement
proof retained. No unresolved failure, distinct GPU test, owner data or capture
was deleted. New logs30,324 B; ending aggregate271,558 B/166 files, net3,437 B
growth. Texture fixture74,477,093 B/132 files (up372,990 B); GPU fixture15,272,322 B/
17 files (up244,400 B). Those two fixture/log scopes grew620,827 B for retained
executables/objects and causal regressions. Material fixture ends8,359,739 B/41
files; its pre-first-build size was not separately sampled, so no exact full-turn
delta is attributed to it. Host outputs/objects and source/Git are separate.

Cleanup free79,931,994,112 B,601,112 B above this continuation's initial reading;
drive-wide activity is not claimed as cleanup savings. The immediate pre/post
cleanup readings differed20,480 B; logical bytes and physical free-space movement
are deliberately separate. All owned producers terminal; owner profile unchanged
at SHA2562F1BC38D763A1B7BDBA31F560684FD4AA7E42A714600B8D344F19DA7F38E23B0.
The10,303,111 B image archive and252,177,116,500 B protected raw inventory receive
no new files and no new cleanup credit. Preserve all outstanding runtime gates.

### Owned scene consumer continuation (2026-09-08)

Original exception/floor62,509,998,080 B remains unchanged. First read before
edits80,516,481,024 B free; producer preflight80,519,471,104 B free. Existing
texture/material/GPU fixture trees measured74,477,093/8,359,739/15,272,322 B,
132/41/17 files; attachment logs271,558 B/166 files. No owned producer was live.
Planned replacement-only outputs: under2 MiB retained fixture growth,128 KiB
logs;32 MiB fixture free-drop and192 MiB host overlap guards,300 s supervisors,
30 s CTest and10 MiB aggregate logs. No new game/image/raw/cook/perf output.

Output49/CPU32, material41/CPU39, GPU51/rigid18 and host129 all completed0;
PIDs, hashes and limits are in `20260908_0135_owned-scene-consumption.md`.
Two C++ fixtures,352 Python checks and55 two-eye Vulkan cases pass, validation0/0.
Host129 is016d3a954 dirty, codegen0 written and no guest objects. New direct
packet-consumer connection is host-built, not live/pixel qualified; sorted
production and all full-frame acceptance gates remain open.

After replacement validation, removed14 exact superseded success logs: output48/
CPU31, material40/CPU38, GPU50/rigid17 and host128 stdout/stderr pairs.27,042
logical B removed once; reproducible logs only. New logs27,900 B, retained total
272,416 B/166 files, net858 B growth. Distinct deferred/screenshot/visibility/query
evidence and every unresolved failure remain. Owner profile unchanged116 B,
SHA2562F1BC38D763A1B7BDBA31F560684FD4AA7E42A714600B8D344F19DA7F38E23B0.

Final fixture sizes75,383,225/8,361,060/15,275,881 B, unchanged file counts;
growth906,132/1,321/3,559 B respectively. These fixture/log scopes grew911,870 B
for the new production-packet regressions and rebuilt objects/executables. Host
outputs/objects and source/Git are separate. Post-cleanup free80,517,013,504 B,
down2,457,600 B from producer preflight; drive-wide movement is not all attributed
to the task. Immediate cleanup free-space gain36,864 B differs from logical log
bytes. No image/raw growth or repeated cleanup credit; the original image-budget
question remains unanswered. All owned jobs are terminal.

### Connected native deferred continuation (2026-09-08)

Original3 GiB exception/floor62,509,998,080 B unchanged. First scoped free read
80,511,225,856 B; producer preflight80,511,295,488 B. No owned producer was live.
Existing texture/material/GPU fixture trees75,383,225/8,361,060/15,275,881 B,
132/41/17 files; attachment logs272,416 B/166 files. Planned replacement outputs
under2 MiB retained fixture growth and128 KiB logs, with32 MiB fixture/192 MiB
host free-drop guards,300 s supervisors and30 s CTest. No raw/image/cook/perf/game
producer. The protected raw/image inventory and unanswered export-budget question
are unchanged; no new allowance or capture-cleanup credit is claimed.

Output50/PID32180, deferred2/PID37152, CPU33/PID36876 and host130/PID37044 all
terminal0. CPU33 runs32 tests, including both changed C++ fixtures;353 Python
checks pass. Host130 is f3131a5d5 dirty, codegen0 written/1 module current and no
guest objects. Exact hashes and behavioral scope are in
`20260908_0206_native-deferred-connection.md`. Native sorted connection is built,
not yet live/pixel qualified. Unchanged rigid18 shader evidence is reused.

Removed10 resolved superseded success logs after replacement: output49,
post_output_cpu32, deferred build1/CPU1 and host129 stdout/stderr pairs.5,485
logical B removed once; reproducible logs only. Immediate physical free gain
12,288 B differs from logical bytes. All distinct GPU/material proofs, unresolved
failures, images, raw sets and user data are retained. New logs7,268 B; total
274,199 B/164 files, net1,783 B growth. Texture fixture76,577,767 B/132 files,
up1,194,542 B for the production queue/callback regressions and rebuilt objects/
executable. Material/GPU fixtures unchanged. These selected fixture/log scopes
grew1,196,325 B; host/source/Git and unrelated drive activity remain separate.

After cleanup free80,508,788,736 B, down2,506,752 B from producer preflight. This
is drive-wide movement, not all attributed task storage. No profile mutation;
SHA256 remains2F1BC38D763A1B7BDBA31F560684FD4AA7E42A714600B8D344F19DA7F38E23B0.
All owned jobs terminal; no duplicate producer or image export was launched.

### Native deferred live continuation (2026-09-08)

Previous checkpoint made implementation progress; current source starts0e67545
clean. No producer was live. First measured free80,507,265,024 B. Original3 GiB
exception/floor62,509,998,080 B and all raw/image limits remain unchanged.
Diagnostic preflight78,451,663 B left only191,537 B below75 MiB, insufficient
for the400 KiB cold-log reservation; no profile change or renderer was launched.

Archived completed962..964 logs under a bounded2 MiB analysis-overlap reservation
inside the existing25 MiB analysis/polling reserve. Input1,575,997 B became
`retained-native-runtime-962-964.zip`,236,981 B; all three expanded SHA256 values
were verified before removing plaintext. Full evidence is recoverable. Logical
saving1,339,016 B; immediate free80,510,533,632->80,511,877,120 B (physical gain
1,343,488 B). Hashes and locations are in `20260908_0225_native-deferred-live.md`.
The latest964 entry was temporarily restored solely to prevent reuse of its log
sequence number, then removed after965 existed. No extra cleanup credit is taken
for that temporary copy. All pixels/raw payloads remain untouched.

Run965/PID28816/session53891 terminal0 (75 s/400 KiB caps), log254,875 B. Run966/
PID35492/session44373 terminal0 (180 s/800 KiB caps), log505,061 B. Both reuse
unchanged host130 and enforce the original cumulative75 MiB diagnostic stop,
192 MiB free-drop and reserve, with captures/CSV/dumps/cooking off. Before966,
diagnostics77,367,522 B left1,275,678 B below stop, fitting its819,200 B reservation;
free80,509,247,488 B. No runtime cache/hlsl/perf files were created or modified
after965 start. No C++/GPU build, fixture growth, new raw or image output.

965 passes the cold field;966 passes strict independent cold/reloaded text epochs,
including native sorted consumption, existing source/GPU regression retirement,
receiver/lighting/caster/cutout checks.357 Python checks pass. No pixel, full-frame
or stereo acceptance is inferred. Both jobs terminal; original116 B profile
restored, SHA2562F1BC38D763A1B7BDBA31F560684FD4AA7E42A714600B8D344F19DA7F38E23B0.

After966 replaced the cold proof, removed965's254,875 B plaintext success log
once. Its exact raw text is retired; small results/hash/procedure are recorded.
Immediate free80,507,211,776->80,507,469,824 B (258,048 B physical gain). New logs
total759,936 B before retirement; final run-log/archive scope is833,955 B smaller
than the continuation's initial state. Current966, all unresolved failure logs
(some in validated archives), distinct GPU proofs, user data and captures remain.
Post-cleanup free80,507,469,824 B,204,800 B above the first reading; this drive-wide
movement is not the logical cleanup saving. Source/Git and unrelated activity
remain separate. Per-image budget question is still unanswered; no export tried.

### Native ordinary visual-effect continuation (2026-09-08)

Original3 GiB exception/floor62,509,998,080 B unchanged. First free80,493,723,648 B;
producer preflight80,497,094,656 B. No producer live at start. Texture fixture
76,577,767 B/132 files; material8,361,060 B/41; GPU15,275,881 B/17; build logs
274,199 B/164 files. Planned replacement-only fixture growth below2 MiB and128 KiB
logs;32 MiB fixture/192 MiB host free-drop,300 s supervisors/30 s CTest and10 MiB
aggregate logs. No new raw/image/cook/perf producers or changed capture allowance.

Output51/PID37604, deferred3/PID29712, CPU34/PID28444 and host131/PID29472/session42367
all terminal0. Two rebuilt C++ fixtures,32 CPU tests,360 Python checks pass.
Host131 is41711b837 dirty, codegen0 written/one module current, no guest objects;
exact binary hashes/contracts in `20260908_0251_native-deferred-effects.md`.
New ordinary native visual scopes are connected and built, not yet live/pixel
qualified. Prior unchanged shader GPU evidence is reused, not rerun.

Removed eight exact superseded success logs after replacement: output50,
deferred2, CPU33 and host130 stdout/stderr pairs.7,268 logical B removed once;
immediate free80,501,141,504->80,501,149,696 B,8,192 B physical gain. Reproducible
logs only; no distinct GPU/material proof, failure, raw/image or user data removed.
New logs7,324 B; ending274,255 B/164 files, net56 B. Texture fixture76,845,900 B/
132 files, up268,133 B; material/GPU fixtures unchanged. Selected fixture/log
growth268,189 B, separate from host/source/Git and unrelated volume activity.
Ending free is7,426,048 B above first reading, not all cleanup savings. Profile
unchanged SHA2562F1BC38D763A1B7BDBA31F560684FD4AA7E42A714600B8D344F19DA7F38E23B0.
Protected raw/image inventory and unanswered image-budget request unchanged.

Final no-original-call review explicitly disables blend reference execution in
the direct producer, including a setting change after preflight. Host132/PID37724/
session81881 terminal0 links the final source;360 Python checks pass again. No
guest objects, shader changes or new C++ fixture rebuild. Current binary hashes
are appended in the effects report. New host132 logs2,524 B; superseded host131
success logs3,313 B removed after validation,4,096 B immediate physical gain.
Total10,581 B/10 logs removed once;9,848 B new, final273,466 B/164 logs, net733 B
smaller than start. Selected fixture/log growth267,400 B. Free80,499,597,312 B.
Before that last log cleanup, runtime preflight77,620,288 B diagnostic retention
fits the819,200 B reload reservation under the existing75 MiB stop. No game run yet.

Run967/PID37656/session64033 terminal0,02:58:51..03:00:55 under180 s/800 KiB,
192 MiB free-drop, original reserve and75 MiB cumulative diagnostic stop. Source
bad7cc7 clean, unchanged host132. Full strict cold/title/reload chain plus new
deferred-effect gate passes independently in both epochs:10,957/2,758 native
packet reads and3,383/234 balanced visual scopes, alongside legacy consumption.
Exact counts/hash/recipe in the effects report. No pixel or timing qualification.
Log509,208 B; no raw/image/cache/hlsl/perf growth. Profile restored byte-exact,
SHA2562F1BC38D763A1B7BDBA31F560684FD4AA7E42A714600B8D344F19DA7F38E23B0.

After replacement passed, removed superseded966 plaintext505,061 B, hash verified
against its recorded report. Full raw text retired; small results/hash/procedure
remain. Preserve959's distinct merging proof, failures and every image/raw set.
Immediate free80,497,496,064->80,498,003,968 B (507,904 B physical gain). Runtime-log
retention grows4,147 B, selected fixture/build/runtime-log scopes271,547 B. Total
logical removal this continuation515,642 B, no double credit. Free4,280,320 B above
first reading is drive-wide activity, not all task cleanup. All producers terminal;
full goal and image budget gates unchanged.

### Native visual identity source checkpoint (2026-09-08)

Commit/push request; no host build, game run, capture or profile override resumed.
Original cumulative floor62,509,998,080 B and all raw/image limits unchanged.
First measured free80,497,643,520 B; texture fixture76,845,900 B/132 files and
build logs273,466 B/164 files match the prior ledger. Reused both existing CPU
fixture targets, under32 MiB free-drop/300 s producer/30 s CTest and10 MiB log caps.
Output52/PID23248, deferred4/PID14720 and CPU35/PID37752 all terminal0.32 CPU tests
pass in7.06 s;361 artifact-free Python checks pass. Host132/run967 remain the
prior binary/live evidence, not qualification for this source connection.

Replacement fixture bytes77,315,936 B/132 files, growth470,036 B. New build logs
4,011 B; after replacement passed, removed six exact superseded output51,
deferred3 and CPU34 stdout/stderr files totaling4,011 logical B. These are
reproducible success logs; no failure evidence, game data, raw/image payloads,
active builds or distinct host/GPU evidence removed. Immediate free
80,493,047,808->80,493,051,904 B (4,096 B physical gain). Build-log retention is
unchanged; selected fixture/log growth470,036 B. Net drive-wide free change
-4,591,616 B includes source/Git and unrelated activity, not just fixture growth.
Ending free about74.96 GiB; no producers remain. Runtime callback/order and
cold/reload/pixel qualification remain pending as recorded in the active queue.

### Writer-ordered visual input integration (2026-09-08)

First free80,488,423,424 B. Original cumulative3 GiB exception and floor unchanged;
no live producer at start. Host133/PID38216/session26287 terminal0,3154 B logs,
codegen0 written/no guest objects. Run968/PID23728/session19334 fails before field
readiness (03:37:22..03:37:50); log85,004 B. Profile restored byte-exact. No raw,
image, cooking or runtime cache/hlsl/perf output. Full causal evidence and hashes
are in `20260908_0337_native-visual-inputs.md`.

Unplanned Windows WER minidump77,048,323 B and Report.wer82,998 B exceeded the
diagnostic ceiling; no budget reset/exception was inferred. Installed cdb with
local symbols identified an uncaught deferred resource-writer refusal at the
water vtable. After retaining exact cause, stack/source/vtable provenance and
hashes, removed only run23728's diagnosed dump. Immediate free
80,176,091,136->80,253,140,992 B,77,049,856 B physical gain. Complete dump memory
is retired; failed log and OS metadata remain. The runner counts that metadata
in the original ceiling. Known deferred C++ failures now flush their cause and
use existing fatal shutdown, not unhandled-exception WER production.

Deferred5/PID38392, CPU36/PID19872 and host134/PID27788 terminal0 under32 MiB
fixture/192 MiB host free-drop,300 s/30 s test bounds and10 MiB logs.32 CPU tests
pass in7.65 s;364 Python checks pass. One changed fixture rebuilt; no new GPU
fixtures/shaders. Host134 builds the source correction, not yet runtime qualified.
Texture fixture77,361,247 B/132 files, up45,311 B this continuation. New build logs
8,484 B. After replacement passed, removed six superseded success files:
host132/deferred4/CPU35 stdout/stderr,6,395 B; immediate free
80,251,715,584->80,251,723,776 B (8,192 B physical gain). Host133 failure-related
build log remains. Final275,555 B/166 build logs, net2,089 B. Selected fixture/
build-log/runtime-failure/OS-metadata retained growth215,402 B. Total logical
removal77,054,718 B counted once, including the diagnosed dump.

Ending free80,251,723,776 B (~74.74 GiB),236,699,648 B below first reading.
Scoped producer files do not explain most of that drive-wide change; host/source/
Git and unrelated system activity are separate, not claimed as cleanup savings.
All producers terminal; fresh writer-ordered cold/reload and pixel gates pending.

Run969/PID35028/session53430 terminal0,03:58:48..04:00:53. Source0784d47 clean,
unchanged host134. Full strict cold/reload chain plus writer-refresh gate passes
independently in both epochs:10,865/3,053 native reads and175/139 refreshes, zero
native pending, with old93/144 retired before new207/434. Exact receipts and
hashes are in the visual-input report. No new raw/image/cache/hlsl/perf or OS dump;
profile restored to the same116 B/hash. Log511,707 B under800 KiB/180 s and192 MiB
free-drop; cumulative75 MiB stop now also includes run968's82,998 B OS metadata.

After replacement passed, retired967 plaintext509,208 B after exact hash check.
Its full text is gone, results/hash/procedure remain. Failed968 log, symbolized
cause/OS metadata,959's distinct merging proof and all pixels/raw/failure evidence
remain. Immediate free80,248,643,584->80,249,155,584 B (512,000 B physical gain).
Success-log retention grows2,499 B; this continuation's selected fixture/build/
runtime/OS-metadata retention grows217,901 B. Total logical removal77,563,926 B,
counted once. Latest free80,248,889,344 B (~74.74 GiB),239,534,080 B below first
reading; most drive-wide movement remains outside attributed producer files.
No producer live. Full native-frame, pixel/sequence/both-eye and Quest gates remain
open; no speedup claim and no change to the unanswered image-budget request.

### Native water program and stereo GPU fixtures (2026-09-08)

Same cumulative ledger, original3GiB exception/floor62,509,998,080B and raw/image
limits. First measured free80,244,531,200B. Before the first producer,
free80,247,488,512B; GPU fixture15,275,881B/17files and build logs275,555B/166files.
No live renderer/build producer at start. Existing shader dumps/source/assets
reused; no extraction, cooking, download, renderer run, image export or raw data.
The ignored existing supervisor gained a water test selector, not a new harness.
GPU build peak estimate48MiB, enforced free-drop48MiB initially/32MiB subsequent;
GPU tests16MiB, host192MiB;300s producer/30s CTest and10MiB aggregate logs.

GPU builds52/PID32272,53/35344,54/35612,55/29132,57/33388 terminal0;
56/33752 terminal1 (Windows max macro collision, corrected). Water1/PID35244,
2/33984 and4/21652 terminal1; failures and exact causes/hashes retained in
`20260908_0435_native-water-program.md`. Water3/37120 terminal0 with the first10
cases; finalwater5/31016 terminal0 with16two-eye cases in1.26s. Rigid19/29656
terminal0 with55cases in1.24s. Both GPU suites validation0errors/0warnings;
one existing GOG overlay loader diagnostic each.368 artifact-free Python tests
pass. Host135/PID32596 terminal0 in9.52s;0codegen writes/no guest objects.
This builds the new program, not a live water queue connection. No game/profile
changes; original116B profile hash unchanged. All named producers terminal.

Final GPU fixture17,190,913B/19files, growth1,915,032B. New compiled water shader
headers300,263B (236,809PS/63,454VS), in the existing generated tree. Host EXE/PDB
grow46,080B combined; their exact identities and the fixture hash are in the
water report. Keep current build products; no duplicate binary/archive copies.
These are needed for the next connected native water producer/queue work.

After final water and rigid replacement passed, removed20exact superseded
agent-owned log files: GPU build51,53..56; water tests1..4; rigid test18, each
stdout/stderr. Logical deletion35,990B. The water report preserves the diagnosed
fixture failure causes/hashes; complete old stdout text is retired. These cases
can be rerun but historic exact output is not retained. Keep first shader build52,
current57/water5/rigid19/host135 plus the prior live host134 build evidence.
No unresolved game failure, original asset/profile, capture, protected raw data
or active build tree removed. Immediate free80,227,532,800->80,227,577,856B:
45,056B physical interval gain. Cleanup counted once, not the replacement sizes.

Build logs now280,732B/172files, net5,177B growth; new gross logs41,167B minus
35,990B retired. Selected fixture/log/shader-header/EXE/PDB growth2,266,552B;
other new object/build metadata and source/Git bytes are separate. Ending free
80,227,577,856B (~74.72GiB),16,953,344B below the first reading. That drive-wide
change includes unattributed system activity, not all producer output or cleanup.
No new raw/images/OS dumps are required by these fixtures. Native material
publication, live queue/image lifetimes, conservative wave bounds and game
art/pixel/sequence/both-eye qualification remain open; full goal not narrowed.

### Native water shared queue/geometry (2026-09-08, source333ce83 dirty)

Same cumulative exception/floor62,509,998,080B and all raw/image/diagnostic caps.
First measured free80,213,200,896B; before first producer80,215,789,568B. No live
renderer/build jobs at preflight. Existing fixture tree17,207,057B/19files
(16,144B above the preceding ledger's metadata measurement), logs280,732B/172files.
No game launch/profile overrides, assets, recooking, extraction, captures or new
shader regeneration. Existing shader programs reused. Planned/enforced free-drop:
GPU build48MiB initial/32MiB later, CPU build32MiB, tests16MiB, host192MiB;
300s producers/30s tests,10MiB cumulative logs. Independent CPU/GPU trees were
built concurrently for the final feedback-guard change, with both supervised.

GPU58/PID36552,59/576,60/3732 pass; water6/32492 fails120validation errors due
to premature setup framebuffer destruction. Corrected lifetime passes water7/
37792 and final8/37092 (16two-eye cases,1.23s,validation0/0). Full failed log cause/
hash remains in20260908_0509_native-water-queue.md; no comparison relaxed.
Rigid20/29716 passes55cases,1.23s,validation0/0. CPU output53/33924 and30/34480
pass; final output54/32200 and31/27540 pass,0.57s.370Python checks pass. Host136/
33668 and137/29092 pass,0codegen writes/no guest objects; two existing unrelated
deprecation warnings in the final host build. No producer live. No live water
caller yet; host134/run969 remains the last game evidence. Profile116B/hash
unchanged. Image110KiB/raw budgets and protected historical evidence unchanged.

Current GPU fixture17,787,549B/19files: growth580,492B from this turn's preflight,
596,636B from prior ledger. Texture fixture77,501,216B/132files, up139,969B.
Host EXE/PDB grow184,320B combined; exact final hashes in the new report. These
replace existing products, no copies; no new compiled shader headers. Other host
object/metadata and source/Git bytes are separate from these selected measurements.

After passing replacement, removed22exact log files: GPU builds57/58/59,
water5/6/7, rigid19, CPU output build53/test30, host135/136 stdout/stderr.
Logical99,972B; immediate free80,210,419,712->80,210,530,304B,110,592B physical
interval gain. Removed water6's diagnosed fixture-failure full text, preserving
cause/hash and the causal passing regression; no unresolved game evidence removed.
Keep GPU60/water8/rigid20/output54+31/host137 plus first shader build52 and live
host134 logs. Read-only cleanup inspection found older CPU29 was a timeout;
it and CPU build52 are preserved, not silently relabeled successful/superseded.

Logs now285,449B/176files, net4,717B retained growth (104,689B new minus99,972B
retired). Selected fixture/log/EXE/PDB growth925,642B against prior ledger;
909,498B using this turn's observed GPU tree baseline. Ending free80,210,530,304B
(~74.70GiB),2,670,592B below first reading. That is drive-wide activity, not all
attributed to selected outputs; cleanup credited once. No protected raw/image set,
original game data, profile, dependency or active build tree was removed.

Final ordering audit in the same bundle: existing opt-in depth/eye sorts and
whole-queue legacy prepass hoisting ignored native ordering barriers. The shared
sort/prepass run helper now preserves them; blended gathering also stops there.
CPU55/PID26132 fails a fixture-local `barrier` name collision; renamed predicate,
CPU56/PID38044 and test32/PID21864 pass0.50s. Host138/PID32948 passes with0codegen
writes/no guest objects. No shader/GPU input change; water8/rigid20 evidence reused
with its actual fixture provenance. No game run, captures or profile change.

Retired8additional superseded logs after replacement: host137, CPU builds54/55,
CPU test31 stdout/stderr;8,122B logical. Immediate free80,209,911,808->
80,209,928,192B,16,384B physical interval gain. CPU55's compile-error cause/hash
remains in the water queue report; full old stdout retired. Both cleanup passes
total30files/108,094B logical,126,976B physical interval gains, counted once.
Current logs283,620B/176files: net2,888B growth,110,982B gross new minus108,094B
retired. Keep GPU60/water8/rigid20/CPU56+32/host138 and protected prior purposes.

Final texture fixture77,690,448B/132files (up329,201B), GPU fixture unchanged
17,787,549B/19files, final EXE/PDB growth193,024B combined. Selected retention
growth1,121,749B against prior ledger (1,105,605B using this turn's observed GPU
baseline). Ending free80,209,928,192B (~74.70GiB),3,272,704B below first reading;
drive-wide movement includes other object/metadata/source/Git/system activity.
All producers terminal. Queue source/fixtures built; live water admission and
full authored/pixel/sequence/stereo acceptance remain unqualified.

### Native water typed image handoff (2026-09-08, sourcef0fa499 dirty)

Same cumulative exception/floor62,509,998,080B,20GiB reserve and raw/image/log
gates. First measured free80,193,146,880B; producer preflight80,192,454,656B.
Selected existing fixture/log totals matched the preceding ledger exactly;
inter-turn drive movement is unattributed system/source/Git activity, not new
capture or duplicate agent jobs. An authoritative process check found no live
renderer/CMake/Ninja/CTest/compiler. No game launch/profile edits, assets, recook,
capture, shader regeneration or new build trees. The existing supervisor enforced
32MiB per CPU/GPU build,16MiB per test and192MiB for the host build,300s/30s
timeouts and10MiB cumulative logs. These caps do not reset the original budget.

CPU output57/PID35536 and test33/PID31072 pass0.48s. GPU61/PID7280 and water9/
PID38340 pass16two-eye cases1.14s,validation0/0. Host139/PID34368/session14299
terminal0,0codegen writes/deletions/no guest objects.371Python checks pass.
All producers terminal; detailed scope/hashes are in
20260908_0940_native-water-image-leases.md. Existing116B profile SHA256 remains
2F1BC38D763A1B7BDBA31F560684FD4AA7E42A714600B8D344F19DA7F38E23B0.
No new water game consumer, pixels or full-frame/stereo qualification claimed.

Current CPU fixture77,893,587B/132files, growth203,139B; GPU fixture18,229,755B/
19files, growth442,206B. Host EXE/PDB combined growth9,216B. Existing products
replaced in place, no archived binary copies; other host object/metadata/source/
Git bytes are separate. Keep these products for connected material/scene work.

After passing replacements, removed10explicit superseded successful log files:
GPU build60,water8,CPU output build56/test32,host138 stdout/stderr.8,059B logical;
immediate free80,190,410,752->80,190,427,136B,16,384B physical interval gain,
counted once. Full old stdout text is retired, checks regenerable and previous
reports retain their outcomes. Keep current61/water9/CPU57+33/host139, first
shader52, prior rigid20 and live host134 evidence. CPU29 timeout/build52 and
all other unresolved failures remain protected. No originals, profiles, assets,
active build trees or historical raw/image captures were removed.

Logs312,819B/176files:37,258B gross new minus8,059B removed =29,199B net growth.
Selected fixture/log/EXE/PDB retained growth683,760B (~0.65MiB), needed for current
owner/fixture evidence. Ending free80,190,427,136B (~74.68GiB),2,719,744B below
first reading; drive-wide activity is not all attributable to these outputs.
Raw/image/OS diagnostic totals unchanged. No capture allowance reused or expanded.

### Native water-bottom pass and framebuffer clear ownership (2026-09-08, sourcef22e607 dirty)

Same cumulative exception/floor62,509,998,080B and all reserve/raw/image/log caps.
First measured free80,187,990,016B; producer preflight80,190,988,288B. No new
build tree, assets, cooking, shader regeneration, raw/image files or binary
archive. CPU/GPU builds normally32MiB, final Plume build64 capped64MiB, tests16MiB,
host192MiB; existing300s/30s timeouts and10MiB aggregate attachment logs remain.
Implementation and causal failure evidence:20260908_1031_native-water-bottom.md.

CPU58/output34, GPU64/water11/rigid21/snapshot20, host140/141 all terminal0;
372 Python checks pass. Real D32 bottom sampling exposed the deferred-clear
framebuffer/view ownership bug; Plume191c31c fixes it without oracle changes.
Keep GPU62 compile failure and water10 causal failure text, as well as unresolved
CPU29 timeout/build52 and prior game failures. Current17water/55rigid/8snapshot
cases have validation0/0. Host141/run970/PID23820 terminal0, strict cold/reload
chain passes with900 fresh scene/shadow emissions per epoch, but the new bottom
pass was not observed. No native water game admission or pixel qualification.
All producers terminal; original116B profile hash restored (see new report).

First run970 preflight refused before profile mutation/launch: existing diagnostic
inventory81,288,631B plus819,200B requested log cap exceeded75MiB by3,464,631B.
Losslessly archived19 explicitly selected completed runtime logs817..911 (exact
IDs and archive hash in new report),4,961,074B plaintext ->598,041B archive.
Verified every entry length/SHA and source hash/path before removal. Net logical
reclamation4,363,033B, counted once; exact text is recoverable from the zip.
Immediate free80,185,847,808->80,190,246,912B: net4,399,104B physical interval
gain (includes the archive); do not also credit the5,001,216B deletion interval.
Resulting diagnostic inventory76,925,598B plus819,200B fit the unchanged cap.
Real run970 used180s/800KiB/192MiB free-drop limits and no capture/perf/dump.

Handoff read-only inventory: CPU fixture77,998,002B/132files, growth104,415B;
GPU fixture18,685,835B/19files, growth456,080B; attachment logs360,520B/198files,
growth47,701B. EXE/PDB combined growth119,296B. Selected build products/logs
grow727,492B, retained for the connected owner/backend regression. New run970
log496,521B; runtime logs/archive net shrink3,866,512B. Combined selected retention
therefore shrinks3,139,020B; other host objects/metadata/source/Git are separate.
Superseded small attachment logs were not removed again for the commit request.
No unresolved failure, original game data, profile or protected capture deleted.

Last measured free after the completed run80,189,005,824B (~74.68GiB),1,015,808B
above the first reading. This drive-wide movement includes unrelated activity,
not just attributed cleanup. Commit handoff reused evidence and did not start
another producer or reset any budget. Next IDs: host142, game971, CPU59/output35,
GPU65/water12/rigid22/snapshot21; another unchanged bottom-inactive boot is not
useful evidence. Full goal remains active, including authored game stereo gates.

### Native planar reflection verification (2026-09-08, sourcea5d66b9 plus GPU fixture)

Same cumulative3GiB exception/floor62,509,998,080B and20GiB reserve (supervisor
21GiB), raw/image/log gates unchanged. First free80,176,353,280B. Both fixture
trees and attachment logs matched the previous ledger; no live renderer/build/
test/compiler in the authoritative process check. Inter-turn drive movement is
not attributed to new captures or duplicate producers. Profile116B hash unchanged.
No shader regeneration, new tree, assets/cooking or raw/image files this turn.

Existing supervisors: CPU/GPU builds32MiB free-drop caps, tests16MiB, host192MiB,
300s build/30s test timeouts and10MiB aggregate attachment logs. CPU59/PID35632,
output35/PID32268, GPU65/PID24556, water12/PID17760, rigid22/PID32140,
snapshot21/PID37028 and host142/PID32328/session1353 all terminal0. Tests cover
native reflection output through the actual18-case two-eye water GPU producer/
consumer and existing55 rigid/8 snapshot regressions, validation0/0.373 Python
checks pass. Codegen0 writes/deletions, no guest objects. Detailed scope and hashes
are in20260908_1110_native-reflection-pass.md; no full-frame/art/pixel acceptance.

Pre-build cleanup removed18 superseded successful attachment logs: CPU57/test33,
GPU61/63,water9,host139/140,rigid20,snapshot19 stdout/stderr.69,715B logical;
immediate free80,176,345,088->80,176,427,008B,81,920B drive interval gain.
After replacement passes, removed14 more: CPU58/test34,GPU64,water11,host141,
rigid21,snapshot20 stdout/stderr.30,832B logical; free80,174,194,688->
80,174,235,648B,40,960B interval gain. Preserve CPU29 timeout/build52, GPU62/
water10 causal failures and all unresolved game evidence. No protected image/raw
baseline, originals, profile or active build tree removed.

Runtime preflight before the second log cleanup: diagnostic inventory77,640,552B
plus819,200B maximum text fits75MiB with183,448B headroom. Actual run971/
PID37076/session32898 enforced180s,800KiB text and192MiB free drop; terminal0 at
11:08:03UTC, strict mixed cold/reload chain passes. Reflection publications3900,
compatibility/faults0; fresh300-publication/camera intervals after readiness in
both epochs. Initial9 camera misses persist unchanged, cause unqualified. No
scene-snapshot report, hence no reflection-phase snapshot claim. Native water
game admission remains absent. No captures; exact original profile hash restored.

After971 replaced the same strict cold/reload purposes, removed hash-verified
logs969/970,1,008,228B. Full old text is retired; dated reports preserve their
outcomes/hashes and971 retains the replacement text. The older lossless817..911
zip and failure968 evidence remain. Immediate free80,173,142,016->80,174,153,728B,
1,011,712B interval gain. Three cleanup operations total34files/1,108,775B logical
and1,134,592B drive interval gains; credit once, not again next continuation.

Final selected CPU fixture78,216,846B/132files (growth218,844B), GPU fixture
18,933,958B/19files (growth248,123B), EXE/PDB combined growth152,576B. Attachment
logs299,998B/180files:40,025B new minus100,547B retired =60,522B net shrink.
Runtime log971524,518B minus retired1,008,228B =483,710B net shrink. Selected
products/logs/runtime therefore grow75,311B net, needed for current owner evidence.
Last measured free80,174,153,728B (~74.67GiB),2,199,552B below first reading;
other host objects/metadata/source/Git/system activity is not all attributed here.
Raw/image sets unchanged; no exception expanded. All producers terminal and
profile restored. Next IDs CPU60/output36,GPU66/water13/rigid23/snapshot22,
host143/game972. Continue the ordered water producer/native mixed-consumer
connection, not another unchanged reflection-only or bottom-inactive boot.

### Ordered water draw connection (2026-09-08, source efa8c7b plus edits)

Same cumulative3GiB exception/floor62,509,998,080B,20GiB reserve (supervisor21),
100MiB diagnostics and10MiB attachment-log caps; raw/image gate unchanged.
First measured free80,170,917,888B; pre-build80,170,790,912B. No live renderer,
compiler/build or test producer found. Existing CPU tree78,216,846B/132files,
GPU tree18,933,958B/19files and attachment logs299,998B/180files match the prior
handoff. No new assets, trees, downloads, raw/images or profile changes planned.
Reusing bounded wrapper: CPU60 and GPU66 each32MiB free-drop/300s; tests16MiB/
30s; eventual host143192MiB/300s. CPU/GPU combined peak estimate64MiB; no game
launch until the connected shader/owner fixtures and integration build pass.
373 artifact-free Python boundary/scenario checks pass before builds. Retain
new producer evidence until replacement passes; preserve all unresolved failures.

Completed: CPU60/PID34016/test36/PID38712 passed; CPU61/PID37792/test37/PID27716
supersede them with node retirement/reuse/ambiguity assertions. GPU66/PID24480
compiled both water shaders; GPU67/PID32032 rebuilt the fixture. Water13/PID33804
failed only the new discard oracle (expected old snapshot instead of actual
post-snapshot clear); keep its full log. Corrected that new expectation, unchanged
old oracles/tolerances: water14/PID37580 passes20 two-eye cases, rigid23/PID36096
passes55 and snapshot22/PID38280 passes8, Vulkan validation0/0.375 Python checks
exit0. Host143/PID35168/session68957 and host144/PID35932 terminal0; codegen0
writes/deletions and no guest object compilation. Host144 is the live binary.

Run972/PID35416/session9141 terminal0,07:44:01..07:46:08Eastern. Same180s/800KiB/
192MiB bounds, no captures/perf/dumps, all22 temporary settings audited, exact
116B profile hash restored. Full strict mixed/deferred-input/cold/reload chain
passes. Water now actually submits/emits/retires in both epochs; last sample3179/
3084/3177 with93 culled and no admission refusals. All prior water packets retired
before the new generation's first submission. No actual game pixel/HDR/bottom/
refraction/stereo qualification; those gates remain open. Details/hashes in
20260908_1146_native-water-draw.md. No new assets, trees, downloads or raw/images.

After replacement checks, removed18 explicit successful attachment logs:
CPU59/test35,GPU65,water12,rigid22,snapshot21,host142,CPU60/test36, both streams;
41701B. Removed hash-verified run971524518B after972 replaced its strict purposes.
19files566219B logical total. Immediate free80146726912->80147308544B,
581632B interval gain; credit once. Full old text retired, dated evidence remains.
Keep water13 new failure, prior CPU29/build52/GPU62/water10 and game failures,
the historical lossless zip and all protected image/raw sets. No originals removed.

Retained CPU tree78238320B/132files (+21474), GPU19063572B/19files (+129614),
EXE48952832/PDB111058944B (+206336 combined). Attachment logs298537B/186files:
40240B new less41701B retired =1461B net shrink. Run972541835B replaces524518B,
17317B net growth. These selected products/logs grow373280B net, needed for current
draw/ownership and causal failure evidence; other host objects/metadata are separate.
Drive first80170917888, post-cleanup80147308544B (~74.64GiB):23609344B less free.
Scoped runtime cache/perf enumeration found no new files; do not label the whole
drive change as task-owned output or cleanup. All producers terminal, profile
restored. Next IDs CPU62/output38,GPU68/water15/rigid24/snapshot23,host145/game973.
Next remove water's remaining material/visual/list adapters and qualify authored
image-role/game-pixel cases, not another unchanged admission-only run.

### Direct water material consumption (2026-09-08, source4c23030 plus edits)

Same cumulative3GiB exception/floor62,509,998,080B and all reserve/diagnostic/log/
raw/image caps; no reset. First free80,140,918,784B, pre-build80,140,828,672B.
No live renderer/build/compiler/test producer found. CPU tree78,238,320B/132files,
GPU tree19,063,572B/19files match previous retained products. Reuse configured
trees; no assets/cooking/downloads/raw/images or binary copies.376 artifact-free
Python checks pass. New direct material lifecycle removes model/resource callbacks
and shader selection, commits owned lights before the late water writer, and
keeps ordered outgoing depth/texture cleanup. Native shaders/backend unchanged.

Planned bounded producers: deferred CPU build6/test1,32MiB/16MiB free-drop and
300s/30s timeouts; host145192MiB/300s,10MiB aggregate attachment logs. Run973 only
after these checks pass,180s/800KiB/192MiB, captures/perf/dumps disabled and exact
profile restoration. Purpose: verify direct material begin/end and fresh native
water consumption plus strict neighboring mixed/reload checks after callback
removal, not another unchanged bottom-inactive boot. Existing GPU/pixel evidence
is reused for unchanged shader/backend; new game pixel/stereo gates remain open.
Retain new evidence through replacement validation, then remove only superseded
successful logs; all unresolved failures and protected raw/image evidence remain.

Completed deferred build6/PID36620 and test1/PID37200 exit0; host145/PID35064/
session91407 exit0, codegen0 writes/deletions and no guest objects. Run973/PID32068/
session54385 exit0 at08:11:24Eastern after128s; strict cold/reload chain passed,
all22 settings audited,0 raws, original116B profile hash exactly restored.3159
balanced direct water materials,3071 emissions and3157 retirements with0 admission
refusals; neighboring legacy checks pass. No new shader/game pixel evidence or
bottom/snapshot/stereo qualification. Detailed source/evidence/hashes are in
20260908_1212_native-water-material.md. All producers terminal.

Removed six explicit superseded successful build logs (host143/144, deferred5,
both streams),3958B, and hash-verified run972541835B after973 replaced its strict
purposes.7files545793B logical; immediate free80142864384->80143417344B,552960B
interval gain, counted once. Full old text retired; dated reports and hashes
remain. Keep current host145/deferred6+test1, CPU61+test37, GPU67/water14 and all
unresolved CPU/GPU/game failures, historical archive and protected raw/image sets.

CPU tree78316360B/132files (+78040), GPU19063572B/19files unchanged. EXE48958464/
PDB111083520B (+30208 combined); attachment logs298633B/186files:4054B new minus
3958B removed=96B growth. Run973529864B replaces541835B,11971B shrink. Selected
retained growth96373B, needed for current runtime/lifecycle evidence. Firstfree
80140918784 -> post-cleanup80143417344B (~74.64GiB),2498560B drive-wide gain;
other host objects/metadata/Git/system activity are separate, not cleanup credit.
No new cache/perf files found in scoped run outputs. No exception expanded.
Next host146/game974, deferred7/test2; other fixture IDs unchanged from above.
Continue visual/list producer ownership and authored image-role/pixel coverage;
do not repeat the unchanged material-only or bottom-inactive probe.

### Shared native water visual scope (2026-09-08, source3102f19 plus edits)

Same cumulative3GiB exception/floor62,509,998,080B; reserve, diagnostics, logs,
raw and image caps unchanged. First/pre-build free80,130,662,400B. No live
renderer/build/compiler/test producer found. CPU tree78,316,360B/132files;
attachment logs298,633B/186files; host EXE48,958,464B/PDB111,083,520B.
377 artifact-free Python checks pass. Reuse trees, shaders and GPU evidence.
No assets/downloads/cooking/raws/images/binary copies. Planned deferred build7/
test2 enforce32/16MiB free-drop,300/30s; host146192MiB/300s; aggregate build logs
10MiB. Run974 only after these pass:180s/800KiB/192MiB, zero captures/perf/dumps,
exact profile restoration. New observation: shared native water visual scopes
plus direct material/draw/retirement in both strict cold/reload epochs, with
neighboring legacy comparisons preserved. No unchanged bottom-inactive retry.
Keep replacement overlap until validation, then retire identified superseded
successful logs. Preserve failures and all protected image/raw evidence. Game
pixel/HDR/stereo and authored bottom/snapshot qualification remain pending.

Deferred7/PID32944 and test2/PID38400 exit0; host146/PID32812/session72587 exit0,
no guest objects/codegen writes. Run974/PID38788/session33672 terminal08:33:14,
127s, full strict chain passed,0 raws/profile restored,541432B log. BUT its
native water visual counter never advanced: the initial visual type8-only
assumption missed the executing resource. This is failed visual reachability,
not delivered callback removal. Keep974 as causal evidence (SHA256
61056EE702A3669E1B9F0C345712615CE4FB90F969C93A69064F5D96A51004B8).
Source confirms extra=1's only differing branches are fur1/11 and receiver14;
the known non-fur/non-indexed types share the same scope. Correct admission
without widening ordinary rigid model routing, add exhaustive type regression,
and record the actual admitted type mask. Retry deferred8/test3,host147/game975
uses the SAME32/16/192MiB build and180s/800KiB/192MiB run limits. Last measured
free80,129,581,056B; no live producer. No new image or raw allowance.

Corrected deferred8/PID29988 and test3/PID22200 exit0; host147/PID38732 exit0,
no guest objects or generated writes.975/PID31256/session33818 terminal08:38:22
after127s, strict mixed/cold/reload chain pass,0 raws/all22 settings/profile
byte-exact. Actual water visual type5;3109 balanced native visual scopes,3159
direct materials/submissions,3066 emissions/3157 retirements in last samples.
Fresh scopes and water output advance in both post-event epochs; old water
generation94 fully retired before208 submitted. Source/hashes/full evidence in
20260908_1236_native-water-visual.md. All producers terminal. No game pixel or
authored bottom/snapshot/stereo claim; prior GPU shader evidence reused.

Removed13 explicit superseded files after replacement: host145/146, deferred6/7,
test1/2 stdout/stderr and verified973 log.538393B logical; immediate drivefree
80128286720->80128835584B (+548864). Count once. Old full text retired; summaries/
hashes retained. Keep current147/deferred8/test3/975, causal974, other current
CPU/GPU fixtures and all unresolved failures/protected raw/image evidence.
CPU78361684B/132files (+45324); EXE48963072/PDB111116288B (+37376combined);
attachment logs297077B/186files (-1556);975529878B plus causal974541432B replace
973529864B (+541446). Selected retained growth622590B for the new connection,
runtime evidence and diagnosed reachability case. First80130662400 to cleanup
80128835584B:1826816B less free drive-wide, separate from attributed outputs.
No new scoped cache/perf files. No allowances reset/expanded. Next host148/game976,
deferred9/test4. Continue native list producer ownership and real image-role/
game-pixel/stereo coverage, not a repeat of the completed visual-admission probe.

### Native water deferred producer (2026-09-08, source681f1fd plus edits)

Same cumulative3GiB exception/floor62,509,998,080B and unchanged reserve,
diagnostic/log/raw/image limits. First measured free80,126,369,792B; no live
renderer/build/compiler/test producer. CPU tree78,361,684B/132files and attachment
logs297,077B/186files match the prior checkpoint. No downloads, assets/cooking,
raws/images or binary copies.377 artifact-free Python checks pass. Native water
staging/consumption now share the existing deferred queue; runtime reachability
and mixed-frame outgoing-state correctness are unverified, not a delivered gate.

Planned bounded CPU build62/test38 (host_post_output_test),32/16MiB free-drop,
300/30s; host148192MiB/300s. Reuse unchanged shaders and GPU fixture evidence.
Game976 only after checks pass:180s/800KiB/192MiB, strict cold/reload chain,
zero captures/perf/dumps and exact profile restoration. Its new observation is
native water producer records staged and consumed without source list entries,
with fresh emissions/retirement and neighboring legacy comparisons. Full game
pixel/stereo and authored image-role qualification remain pending. Preserve
replacement overlap and failures; retire only superseded successful outputs
after their replacements are verified. No storage allowances reset or expanded.

CPU62/PID27448 failed on Windows max macro expansion; corrected kRecordBytes,
CPU63/PID24108 and test38/PID29676 pass. Host148/PID34560/session22369 and
149/PID27236 pass, codegen0/no guest objects. Run976/PID31932/session32890
terminal09:10:25 after127s, strict cold/reload/mixed chain passed but NO water
producer records staged. This does not qualify source-list removal.531733B,
SHA2560A98E18569D4251538E61EB4837F73D3C6BFD626C4360C437342B0623357E052.
Existing water visual/material/draw path still passes: frame4559 submitted3180,
emitted3092/culled86/retired3178, zero admission refusals; type5 scopes3120 paired
at4500.0 raws/22 settings/exact116B profile restored. Host149 EXE
4AA97120396AC22ADEA1C84002C86323CE61D725511DA5987B51E80D34628068; PDB
6C2B9EDE1036797E2F180F2A076CEDBF485E212292FB48269DDDFA4F94C71471.
CPU63 BC3FBD064DA10B3536ECB777746B53922E82B9EFCA8296D7CCEFA4B940A278DE.

Host150/PID31508 pass adds bounded producer eligibility/scope diagnostics; no
new renderer behavior claimed. Material42/PID36108 and test40/PID29212 pass,
including exhaustive environment-sampler axes. Next run977 preflight stopped
at75MiB diagnostic overlap BEFORE profile writes/launch. No process existed.
Resolved by retiring11 explicit superseded successful outputs after976 replaced
975's strict/visual/material/draw purposes and CPU63/test38 replaced61/test37:
hash-verified975 plus host147/148/149 and CPU61/test37 stdout/stderr.538809B
logical; free80125444096 ->80125997056B (+552960 interval), counted once.
Old full text retired; reports/hashes remain. Keep current150,CPU63/test38,
material42/test40, failure62, causal976/974 and all other protected failures,
GPU/image/raw evidence. Next977 observes bounded eligibility inputs and can
stop as soon as that causal evidence arrives; it is not another full gate retry.

977/PID29572/session89160 observed type5, node1/instance145/generation94, one
known deferred canonical water primitive, no skin/bones/overrides, but normal
mapping requested. This missing ordered normal-table selection caused refusal.
Stopped owned renderer after collecting causal inputs, terminal09:16:07 after82s;
wrapper exit1 is incomplete gate by design, not a qualified run. Profile restored;
353801B causal log retained. No new raw/image allowance. Decoder now owns the
normal-table action including repeated-command elision, disable/Keep and phase1
selector0 behavior; source-list export remains distinct from actual binding.
Material43/PID34572+test41/PID36976 and CPU64/PID27688+test39/PID38204 pass.
Host151/PID33228/session97704 pass; no guest compilation/codegen writes. EXE
6A5BC4842555A640D5975A5AAFD9C6D8242F100A841FC56C7B2FB3B2B8A2AC73;
PDB2EDCF42D26A2A86DB57D47578EC5D101F5C4B1B41B04BC988A2248D200CFB7D1.

Lossless archival replaces protected causal974, not discarding evidence.
541432B source, bounded600KiB compression overlap; initial96KiB retained estimate
was low, actual108282B fits128KiB and the unchanged aggregate limits. Source was
preserved at the first size check. Then verified the sole ZIP payload name,
length and SHA25661056EE...004B8 before removing only the redundant original.
Retained-water-visual-974.zip SHA256
713BC52D0B39C79D9AD412DE9872E88468C373F8FDC73370655EE51299DA0666.
Net retained reduction433150B; original-removal free interval80122474496 ->
80123019264B (+544768; archive creation already consumed space). Count once.
Next978 uses the same180s/800KiB/192MiB strict capture-free gate, now testing
the corrected normal-selection producer and actual water staging/consumption.

978/PID38132/session65677 terminal09:25:16 after129s; wrapper exit0. Strict cold,
reload, movement and mixed-consumer checks pass. Actual new queue evidence is
separate from the rigid parser: ready-field1800..2100 stages/consumes1137->1437;
reload4200..4500 stages/consumes2811->3111. No source-list entry or node command
interpreter is used for these admitted water nodes. GPU1563..1863 submits901->1201,
emits885->1167, retires899->1199; reload4250..4550 submits2862->3162,
emits2793->3065, retires2860->3160. Generation94 finishes1661 retired before
generation208's first submission1662. Rigid mixed4200..4500 stages/consumes9450
additional records with9450 effect reads,5436 legacy draws and10872 material bridges.
The new water counters are intentionally separate; no strict parser was weakened.
All22 settings took effect,0 raws,116B owner profile restored exactly; SHA256
2F1BC38D763A1B7BDBA31F560684FD4AA7E42A714600B8D344F19DA7F38E23B0.
978 log538293B SHA256786501B09EC52DEBC7CB50EAC75657AA7FFCA941D5317BDC90751D747F975A94.
Causal977 log353801B SHA256D5E88698F93EE29B9C5FFE52D88BAF33F8F1C9A729640A5D062893DCAAC8E268.

The source list is removed only for admitted regular whole-node water producers.
Special/early/late-image overrides and direct/skinned families remain legacy.
The bounded opaque bridge owns outgoing source image keys plus checked native
leases, not source geometry/world/register copies; a visual key still feeds the
late material adapter. Mixed neighbors still need image/sampler/alpha/world/cull
exports. No full-frame, game-pixel, HDR, authored shore/snapshot or stereo claim.
Unchanged GPU67/water14 shader evidence is reused, not restamped game acceptance.

Commit preflight rechecked377 artifact-free guards, existing CPU64/output39 and
material43/test41 passes, and the host151 link. No additional build/run/capture.
Current CPU texture tree79049778B/132files (+688094); material tree8402235B/41files
(no initial measurement this bundle, so no attributed delta). EXE48992256B and
PDB111308800B (+221696 combined); attachment logs309706B/202files (+12629).
Current976/977/978 plus archival974 total1532109B versus prior974/9751071310B
(+460799). Selected measured retained growth1383218B, excluding unknown material
tree delta and source/Git files; preserves causal failures and the new integration.
Earlier cleanup538809B plus net archival reduction433150B reclaimed971959B once;
no additional deletion at commit preflight. Terminal runtime free80121798656B
versus first80126369792B:4571136B less free drive-wide, not all task-attributed.
All producers terminal; budgets unchanged. Preserve current151/CPU64/output39,
material43/test41/978, causal976/977 and archived974 plus other protected evidence.
Next host152/game979 only for a named new implementation or coverage observation,
not to repeat the established water eligibility gate.

### Native water frame-image connection (2026-09-08, source42fd400 plus edits)

Prior turn made progress: connected water producer committed42fd400, all producers
terminal; remote push rejected by security review pending explicit destination
approval. No retry/workaround. Same cumulative3GiB exception/floor62,509,998,080B,
20GiB reserve and diagnostic/log/raw/image limits. First free80112410624B; no
renderer/build/compiler/test processes. Existing CPU79049778B, EXE48992256/PDB111308800B,
attachment logs309706B. No assets/cooking/downloads/new trees or raw/image captures.
New native reflection publication and direct snapshot return connect completed
producer leases to the water material output, removing consumer getter readback.
378 artifact-free guards pass. CPU65/output40 will verify completion/stale/reset/
replacement/lease retention with32/16MiB free-drop and300/30s bounds; host152 uses
192MiB/300s, one incremental target. Reuse unchanged GPU programs/evidence. A new
capture-free integration will test exact primary-plane publication and consumption,
not repeat eligibility; authored snapshot/game-pixel/both-eye gates remain pending.

Before new outputs, inspected and retired11 exact superseded files: host150,
CPU63/output38, material42/test40 stdout/stderr and hash-verified976. Their passing
purposes are replaced by151/CPU64/output39/material43/test41/978;976's failed water
reachability is now resolved by the causal normal test and978's actual new route.
Retain977's causal inputs and archived974. Removed537422B logical;976 full text
is retired, its prior result/hash remain above. No protected unresolved evidence,
current binary, asset or profile removed. Count this cleanup only once.

Cleanup interval80112001024->80112549888B (+548864 drive-wide). CPU65/PID18836
and output40/PID35532 exit0 (0.47s). Host152/PID36116 and153/PID36232 failed
on missing outer namespace qualification in the two global hooks; corrected
frame counter/native image type. Host154/PID30172 exit0; codegen0/no guest objects.
979/PID24896/session53964 terminal09:43:01 after127s, strict cold/reload/mixed
chain pass,22 settings/0 raws/exact profile restoration. Native primary-plane
publications/reads advance1200/1191 at1860 to3900/3150 at4560; last water4575
submitted3166/emitted3078/retired3164, unavailable0. Fresh cold/reload water queue
windows1800..2100 and4200..4500 each stage/consume300 records. This qualifies
the connected image lease route, not the subsequently added mirror-alias guard.
979 log536971B SHA2566067493B7543798D51A3087C548C937B0E71F4EBAB52DC2C974E45B28B781348.
Host154 EXE904278E2DEDD1BAADFF1639EDE4D2E431B5A3F3B8B0D489FB396507FE18045BA;
PDB8D03832F4B53C13CF0B2D1CE76800FF000051199F414DD4E7A06132EE2497502.

Review added a strict outgoing reflection-mirror identity check: the native image
is selected only from its completed producer, but a late alias changing the
compatibility getter must refuse rather than silently diverge. Host155/PID38448
exit0. New source guard needed its expected fully qualified frame-counter spelling
updated after the compile fix; all378 guards pass, no comparison weakened.
Retired7 inspected superseded files after replacement: host151/CPU64/output39
stdout/stderr plus hash-verified978.542574B logical; interval80111038464->80111587328B
(+548864), counted once. Retain979 until the guard-qualified replacement passes.
Next980 validates that final mirror check under the same180s/800KiB/192MiB,
capture-free strict chain. No allowance reset or additional game-pixel/stereo claim.

980/PID35532/session59700 terminal09:47:28 after126s, exit0. All22 settings/0raws/
exact116B profile restoration. Strict cold/reload/mixed checks pass. Fresh reflection
1873..2173 and4273..4573 each add300 publications/reads; last3900/3171. Water cold
1581..1881 submits/retires300 and emits290; reload4259..4559 submits/retires300 and
emits279. Generation94 retires1657 before208 submits1658. Last3158 submitted,
3069 emitted,87 culled,3156 retired, unavailable0. No authored snapshot/bottom or
new game-pixel/HDR/stereo claim. Full evidence in20260908_1345_native-water-images.md.
980538768B SHA256E997247279093E05F317DB1BB9B80746DEBD2FF03F59BE24C82306EDDE51D6A7.
Host155 EXECB7C66A2A0BAA1648182FD0645BE985B495F1D75017E37E8310BFD795B18316F,
PDB11BF3C812C5B3F08ACA57B0CEE7CA1B0CF592B036329F6D9F6F3A8DE752C2E62.
CPU6540768B79E4E14A7DFAFA180266A0132BB6441EE5F8954EBA3718EB06739C2CA9.

After final replacement passed, retired7 inspected superseded files: resolved
namespace failures152/153 and pre-guard154 stdout/stderr, plus hash-verified979.
547403B logical; interval80110436352->80110993408B (+557056). Count once.
Total this bundle25 files/1627399B logical removed. CurrentCPU79082588B/132files
(+32810); EXE48994816/PDB111312896B (+6656); attachment logs302033B/192files
(-7673). Retained977/980/archived9741000851B vs1532109B (-531258). Selected
retained change-499465B; other objects/metadata/source/Git/system changes separate.
First80112410624 to ending80110993408B:1417216B less free drive-wide, not all task
attributed. No new caches/raws/images/downloads/assets; limits unchanged. Keep
155/CPU65/output40/980, material43/test41, GPU67/water14/rigid23/snapshot22,
causal977/archived974 and all other protected unresolved/raw/image evidence.
All producers terminal. Next host156/game981,CPU66/output41 only for a new
connected change or explicitly scoped missing coverage, not another planar-read probe.

### Native joint-local skin assets (2026-09-08, source f04aaa8 plus edits)

Previous turn made progress:42fd400/f04aaa8 now pushed; origin/main equals HEAD.
No live producer on resumption. Same cumulative3GiB exception/floor62,509,998,080B
and diagnostic/log/raw/image limits; no allowance reset. Last preflight free
80,071,610,368B. Earlier post-Git free-space movement was drive-wide, not claimed
as renderer growth; scoped logs/cache inspection and process check precede the
next producer. No new game output, capture, downloads, assets or build tree.
Cooker/schema changes and source-free indexed deformation use the existing mesh
fixture. Plan native_mesh_test14 (32MiB/300s) and mesh_cpu12 (16MiB/30s CTest),
within the existing supervisor's cumulative floor and10MiB aggregate logs.
Native GPU palette/program/scene/shadow admission remain unconnected; do not
restamp host155/run980 or infer native character/game-pixel acceptance.

Scoped preflight found no live renderer/compiler/test process and no new cache
or log payload after980 (its existing log has the boundary timestamp). Mesh tree
1537443B/30files; attachment logs302033B/192files. No new raw/image/game outputs.
Build14/PID37340 and CPU12/PID33928 exit0. After added source-pair/normal/bounds/
shared-disk regressions, build15/PID29008 and CPU13/PID36612 exit0 (CTest0.12s,
0.14s total). All378 artifact-free guards pass. Mesh EXE SHA256
8A87AD2B06F099748644CD7AB8CF892269D4BF4E7E4F1DAD587FFEDF442BC3E7.
No host/shader build, game run, profile edit, data cook or new capture; host155/
run980 remain unchanged. Full skin GPU/live/pixel acceptance stays open.

After validating replacement15/13, inspected and removed8 exact superseded
mesh-build13/14 and mesh-CPU11/12 stdout/stderr logs,4059B logical. Old13/11
were passing rigid/storage fixture evidence;14/12 were passing intermediate
skin evidence. Their results remain recorded; no unresolved failure, raw,
image, asset, save or active binary was deleted. Current15/13 logs retained.
Cleanup free interval80074461184->80074473472B (+12288 drive-wide), count once.
Closing selected mesh tree1653096B/30files (+115653), attachment logs302234B/
192files (+201):115854B net retained fixture/log growth for skin coverage.
Ending free80074473472B vs preflight80071610368B:2863104B more free drive-wide,
not attributed as cleanup beyond the measured interval. Source/Git/other drive
activity is separate. All producers terminal; same cumulative limits. Next
mesh build16/CPU14; host156/game981 remain unused and require a real connection.

### Native skin caster connection (2026-09-08, source319586c plus fixes)

Previous turn made progress: source connection319586c committed and pushed, with
379 Python guards passing and compilation/live verification explicitly pending.
No renderer/build/test producer is active. Preflight free80,049,041,408B; mesh
tree1653096B/30files, texture fixture79082588B/132files, GPU fixture19063572B/
19files, attachment logs302234B/192files. Reuse the same cumulative3GiB owner
exception/floor62,509,998,080B and all existing diagnostic/log/raw/image limits.
No allowance reset. Prior free-space movement is drive-wide, not attributed to
renderer outputs. Existing fixture payloads are unchanged; no new cache/game run.
Plan mesh16/CPU14 (32/16MiB), output66/CPU41 (64/16MiB), GPU68/rigid24
(96/32MiB), then incremental host156 (192MiB), with existing300s build/30s CTest
supervision and10MiB aggregate logs. No guest rebuild, new tree, download, asset
copy/cook or raw capture. Retain replacement fixtures/logs; remove identified
superseded passing logs after replacement. This tests actual native joint-local
caster vertices/palettes, animated bounds and owned queue/fence lifetime; game
submission/emission/reload/pixels remain pending until a fresh targeted run.

Mesh16/PID27316 and CPU14/PID35608 passed; output66/PID23632 and CPU41/PID38828
passed. GPU68/PID29516 failed on the older water fixture's positional initializer
after a production field addition. Replaced it with explicit fields, without
changing its shader/oracle. GPU69/PID35828 passed; rigid24/PID26808 passes all61
cases (six new1/2/3-influence single/instanced skin cases), water15/PID34340 all20.
Vulkan validation0errors/0warnings; one unrelated missing GOG overlay manifest
loader message; no raw/image files. Added weight-sum/FP32 bounds padding and
exact palette-capacity/shared-pose/prefix-split regressions. Final mesh17/PID27680,
CPU15/PID30628, output67/PID38448 and CPU42/PID37484 pass. All379 Python guards pass.
Host156/PID29368/session95103 terminal exit0; codegen0written/no guest objects.
EXE49033216B/PDB111661056B. Post-link free80,039,821,312B. Next981 is a named
skin reachability and fresh cold/reload emission/retirement check, retaining the
strict mixed-consumer regression. Wrapper explicitly sets bd_native_skin_shadow;
same180s/800KiB/192MiB bounds, exact profile restoration, zero capture/persistence.

Snapshot23/PID36544 passes8cases, validation0/0. After replacement, retired26
inspected superseded fixture logs (mesh15/13,16/14,output65/40,66/41,GPU67/68,
rigid23/water14/snapshot22 stdout/stderr):38387B logical, free80039010304->
80039067648B. First runtime preflight refused diagnostic overlap before any
profile write/launch. Losslessly archived958/959 (1013088B input) to
retained-native-runtime-958-959.zip156174B; both entry SHA256/lengths match before
plaintext removal. Logical saving856914B, free80037871616->80038727680B. Archive
SHA256EA1F252DE014B378907ABF30A355FBE89CEBD896BD14E553893E56C4140031DB;
959's distinct merging proof and958's full evidence remain recoverable. No raw
or image deletion. Each cleanup credited once; all prior failure protection stays.

981/PID24776/session84710 launched10:49:36, terminal10:51:20 after104s when the
192MiB free-drop supervisor stopped it during reload. Profile restored exactly.
No native skin-shadow emissions observed; unchanged skin replay remains active.
No runtime error detected before the stop; full reload gate not completed.
Only new game output is365353B log, SHA256899E71D4F3A0F091B2FDA634B55C62994CA968F8A92FD2B3FACCE3D61D84483D.
Post-stop free79801425920B; roughly237MB drive-wide drop is not attributed to
that log. Scoped game cache/HLSL/raw and recent NVIDIA DX/GL caches show no new
payload. Pagefile allocated2048MiB/current410MiB/peak1524MiB; no earlier pagefile
sample proves its delta. No live renderer/compiler remains. Keep981 and add
bounded skin admission evidence before another attempt; do not repeat unchanged
eligibility or label absent skin emission a pass. Same limits remain in force.

Host157/PID29124 builds. Probe982/PID29676/session92826 stops intentionally after
observing admission at10:55:32 (23s); native skin geometry exists, route remains
Legacy. Added exact refusal reasons and CPU regressions. Final output68/PID20020,
CPU43/PID38668 and host158/PID30588/session68209 pass;379 Python guards pass.
983/PID20444/session36558 stops intentionally10:59:49 after21s: ordinary
generation97 nodes47/62 refuse cutout siblings (4/6primitives); generation38
node1 (44primitives) refuses technique1/effect participation. No new skin emission,
no runtime correctness failure before diagnostic stop, and no field/reload/pixel
qualification. Exact profile restored; no new raw/image/cache payload. 98389700B
SHA2564796FBE511641E26E0D429D9ED31E4051D0794C028C592523D6DE2D30C05F7CC.
Keep981's storage-stop evidence and983's causal admission evidence. Retired9
inspected superseded logs: host156/157,output67/CPU42 stdout/stderr plus hash-
verified98271496B (its evidence replaced by983).77221B logical, free79327309824->
79327395840B. 982SHA256DC306048C9F9B1C330BB2D455C07A6589EDCAE5E3BC605A55AB8EADB17E7CE34.
Total this turn35 superseded logs115608B removed plus856914B net lossless archival
reduction=972522B logical reclaimed, counted once. Two historical plaintext logs
remain fully recoverable from the checked ZIP. No protected image/raw or asset lost.

Closing mesh1703082B/30files (+49986), texture82677563B/132files (+3594975),
GPU19836673B/19files (+773101), EXE49037824B (+43008), PDB111685632B (+372736),
attachment logs306127B/194files (+3893). Runtime981/983455053B plus archival
reduction856914B gives selected net retained growth4435838B for the new fixtures,
host code and causal evidence. Other objects/metadata/source/Git remain separate.
Ending free79327395840B vs first80049041408B:721645568B less free drive-wide,
mostly unattributed by scoped output checks; do not call it renderer output or
cleanup savings. No owned producer remains. Same cumulative limits, no reset.
Next host159/game984 only after the connected cutout or technique-participation
change; no repeat opaque-only probe. See20260908_1100_native-skin-caster-verification.md.

### Native skinned cutout connection (2026-09-08, source017ace5)

Previous turn made progress: connected cutout source/fixtures committed and
pushed,379 Python checks pass, native compilation/GPU/live gates pending.
Preflight11:18:45 EDT: free79,372,439,552B; no live renderer/compiler/test process.
Mesh1703082B/30files, texture82677563B/132files, GPU19836673B/19files,
attachment logs306127B/194files. Same cumulative3GiB exception and62509998080B
floor; diagnostic/log/raw/image limits unchanged. No new allowance or cleanup
credit. Reuse existing trees: mesh18/CPU16 (32/16MiB), output69/CPU44
(64/16MiB), GPU70/rigid25 (96/32MiB), host159 (192MiB), each supervised at300s
build/30s CTest and10MiB aggregate logs. No guest rebuild, asset copy/cook,
download or raw capture. Validate replacement before retiring superseded logs.
The changed consumer must pass actual alpha/overlap pixels and live admission;
the full desktop scene/sequence/both-eye gate remains open. A new game run will
check native skin emission, not repeat the resolved opaque-only refusal probe.

Mesh18/PID28564 and CPU16/PID25476 pass (0.15s total); output69/PID32680 and
CPU44/PID38316 pass (0.50s). GPU70/PID38292 compiles both skin shaders; rigid25/
PID38164 passes85cases (30skin), validation0/0, raw0,1.44s total. Host159/
PID27660/session29734 terminal exit0: codegen0written and no guest objects.
EXE SHA25694DFEC25B475AFE10F45DFC3AE63273D6E6F53E53A3D205D1FDCE11DF4B4D1A0.
After replacement, removed14 inspected superseded build/test logs (27181B
logical, free79370121216->79370162176B). Runtime overlap still needed headroom;
losslessly archived980/981's904121B full text to176180B ZIP, verifying both
original SHA256/lengths before plaintext removal. Saving727941B logical;
free79369519104->79370248192B. Archive retained-native-runtime-980-981.zip
SHA2562F142C222FC37A0860946E2BAA58ECAB9E06AA0F3636707D0768F3BE0E37EC72.
Both strict980 and storage-stop981 remain fully recoverable. No raw/image/asset
deletion. Next984 uses existing strict cold/reload/mixed gate,180s/800KiB/192MiB,
capture/perf/persistence off and exact profile restoration. Skin emission will
be inspected separately; old rigid counters alone cannot qualify the new path.

984/PID38324/session45914 terminal11:26:56 after129s, exit0. Full strict
cold/reload/mixed chain passed; all23 settings effective, raw0, profile restored.
Log541083B. No native skin emission; only technique1 skin admission refusals
(generation29/node1, then152/node1 after reload,44primitives). This is fresh
regression evidence, not acceptance of native skin. No repeat of the same probe:
next connection supplies the existing object-scope's texture-classified policies
to native shadow admission, animated bounds and packet construction. Whole-node
equality checks preserve alpha/cull/order and refuse late volume/unknown siblings.
No new policy/image cache or shader change; reuse GPU70/rigid25. Source provenance:
generated64 sub_82174270 phase1 direct shader24 except wind3; generated90
sub_82286228 table dispatch; existing texture routing audit20260907_0047 and
PrepareMaterialMesh's owned base-table/early-override classifier. Visual callbacks
remain outside the replaced node consumer. Added44-sibling ownership fixtures,
late unknown/volume/mismatch/forced-pass refusal and source retirement. All379
Python guards pass after updating the expected admission signature.
Plan output70/CPU45 (64/16MiB) then host160 (192MiB), existing limits and trees.

Output70/PID36920, CPU45/PID32804 and host160/PID33844/session18923 pass.
Host160 EXE006AE437469881E2C9F3F6E9C08288049E2DEA9439A1EF8D9D671E2F718E3385;
no guest objects rebuilt.383 Python checks pass, including a new fresh skin
emission/fence gate required independently in each reload epoch. No new shader:
GPU70/rigid25 evidence remains applicable, not restamped.
Losslessly archived983/984 (630783B to122511B), SHA256/length-verified before
plaintext retirement, saving508272B logical. ZIP7764BD303423FE04071A83A8DE22A1046B52075DCF3B862A3EE465BFC2AD36BB;
free79366017024->79366529024B. Keep both full causal and strict-regression texts
recoverable; no raw/image deletion. Aggregate images10303111B/11files leaves
182649B under10MiB. Next985 retains all strict reload/mixed checks plus skin
emission/fence checks; only after those pass request one110KiB maximum,
full-resolution PrintWindow JPEG. Same180s/800KiB/192MiB/no-raw bounds; no
sequence/stereo qualification implied, no storage cap raised.

Actual host160 run used reused filename978 (higher plaintext numbers had been
archived), PID37048/session44518,11:37:29..11:39:05, NOT historical water978.
First skin submission frame657=44; fresh cold1857..2157 adds13200 submitted/
emitted/fence-retired. Reload stopped at192MiB drive-free-drop before new field
or requested image; exact profile restored, no runtime correctness error logged.
Current978310025B SHA2560848E2709BA5F32D8BA15B38C4198C490FA586A18895622A0291F4F555E07067.
Only new scoped game output is this log; no game cache/HLSL/raw/asset payload.
Existing NVIDIA GLCache .bin/.toc modified (430261257B total, not known growth);
no earlier size baseline proves delta. Pagefile remains2048MiB. Other ninja/
linker briefly observed, then gone before command-line ownership inspection;
not stopped. No cap increase or unchanged runtime retry.

Final cleanup6 exact superseded output69/CPU44/host159 logs=4113B logical,
free79098605568->79098613760B. Total20 log removals31294B plus1236213B archival
savings=1267507B logical this continuation, credited once. Both archives remain
hash-verified recoverable; no raw/image removal. Mesh1714796B (+11714),
texture82806622B (+129059), GPU19900651B (+63978), EXE49047552B (+9728),
PDB111693824B (+8192), attachment logs316904B/194files (+10777). Selected
retained change233448B minus385105B runtime/archive reduction=-151657B.
Ending79098613760B vs initial79372439552B:273825792B less free drive-wide,
mostly not assigned by these scoped measurements. Source/Git/other objects
remain separate. All owned producers terminal. Keep current160/CPU70/45,
GPU70/rigid25 and mesh18/16; older water15/snapshot23 remain unchanged-program
evidence. Full host159/run984 archived; current978 retains skin emission and
reload storage-stop evidence. Next actual renderer log979 (not an assumed985),
only after storage reconciliation and for missing skin reload/pixels or a new
connected consumer. Full details20260908_1143_native-skinned-shadow-consumer.md.

### 2026-09-08 ordinary skin scene connection (source07c4623 plus work)

Prior turn made progress: native shadow emission evidence and pushed07c4623.
At11:57:02 preflight free79040143360B, no live renderer/compiler/test producer;
existing attachment logs316904B/194files. Previous game outputs remain accounted
for in the preceding entry; unresolved drive-wide activity is not attributed to
the game. No runtime retry, cache deletion, new raw/image or budget increase.
Reuse the same trees and cumulative floor62509998080B. Planned CPU output71
(32MiB free-drop), CPU46(8MiB), GPU71(64MiB)/rigid26(32MiB), host161(192MiB),
one job at a time. Expected final fixture/host growth below2MiB; overlap bounded
by the existing supervisor and10MiB aggregate logs. Preserve passing/failure
evidence until replacements pass, then remove only superseded build/test logs.
New shaders and the shared queue/pose connection need fresh CPU/GPU/host tests;
live ordinary skin scene/reload/pixels stay pending until actually exercised.

Output71/PID37940 built; CPU46/PID34868/session82342 terminated at CTest30s
after old fixture assertion (missing owned shader metadata). Corrected fixture
output72/PID32380 and CPU47/PID32320 pass,0.55s. GPU71/PID8168 compiled both
new scene shaders/shared skin headers. Rigid26/PID22224 rejected the new fixture
asset's unsorted semantic order; corrected GPU72/PID30468 and rigid27 pass154
cases,1.95s, validation0/0, no raw/images. Host161/PID32304/session30288 passed,
codegen0written/no guest objects.384 Python checks pass. Exact hashes and
coverage in20260908_1210_native-skin-scene-shading.md.
Pre-runtime12:08 free78997954560B; existing game GLCache429851081+410176B
unchanged, fixture-cache03e9307f1e996973 now803634+1856B (no initial delta claimed).
Texture fixture82900615B (+93993), GPU20052610B (+151959), EXE49078272B
(+30720), PDB111747072B (+53248). Removed16 verified superseded build/test logs
98827B logical, free78997434368->78997544960B (+110592B observed). No assets,
profiles, images, game logs or active trees removed. Current log aggregate341974B
versus initial316904B (+25070). Selected retained growth354990B excluding new
shader headers/other objects/source/Git/system activity. Same cumulative limits.

Source checkpointa83d076 pushed before live integration. One changed-consumer
run planned with the existing full strict reload/mixed chain and skin-shadow
gate; ordinary scene counters inspected separately, not inferred from shadow
counts. Explicit skin-scene cvar added to temporary profile. Same180s/800KiB/
192MiB free-drop supervisor; at most one110KiB PrintWindow JPEG only after the
full requested gate, no raw/perf/persistence. Exact owner profile restored in
finally. Prior GLCache baseline is now measured, no global cache eviction.

Initial runtime preflight rejected before profile/producer mutation: diagnostic
used79412702B plus931840B text/image reservation exceeded75MiB by1701342B
(plus retained WER metadata). No limit raised. Lossless bounded archival:

-941/945/948/956 input1821508B ->267783B, saved1553725B logical;
  free78986665984->78988144640B. ZIP retained-native-runtime-941-945-948-956.zip
  SHA25675EBF996355F93369C068E26D96B552E046E9310E0E220214861350ECA824C0C.
  Verified plaintext entry hashes941 F177A5A1AF65403CBAE585E18474BBD86997BA74614E376A12832947751B4C36;
  945 BBBB860A54C78D6E802C66D0C2B08C254BA9337BC30B1E1C73370C13DAE1CD8E;
  948 BB9A0485A1939C7EDBC405D6E927583377FFFEB4F4E76C90780F566194841D9A;
  956 8C17A3C0598796A990280E026F3EC5447C933D249AA013C95EF690132748CD5B.
-977/978 input663826B ->128749B, saved535077B logical;
  free78987997184->78988533760B. ZIP retained-native-runtime-977-978-20260908.zip
  SHA256522363052544836A2FB7BF9F744223515253F3E4F8C1D197FE4CAC69218AF2A5.
  Verified entry hashes977 D5E88698F93EE29B9C5FFE52D88BAF33F8F1C9A729640A5D062893DCAAC8E268;
  current11:37 skin978 0848E2709BA5F32D8BA15B38C4198C490FA586A18895622A0291F4F555E07067.

All archived logs fully recoverable, including unresolved visual failures.
Together with16 obsolete build logs,2187629B logical reclaimed this continuation;
do not recredit previous1267507B cleanup. No assets/saves/raw/failed images removed.

One actual live run: host161/PID22748/session6707,12:15:08..12:17:09 success.
Reused filename969, distinguish from historical969. Full strict cold/reload/mixed
chain and skin-shadow windows pass:93/144->207/385,13200 skin-shadow emissions/
fences per300frames each field epoch. Scene skin reached during both opening
events (5661 emitted/retired each), then flat post-event. Separate scene verifier
is correctly Pending;387 Python tests pass. Exact116B owner profile restored,
24 effective settings. Log570640B, JPEG105571B1920x1080 inspected; raw0/perf0/
new game cache0/HLSL0. Binary/log/image hashes and visual limits in1210 report.
Final12:18:59 free78979936256B,60207104B less than initial79040143360B drive-wide.
Selected retained change354990+570640+105571-2088802=-1057601B. Other objects,
shader headers/source/Git/system separate; game GLCache delta69389B measured,
ending429920374+410272B. Image aggregate10408682B/12files,77078B headroom.
No owned producer remains. Keep host161/output72/CPU47/GPU72/rigid27, mesh18/16;
current969 and image are new integration/visual-limit evidence. Historical
host159/run984 is no longer the latest full strict run. No new raw approval.

### 2026-09-08 Toon surface consumer (after d7cae75)

Previous continuation made progress: native skin scene consumption/reload evidence
and pushed d7cae75. At12:37:12 preflight free78770884608B, no renderer/compiler/test
producer remains. Earlier unrelated ninja20700 was observed12:34 and is now gone;
it was not stopped. Same cumulative limits/floor62509998080B. No guest/codegen
inputs, assets, caches, profiles or raw/image outputs changed. Initial retained
fixture sizes: texture82900615B/132files, GPU20052610B/19files; prior host161 and
runtime969/image remain the live baseline. Drive-wide use since prior ending is
not assigned to this source-only work.

This bundle connects explicit owned Toon surface inputs to native rigid/skin
plans, shared instanced GPU shading and fences, without a second renderer or
register-file API. The live technique1 producer remains closed until its exact
light/ambient adjustments, texture multipliers and ignore-alpha/lattice contract
are owned; no guessed white/default values or inherited shader rows. Planned
output73/CPU48 (32/8MiB free-drop), GPU73/rigid28 (64/32MiB), host162 (192MiB),
one producer at a time, existing300s build/30s CTest/10MiB log supervisor.
Expected selected retained growth below2MiB; actual overlap enforced by wrappers.
Keep passing fixtures until replacement passes, then retire their obsolete logs.
No live game run or capture until the missing producer is actually connected.

Actual output73/PID25660 and CPU48/PID28192 pass (0.56s test); GPU73/PID35736
rejected HLSL struct conditional, GPU74/PID35852 rejected fixture Windows max
macro. Both corrected, unchanged eligibility/tolerances. GPU75/PID35196/session93552
passed, binary written12:41:44; rigid28/PID24556 completed12:42:07 with345 cases,
191 new Toon/mixed cases,5.72s, validation0/0. Host162/PID37484/session19043
passed, codegen0written/no guest objects.388 Python checks pass. All handles
terminal, no runtime/profile/image/raw producer. Full identities/limits in
20260908_1237_native-toon-surface.md; live technique1 still unconnected.

Removed16 exact superseded attachment logs: output72/CPU47/GPU71/GPU72/
rigid27/GPU73/GPU74/host161 stdout+stderr. Their replacement passes and the two
resolved compile failures are recorded above. Logical66495B reclaimed; measured
free78760628224->78760706048B (+77824B). No game logs, assets, profiles, images,
active trees or unresolved failures removed; do not recredit prior cleanup.
Final12:46:21 free78760706048B,10178560B less than12:37 preflight drive-wide.
Selected retained texture82988826B (+88211), GPU20177662B (+125052),
EXE49085952B (+7680), PDB111755264B (+8192), attachment logs408056B/194files
(+66082 after cleanup): net+295217B. Retain current CPU73/48/GPU75/28/host162
for new surface/ABI evidence; other objects/shader headers/source/Git/driver/
system activity separate. No raw/image growth or budget reset. Prior host161
runtime969 remains the live evidence, not restamped as a run ofhost162.

### 2026-09-08 authored Toon object connection (after f1808f1)

Previous goal turn made progress: implemented/tested shared Toon GPU consumer,
now committed/pushed f1808f1. Source tracing identifies the actual visual begin
sub_82174648, not the generic descriptor hypothesis: scene/object remaps and an
explicit disabled identity branch. Node setup/texture commands own ordered tints;
ordinary texture classification excludes volume-only ignore-alpha. This bundle
connects those inputs, Toon specular, whole-node admission and animated pre-cull
to the existing packet/indirect/fence path. Fur/outline/lattice and deferred Toon
remain unconverted; no new shader, asset format, cook, tool or parameter cache.

13:04:01 preflight free78730932224B, no compiler/game/test producer observed.
Same cumulative3GiB exception/floor62509998080B and all capture/log limits.
Retained texture82988826B, GPU20177662B, host EXE49085952B/PDB111755264B,
attachment logs408056B. Prior drive-wide change is not attributed to source edits.
Plan output74/CPU49, host163, one producer at a time; wrapper300s/CTest30s,
32/8/192MiB free-drop respectively,10MiB cumulative logs. Selected retained
growth estimate below2MiB; host link overlap bounded by supervisor. GPU shader
unchanged, reuse345-case evidence. Preserve prior passing logs until replacements
pass, then remove superseded logs. Any live run needs a separate profile/output
preflight under this same ledger; no raw/image producer approved by this entry.

Output74/PID33980 linked unsuccessfully: the new packet behavior test reached
ComposeNativeMaterialAsset, absent from this fixture target. Added its actual
production implementation/dependency, not a stub. Output75/PID37720 and CPU49/
PID38256 pass0.53s. Host163/PID30828/session97878 passes, codegen0written and no
guest objects. Further review corrected omitted specular RGB ownership: a prior
node may have authored it, so it must not become guessed black; object-disabled
power alone has the visual-begin zero contract. Final-source rebuild output76/
PID29240 was stopped by32MiB free-drop guard after object compilation, before
link completion. All these producers are terminal; no game launched/profile write.

13:10..13:11 storage investigation: current free78489706496B; fixture85137350B,
host49090560B/111763456B,29 recently replaced host objects34530370B total (not
delta). No compiler/game remains or recent top-level compiler TEMP payloads;
pagefile allocated2048MiB/current437MiB, with no prior size baseline. Drive-wide
growth cannot all be assigned to these files or specifically to pagefile. No
unrelated process/cache was stopped/deleted. The wrapper now prints its actual
producer-start free/floor and checks final free too; historical preflight is not
necessarily its launch baseline. Resume completed CPU object with64MiB overlap,
then CPU50/host164 under8/192MiB; still the same cumulative limit/floor, not a new
exception. CPU target now retains two real material objects; expected selected
growth revised below4MiB. No repeat of already compiled GPU shaders.

Final output77/PID28576 passes: launch/end free78485852160B,64MiB guard.
CPU50/PID31228 passes0.54s/CTest0.56s; launch/end free78485684224B.
Host164/PID21712/session92675 passes, no guest objects/codegen0written;
actual launch free78484983808B, end78481334272B (3649536B drop).
All producers terminal. Owner requested commit/push before the next game run;
no new live/profile/image work. Final388 artifact-free Python checks pass.
See20260908_1318_native-toon-producer.md for exact source/evidence/pending gates.

13:14 cleanup removed14 exact superseded attachment logs: output73/74/75,
CPU48/49,host162/163 stdout+stderr, after replacement passes. Logical13234B;
measured free78481440768->78481469440B (+28672B). Retain output76 storage failure,
current77/50/164, prior GPU75/rigid28 and all game/pixel/failure evidence.
No assets, profiles, active trees or unrelated outputs removed; no old credit.
13:18 free78473715712B,257216512B less than13:04 drive-wide. Selected retained
texture85137417B (+2148591), EXE49090560B (+4608), PDB111763456B (+8192),
attachment406877B/196files (-1179): net+2160212B for real material-linked fixture
and current evidence. Other host objects/source/Git/system activity separate;
no raw/image growth or budget reset. Live post-event scene/reload/pixels pending.

### 2026-09-08 native Toon live connection gate (after 3ba183e)

Previous turn committed the connected source/fixtures/docs locally; push remains
blocked by the safety reviewer pending exact owner approval. Local verification
is independent of that external-write restriction.13:22:35 preflight free
78464479232B; no renderer/compiler/test process observed. Host164 EXE SHA256
81A5E42635335A036A9FBE1942392C37DE173248C3539D015229F846AAD63B36 matches;
owner profile116B SHA2562F1BC38D763A1B7BDBA31F560684FD4AA7E42A714600B8D344F19DA7F38E23B0.
Images10408682B/12 leave77078B; new native_skin_scene_window.jpg absent.
Plan one180s desktop cold/reload run with full strict mixed chain and separate
fresh --skin-scene in both post-event epochs. No raw/cook/perf producer; log800KiB,
one full-resolution JPEG<=70KiB,192MiB free-drop and existing75MiB diagnostics
stop/full-overlap preflight. Same cumulative exception/floor, not a reset.
Retain failure/current baseline until replacement proves its distinct purpose;
profile restoration and owned-process shutdown are guaranteed by the supervisor.

Run970/PID37808/session85183,13:23:21..13:26:04, terminal1 only at JPEG size
gate after the complete strict cold/reload/mixed/skin-scene text chain passes.
Old93/instance144 retires before207/479. Cold1257..1557 skin scene+13200 emitted/
retired; reloaded3832..4132+13198, with13200 submissions and13200 shadow emissions/
fences. JPEG exceeds70KiB even at bounded encoder qualities; no file written.
No comparison failure, no raw output; original116B profile restored exact hash.
Log537389B SHA256B1A768118963620826B382AB987CA4FEF4411E5B5F064F65C61BDE5755888DA6;
ending free78458388480B. New live scene consumption established, pixels pending.

13:28 lossless existing-PNG repack, <=4MiB temporary overlap: native_toon_material_window.png
3333941->3258478B,75463B logical savings, measured free78426898432->78426972160B
(73728B). Exact filtered data, all non-IDAT chunks and1920x1080 RGBA bytes match;
decoded SHA2567b04541231a41a0aa0c9f7d80582769ebff2fa8ce56a1510f706a3d66883fc1f.
New file SHA256e44a2c0821eadc08aa947cd5836b45a01bac37296b0d375c2198b309206d457a;
historical report retains old encoded hash. No evidence/pixels removed or altered.
Image total10333219B,152541B headroom. No previous cleanup credited again.
Plan one follow-up same180s strict chain for the missing image, now normal110KiB
JPEG within unchanged10MiB aggregate; no quality/scene comparison threshold
changes. This is an output-capacity correction, not a renderer fix or blind
retry of a comparison failure. Existing800KiB log/192MiB growth/75MiB diagnostics
limits and profile restore remain; preserve970 until replacement proves live
text and image outcomes. No rebuild, raw/perf/cook or new tool download.

Run971/PID37324/session17745 terminal0,13:29:05..13:31:38. Actual preflight
diagnostics77185187B +819200B log +112640B image fits75MiB; free78423748608B,
floor78222422016B. Full strict cold/reload/mixed/skin scene+shadow chain passes;
cold1428..1728 andreload3964..4264 scene+13200 emitted/fence-retired each.
Old93/144 retires before207/480. Exact owner116B profile restored; no raw output.
Ending free78418616320B. Log537422B SHA25603605F5ED86CA97C3665CCACDB0906C118C3A78C0C5038436CEFB2DF47F61428.
Window1920x1080 JPEG101438B SHA2564A19BA8DDEE6B913F536C599B5686FAB0E80F5B05AD7B742DA995A660DB32881
shows title-logo/village, not logged interactive field: visual acceptance FAIL,
reproduces962 discrepancy, no cause/fix claimed. Preserve image; no further
unchanged PrintWindow retry. Existing fence-owned probe needs its distinct
per-image approval/aggregate-overlap preflight; source work need not stop.

Completed checksum-verified archive of969/970:1108029B plaintext ->222514B
retained-native-runtime-969-970-20260908-skin.zip. Both exact entry hashes match
recorded originals before individually resolved/revalidated plaintext deletion.
Archive SHA25696EB3EB08C905CCACB513363CCA0AD62BFB49C6128CFF3164CA38252EC730FA9.
Logical885515B saved; actual start78415351808B, pre-delete78415060992B,
end78416175104B (+823296B across operation). All evidence recoverable; current971
plaintext and all images retained. Repack+archive total960978 logicalB /897024B
measured gain, once. Selected log/archive/image net+215271B for reached native
scene and visual failure. Image10434657B/13 leaves51103B.13:34:23 free78415912960B,
48566272B below13:22:35 drive-wide; other activity separate. No new cache/hlsl
files observed. Source/host/GPU binaries unchanged, no raw growth/budget reset.
See20260908_1334_native-toon-live.md. Push approval still pending; no retry/bypass.

### 2026-09-08 render-pose integration after source checkpoint11c193a

Previous turn made progress: committed timed pose ownership and its native
culling/scene/shadow connection;388 artifact-free Python checks pass. The new
C++ behavior source and host were not built. Push again rejected by security
review; exact owner approval remains pending, no retry on this continuation.
This bundle verifies the existing owner through actual rigid/skin bounds,
scene/shadow plans, shared palette and delayed lifetime, without new shaders,
assets or another owner. Original animation/conditional source copy remain.

14:20 preflight free72185679872B; material8402235B/41files,
texture85137417B/134files, attachment logs406877B/196files, host
EXE49090560B/PDB111763456B unchanged. Transient compiler32152/ninja31824
were observed, then both IDs were confirmed absent before launch; no owned
producer was resumed or duplicated. Read-only pagefile inspection shows2048MiB
allocated, unchanged from the prior checkpoint. No recent host object writes
observed, and selected fixture/log sizes do not explain the drive-wide decline
since13:34.14:23 free71245791232B; do not attribute unrelated drive activity to
source or claim reclaimed bytes. No cleanup or producer output yet.

Plan existing material44/CPU42 and output78/CPU51, one producer at a time,
64MiB per-build and8MiB per-test free-drop guards; then host165 with192MiB
link-overlap guard only if current storage fits. Actual launch free/floor printed
by existing300s supervisor;10MiB aggregate logs and original cumulative3GiB
exception/floor62509998080B unchanged. Selected retained growth estimate below
4MiB. Preserve prior pass/failure evidence until replacement passes, then
remove exact superseded build/test logs. No raw/image/profile/game producer
authorized by this preflight; any live validation needs its own bounded check.

Material44/PID29428/session1133 passes17 host fixture edges, no guest work;
actual launch71454695424B/end71454560256B. CPU42/PID37928 passes native_material_data
0.15s (CTest0.18s), including all new timing/lifetime/budget behavior.
Output78/PID32904 stops at the64MiB free-drop guard within5s, before emitting
an object or compiler diagnostic. The supervisor terminates its exact child
tree; subsequent process inspection finds no compiler. Its stdout/stderr and
temporary object are0B; retain the stop condition here, not a claimed code failure.
14:26 free70317436928B;14:27 free69872472064B. Selected fixture growth is only
143771B (material now8546006B), texture remains85137417B; new logs3559B.
Read-only current write-rate inspection shows no large active writer at that
instant; it does not explain prior allocation or authorize stopping other apps.
Owner was asked asynchronously about concurrent downloads/builds.14:28:37 and
14:29:13 free68136939520/68136763392B show the rapid decline has paused. Retry
output79 only after a fresh stable-space/process preflight, with the SAME64MiB
guard and cumulative floor, not a budget increase or duplicate producer.

14:29:53 free68136730624B confirms stability. Output79/PID35532 passes two
edges, actual launch68199460864B/end68199329792B. CPU51/PID17556/session62003
fails the new fixture's model publication assertion, then CTest terminates at
30s. Cause is the fixture's omitted per-range shadow-policy vector; production
ModelMaterialRegistry correctly refuses mismatched vectors. Added explicit
Receive policy, with no change to production admission. Owner requests no new
PrintWindow retry remain in force; no renderer was launched.

The connected source audit also found native water still consuming raw poses.
Extend its existing deferred record with the same registry-owned render lease:
sort, native water inputs and wave bounds use it; outgoing legacy world exports
keep the raw completed pose. No new source index or copied matrix cache.
Add real queue/retirement and identity tests to the same post-output fixture.
388 Python checks pass. Rebuild output80/CPU52 under unchanged64/8MiB guards;
14:34 free68192808960B, no rapid decline or owned producer remaining.

Output80/PID28908 passes, launch68192673792B/end68192649216B. CPU52/PID36244
passes0.63s (CTest0.65s), launch68192387072B/end68192276480B. Host165/PID36932/
session41506 passes, codegen0written and no guest objects; actual launch
68191940608B/end68191191040B.390 artifact-free Python checks pass. All producers
terminal. No game/profile/image/raw/GPU-shader run; unchanged shader evidence
is not new unlocked-FPS motion or pixel evidence. Exact binaries and remaining
interfaces are recorded in20260908_1440_native-render-poses.md.

14:39 pre-cleanup free68190777344B,3994902528B below14:20 drive-wide. Selected
material8546006B (+143771), texture85276304B (+138887), host49102848B (+12288),
PDB111812608B (+49152), logs417805B (+10928). Removed14 exact superseded logs:
material43/CPU41, output77/CPU50, output79/CPU51 and host164 stdout/stderr,
after44/42,80/52 and165 pass. Also removed the identified0B interrupted output78
object. The fixture-policy failure is explained above and covered by52; retain
the storage-stop logs78/76 and all game/pixel/unresolved-failure evidence.
Logical8308B reclaimed, measured free68188495872->68188516352B (+20480B).
Logs now409497B/198; texture134files, same bytes. Selected net retained growth
346718B for current fixture/host binaries and verification, not duplicated trees.
Other object/source/Git/system activity is separate; no old cleanup recredited,
no game data/profile/assets removed, no raw/image growth or budget reset.

### 2026-09-08 host165 live render-pose gate after b45dbc0

Previous turn made progress: the strict scenario gate and regression tests were
committed as b45dbc0;394 artifact-free Python checks pass. Push was rejected
again by security review. The exact payload/destination approval question is
unanswered; this continuation does not retry or bypass the restriction.
Local renderer validation can proceed independently. No C++ or shader change
since host165; no rebuild or restamp is needed for the Python/operator changes.

14:54:49 free68242821120B; no renderer/compiler/test producer observed. Host165
EXE SHA25609ADF4E2269BB08D2A09F739BBD77585FFE23415865ADDE5A05727FAAF19C791
and owner116B profile SHA2562F1BC38D763A1B7BDBA31F560684FD4AA7E42A714600B8D344F19DA7F38E23B0
match.14:55:58 free68240539648B. Existing diagnostics76839714B plus full819200B
log reservation =77658914B, below the unchanged75MiB stop. Build logs409497B,
recent logs839934B, archives2411452B, recent cache18252B. No new images/raws.

Plan ONE180s text-only normal mono/MSAA run of host165, temporary flat-TOML
bd_fps_limit=60. Reuse the existing operator with RenderPoseVerify and the full
strict cold/title/reload/mixed/skin scene+shadow chain. Fresh ready-field blends
and shared reads must independently advance in both epochs; startup-only/snap-
only samples, counter resets and refusals cannot pass. This is native consumer
evidence, not motion/pixel parity, achieved60FPS or speedup. Existing800KiB log,
192MiB free-drop, cumulative floor62509998080B and guaranteed exact profile
restore/owned-process shutdown remain. PowerShell parser accepts the operator.
Retain the new log for this distinct timing gate and all existing pixel failure
evidence; assess eligible superseded text after the result. No asset cook,
perf/raw capture, PrintWindow retry, new tree or diagnostic budget reset.

Run972/PID35388/session74082 terminal1 at14:58:08: the192MiB free-drop guard
stops during the opening event, before ready-field/reload evidence. Actual
launch14:57:06 free68240273408B, floor68038946816B. Log169284B SHA256
CD5335186604FFD08933EBA344FF5082A08AC69EDB3BE52AF0C5DEDA79765FA1; all25
temporary settings took effect. Startup poses read/reuse with0refused and
0blends, not an active-field interpolation failure or pass. No recent cache,
hlsl,perf/raw files; exact116B owner profile hash restored. Renderer absent.

14:58:36/14:59:07/14:59:36 free68002377728/68002316288/68001746944B now stable
within0.61MiB for a minute after the larger decline. Pagefile allocation remains
2048MiB; instantaneous process write rates show only1856B/s for Codex, not an
explanation for prior volume allocation. No other process stopped. Retain972
as the storage-stopped attempt. One retry is now justified by stable-space
checks, not weakened acceptance or a larger allowance: same180s/800KiB/192MiB
guards, same cumulative floor and aggregate. Diagnostics77008998B plus819200B
reserved next log =77828198B fits75MiB. No rebuild, image or raw producer.

Run973/PID36304/session23957 terminal0 at15:02:42; launch15:00:19 free68001742848B,
floor67800416256B. Full strict cold/title/reload/mixed/skin scene+shadow chain
AND fresh --render-poses pass in both epochs. Cold1870..2170 poses add52361
reads,1287blends,37961reuses,13113snaps. Reload4187..4487 adds51929reads,
951blends,37529reuses,13449snaps. Refusals0 throughout. Skin scene1809..2109
emits/fence-retires13198;4135..4435 adds13200. Skin shadows add13200 in each
matching window. Model93/instance144 fully retires before207/419. These are
repeated consumers, not unique assets; merged instances0, no speedup claim.
All25 temporary settings took effect; no raw/perf/cache/hlsl files produced.
Exact original116B profile restored, renderer terminal. Ending free67998818304B.
Log550521B SHA256CB6B3511FC826F1A215F8BB624A943C5FAA1FE29EE4D9A0EB3E884A29F09B23D.
No new pixels: preserve the971/962 discrepancy and existing image gate.

15:03:26 free67994984448B; images unchanged10434657B/13. Plan checksum-verified
archive of971+972 (706706B combined) under the existing2MiB analysis-overlap
bound. Retain all bytes of971's visual-failure log and972's storage stop in one
lossless ZIP; remove only their exact plaintext files after entry length/hash
validation. Keep current973 plaintext, all images and every other artifact.
No renderer active and no new diagnostic allowance; actual savings recorded
after the operation, not inferred from this plan.

Archive completed:144117B SHA256D5EF7766701C04BFA7BD0A90970D02E5CDA2064400E2E0A8C748B889D7C60848.
Both original entry hashes match; only the exact971/972 plaintext files were
removed. All content remains recoverable; no images/raw/game/profile/assets
removed. Logical562589B saved. Actual start67559735296B, pre-delete67554144256B,
end67554627584B: deletion-phase gain483328B, but whole-operation net free decline
5107712B amid concurrent allocation. Do not claim a net drive cleanup gain.
New973 plus archive minus old971 yields selected retained growth157216B;
all earlier savings count only once. Runtime diagnostics now76996930B.

Drive-wide free falls further to66042277888B at15:05:31,65629523968B at15:06:23,
and65378684928B at15:06:57 (2864136192B below this turn's14:54:49 sample).
Read-only inspection identifies a separate Gradle/Ninja build: PID30924,
parent34104, started15:05:52, linking rpcsx-android under Documents/ps3-thor.
It is NOT this workspace's producer and was not stopped or altered. This proves
concurrent build activity, not attribution of the entire earlier drive decline.
Pagefile allocation remains2048MiB. Our host165 binary was reused unchanged,
no reblue renderer remains, and our two small text logs account for none of the
GiB-scale decline. Further output-producing reblue work needs fresh stable-space
checks; no new job is queued by this handoff. Source/Git edits are separate.
See20260908_1505_native-render-pose-live.md for exact live gates and limitations.

### 2026-09-08 15:32 opt-in native skeleton source checkpoint

The owner's commit/push request checkpoints the connected ordinary-skinned
hierarchy work; it does not resume a game/capture matrix. Load-owned model joints,
checked update-time channels and native whole-pose evaluation now feed the
existing instance update owner. The original completed/late-writer handoff remains.
`bd_native_skeleton` defaults false. Host integration compilation, original/live
comparison, animated pixels and unsupported camera-facing/sparse/secondary/
unskinned routes remain pending; host165/run973 is NOT skeleton evidence.

394 artifact-free Python guards pass. Existing material fixture build45/PID35812
passes six incremental edges, then CPU43/PID31520 passes native_material_data in
0.13s (CTest0.14s). No guest build, renderer, shader, profile change or capture.
Fixture executable713216B SHA256
F345D6B1D2FDC877885B1637492BDAF916C0015D389B76471E04228E0ADA3E92.
Fixtures cover radians/quaternion order, parent-scale compensation, root siblings,
channel overrides, rejection, source destruction, model/pose lifetime and budget.

Same cumulative ledger/floor62509998080B and aggregate limits; no reset.
15:30:26 free63963115520B, stable against the prior15:22 sample; no observed
compiler/game producer. Reused material tree with64MiB build/8MiB test free-drop
guards,300s supervisor/30s CTest timeout and10MiB aggregate log cap. Build45 free
63962955776->63962796032B; CPU43 free63962566656->63962497024B. Both terminal0.
Fixture tree8729343B (+183337B). New logs2437B; after both pass, removed only four
superseded material44/CPU42 stdout/stderr logs (3559B), preserving current45/43,
all host/runtime evidence and unresolved failures. Old textual logs are no longer
retained; equivalent fixture checks are rerunnable and old outcomes remain above.
Cleanup measured63962214400->63962222592B (+8192B); aggregate logs408375B/198.
Selected retained growth182215B for updated fixture artifacts and current logs.
Ending measured free63962222592B (~59.57GiB), net decline892928B from15:30:26;
drive-wide activity is not attributed to selected files. Source/Git bytes are
separate. No assets, saves, profiles, build trees, images or raw evidence removed.

### 2026-09-08 native skeleton host integration after17f5c33

Previous turn is progress: committed source connection and passing C++ fixtures.
Push approval remains unanswered; no retry.15:34:04 free63957352448B,15:37:04
63954649088B, no observed compiler/game process; no rapid drive-wide decline.
Plan existing material46/CPU44 (64/8MiB guards), then host166 (192MiB guard)
for grouped ABI/source-boundary tests and the opt-in producer. Same300s bounded
wrapper,10MiB logs and cumulative floor62509998080B; no guest rebuild/new tree.
Keep current45/43 evidence until replacement passes. Host/PDB peak estimate
under192MiB including link overlap; shaders unchanged, no GPU fixture needed.
No game or pixels queued yet; a later runtime preflight must separately prove
full log overlap fits and preserve exact profile bytes. New raw/image allowance0.

Material46/PID29280 passes, launch63949291520B/end63949275136B. CPU44/PID31804
passes0.14s (CTest0.16s), launch63949271040B/end63949266944B. Host166/PID28036/
session87870 terminal0, launch63949262848B/end63948251136B; codegen0written,
no guest object compilation.400 artifact-free Python checks pass. Host EXE
49138688B SHA25620666DA6D6F939A10EBE0E68F53D218D3CBC777C8C5F0A0EBEA70DA111AB8BF9;
PDB112095232B SHA25648E9D305FBC26874BC1368591A70DBAE0CC9B94DF5E2D12C377ACE30CAAF32BD.
Material EXE723456B SHA256138295188EEE037458683475B095512A35D9E3D634E21123072B9A5201F4EBB7.
Native root ABI now has a production decoder exercised by the existing fixture;
noncommuting pre/selected/post rotations and source-free ownership guards pass.
Bounded unsupported reasons and first drift component explain an unsuccessful
admission without weakening original comparison.400 checks include a new strict
--skeleton gate for each independent ready-field epoch; zero/startup-only native
production, missing comparisons, failed publication and drift cannot pass.

Plan one180s normal mono/MSAA/native-post run of host166, temporary60FPS cap,
bd_native_skeleton=true plus original material comparison and the full existing
render-pose/skin scene+shadow/reload chain. This tests reachable ordinary native
hierarchy production, not motion pixels or full animation ownership. Reuse the
operator with new SkeletonVerify guard; parser passes. Exact116B owner profile
hash remains2F1BC38D763A1B7BDBA31F560684FD4AA7E42A714600B8D344F19DA7F38E23B0.
Keep800KiB log/192MiB free-drop, full75MiB diagnostic overlap preflight and exact
profile restoration/owned-process shutdown. No image/raw/perf/cache allowance,
no PrintWindow retry or changing normal defaults. Retain all prior failures.

Run974/PID36188/session23424 terminal1 at15:45:17, started15:42:49. Preflight
diagnostics76995532B plus819200B log reservation fits75MiB; free63945359360B,
floor63744032768B. Original116B profile restored exactly. Log551337B SHA256
CA9C48D1A60FBFBB982582B3655B78078B55B9DF9EF0695E57FDFF8B15C677AB retained as
failure evidence. Native evaluation/publication/comparison reaches36331 with
zero unavailable/drift and both interactive fields, but the complete gate FAILS
ordinary deferred scope/input conservation; this is not full integration PASS.
Exact cause: frame600 reports ordinary consumed/effect reads0 but begins/ends1
and input batch/visual/refresh1; separate water receipts show one water packet
and one water scope. Those water events incorrectly incremented ordinary counters.
Fix partitions ordinary scope receipts and deduplicated ordinary input identities
from water while preserving the SAME all-family publication and rendering path.
Existing strict verifier unchanged; add water-only/mixed/duplicate queue fixtures
and a causal scenario regression before retry. Preserve974 and old visual failures.

Before974, removed six superseded material45/CPU43/host165 logs only after
46/44/166 passed:6233B logical, measured63945875456->63945883648B (+8192B).
All prior results remain recorded; fixtures/builds rerunnable, old text no longer
retained. Logs408099B/198. No other outputs removed; do not recredit earlier cleanup.
Next reuse output81/CPU53 under64/8MiB guards, then host167 under192MiB guard;
same floor/budgets, no guest/shader build or new tree. No second game until the
causal fixture passes and runtime preflight includes retained974 plus new log.

Output81/PID32120 passes, launch63942201344B/end63941668864B. CPU53/PID34572
passes0.62s (CTest0.64s), launch63939268608B/end63939264512B. Host167/PID26384
terminal0, launch63939264512B/end63939235840B; only the affected consumer object
and host link, codegen0written, no guest objects/shader changes.402 artifact-free
checks pass; strict deferred verifier unchanged. New regression reproduces974's
water-only contamination as a failure and accepts zero ordinary receipts while
separate water counters advance. All-family input publication stays intact.
Retry once with the same full SkeletonVerify chain and unchanged800KiB/180s/
192MiB/75MiB-overlap limits. Retain974 failure plus current host166 build evidence;
new preflight must include all retained bytes, not treat a retry as a reset.

Run975/PID35800/session69324 terminal1 at15:55:29 after the180s guard, not a
successful early operator stop. An attempted sandbox Stop-Process was denied;
the later approved identity check found the renderer already absent. Supervisor
finally restored the exact116B profile. Launch15:52:26 diagnostics77547774B plus
819200B log reservation fits75MiB, free63936864256B/floor63735537664B. Log531187B
SHA2564E7BFEE8952482F056934299C9C75E2928109D2708D5CE62C16AAAC9BB603517 retained.
No skeleton drift/unavailability. Corrected cold ordinary scope/input counters
pass their unchanged gate, but full reload remains FAIL: readiness window starts
at selected scene755/shadow756; a260432us poll gap resets walking at1598/1599,
only843 of900 new emissions. It restarts at1617/1618, then natural stage exit
interrupts at1860/1861. Source93 retires and stage changes tobg01_01 without the
required title request/return. No full PASS or threshold relaxation; no unchanged
retry. This is a distinct readiness/scenario failure, not a skeleton mismatch.

After81/53 passed, removed four exact superseded output80/CPU52 logs:1526B
logical, measured63937064960->63937069056B (+4096B). Retain current fixtures,
host166 failure-build evidence, host167 and every runtime/pixel failure.
Plan checksum-verified lossless archive of973+974 (1101858B combined) under the
existing2MiB analysis-overlap guard; keep975 plaintext. Delete only those two
original text logs after exact ZIP entry length/SHA validation. All original
bytes remain recoverable, no pixels/raws removed and no new diagnostic budget.

Archive completed:225232B SHA256
38FD30CF1FD4088BD75930E0B51D2926B7D14FBEE108ECB522EC26DC3CC8B144. Both original
entry hashes match; exact973/974 plaintext removed, all content recoverable.
Logical876626B saved, actual start63933194240B/pre-delete63932968960B/end
63934074880B: operation net reclaim880640B (deletion phase1105920B, not additive).
Including the two distinct build-log cleanups this turn:884385B logical removed,
892928B measured reclaimed; no earlier cleanup counted again.

15:59:13 free63931260928B (~59.54GiB),26091520B below15:34:04 drive-wide.
Selected retained growth1086956B: material+27058B, texture fixture+529787B,
EXE+36864B, PDB+286720B, build logs+629B and retained runtime logs/archive+205898B.
These are changed current artifacts, not duplicated trees; other object/source/
Git/system activity is separate. Runtime diagnostics77202335B, build logs409004B/
200, images unchanged10434657B/13. No raw/perf/new cache files from either run.
Exact owner profile hash verified again; no observed renderer/compiler/test
producer. No next run queued. Keep975 timing/field-exit failure, archived974
accounting failure and all prior visual evidence. Research/active queue/README
record scoped progress and pending full reload/motion/both-eye/Quest gates.

### 2026-09-08 keyed animation ownership after2e7745f

Previous implementation goal turn is progress (native skeleton integration);
the intervening push-only request leaves all eight commits local because the
reviewer rejected the external upload. Its specific approval is still pending;
automatic goal continuation is not that approval. No push retry in this bundle.
Source work traces skeletal clip loading/sampling, not D2 UI curves. Plan an
owned keyed-clip importer/sampler feeding existing channels, skeleton evaluator
and instance ownership in the existing CPU fixture. Runtime load/slot binding,
layer blending and sampler replacement remain pending, not a full conversion.

16:12:15 free63913181184B; no observed game/compiler/test producer. Existing
material tree8756401B/41 and aggregate build logs409004B/200. Same cumulative
floor62509998080B, 3GiB owner exception, 100MiB diagnostics/10MiB build logs;
runtime diagnostics77202335B and images10434657B/13 unchanged. New raw/image0.
Reuse material47/CPU45 under64/8MiB free-drop and300s guards. Estimate <8MiB
incremental fixture peak (one new test translation unit, link overlap) and
<16KiB logs; keep46/44 until replacement passes. No host build, game, new tree,
cache, asset file or capture is needed for this source/CPU-only prerequisite.
Two scalar constants were decoded in memory from the owned XEX; no decrypted
image/session key was written or printed. Builds run only after source checks.

Material47/PID28364 terminal0:16:19:44 free63906963456->63906693120B;
onlyanimation/instances fixture objects and link (CMake regenerated this same
tree after adding the test source). CPU45/PID35028 terminal0 at16:20:28,
free63889944576->63889940480B,0.13s test/0.15s CTest. Initial CPU behavior passed.
Independent source-unit angular checks were added before any runtime wiring:
material48/PID35884 terminal0,free63886479360->63886458880B;
CPU46/PID37604 terminal failure8 (supervisor tool exits1),free63886393344->
63886389248B,0.04s test. Cause: early float-radians conversion reverses some
exact half-turn arcs. Preserve48/46 text; native turn fractions fix the cause,
not the comparator. All retries share the same64/8MiB guards and cumulative cap.

Material49/PID35528 terminal0 at16:23:26,free63886266368B unchanged;
CPU47/PID26460 terminal0 at16:23:49,free63889575936->63889571840B,
0.12s test/0.14s CTest.384 independent half-turn/adjacent spans and the complete
existing material/instance suite pass.403 artifact-free Python checks pass in
0.246s; git diff whitespace check passes. EXE785408B SHA256
FA8102E7F2BDF31E8A240A249E83DF37BA923D4C277FDAC050C6D6A4CC5932CF.
Material tree9043673B/43, +287272B vs16:12:15. No host/shader/guest build or game
run: host167's binary and preserved full-reload/pixel failures are unchanged.

Removed exactly8 superseded logs (material46/CPU44 and material47/CPU45 stdout/
stderr), only after49/47 passed:4485B logical. Current49/47 and causal48/46
failure logs stay. Those deleted logs are reproducible from tests; prior result
summaries remain. No raw, image, game data, profile, build tree or failure
evidence was deleted. Volume free changed63886540800->63886024704B during this
small cleanup (-516096B), so no positive measured net reclaim is claimed amid
other drive activity. Do not credit earlier cleanup again. Remaining build
logs410557B/204, +1553B vs this turn's snapshot; selected retained output growth
288825B (~282KiB) is the current fixture and its bounded current/failure logs.
Source/Git/other disk activity is separate. Profile SHA remains
2F1BC38D763A1B7BDBA31F560684FD4AA7E42A714600B8D344F19DA7F38E23B0;
all six owned build/test handles are terminal, no next producer queued.

16:28:06 closing pre-commit free63883071488B (~59.50GiB),30109696B below the
16:12:15 snapshot (~28.72MiB drive-wide use). This is not all attributable to
the288825B selected retained output growth. No measured positive cleanup credit,
no extra capture allowance and no pending live producer. README/active queue
and the clip contract record the CPU-only scope and pending runtime producer.

### 2026-09-08 loaded animation sampling after566af49

The prior implementation turn made progress (owned clip CPU prerequisite); the
intervening push request remained blocked by external-upload review. No push
retry is authorized by automatic continuation. Continue the connected motion
loader/retirement -> owned named tracks -> whole channel production -> existing
native skeleton/instance consumer. Preserve original slot clocks/layer blending,
collision/effect side effects and all pending desktop/pixel acceptance gates.

16:39:03 free63877292032B; no observed renderer/compiler/test producer. Material
tree9043673B/43, build logs410557B/204, exact116B owner profile unchanged (SHA256
2F1BC38D763A1B7BDBA31F560684FD4AA7E42A714600B8D344F19DA7F38E23B0).
Same cumulative62509998080B stop floor,3GiB exception,100MiB diagnostics,
10MiB build logs; runtime77202335B and images10434657B/13. New raw/image0.
Reuse material50/CPU48, then host168 for connected runtime hooks. Estimate
<16MiB incremental CPU fixture peak and <192MiB host relink/changed objects;
supervise under64/8/192MiB free-drop and300s guards. Budget <32KiB build logs.
Keep49/47 current and48/46 causal failure until replacement passes; no new tree,
download, shader/guest rebuild, cooked asset, cache or game capture is planned.

Material50/PID37032 terminal0,16:46:27 free63870423040->63871205376B;
CPU48/PID36916 terminal0,16:46:53 free63871139840->63871135744B,0.14s test/
0.15s CTest.405 Python source/scenario checks pass0.262s. Host168/PID35812,
session11075 terminal0,16:47:25 free63870873600->63868436480B. Existing CMake
regenerated for the new bridge translation unit; codegen reports0 written,
0 unchanged,0 deleted,1 module up-to-date. No guest objects or shaders compiled.
EXE SHA33745F1F9E348F8AEB2F4A1B2A914F44A0BCF683E85561469F5C032E7D7C7E19,
PDB SHA2CCEB56D805777264823D0C88EFFBEF26E4CDD59D2AA32625918FDE7E8B82B93.
Material EXE SHAD41793F2D60A9CF0DC528F162D39B9C5320CAC071EC4AB1C2424DE087787F3E5.
16:49:09 free63868043264B, material tree9249678B/43, build logs417201B/210.

Next one capture-free admission observation on changed host168: determine
whether actual loaded type2 motions reach whole-channel replacement and original
comparison, or which source family/contract prevents admission. This is not an
unchanged975 reload retry and cannot qualify field/reload/motion pixels. Reuse
the existing profile-safe operator with an explicit AnimationProbe (60s,400KiB
text,192MiB free-drop,75MiB aggregate diagnostic stop, raw/image/perf0). Probe
stops distinctly as diagnostic-only after a matching sampled/check receipt;
all original scenario thresholds remain unchanged. Exact profile restoration
and owned-process shutdown are in finally. Full integration remains pending.

Host168/run976/PID27188,session50191 is terminal at16:50:22. The diagnostic
stopped deliberately (tool exit1) after its first matching admission receipt,
not a scenario PASS: frame385,69 loaded,992 refused,69 resident/6201072B,
sampled1 whole0 preserved1 unavailable0 checked1 wrong0. The first four sampled
load-refusal messages identify type3; this does not classify all992 refusals.
Source compressed motion import remains implementation work. No fresh field,
reload, nonconstant animation, GPU draw/pixel or live retirement is qualified.
All11 temporary settings took effect. Exact owner profile hash restored; no
observed renderer/compiler/test producer at16:51:33. Log39663B SHA256
9BFDE6156EAE47CE67BEA8B67E60B61FB25AFD6A6803C545D3C4E54B29C8EE99 retained as
initial new-path evidence. Runtime aggregate77250195B, no new raw/image/perf.

Because the shared model owner gained authored animation bindings, rebuild the
existing output82/CPU54 consumer fixture too (64/8MiB free-drop guards, estimate
<16MiB incremental peak, same300s/log caps). Current texture tree85806091B/134;
keep81/53 until replacement passes, preserving all earlier unresolved evidence.

Output82/PID36504 terminal0,16:52:18 free63863996416->63863926784B;
CPU54/PID34544 terminal0,16:53:04 free63863685120->63863681024B,0.53s test/
0.55s CTest. Current output EXE1848832B SHA256
B0C024EB9B0E2388B0E19E0E8988BD75ED0EA7FB15BF2BABB09DCCBB6573A65F;
texture tree85868113B/134 (+62022B). Material tree9249678B/43 (+206005B),
EXE837120B. Host EXE49198080B (+58368B), PDB112742400B (+643072B).

After replacements passed, removed8 exact superseded text logs: material49/
CPU47 and output81/CPU53 stdout/stderr. Their logical3839B was reproducible;
prior results remain documented. Verified literal paths/non-reparse ancestors
before deletion. Free63862177792->63862185984B,8192B measured net reclaimed in
this scoped removal; no earlier cleanup credited. Preserve48/46 causal failure,
current50/48,82/54,host168/run976,975 and all prior unresolved visual evidence.
No source/build tree/game data/profile/capture was deleted.

Closing read-only accounting: runtime diagnostics77248032B, build logs415038B/
206, images unchanged10434657B/13. Selected retained growth1013611B (~0.97MiB):
changed material/output fixtures, host EXE/PDB, build logs+4481B and run976's
39663B text. This is not a new tree; other objects/source/Git/system activity is
separate. All five build/test handles and run976 are terminal, no next producer
queued, exact profile hash restored. The refreshed guest-source/devloop skills
kept work at completed load/retirement boundaries and CPU-first verification;
the only new game probe has explicitly limited admission evidence.

16:59 closing pre-commit free63862149120B (~59.48GiB),15142912B below the
16:39:03 snapshot (~14.44MiB drive-wide use), not all attributable to selected
outputs. Full renderer goal remains active/incomplete; no Quest or new capture
allowance. README and active queue distinguish connected source, passing CPU/
build evidence, initial live admission and pending full desktop acceptance.

### 2026-09-08 compressed animation afterc087272

Previous goal turn is progress: owned keyed load/runtime connection. Push remains
blocked; automatic continuation is not specific external-upload approval. Extend
the existing clip asset/sampler with native cubic Hermite channels from type3
motion data, not a new per-tick decoder or resampled disk representation.
17:03:47 free63747432448B. Read detailed disk policy and reused current operator/
ledger. Scoped outputs unchanged: material9249678B/43, logs415038B/206, newest
runtime976/39663B; no observed game/compiler/test producer. The drive-wide
105381888B decline since prior post-commit free63852814336B is not explained by
these unchanged outputs; keep enforced cumulative/per-job floors, no cleanup
credit or budget reset. Existing runtime77248032B, images10434657B/13, raw0 new.

Plan material51/CPU49 (64/8MiB free-drop,300s, estimate<8MiB peak,<16KiB logs),
then connected host169 (192MiB free-drop,300s). Keep current50/48 until replacement
passes; keep48/46 half-turn failure. No shader/guest rebuild, new tree, asset,
capture, perf/cache or download. A few decoder constants were read in memory
from the owned XEX; no decrypted file, image or session key was written/printed.

Commit-request checkpoint: material51/CPU49 passed, then material52/CPU50 passed
with asymmetric tangent fixtures (CPU0.12s, CTest0.14s). Host169 completed with
exit0: animation bridge/version consumers and host link only; codegen0 writes,
no guest objects or shader rebuild. Existing405 artifact-free guards passed
before the final fixture-only tangent adjustment. No game run or capture was
started for this checkpoint; run976 is still only the earlier type2 admission.
The new cubic counter is observational, not live qualification. Next probe must
require repeated bound cubic sampling and retain the strict original comparison.

Recovered type3 contract: sub_82199178/82199240/821994F8 relocate three scalar
count/pointer pairs per nonconstant channel; scalar keys are8B signed-frame,
compact-value and float-tangent records. sub_823B8758/823B8608 and
sub_8272EAF8/EB38/EB40/EC00/EDC8/EED0 establish timebase, endpoint hold, near-key
tolerance, compact-float expansion and Hermite evaluation. Adjacent angular keys
use a short linear arc; longer segments preserve authored tangents without global
unwrapping. The importer retains owned native seconds/turns/derivatives, not guest
decoder scratch. Strict live clock/interior-key precision remains unverified.
Normal defaults, weighted/subtree/layer fallbacks and previous failure evidence
are unchanged. Latest bounded host-build free-space receipt63756955648B; no
cleanup/reclaimed-byte claim. This user-requested Git checkpoint starts no new
build, runtime probe, capture or cleanup.

### 2026-09-08 repeated cubic runtime observation after af0020d

Previous goal work made progress (owned compressed curves, passing CPU/build);
Git checkpoint af0020d is local. External push was explicitly rejected again;
this automatic continuation is not upload approval. Read guest-source/devloop,
disk policy, current queue and loaded-motion evidence. Reuse host169 without a
build/restamp: EXE49207296B SHA256
`66D30017BAB86A69E59902BC044233EE82D1E2CADC23A0B6EE9625B83069BF26`;
PDB112828416B. Material52 EXE930816B SHA256
`FB86D9B3353F3745256E539CD25EDF0C98D3E803014B751E58159765B7D23F1A`.
17:23:38 free63755239424B; no observed game/build producer; owner profile hash
still2F1BC38D763A1B7BDBA31F560684FD4AA7E42A714600B8D344F19DA7F38E23B0.

New observation: at least256 actually bound cubic sampling calls with the same
strict original comparison, not another type2 first call. Extend the existing
ignored operator with an explicit cubic-probe switch; other scenario gates cannot
satisfy it. This answers whether native time/tangent conversion survives real
motion clocks, deciding what causal boundary regression is needed before further
layer ownership. Not a full field/reload/skin/pixel qualification. Reuse60s,
400KiB log,192MiB free-drop and cumulative floor62509998080B; aggregate75MiB
operator stop/100MiB diagnostics. Estimated new retained log<400KiB, runtime
peak<192MiB; raw/perf/cache/images0 new, exact profile restoration in finally.
Retire only superseded material50/CPU48 and material51/CPU49 text logs after
verified52/50 pass; preserve48/46 failure, host169 and run976 admission evidence.

Run977/PID29776 terminal17:26:15 (60s probe, exit1), exact profile restored.
Log217191B retained as residency-failure evidence. At frame1291:124 resident,
8388240B (368B below8MiB), loads124/refused946, sampled/checked/cubic0,
unavailable8670. This contradicts usable selected-motion residency, not cubic
parity. Preserve the failure; no unchanged retry. Actual cleanup above removed
8 exact superseded text files/4786B logical; free63754514432 ->63754526720,
12288B observed reclaimed. Regenerable logs only; no raw/asset/profile deletion.

Changed design: same residency index registers completed type2/3 source lifetime,
then actual selected visual slots prepare native immutable curves before sampler
entry. Source-backed dormant catalog is an explicit temporary streaming boundary,
not full native asset cooking. Reuse existing loader retirement and shared leases;
charge catalog, active payloads and retired pins within8MiB/4096entries. Dormant
unleased payloads may evict after two unused frames; failed imports retry only
when budget improves or the source generation changes. No sample-time key decode,
new model/pose owner or per-tick failed decode loop. Read full slot-update body
file89:2624 and selection writer file86:2636: slot*56+visual1920 ->ready entry+12.
Frame-interpolation hook TOML read; no new hook site/codegen changes.

Plan causal CPU material53/CPU51 then host170, existing300s/64MiB/8MiB/192MiB
guards. CPU fixture fills a dormant catalog before selecting a working set,
exercises pressure, recent-use/alias pins, retirement/reuse and source destruction.
Estimate fixture<8MiB peak,<16KiB logs; host<192MiB peak. Same cumulative budgets.
No new runtime retry until this changed ownership boundary passes CPU/build.

Material53/PID28916 and CPU51/PID29108 terminal0 (0.12s/CTest0.13s). All406
artifact-free source/scenario checks pass0.227s. Host170/PID31072 terminal0,
codegen0 writes/no guest objects or shaders; animation bridge plus version
consumers/link. Host end free63757455360B. Superseded52/50 text logs may now
retire; keep host169/run977 residency failure. Next one identical bounded cubic
probe on the changed activation-residency code, not a retry of host169.

Run978/PID35428 terminal17:41:59, exit1 at60s: repeated cubic observation NOT
reached. Frame1931 sampled/checked12989,wrong0,whole27,preserved12962,cubic1;
loads14,registered1070,catalog1067,resident9/581952B,unavailable462,
prepare-refused2 (unclassified). Reported resident peak942672B. No drift, but
one cubic call is not advancing/interior-key qualification. This changes the
next action to weighted/subtree/layer ownership and classifying the two refused
selected clips, not another unchanged boot. Full field/reload/pixel gates stay
pending. Owner profile hash restored exactly; both runtime sessions terminal.

Current host170 EXE49209856B/PDB112852992B, material53 EXE949760B,
tree9561657B/43. Hashes and complete scoped evidence in
`research/20260908_1745_selected-motion-residency.md`. No new raw/cache/dump/perf
files since17:23:38 (scoped timestamp inventory); no images generated. Current
and failure logs977/978 retained217191+218900B. After replacement53/51 PASS,
removed52/50 four text files2163B; free63757455360 ->63757459456,4096B reclaim.
After978 replaced976's first-call admission, verified976 SHA and host168's
successful build/benign GLOB regeneration warnings, then removed those three
superseded text logs43684B; free63755874304 ->63755919360,45056B reclaim.
Total this turn15 files/50633B logical,61440B observed reclaimed, not double-
credited earlier removals. Regenerable diagnostic text only; reports/hashes and
all unresolved failure evidence remain. No game data/assets/raws deleted.

Pre-commit ending free63755919360B (59.38GiB), net+679936B from63755239424.
Drive-wide activity is distinct from attributed retained change: runtime logs
+396428B after976 retirement, build logs421892 ->415613B (-6279B), selected
EXE/PDBs+46080B. Combined enumerated retained change+436229B, excluding other
objects, source, Git and system activity. Runtime operator accounting now
77645035B by last preflight plus978 minus retired976/host168 logs; build logs
415613B/208, images unchanged10434657B/13, raw0 new. Same100MiB diagnostics,
75MiB operator stop,10MiB build logs and cumulative floor; no budget reset.
README/active queue updated once for the connected result. All producers are
terminal, no queued run, and full renderer goal remains active/incomplete.

### 2026-09-08 weighted/subtree animation connection after 9c41128

Previous implementation made progress; owner-requested checkpoint9c41128 saved
unfinished source. Push of13 pending commits was denied before execution; this
automatic continuation supplies no new upload authorization. Full goal stays
active. No observed game/compiler/ninja/cmake producer; starting free63750860800B,
4,706,304B below prior post-commit receipt63755567104B (source/Git/system activity,
no new build/run/capture). Existing same cumulative floor62509998080B and caps.

Bundle: owned authored rest channels plus weighted keyed/cubic application and
hierarchy subtree masks, connected through current model/clip/instance owners.
Broader bdAnimationUpdate scope prepares the six physical selected slots before
samplers; original clocks, buffer-layer mixing, collision/effects remain. Reuse
the completed source audit of8228A3E8,8228A090,T/R/S helpers and824931B8;
strict original output comparison remains the live acceptance criterion. No new
guest hook sites, generated edits, shaders, assets or native owner framework.

Plan material54/CPU52, shared-header consumer output83/CPU55, all-boundaries and
host171 only after CPU passes. Existing bounded wrappers: fixture64MiB/free-drop,
CPU8MiB, host192MiB;300s timeout,10MiB aggregate build logs. Estimate each fixture
<16MiB peak and<16KiB logs; host<192MiB peak. Reuse existing outputs without
copies; new raw/perf/cache/images0. After replacement PASS retire only prior
material53/CPU51 and output82/CPU54 diagnostic text; preserve angular48/46,
host169/run977, host170/run978 and all unresolved scenario/pixel failures.
No runtime launch yet; any new probe needs explicit changed-code observation,
remaining75MiB operator headroom and exact owner-profile restoration.

Material54/PID32096 and output83/PID26556 terminal0; CPU52/PID37496 PASS
0.11s/CTest0.13s, CPU55/PID37044 PASS0.51s/CTest0.53s;407 artifact-free checks
PASS0.233s. Host171/PID27460/session78866 terminal0: codegen0writes, no guest
objects/shaders, shared model/skeleton consumers and animation bridge rebuilt.
Host end free63747538944B. After successful replacement, removed8 superseded
material53/CPU51 and output82/CPU54 text logs3839B logical; measured free
63747538944 ->63747547136B,8192B recovered. Regenerable text only; no game data,
raws or unresolved failures removed. Host EXE SHA256
8BC48CB126D0745E4FDDBE14C6A910745D75D4E133DE58298EBDECD02B1BF611.

Next one changed-code weighted/subtree probe, not a cubic retry: require at least
256 weighted sampled/comparison calls and one subtree call, all compared with
the unchanged1e-4 active-channel tolerance and exact flags/inactive bytes.
This tests whether actual slot weights and partial roots reach native application
and whether quaternion/rest/mask parity holds; mismatch changes the specific
contract to fix. Separate prior256-cubic gate stays open even if this probe
succeeds. Existing operator gains explicit WeightedAnimationProbe switch;
60s/400KiB log/192MiB free-drop, same75MiB aggregate stop and cumulative floor.
Reserve full400KiB before launch; raw/perf/cache/images0, exact profile restored
in finally. Owner profile hash verified unchanged. Retain979 as current weighted
observation;977/978 remain failure evidence until their respective gates pass.

Run979/PID35112/session2389 terminal18:16:43 exit1: requested256 weighted plus
positive subtree observation NOT reached. Last frame2352 sampled/checked13822,
wrong0,weighted113,subtree0,cubic380; whole402/preserved13420,unavailable1 model,
prepare-refused2 unclassified. Thus weighted comparisons reached113 and cubic
call-count threshold256 was observed, but no partial-subtree or scoped advancing
interior-key/pixel qualification. Log260831B SHA256
A8228C1CAB59B3605461FF36F9619837683CD6FB46B42E74B154727A11E86DB0;
exact owner profile restored. Free63747280896B at18:17:03.

Extend same bundle to whole-buffer layer composition, the remaining connected
sampler consumer. Full sub_82284BE0 read(file39:9943..10491), both callers in
bdAnimationUpdate read(file53:3196..3385). T/R lone channels copy, scale fades
to/from unit, inactive defaults0/identity/1, flags union and left hash. Crucial
in-place contract: output union flags publish before input flags reload; caller
uses output==left for third/subsequent layers. Keep that ordering only in the
outgoing adapter; native channel mixing is source-independent. Partial-overlap
arrays refuse before execution. Same strict original compare, no epsilon or
parity tolerance changes. CPU tests enumerate448 activation/weight combinations,
in-place left/right/both, poisoned inactive bytes, rollback and actual hierarchy.

Plan material55/CPU53 and host172 after tests, existing bounds/no guest/shaders.
Shared skeleton header unchanged since171: output83/CPU55 stays valid. One
further bounded live probe only for newly connected layer mixing (>=256 matching
mixes) plus the unchanged>=256 cubic count criterion. Weighted/subtree gate from
979 stays failed; no changed threshold. Same400KiB text/60s/192MiB limits and
75MiB aggregate preflight; no new raw/perf/cache/images. After55/53 PASS retire
only54/52 text; preserve host171/979 until its failed subtree gate is replaced.

Material55/PID36384,CPU53/PID24040 PASS0.11s/CTest0.13s;408 artifact-free
checks PASS0.226s. Host172/PID29060 terminal0 at18:23:11, only animation bridge
and link, codegen0writes/no guest/shaders. Free63750340608B; independent drive
activity gained space since979, not attributed to cleanup. Retired54/52's four
superseded text logs2623B after replacement PASS: free63750340608->63750348800,
8192B observed reclaim. This turn total12 files6462B logical/16384B observed
cleanup, not recrediting prior turns. Runner parsed successfully after adding
explicit MixAnimationProbe; run980 remains gated by full400KiB overlap preflight.

Run980/PID22724/session76931 terminal18:24:54,exit1: requested>=256 matching
mixes plus>=256 cubic calls NOT reached. Last frame2355:13858 sampled/checked,
wrong0,23 mixed/checked,113 weighted,0 subtree,382 cubic,whole405/preserved13453.
Only one unavailable model; two selected-clip preparation failures remain
unclassified. Reported resident peak970672B, final11/682432B. Thus actual mixer
and weighted comparisons pass for reached calls, but desired coverage does not;
no full field/reload/advancing interior-key/pixel qualification. Preserve979/980
and choose an authored layered/subtree scenario before another probe. Exact
profile restored; no live producer/session or queued runtime remains.

Current host172 EXE49228288B/PDB112939008B; material55 EXE1017344B,
tree9765246B/43; shared output83 EXE1848832B, tree85872813B/134. Hashes and
source contracts in20260908_1824_native-animation-layers.md. Scoped timestamp
inventory since18:00 finds0 new capture/perf/cache/dump files. Images/raw budgets
unchanged; no new image allowance. Both runtime logs remain260831/263220B.

The old978 purpose (>=256 repeated cubic calls with matching output) is now
observed at380/382 in979/980, despite their distinct weighted/subtree/mix gate
failures. After validating978's preserved hash and host170's successful build,
retired978 log and host170's two superseded build logs:3 files221428B logical,
free63748227072->63748452352,225280B observed reclaim. This removes obsolete
text evidence only; prior counts/hashes/reports remain. It does not claim
advancing cubic motion or relabel either new probe as PASS. Preserve977's
starvation diagnosis,979/980's insufficient scenario coverage and prior reload/
pixel failures. No game data/assets/saves/profiles/raws deleted. All deleted
files this turn are regenerable diagnostics,15 files227890B logical and241664B
observed reclaim, excluding previously credited cleanup.

Pre-commit end free63748452352B (59.37GiB), net-2408448B from63750860800B.
Enumerated retained growth620124B: runtime+305151,build logs+2236,material tree
+203589,output tree+4700,host EXE/PDB+104448. Source/Git/other objects/system
activity are outside that attributed subtotal. Build logs417849B/210; runtime
operator accounting77952422B, same75MiB stop/100MiB diagnostics and cumulative
floor62509998080B. README/active queue updated once for connected172 scope.
Local source/test/docs commit is authorized; external push remains denied from
the preceding user turn, with no new payload/destination approval supplied.

### 2026-09-08 named animation inputs and importer contracts after52a9510

Previous goal turn progress: connected weighted/layer math, tests and scoped
live comparisons; full goal remains active. Clean main ahead14 on entry; push
restriction unchanged, no upload retry. Main read guest-source/devloop/disk
policy, current queue/evidence and relevant model ownership frontier. No live
renderer/build producer found; first free63746904064B. New bundle owns bounded
inline joint names and direct-search versus weighted-prune name/exclusion masks,
corrects ignored constant timestamps and bounded scale-tail import eligibility,
and adds bounded import failure provenance to classify the two live refusals.
Same model/clip/instance owners and8MiB clip budget; strict comparisons retained.

Source audit: weighted traversal8228A090(file23:10240) fully reread, direct
bdAnimKeyframeSample routing/control flow(file89:9299..10452), dispatchers
82289888(file50:9867) and8228A3E8(file43:9917), scale82288B38(file101:10014),
and controller exclusion writer(file53:2841..2950). Inline node names are16
bytes at+64; controller clamps exclusion pointers at0x82DBEC70 to30. Included
name wins over exclusions; direct calls ignore exclusions. Direct traversal
writes headers/searches children even without a matching parent; weighted
traversal prunes that branch and writes neither header nor channels. Depth0 is
the admitted outer boundary; nonzero depth/Euler modes remain explicit refusals.
Constant helpers ignore key timestamp; scale holds final key after its bounded
scan, unlike T/R's terminal bracket requirement. Do not assume these explain
the live refusals until new provenance identifies the actual failing input.

409 artifact-free checks PASS0.232s. Before builds,18:43:46 free63509061632B:
237842432B unexplained drive growth since turn entry. Scoped out inventory since
18:31 finds0 modified output files/0B; no project producer was launched. CIM
process query denied under sandbox; ordinary process inspection is available.
Treat unrelated/unattributed drive use against the same cumulative floor rather
than resetting/ignoring it; no user data cleanup. The current reserve still fits
bounded verification, and all supervisors enforce live free-space limits.

Plan material56/CPU54 and shared output84/CPU56, then host173 after CPU success.
Reuse trees/wrapper:64MiB fixture/8MiB CPU/192MiB host free-drop,300s timeout,
10MiB aggregate build logs and floor62509998080B. Expected each fixture<16MiB
peak/<16KiB logs, host<192MiB peak; no guest object/shader rebuild, raw/perf/
cache/images0 new. Retire only prior55/53 and83/55 text after replacements pass;
retain host171/979 andhost172/980 unresolved scenario coverage. No runtime
probe yet; any new run needs new import-stage observation, full400KiB overlap
preflight, same75MiB aggregate stop and exact profile restoration.

Material56/PID29700 PASS;CPU54/PID24184 PASS0.12s/CTest0.13s. Output84/
PID34072 PASS;CPU56/PID33332 PASS0.48s/CTest0.51s. Host173/PID30060/
session5331 terminal0; codegen0writes/no guest objects/shaders, shared-name
representation consumers and bridge rebuilt. End build free63507972096B.

New observation for981: after>=4000 strictly matching samples, inspect all
bounded prepare-refused trace lines (maximum4) and current refusal count. This
classifies validation stage/track/channel/key plus first unreadable word for
the selected inputs and checks expanded importer admission. It is not another
attempt at979/980's absent subtree or insufficient mix coverage. Added explicit
ImportAnimationProbe to the existing ignored operator; cannot combine it with
weighted/mix/cubic observation requests.60s maximum,400KiB text/192MiB free-drop,
same75MiB stop and exact owner-profile restoration. No new raw/perf/cache/images;
operator syntax parsed and profile hash verified unchanged before launch.

Run981/PID27356/session75290 terminal18:50:16,exit1 diagnostic-observed stop,
not scene/motion qualification. At frame1138 sampled/checked4002,wrong0;
weighted104,mixed/checked8,cubic6,filtered/subtree0. Both refused preparations
are the SAME source25A973FC, budgets7890912 and8116992, stage model-bindings,
no missing word. Asset importer constructs dense indices itself, so this stage's
failure is duplicate authored descriptor hashes, not exhausted residency or
unreadable data. Earlier "two clips" interpretation is unsupported: two attempts
at one source. Constant-time/scale-tail fixes are independently valid source
contracts, but do not explain this duplicate-name refusal.

New connected correction: with unique admitted MODEL hashes, the direct cursor
and weighted full scan both choose each name's first descriptor. Canonicalize
the loaded asset to first-match tracks; skip shadowed duplicate curve payloads,
retain unique dense native track indices and the same exact residency budget.
Duplicate model names still refuse; do not guess that separate binding contract.
CPU regression tests unreadable shadowed data, source destruction, exact budget,
243 descriptor-name sequences x6 model traversal orders against an independent
original-cursor reference. Log at most4 successful duplicate canonicalizations,
so the next observation can prove actual formerly refused input import.
Plan material57/CPU55 then host174 (private animation headers/bridge only;
shared output84 remains valid), existing limits. New981 failure evidence stays
until this changed first-match contract is verified; no unchanged retry.

Cleanup already completed after56/54 and84/56 PASS:55/53 and83/55's eight
superseded text files3839B; free63507972096->63507980288,8192B observed reclaim.
No asset/profile/raw deletion. Runtime981 log remains current provenance;
full hash/accounting follows after its terminal output is inspected.

User-requested Git checkpoint,2026-09-08 19:01: material57/PID34768 had already
completed0 before interruption. Ran only the existing fixture:CPU55/PID37340
terminal0 at18:59:40, test0.12s/CTest0.14s;409 artifact-free guards PASS0.234s.
No new build/game/capture launched. Host174 and changed-importer live probe
remain pending; host173 is NOT the first-match-canonicalization binary.
Current material57 EXE1125376B, SHA256
0CC7013D2BAE86D3A270E0B9B62FE0329D439C66A9344805D054D20071BD000B.
Run981 log116936B, SHA256
E88359852BA237E6C51ECCEFC34778D4AED124F44738332CE1DBB4FEF497A8A1;
preserve its causal refusal evidence until the new importer is verified live.
Owner profile hash independently matches the original
2F1BC38D763A1B7BDBA31F560684FD4AA7E42A714600B8D344F19DA7F38E23B0.
No renderer/compiler/build/test producer was active on checkpoint entry.

CPU55 wrote2027B stdout/0B stderr; supervisor enforced8MiB free-drop, the same
cumulative floor and10MiB log cap. After checking replacements57/55, removed
only56/54's four superseded text logs2623B; exact literal paths validated,
no recursive deletion. Free63506051072->63506059264,8192B observed reclaimed.
Earlier8192B cleanup is separate, not recredited. These reproducible logs are
deleted, not recoverable from Git; no game data/assets/profiles/failures removed.
Build/test log aggregate after cleanup421834B/212. Checkpoint-entry free
63506685952B, post-cleanup63506059264B (59.14GiB): net626688B drive-wide growth,
not all attributable to this task: CPU stdout is2027B; source/Git/CTest/system
activity is separate from that measured file subtotal. No raw/image producer.
README/queue now distinguish implemented filters, CPU-verified canonicalization
and outstanding live/desktop gates. Stage only this source/test/docs bundle.

### 2026-09-08 first-match host verification after717b499

Previous turn progress: committed the connected names/importer bundle with
material57/CPU55 and409 passing guards. Goal remains active, not complete or
blocked. New entry main ahead15, no active renderer/build/test producer, free
63504236544B. Explicit external push was denied again; automatic continuation
does not answer the payload/destination approval question, so no upload retry.
Main read devloop/guest-source/disk policy and the active queue/current ledger.
Next host174 reuses the configured tree, j4,192MiB free-drop,300s timeout,
10MiB cumulative build logs and floor62509998080B. Expected private bridge
incremental rebuild, no guest objects/shaders; retain small build receipt.
After build success, one changed-code import probe should establish positive
canonicalization of the formerly refused input plus>=4000 matching samples;
not absent-filter/subtree/motion/pixel qualification. Existing operator must
reserve the full400KiB text overlap against75MiB,60s runtime/192MiB free-drop,
zero new raw/perf/cache/images and exact owner-profile restoration. Preserve
981 until this causal replacement is verified; broader979/980 failures remain.

Host174/PID27340/session12681 completed0: codegen0writes, no guest objects or
shaders; bridge and version consumers rebuilt. EXE49235968B, SHA256
8F1AFCBE9BEC892BC3154296381BB20A77C79E9DFFE8E3328FAFFC784A13D0DC;
PDB113000448B. Build end free63503761408B. Runtime preflight78075871B
diagnostics plus409600B reserved log, free63469105152B: intervening34656256B
drive growth not explained by the small build/log deltas. Scoped current build
inventory finds existing build metadata,10 host objects,EXE/PDB/version header,
restored profile and one149357B runtime log; no new raw/perf/cache/dump files.
No unknown cleanup or budget reset; all further producers enforce the samefloor.

Run982/PID29656/session75098 terminal19:06:00 exit1 diagnostic-observed stop,
not full scene/motion/pixel qualification. Positive canonicalization:122 source
descriptors ->121 unique tracks,93424 retained asset bytes. Source address is
25B5EF7C this run, not a persistent identity. Last frame1433:7590 sampled/checked,
wrong0,whole8/preserved7582,weighted113,mixed/checked8,cubic6,filtered/subtree0.
All1070 registrations admitted;16 resident clips1290176B;prepare-refused0,
one unavailable model. This verifies the duplicate-descriptor import correction
and sustained matching channels, not those unobserved scopes. Profile restored
and independently matches2F1BC38D... prior full hash. Run982 log149357B,SHA256
AAFCE6EB640ECAF3076FC93AC8524B1F39415F8B47646F03B4B5F4004FAB49EE.
After replacement verified, retired981 provenance text and host173's two build
logs, retaining their hashes/source diagnosis here and982 as current evidence.

Next connected bundle uses the SAME clip/assets/layers/skeleton/instance owners
for indexed T/R and T/R/S formats. No source keys/name scan in sampling; native
asset carries joint-index binding and channel participation, not packed records.
Source82288C00(file60:9706),82288DC0(file24:10102),82289EF0(file95:10198),
82289FA8(file88:9743) read completely. Direct dense dispatch resets flags/writes
headers; weighted dense dispatch preserves headers and ignores keyed name/
exclusion/forced-subtree rules. Type0 leaves scale bytes/flags untouched.
Both dense dispatchers leave sampler globals unchanged; inherited compressed
mode and nonzero Euler mode remain explicit unsupported runtime contracts.
Header initializer82288680(file66:9955) read completely; keyed loader fixup
82198DE0/98F30/98FF8 uses36-byte records and is NOT proof of live dense content.
No dense loads occurred in982; real dense loader/content coverage stays open.
Reuse prior quaternion/channel math evidence, no new shader or visual claim.
CPU fixtures cover source destruction,129 advancing samples in each format,
reordered identities, ignored filters/scale/header invariants, exact budgets,
bounded failure and retired assets feeding completed native instance poses.
Plan material58/CPU56 thenhost175 after focused/all-boundary checks. Existing
64MiB fixture/8MiB CPU/192MiB host free-drop,300s/10MiB log/floor62509998080B
limits; estimated<16MiB fixture and<192MiB host peak, small text receipts.
No new live run until a useful indexed-content observation can be specified.

Material58/PID37628 build0, CPU56/PID28036 PASS0.12s/CTest0.13s;410 guards
PASS0.221s. Host175/PID31752 build0, private bridge+link only/codegen0writes,
no guest objects/shaders. All producers terminal; no queued runtime. Current
host175 EXE49236480B/PDB113004544B and material58 EXE1208832B,
fixture tree10247849B/43; hashes/source/runtime limits in
20260908_1916_native-indexed-animation.md. Host174/982 remains the latest live
animation evidence, NOT a live qualification of175's indexed connection.

Completed cleanup this turn:981/173's three text files120921B,free
63468986368->63469109248,122880B observed reclaimed; then57/55's four text
files2163B,free63467970560->63467974656,4096B observed reclaimed. Total7
superseded agent text files123084B logical/126976B observed reclaim. Exact
literal targets validated, no recursion/reparse or game data/profile/raw deletion.
These logs are reproducible but removed; recorded981 hash/diagnosis remains,
982 and979/980 plus current baseline/pixel/reload failures stay retained.
No cleanup from earlier turns is credited here.

19:20 pre-commit free63467945984B (59.11GiB), net36290560B drive-wide use
from63504236544B entry, not wholly attributed to this task. Enumerated partial
net retained growth131053B: runtime text+32421B,build logs-696B,material EXE
+83456B,host EXE/PDB+15872B. Other object/build metadata,source/Git and system
activity are outside that subtotal; the34656256B pre-runtime discrepancy above
is not explained away. Fixture ending tree and all producer caps are recorded.
Build/test logs421138B/214; runtime-operator diagnostic accounting78105068B
by unchanged-category deltas, below75MiB stop; next producer must remeasure
full overlap rather than reuse this derived value. Timestamp inventory finds no
new raw/perf/cache/dump/image files; owner profile independently restored.
README/active queue updated once for this bundle. Explicit local source/test/docs
commit only; external push still denied with no new upload approval supplied.

### 2026-09-08 native controller/channel ownership afterb3d1f7b

User-requested push succeeded: all16 pending commits uploaded to the configured
origin/main throughb3d1f7b; clean synchronized main before implementation.
Continuation progress: native clock/layer plan and native TRS working buffers
replace admitted controller scratch traffic; one-shot generation-checked values
connect to the existing skeleton evaluator. The outgoing48-byte boundary still
must compare unchanged to detect unconverted late writers; that read is NOT
claimed removed. Complete original-controller comparison runs side effects once,
never replays a partly published native transaction. Unsupported plans refuse
before changes. No new guest hooks/codegen inputs or shader edits.

First measured free63454601216B (after source work), pre-build63453863936B;
no active renderer/compiler/build/test process. Previous recorded ending free
63470657536B was higher; this inter-turn drive change is not attributed to the
new source files or called cleanup. Scoped current build logs remain421138B/214,
no producer has run this turn. Same cumulative floor62509998080B, no reset.
Plan material59/CPU57 thenhost176, existing j4 wrapper: fixture64MiB,
CPU8MiB,host192MiB maximum free-drop;300s,10MiB cumulative build logs.
Expected fixture<16MiB overlap/host<192MiB; retain small text receipts and
replace only superseded passing fixture/build logs after new checks succeed.
No new raw/image/perf/cache producer. Live controller qualification remains
pending until a distinct bounded observation and complete diagnostic overlap
fit the unchanged75MiB runtime stop/100MiB diagnostic ceiling.

Material59/CPU57 pass,412 artifact-free guards pass. Host176 build0, no guest
objects/shaders; EXE49273856B hash10E455F95DCF08C9F305F45B13C4F515A80FE54212890557E80CDE6BBEFEEC86,
PDB113270784B. Tail audit confirms effect reads channel data while collision
records are separate; signed fractional UV wrap needs vector-denormal semantics.
After grouped tail/observation fixes, material60/PID32648 build0,CPU58/PID27428
PASS0.12s/CTest0.13s; host177/PID35888 build0, private bridge+link/codegen0writes.
All jobs terminal. Host177 EXE49275904B/PDB113266688B, SHA256
4C11FC7D3BA6B601EDEA8A60522B7ABADD7073AB554DDBFE5AFA0DC1FD41D3EF.
Fixture60 EXE1263104B, SHA2566208293D4C0349278199BB8683AC87A91A064C8C45656D6C1830541A6A689A6B;
tree10407859B/43files.60/58 replaces59/57 and58/56;177 replaces175/176 host receipts.
Keep174/982's latest live import evidence and all unresolved failures.

Next capture-free changed-controller observation reuses the opening sequence's
previously observed layer compositions: >=1000 full controller comparisons,
positive native mixes, >=256 interior-CLIP sample times, >=256 actual advancing
slot clocks and >=256 validated native skeleton handoffs. Not interior-key,
authored-subtree or scene/reload/pixel qualification. Existing ignored operator
adds an exclusive ControllerAnimationProbe; prior thresholds remain unchanged.
60s/400KiB log/192MiB maximum free-drop, full75MiB preflight overlap, no new
raw/image/perf/cache/dumps, exact owner-profile restore. New observation changes
whether complete controllers can replace standalone sampler dispatch in live
objects, not whether previously absent subtree content eventually appears.

Run983/PID25056/session81327 terminal19:50:13,exit1 explicit diagnostic-observed
stop; original profile byte-exact hash independently verified. At frame1162:
30460 matching controller transactions (including empty plans),3996 sampled,
8 mixes,3839 interior-CLIP samples,4012 advancing slot clocks,3985 handoffs,
6 changed/unreadable handoff refusals,5028 model/controller admission refusals.
No drift; standalone6sampled/checked,wrong0; skeleton1122 has3524 matching
evaluations/publications. Frame1137 isFieldActive,event1, not full field/reload
qualification. No authored subtree, normal-mode tail pixels or game-stereo claim.
Canonicalization reverified122->121tracks,93432B current charge,prepare-refused0;
that replaces982's importer provenance. Keep979/980 and975/971/962 failures.
Run983 text133297B SHA25664868E4469A9D62DF9E8642CC72405F9F227EDED4765FEEBE4B14F48D50C7D22.

Final source extent review adds source-only multi-layer512-joint overlap refusal;
native/sequential4096 capacity remains. Material61/PID32820 andCPU59/PID25736
PASS0.12s/CTest0.13s,412 guards PASS0.231s; host178/PID32484 build0, one private
bridge+link/codegen0writes, no guest objects/shaders. No further run;178 does NOT
inherit177/983's exact binary qualification. Current hashes, source contracts
and remaining interfaces in20260908_1951_native-animation-controller.md.
Current material tree10409344B/43files,EXE1263104B. HostEXE49275904B/PDB113266688B.
All producers terminal, owner profile restored, no queued build/runtime.

Completed cleanup, each exact literal target inspected and no recursive deletion:
58/56 plus59/57 and175/176 text:12files7736B,free63452467200->63452483584,
16384B observed reclaim. After983 reverified import,982/174 text:3files151885B,
free63455219712->63455375360,155648B observed reclaim. After61/59 passes,
60/58 text:4files2163B,free63454162944->63454167040,4096B observed reclaim.
Total19 superseded agent text files161784B logical/176128B observed reclaimed
(172KiB); no earlier cleanup credited. Logs are removed, not recoverable from
Git; tests/builds reproducible,982 source/hash history retained. No game data,
profiles, assets, raw/image evidence or unresolved failure logs removed.

Final pre-commit free63454167040B (59.10GiB), net434176B drive-wide use since
the first63454601216B measurement AFTER source work (not a measured full-turn
start). A later3.18MiB drive-wide free-space gain preceded cleanup and is not
claimed as reclamation. Attributed partial retained growth445236B: fixturetree
+161495B,hostEXE/PDB+301568B,buildlogs-1767B,runtimelog-16060B. Other hostobjects,
buildmetadata,source/Git and system activity are outside that subtotal.
Fresh full operator accounting78087241B diagnostics;419371B/214build logs.
No new raw/perf/cache/dump/image output;13window images still10434657B.
Existing capture gate,75MiB runtime stop/100MiB diagnostic ceiling and cumulative
floor62509998080B remain. README/active queue updated once for this bundle;
only explicit source/tests/docs to commit/push, no binaries or diagnostic files.

### 2026-09-08 late native animation continuation after827d36a

Main re-read canonical instructions, guest-source/devloop skills and full disk
policy. Clean synchronized main at entry, no active producer. Source establishes
post-controller order: AnimeData_method_4638 ->bdAnimationUpdate ->late slots
sub_822D3CB0 ->InitBones. Late slots4/5/3 use unit-step clocks; slot5 selection504
is skipped. The standalone SlotUpdate also has a separate attachment caller.
New bundle reuses native clip/controller channels through these late consumers
and republishes before native skeleton evaluation. Untracked writes still fail
the unchanged word comparison; their first differing word now has bounded
provenance. Correlation between983's six samples/six changes is not yet proof.

First measured free63449755648B, pre-build63450009600B (253952B drive-wide gain,
not cleanup). Same cumulative floor62509998080B and100MiB diagnostic ceiling;
75MiB runtime stop/10MiB build logs unchanged. Existing logs419371B/214files,
material tree10409344B/43files, hostEXE49275904B/PDB113266688B. No budget reset.
413 artifact-free guards/scenario tests pass0.239s. Plan material62/CPU60 then
host179 with inspected existing j4 wrapper:64MiB/8MiB/192MiB free-drop caps,
300s timeout. Expected fixture overlap<16MiB and host<192MiB. Keep current
passing61/59 and178 until replacements pass, plus177/983 live evidence and all
unresolved failures. No new raw/image/perf/cache/dump producer. A later runtime
must reserve full400KiB overlap and establish positive late-native consumption,
not just repeat the prior controller count or replace missing broader gates.

Material62/PID37152 build0; CPU60/PID33600 PASS0.12s/CTest0.13s. Host179/
PID22264/session13205 terminal0: private animation bridge plus version consumers,
codegen0writes, no guest objects/shaders. HostEXE49293824B/PDB113348608B;
fixtureEXE1285632B. End build free63449174016B. Reused inspected runtime operator
adds LateAnimationProbe only as an extra condition on the unchanged controller
observation: positive matching late transactions, positive samples, reused native
layers and downstream late skeleton handoffs. No subtree/indexed/motion/pixel
claim.60s,400KiB text,192MiB free-drop; reserve full75MiB overlap before profile
mutation, zero raw/image/perf/cache/dump allowance, guaranteed exact restoration.
Current passing179/62/60 supersedes178/61/59 build/test text;177/983 still retained
until the new run re-verifies its controller and import contracts.

Run984/PID32680/session49310 terminal20:21:19, exit1 observation-complete stop.
Main30460matching transactions,3996samples,8mixes,4012advancing clocks; late6
completed/checked,6samples,6native reuses and6native skeleton handoffs. Total
handoffs3991,changed0 versus983's3985/6, with identical main controller counts.
This source/causal-regression/live bundle resolves the six observed late writes;
5028model/controller refusals still unclassified. Standalone attachment sampling
not reached. Skeletonframe1084 has3706matching evaluations/publications; context
1085 is opening event, not interactive/reload/motion/both-eye qualification.
Run984 also re-verifies canonicalization122->121/93432B,prepare-refused0. Exact
hashes/contracts in20260908_2021_native-late-animation.md. Profile independently
matches original2F1BC38D...; no new raw/perf/cache/dump/image files by scoped
timestamp inventory. No producer remains live, no next build/run queued.

Completed cleanup:178/61/59 logs6files2924B logical, free63448608768->63448616960,
8192B reclaimed; after984 replaces prior live coverage,177/983 logs3files134058B
logical,free63451828224->63451967488,139264B reclaimed. Total9superseded agent
text files136982B logical/147456B observed (144KiB) reclaimed this turn only.
Deleted text is not in Git; builds/tests reproducible,983source/hash/results
remain historical in its controller report. No protected failure/game data,
asset/profile/build tree/image/raw deletion. Preserve975/979/980/971/962 evidence.

Pre-commit free63451967488B (59.09GiB),2211840B drive-wide GAIN since first
63449755648B. Most of that gain preceded cleanup and is NOT attributed to it.
Partial attributed retained growth147266B: fixturetree+60722B,hostEXE/PDB+99840B,
buildlogs+1006B,runtime984replacing983-14302B. Host objects/build metadata,
source/Git and unrelated activity are outside this subtotal. Final diagnostics
78073945B;420377B/212buildlogs. Same75MiB stop/100MiB ceiling/floor62509998080B,
13window images10434657B unchanged. README/active queue updated for this bundle;
commit/push explicit source/tests/docs only. Full goal remains active/incomplete.

### 2026-09-08 selected-track/root-motion verification aftercecde3e

Previous checkpoint made progress: selected sampling/root hook source and415
guards were committed/pushed with C++/live acceptance explicitly pending.
Current clean synchronized main inspected; no producer remains live. Main read
devloop/guest-source skills; the full disk policy and same cumulative ledger
remain authoritative. Added source-free keyed/cubic selected-root parity,
indexed descriptor0, dormant payload, ordinal, missing-target and selected-only
overflow/refusal fixtures before launching any producer.

First measured free63447175168B after fixture edits; buildlogs420377B/212files,
fixturetree10470066B/43files, hostEXE49293824B/PDB113348608B. No budget reset.
Plan material63/CPU61 thenhost180 in existing trees/j4 bounded wrapper. Fixture
64MiB/CPU8MiB/host192MiB maximum free-drop,300s,10MiB aggregate build logs;
same floor62509998080B/100MiB diagnostics/75MiB runtime stop. Expected fixture
overlap<16MiB and host<192MiB. Retain latest passing62/60/179 receipts until
replacements pass,984 live evidence and all unresolved failures. No new raw,
image, perf, cache or dump producer. Live root comparison requires a new bounded
observation and full diagnostic overlap to fit before profile mutation/launch.

Material63/PID31120 terminal1: fixture-only nested std::array CTAD copied a
record instead of creating a one-record span. Explicit ChannelRecord,1 extents
fix all occurrences; material64/PID38820 build0,CPU61/PID34880 PASS0.12s/CTest0.13s.
Host180/PID14964/session49891 terminal0, private animation bridge and version
consumers; codegen0writes, no guest objects/shaders. EXE49301504B/PDB113389568B,
SHA25600370FA4BA7FBD8E7F0E135880C8273C82CC9EDAC67A9944B9D7CDE74F9D25B6.
FixtureEXE1325056B SHA25685F283CF0EFACB3E609A59E54B1C624751E87B0D3336FEAD3502377200CBABB0;
tree10574334B/43files.415 Python guards/scenario tests PASS0.226s.

After passing replacements, eight exact verified ordinary files removed:
material62/CPU60/host179 receipts and resolved63 compile-error text.24483B logical;
free63446626304->63446654976,28672B observed reclaimed. Logs are reproducible,
not recoverable from Git; concise63 cause retained here. No active build tree,
game data or unresolved failure evidence removed.984 remains latest live text.

Inspected full existing runtime operator; RootAnimationProbe adds positive equal
root completed/checked counts and positive sampled tracks to UNCHANGED controller
and late-layer observations. Its actual selected source/root output comparison
is new coverage, not a retry for absent authored subtrees.60s/400KiB text/192MiB
free-drop; no raw/image/perf/cache/dumps, exact profile restoration. Preflight
diagnostics78073945B; plus full400KiB=78483545B below78643200B stop (159655B
headroom), buildlogs420377B/212files, free63446654976B. Original profile matches
2F1BC38D... independently. No previous process live; run985 is the next producer.

Run985/PID22788/session2473 terminal20:51:26, operator exit1: requested root
observation not reached in60s. No root completion/absence/refusal record; do NOT
call the combined observation passing. Controllerframe2290 has108986 matching
transactions,13815samples,23mixes,13240interior-CLIP times,14601advancing clocks,
13805handoffs,changed0;18116admission refusals remain unclassified. Late6matching
transactions/samples/reuses/handoffs,refused0. Skeleton2251 has14421 matching
evaluations/publications,wrong0/unavailable0. Context2266 is post-eventFieldActive,
not independent reload/motion-pixel/both-eye evidence. Canonical import122->121/
93432B is observed again,prepare-refused0.984's controller/late/import evidence
is replaced by985; root/model/name/state runtime acceptance stays pending.
Next needs a reachable root-request caller/content condition or changed
observation; no unchanged retry, threshold reduction or speedup claim.

Run985 text267079B SHA256EE9C04E7E05A1529B8A946417356946D2026715724D46E15B3B160DFF30E7B44,
retained for current comparison and unresolved root coverage. Exact profile
hash2F1BC38D... independently restored. No producer remains live; timestamp
inventory found no new raw/perf/cache/dump files. No further producer queued.
After validating replacement purposes, exact984log118995B removed (old hash
03C9EECD... remains in its historical report). Free63445491712->63445614592,
122880B observed reclaim. Total this turn9text files143478B logical/151552B
observed reclaimed (148KiB), not prior cleanup credit. Deleted text is not in
Git; tests/builds reproducible,984source/hash findings retained. Preserve985
and975/979/980/971/962 unresolved evidence, game data, profiles and active trees.

Pre-commit free63445598208B (59.09GiB),1576960B drive-wide additional use since
first63447175168B measurement after fixture edits. Partial attributed retained
growth300992B: fixturetree+104268B,hostEXE/PDB+48640B,buildlogs0B,985replacing984
+148084B. The larger log retains newly tested post-event controller behavior
and the missing-root observation; other hostobjects/buildmetadata/source/Git and
unrelated activity are outside this subtotal. Diagnostics78222029B from prior
full accounting plus net runtime text; buildlogs420377B/212files remeasured.
Full400KiB incoming overlap would leave only11571B under75MiB, so any next
producer must remeasure/reconcile rather than assume it fits. Same100MiB ceiling,
62509998080B cumulative floor,13images10434657B and historical raw gate unchanged.
README/queue distinguish CPU/build/live controller evidence from unqualified
root/cutscene integration. Full goal remains active, desktop/VR gate incomplete.

### 2026-09-08 one-joint consumer connection after042c75e

Previous goal turn was progress: selected-track CPU/host/live controller evidence
changed authoritative qualification; root985 coverage stayed missing. Clean
synchronized main at entry, no producer live. Read guest-source/devloop skills,
full disk policy and hook TOML. Reused prior contracts; complete8227EF60 lookup
and8228A4E8 dispatcher now inspected. All four non8218FC98 one-joint callers
resolve a model/index immediately before sampling (100:11924,63:11507,
65:20217,88:7016). New bounded one-shot lookup provenance connects them to
generation-checked load-owned joint/rest data and existing selected clip residency.
Original lookup/caller placement remains; no new address map/model lease cache.
Weighted one-joint math reuses SampleRootMotion, including <=0/epsilon no-ops,
upper weight clamp, indexed descriptor0 and owned rest blending. Exact dormant
word comparison distinguishes unsampled/TR-only fields from active finite values.
No model-name uniqueness guard was relaxed: direct cursor semantics still make
duplicate model names distinct from duplicate asset descriptors.

First measured free63445581824B after source/tests, fixture10574334B/43files,
hostEXE49301504B/PDB113389568B;420377B/212build logs.416 Python guards PASS0.220s.
No budget reset: floor62509998080B,100MiB diagnostics,75MiB runtime stop.
Plan material65/CPU62 thenhost181 using inspected existing j4 wrapper:
64MiB/8MiB/192MiB maximum free-drop,300s,10MiB aggregate build logs. Expected
fixture overlap<16MiB/host<192MiB, no guest rebuild or new shader/image/raw/perf/
cache/dump producer.64/61/180 receipts retire only after replacements pass;
985 missing-root evidence remains protected. Live one-joint observation is a
different changed-code route, not a relabelled8218FC98 pass or unchanged retry;
require fresh positive joint comparison and full budget overlap before launch.

Material65/PID33868 build0;CPU62/PID33524 PASS0.12s/CTest0.13s. Host181/PID36312/
session2382 build0; final182/PID29180 build0 adds sampled-vs-no-op provenance.
No guest objects/shaders;182 rebuilds only bridge+link,codegen0writes. Final
hostEXE49311232B/PDB113426432B, SHA256BC0D87BCC2E0742916DB91F5F553F6E45A33C453B38FCF39F41C75F648600F00.
FixtureEXE1329664B SHA25667F0C7E4C11F15A6504A0443E5F5AFD8A1B71DC2C4D051781229BF1BB1D91492;
tree10588180B/43files. All producers terminal before next launch.

Passing65/62/182 replaces64/61/180/181 receipts. Eight exact ordinary logfiles,
7219B logical, removed after inspection;free63444844544->63444856832,12288B
observed reclaimed (12KiB). Reproducible build/test text, not in Git. No runtime
evidence, active tree, game data, profile or unresolved failure removed.
Fresh operator accounting78220262B diagnostics,418610B/212buildlogs. Full400KiB
overlap=78629862B leaves13338B under unchanged75MiB stop. Free63444856832B;
original profile hash2F1BC38D... confirmed. Native single-joint observation
requires positive completed==checked AND positive sampled counts in addition
to unchanged controller/late criteria. It excludes the distinct RootAnimationProbe
so cannot restamp985's missing8218FC98 evidence. Same60s/400KiB/192MiB limits,
zero raw/image/perf/cache/dump allowance and guaranteed exact profile restore.

Run986/PID34464/session14460 terminal21:12:21, explicit observation-complete
stop (operator exit1, not a crash). Jointframe1144 has289 completed/checked/
sampled,weighted0,refused0. Controller30364 matching,5012 admission refusals,
3984samples,8mixes,3827interior-CLIP,4000advancing clocks,3979handoffs,changed0;
late6 matching updates/samples/reuses/handoffs,refused0. Skeleton1119 has3706
matching evaluations/publications,wrong0/unavailable0. Context1120 isFieldActive
bg41_01,event1: opening-event coverage, not post-event/reload/motion/both eyes.
No caller attribution or fractional-weight observation; no new8218FC98 root
evidence. Retain985 for its separate missing-root gate and longer post-event
controller baseline. Canonical122->121track/93432B import again,prepare-refused0.

986text120282B SHA25662CF629B8D43403A712B87B8E1D0F64B3A9FFDF7BE302551B5F77058437C9B90.
Profile2F1BC38D... independently restored; no producer remains, no new raw,
capture/perf/cache/dump files in the scoped timestamp inventory. Final416 Python
guards/scenario tests PASS0.227s. No further producer queued.
Post-run free63444729856B,851968B drive-wide use since first63445581824B.
Partial attributed retained growth178953B: fixture+13846B,EXE/PDB+46592B,
buildlogs-1767B,new distinct986log+120282B. Excludes objects/buildmetadata/source/
Git/system activity. Current cleanup credit remains12288B observed,7219B logical.
Buildlogs418610B/212files; calculated diagnostics78340544B. Next full400KiB
runtime overlap78750144B exceeds unchanged75MiB stop by106944B: reconcile eligible
superseded artifacts before another run; do not raise the cap or delete protected
985 just to fit. Source work can continue; full goal is not marked blocked.
Pre-documentation/commit recheck free63443554304B,2027520B additional drive-wide
use since first measurement; unrelated drive activity is not attributed cleanup.
README/queue and the new single-joint report record this connected checkpoint.

### 2026-09-08 load-owned joint selection after4df3e0d

Previous turn was progress:4df3e0d verified/committed/pushed one-joint consumers;
working tree clean at entry. Read guest-source/devloop skills, full disk policy,
render hook TOML and loader ownership frontier. Reused prior complete lookup/
sampler/caller contracts and checked current source. The load producer now owns
dense pose-to-source outgoing aliases in the existing model registry, separately
from source-free NativeModelRenderData. This connects native joint lookup to
the one-shot sampler and replaces controller SourceNode traversal and excluded
joint name rereads with owned values. Legacy pointer/table exports remain.
Generation, malformed/duplicate/partial/overflow bindings, source destruction,
reordered hierarchy and exact shared-budget fixtures precede any producer.

First measured free63437365248B after source edits; pre-build63442452480B.
Fixturetree10588180B/43files,EXE49311232B/PDB113426432B,buildlogs418610B/212files;
no matching producer live. Same cumulative floor62509998080B,100MiBdiagnostics,
75MiBruntime stop,10MiBbuildlogs,13images10434657B and historical raw gate.
Plan material66/CPU63 andhost183 in existing j4 trees:300s limits and64/8/192MiB
maximum free-drop, fixture expected<16MiB,host<192MiB. No guest/shader build or
new image/raw/perf/cache/dump producer. Keep65/62/182 until replacements pass.
Run985 remains distinct protected missing-root/post-event evidence;986 remains
the single-joint baseline. Current calculated diagnostics78340544B cannot fit
full400KiB incoming runtime. Plan lossless compression of exact terminal985log
267079B to one .gz, bounded1MiB overlap, verify full decompressed SHA256 before
removing only its redundant plaintext. This preserves all failure evidence,
does not lower checks, and is not permission for another boot until reaccounted.
Operator already accounts retained-*.zip archives, so use retained-root985.zip
instead of .gz; no accounting exclusion or new compression framework. Both are
lossless; the retained member must reproduce all267079B and original SHA256.

Material66/PID33608 build0; CPU63/PID31196 PASS0.12s/CTest0.14s.417 Python
source/scenario checks PASS0.225s. Host183/PID28668/session99321 terminal0:
shared model consumers and version users rebuild; codegen0writes, no guest
objects or shaders. EXE49315840B/PDB113446912B; EXE SHA256
28B26DA985D193602BC5BF9D79F216185AE98B4B71B30FE6C8AB8809B18D71D5.
FixtureEXE1353728B SHA256251CF22468AB63F6E07B5A77A161DFDBE45D18E788A5B4FB9AB4BFBE4FC94425;
tree10682023B/43files. All matching producer processes0 before cleanup/runtime.

Lossless retained-root985.zip is44650B, SHA256
D1493DE16E6AC0EA0665905EA10A150EE021605DCD8FB475201B0AB9CAF822DA.
Its sole reblue_985.log member is267079B and full decompressed SHA256 equals
EE9C04E7E05A1529B8A946417356946D2026715724D46E15B3B160DFF30E7B44.
Compression free63439679488->63439634432 consumes45056B observed. Then remove
only exact verified redundant985plaintext and superseded65/62/182 build/test
receipts:7ordinary files270003B logical,free63439306752->63439585280,
278528B observed deletion reclaim. Net observed compression/deletion saving
233472B (228KiB); no earlier cleanup credit.985 remains fully recoverable from
the ZIP, test/build receipts reproducible and historical hashes retained.
No game/profile/active tree or unresolved evidence discarded.

New JointSelectionProbe adds positive matching lookup/found observations to
UNCHANGED single-joint/controller/late requirements. Owned exclusions are
reported separately, not assumed present. Parser clean. Runtime full accounting
78121884B plus400KiB=78531484B leaves111716B below unchanged75MiB stop;
buildlogs422379B/212files,free63438999552B. Profile2F1BC38D... independently
confirmed. Run987 planned60s/400KiB/192MiB free-drop, no raw/image/perf/cache/
dumps, guaranteed exact profile restoration. This tests changed load-owned
lookup reaching the sampler, not an unchanged retry or root985 qualification.

Run987/PID29524/session40387 terminal21:37:48, explicit observation-complete
operator exit1, not a crash. At1092:732 matching/found native lookups,refused0;
291 completed/checked/sampled single joints,weighted0,refused0. Main30556
matching transactions,5044 admission refusals,4008samples,8mixes,3851interior,
4024advancing clocks,4003handoffs,changed0; late6 matching updates/samples/reuses/
handoffs,refused0. Owned exclusions0, so that changed authored route remains
CPU-only. Skeleton1054 has3563 matching evaluations/publications,wrong0/
unavailable0; context1069 isFieldActive bg41_01,event1. Lookup441 at792 grows
to732 at1092 with291 new samples. Not post-event/reload/motion/both-eye evidence.
Canonical122->121track/93432B import passes again,prepare-refused0.

987log119516B SHA256BD44625AA229AE3BF952BC149E70970311A03454C069B33CE8C9D1E6EF433317.
Profile hash2F1BC38D... independently restored. Matching producers0; timestamp
inventory since21:37:16 finds0new capture/perf/cache/dump files. No next producer.
Validated987 replaces986's single-joint/controller/late/import purposes; exact
986text120282B removed, hash62CF629B... retained in the historical report.
Free63438856192->63438979072,122880B observed additional reclaim. Total current
turn8files390285B logical deleted,401408B observed deletion reclaim, minus45056B
archive allocation =356352B net observed cleanup (348KiB).985 is fully recoverable
from ZIP;986 is superseded text, not recoverable from Git, with findings retained.
No prior cleanup credited again; all unresolved failures and game data preserved.

Partial attributed retained change -100495B: fixture+93843B,EXE/PDB+25088B,
buildlogs+3769B,987replacing986 -766B,985lossless archive -222429B. This excludes
hostobjects/buildmetadata/source/Git/system activity. Calculated diagnostics
78121118B and buildlogs422379B/212files; next full400KiB overlap would leave
112482B under75MiB, requiring fresh remeasurement. Pre-doc free63438979072B,
1613824B drive-wide free increase since first63437365248B, not all cleanup.
README/queue and the new report distinguish live lookup/sampling from pending
owned exclusions, root/content, motion/pixels and full desktop/VR acceptance.

### 2026-09-08 attachment placement after49d785a

Continue the SAME cumulative ledger: first scoped free63435210752B, no matching
game/build/test producers via Get-Process (CIM denied, not used as proof).
Fixture10682023B/43files,EXE49315840B/PDB113446912B,buildlogs422379B/212files.
Floor62509998080B, owner3GiB exception,100MiBdiagnostics/75MiBruntime stop,
10MiBbuildlogs,13images10434657B and historical no-new-raw gate unchanged.
Plan material67/CPU64/host184 in existing j4 trees,300s and64/8/192MiB maximum
free-drop; expected fixture<16MiB, host<192MiB peak. No shader/guest build,
image/raw/perf/cache/dump producer. Keep66/63/183 receipts until replacements
pass;987 and protected985ZIP remain. Runtime requires fresh aggregate accounting
and a placement-specific observation, not another unchanged root-coverage boot.

Material67/PID37792 build0; CPU64/PID28624 fails because the new one-joint
fixture requested transfer count3. Production strict count check is correct.
Fixture now explicitly rejects3 and transfers pose.size(); no threshold changed.
Material68/PID26572 build0,CPU65/PID20676 PASS0.11s/CTest0.13s.418 Python
guards/scenario tests PASS0.225s. Host184/PID33632/session21669 terminal0:
new header triggers CMake glob recheck, codegen0writes; only host/version users,
no guest objects/shaders. EXE49336832B/PDB113524736B, EXE SHA256
255A033A8906F8C91C6CCAD6585A1E9E57C1C597145CBFFC972A64A08755DA41.
Fixture1371648B SHA25672E39D6EFBC961A2F49D1437754FC8876668A14CAE8478F08E9A9011EC018A61,
tree10731178B/43files. All matching producer processes0.

After passing replacements, removed10 exact ordinary superseded66/67,63/64,
183 receipt files,8087B logical; free63433416704->63433433088,
16384B observed reclaim. Resolved CPU64 setup error retained above and now
covered by an explicit negative assertion. Receipts reproducible; no game data,
profiles, active trees or protected runtime failures removed. No prior credit.
Current diagnostics78119237B/buildlogs420498B/212files; full400KiB runtime
overlap leaves114363B below unchanged75MiB stop. Profile SHA2562F1BC38D...
unchanged. New AttachmentPlacementProbe requires positive matching sampled
placements/tracks and native skeleton roots, changed0, plus unchanged controller/
late gates. Copy-mode coverage reported separately. Run988 planned60s/400KiB/
192MiB free-drop, no images/raw/perf/cache/dumps, exact profile restoration.

Run988/PID34620/session90601 terminal22:07:06, operator exit1 for missing
requested placement observation, NOT PASS. No positive placement/refusal line;
cannot distinguish unreached callback from disabled/unattached. Both modes and
actual native root consumption remain CPU/source-only. No unchanged retry.
Existing frame2311:109082 matching controllers,18132 unclassified refusals,
13827samples,23mixes,13252interior,14613advancing,13817handoffs,changed0; late6
matching/samples/reuses/handoffs. Single659 matching/sampled,9 fractional-weight,
refused0; lookup1100 matching/found,refused0,owned exclusions0. Skeleton2268=
14409 matching evaluations/publications,wrong0/unavailable0; context2285 is
FieldActive bg41_01,event0. Canonical122->121/93432B import reverified.
No motion/reload/pixels/both-eye/Quest claim. Profile116B restored SHA256
2F1BC38D763A1B7BDBA31F560684FD4AA7E42A714600B8D344F19DA7F38E23B0;
matching producers0, timestamp inventory0new capture/perf/cache/dump files.

988log269258B SHA25681FD1C911F4C9C186D7D18C3B648EDDCA82DD4A8A329212A981C4F19B88C4E65
retained losslessly in retained-placement988.zip45113B, SHA256
ED7655643F3BCE40B912AB2876E7ABE3B9610719248F9EB09968DD3B0E9CA851.
Sole member's full decompressed hash/size verified before plaintext removal.
First archive command lacked System.IO.Compression assembly and failed before
creating output; explicit assembly load succeeded, no duplicate producer/file.
Compression free63530446848->63530397696 consumes49152B; overlap<1MiB.
After terminal/hash checks removed exact redundant988plaintext and superseded
987log119516B (hashBD44625A...);988 covers its lookup/sampler/import purposes.
2files388774B logical,free63530397696->63530790912,393216B observed reclaim.
988 remains fully recoverable from ZIP;987text is superseded, findings/hash in
prior report, not recoverable from Git.985ZIP/975/979/980/971/962/964/968 and
other unresolved evidence unchanged. Total this turn12files396861B logical
removed;409600B observed deletion minus49152B archive allocation=360448B net
cleanup352KiB. No earlier savings credited again.

Partial attributed retained growth71687B: fixture+49155B,EXE/PDB+98816B,
buildlogs-1881B,988ZIP replacing987text-74403B. This excludes host objects,
buildmetadata/source/Git and external activity. Calculated diagnostics78044834B,
buildlogs420498B/212files; next full400KiB would leave188766B below75MiB,
requiring fresh remeasurement. Pre-doc free63530790912B,95580160B drive-wide
free increase vs first63435210752B; most is NOT attributable cleanup. No next
producer queued. README/queue/new source-contract report preserve the failed
authored-placement gate and point to caller conditions before a new probe.

### 2026-09-08 ready slot selection after8e6fcd6

Same cumulative ledger/budget, not a new allowance. First preflight free
63519076352B; no matching game/build/test processes. Fixture10731178B/43files,
EXE49336832B/PDB113524736B, buildlogs420498B/212files. Floor62509998080B,
owner3GiB exception,100MiBdiagnostics/75MiBruntime stop,10MiBbuildlogs and
historical no-new-raw gate unchanged. Plan material69/CPU66/host185 in the
existing trees,300s with64/8/192MiB maximum free-drop; expected fixture<16MiB
and host<192MiB peak. No guest/shader rebuild or image/raw/perf/cache producer.
Retain68/65/184 receipts until replacements pass; protected988/985ZIPs stay.
419 artifact-free guards pass0.225s. New CPU cases exercise selection through
controller/skeleton/instance owners, pending first-duplicate refusal and exact
slot words. Runtime needs fresh aggregate preflight and a selection-specific
observation; neither unchanged attachment nor whole-root probe is queued.

Material69/PID34800/session13188 build0. CPU66/PID38376 fails on the new
fractional-blend consumer fixture lacking authored rest availability; production
correctly refuses. Fixture now explicitly asserts that refusal, then supplies
the existing owned rest fields. Material70/PID37640 build0; CPU67/PID32096
PASS0.13s/CTest0.14s. Host185/PID33608/session55045 build0: CMake glob check,
codegen0writes, only host/version users; no guest objects/shaders. Fixture
1402368B SHA256 D6F5A626B1DF00353AAC1396BCECB0968378DD60D3215A49F638DBC09C0072B3,
tree10809821B/43files. EXE49346048B/PDB113561600B, EXE SHA256
24699648FADFA4BFFCBA39A46D06428EB081DED5BF72C34671CAFEA85185A078.
All matching producers terminal. After replacements pass,10 exact superseded
68/69,65/66,184 receipt files removed:6260B logical,free63516741632->63516753920,
12288B observed reclaim. Resolved CPU66 fixture omission is preserved above and
as a negative assertion; receipt logs reproducible, no protected data removed.

Fresh diagnostics78044731B,buildlogs420395B/212files;400KiB log overlap leaves
188869B below75MiB. Free63516688384B, profile original hash2F1BC38D... unchanged.
SlotSelectionProbe requires positive completed==checked, ready assets and actual
restarts plus unchanged controller/late observations. Wrapper parse passes.
Plan989:60s/400KiB/192MiB maximum free-drop, no raw/image/perf/cache/dumps,
guaranteed exact profile restoration. This tests the changed selection path;
988's attachment and985's whole-root coverage failures stay unqualified.

Selection run/PID27880/session86472 terminal22:34:17; explicit observation-complete
operator exit1, not a crash/full game pass. Start22:33:32. Planned989 was not
the actual log label: rotation reused reblue_981.log after old text retirement.
This is host185/time/PID-identified evidence, not a restamp of historical981.
At1133:208 matching selections,23restarts,56ready,152absent,1unclassified refusal;
controller31036 matching,5124unclassified refusals,4068samples,8mixes,3908interior,
4084advancing,4063handoffs,changed0; late6matching/samples/reuses/handoffs.
Lookup737matching/found,single296matching/sampled,weighted0,owned exclusions0.
Skeleton1102=3719matching,wrong0/unavailable0; context1103FieldActivebg41_01,event1.
Opening-event coverage only; no post-event/reload/motion/pixel/both-eye/Quest claim.
All11 settings took effect; original116B profile restoredhash2F1BC38D...;
matching producers0; timestamp inventory0new raw/image/perf/cache/dump files.

Log122054B SHA25698B9EC34565EB3BDA7049B781CD527DCA0A1B555764BA5CE72C50B5276A46F0A
retained losslessly as retained-slot-selection185.zip22757B, SHA256
136BD368C9AF80681A586FCA2501DD7205057CB613052D1FDB9D405A8DBC8156.
Sole member/name/size/full decompressed hash checked before exact plaintext
removal. Compressionfree63516467200->63516442624,24576Ballocation; deletion
free63516442624->63516565504,122880Bobserved reclaim,98304Bnet archive saving.
Total11files128314Blogical removed;135168Bobserved deletion minus24576Barchive
allocation=110592Bnet current cleanup108KiB. No prior savings credited again.
Plaintext fully recoverable from ZIP; superseded receipts reproducible.988/985
ZIPs and all other unresolved failures/game data/profiles remain protected.

Partial attributed retained growth147377B: fixture+78643B,EXE/PDB+46080B,
buildlogs-103B,new selection ZIP+22757B. Retaining this small new-purpose log is
necessary until its selection/consumer observation is superseded; existing
protected988/985 evidence is not equivalent. Excludes hostobjects/buildmetadata/
source/Git/external activity. Calculated diagnostics78067488B,buildlogs420395B/
212files; next400KiBoverlap leaves166112B below75MiB, remeasure before any job.
Pre-doc free63516565504B,drive-wide consumption2510848B versus first63519076352B,
not all attributable build growth. No next producer queued. README/queue/report
preserve compatibility boundaries and full desktop/both-eye acceptance scope.

### 2026-09-08 controller-to-effect material motion afterb9f54bb

Same cumulative ledger and owner3GiB exception; floor62509998080B,
100MiBdiagnostics/75MiBruntime stop,10MiBbuildlogs and no-new-raw gate unchanged.
First measured free62866153472B, then62865887232B at22:56:18. Prior final
63515893760B: ~650MB drive-wide reduction, not attributed to this source work.
Scoped verification/build-root/log inventories show only the already-accounted
22757B selection ZIP since22:34:17; fixture10809821B/43files, EXE49346048B,
PDB113561600B and buildlogs420395B/212files unchanged. Get-Process and elevated
CIM confirm no game/build/test producers. Sandbox CIM refusal is not a live job.
420 artifact-free guards pass0.244s. Reuse material fixture71/CPU68 and host186;
300s,64/8/192MiB free-drop supervisors, expected fixture<16MiB/host<192MiB peak.
No guest/shader rebuild, raw/image/perf/cache producer. Retain previous receipts
until replacements pass; protected selection185/placement988/root985 ZIPs stay.
Fresh runtime preflight and changed-effect observation required before boot.

Material71/PID37244 build0. CPU68/PID37092 fails the new equal-entry fixture:
its second active cue lacks +312 duration; production correctly refuses before
mutation. Added an explicit missing-duration rejection then supplied that cue's
duration. Production unchanged. Material72/PID38496 build0; CPU69/PID36520
PASS0.12s/CTest0.13s. Fixture1459200B SHA256
A69AA8934B798A82A1517C23090094E0C4541B20123454E8CB0C240F32FB5BFB.
Host186/PID21032/session73760 build0, CMake glob check/codegen0writes, only
host/version consumers, no guest objects/shaders. EXE49355776B/PDB113659904B;
EXE SHA256839F7AD2397A2240E93B565B1799CD6BF7671990DF91572C8D5531972258AEDA.
Implementation/test checkpoint196d85d pushed; live and pixel gates still pending.
All current producers terminal. Retire superseded70/71,67/68,185 build/CPU
receipts only after exact inspection; CPU68 omission remains in this ledger and
its negative fixture. Keep72/69/186 and all protected game/runtime evidence.

Cleanup completed:10 exact superseded receipt files6125B logical, free
62830424064->62830436352B,12288B observed reclaim; reproducible logs, not
game assets or protected failures. Fixture now10972220B/43files (+162399B).
Diagnostics78067488B,buildlogs420395B/212files;400KiB overlap leaves166112B
below75MiB. Original116B profile SHA2562F1BC38D763A1B7BDBA31F560684FD4AA7E42A714600B8D344F19DA7F38E23B0.
Runtime plan: inspected wrapper EffectAnimationProbe requires>=256 matching
effects, positive UV and native translation/rotation drivers, plus unchanged
controller/late/ready-slot observations.60s/400KiB/192MiB free-drop; fresh
file/time/PID discovery, no raw/image/perf/cache/dumps, exact profile restoration.
Rotation/transition-specific live counts will be reported, not inferred from
aggregate driver coverage. Protected authored attachment/root and pixel gates
are neither replaced nor relaxed.

Runtime host186/PID35436/session9932 terminal23:04:46 after23:03:45 start;
exit1: required native joint-driver observation NOT reached within60s. No crash
or comparison mismatch, but this is a FAILED coverage gate. Logger reused981;
time/PID/hash identify this new run. At2335:1111 matching effect transactions,
2220 UVs,translated0/rotated0/transitions0; fresh post-event1735/2035/2335
samples811/961/1111 and1620/1920/2220UVs. Controller108314matching,
18004unclassified admission refusals,13731samples,23mixes,13156interior,
13721handoffs/changed0,14533advancing. Late6matching/reused/handoffs;
selection1399matching,27restarts,1245ready,154absent,2unclassified refusals.
Skeleton2308=14409matching,wrong0/unavailable0. Material2309=1232override
publications,246323checks/wrong0,184513UVblocks; not per-effect provenance.
All11settings applied; original116Bprofile restoredhash2F1BC38D...;
timestampinventory0new raw/image/perf/cache/dump files. No unchanged retry.

Log273696B SHA256B3CBB2E21667EB4C0F086F95BBB9FBB1072DC6AC885F4477F910A0A66534B12E
losslessly archived as retained-effect186.zip49644B, SHA256
BECDA36519081446C5060D201C0C4C926D07B7F4511AA30C50C6E12262DBF7A1.
Sole member/name/length/full decompressed hash verified before plaintext removal.
Compressionfree62958125056->62958071808B,53248Ballocation. Exact273696B
plaintext plus superseded22757Bselection185ZIP removed after hash checks:
2files296453Blogical,free62957920256->62958219264B,299008Bobserved reclaim.
New full ZIP preserves scrolling evidence and missing-driver coverage failure,
and supersedes185's selection/controller/late purpose. Current text recoverable
fromZIP; older185text is superseded/not recoverable fromGit.988/985 and all
other protected failure evidence/game data/profiles remain. Retain186 until its
missing native-driver observation is explained and purposefully superseded.

Total current cleanup12files302578Blogical;311296Bobserved deletion minus
53248Barchive allocation=258048B net252KiB. No prior cleanup credited again.
Partial attributed retained growth297318B: fixture+162399B,EXE/PDB+108032B,
buildlogs0B,newZIP replacing185ZIP+26887B. Excludes hostobjects/buildmetadata/
source/Git/external activity. Pre-final-doc free62958206976B,drive-wide gain
92053504B vs first62866153472B; most is not attributable cleanup. Diagnostics
78094375B,buildlogs420395B/212files; next400KiB overlap leaves139225B below
75MiB and requires fresh measurement. All producers terminal; no next job queued.
Final420guards pass0.225s. README/queue/report distinguish scrolling from the
failed joint-driver gate; no motion/reload/pixel/both-eye/Quest/speedup claim.

### 2026-09-08 effect input/admission census after876b633

Previous owner turn confirmed push only (no renderer progress). Revalidated clean
source and no game/build/test producers. First free62956847104B; pre-build
62957199360B. Same cumulative3GiB exception/floor62509998080B and diagnostic/raw
limits, no reset. Buildlogs420395B/212files, previous diagnostics78094375B.
Changed observation: verification-only effect driver census BEFORE all native
controller admission, partitioned by rejection stage. Unknown input is separate
from zero authored drivers; no source writes, residency or changed eligibility.
This resolves whether host186's missing drivers are absent or rejected, not a
rendering dependency removal or replacement for its failed positive-driver gate.
421 artifact-free guards pass0.233s. Reuse material73/CPU70/host187,300s and
64/8/192MiB free-drop supervisors; expected fixture<16MiB/host<192MiB peak.
No guest/shader rebuild or raw/image/perf/cache output. Keep72/69/186 receipts
and all protected runtime evidence until actual replacement; remeasure full
diagnostic overlap before the single60s/400KiB/192MiB runtime observation.

Material73/PID37800 build0; CPU70/PID27768 PASS0.12s/CTest0.14s. Fixture
1466880B SHA2562A3A6FBEBE586365A26F09777E8AA9C32F9F90321E83E0A55F00A278D71D9E99;
fixture tree10993413B/43files (+21193B). Host187/PID32800/session26758 build0,
codegen0writes, no guest objects/shaders. EXE49360384B (+4608B),PDB113684480B
(+24576B),EXE SHA2561A977043C228F94F5DF3BFB02FDE715ADCF8BEE64671D587E3667A5ECCCA5595.
Source/test checkpoint5006b1e pushed. All build handles terminal. Removed6 exact
superseded72/69/186 receipts4709Blogical after replacement; free62956752896->
62956761088B (8192B observed reclaim). Keep73/70/187, protected runtime evidence.

Runtime preflight diagnostics78094357B,400KiBtext overlap fits; free62956761088B,
floor62755434496B. Host187/PID31124/session79453 starts23:24:46,terminal23:25:48,
exit1 requested positive-driver observation NOT reached. Fresh census2342:
108890admitted/18100boundary-refused observations,unknown0/named0 in BOTH groups;
no other rejection stages. Effect1117matching/2232UVs,drivers/transitions0;
post-event1742/2042/2342 advance817/967/1117transactions and1632/1932/2232UVs.
Controller108890matching/13793handoffs/changed0;late6matching;slot1475matching;
skeleton14409matching,wrong0;material246188checks/wrong0. See the effect-input
report for all limits and the distinction between ordinary UV writes and motion.
All11settings applied; exact116Bprofile restored SHA2562F1BC38D763A1B7BDBA31F560684FD4AA7E42A714600B8D344F19DA7F38E23B0.
Timestamp inventory0new raw/image/perf/cache/dump files. No unchanged retry.

Read-only source/content scans reuse default.image and all1673installed IPKs;
only1404selected .mdl records (2677866B) decompressed in memory,32MiB/30s limits.
No UVCON,415UVEYE: next connected producer is authored gaze/eye UV, not a presumed
opening-scene joint driver. No assets/tools/caches extracted or written.

Full278014Blog SHA256909AC0C686F112D090A27FA85FD34BBCF08FF7EE31D5B864012A352A53801DD2
retained losslessly as effect187ZIP50311B, SHA256
9861ADCE4B92F7956A40C58A393D4E30657ACCFC500B7114225955E2506FE713.
Sole member/name/length/full decompressed hash verified. Archive allocation:
free63597686784->63597633536B (53248B). Deleted exact plaintext278014B and
superseded effect186ZIP49644B after rechecking hashes,327658Blogical;
free63597465600->63597797376B (331776B observed deletion reclaim). Current log
is recoverable fromZIP; older186text is superseded/not recoverable fromGit.
187preserves the failed gate and now explains absent-driver observations;
retain until purposeful authored-effect replacement.988/985/other protected
failure/pixel evidence unchanged. No image/motion/reload/both-eye/Quest gate waived.

Total cleanup8files332367Blogical;339968Bobserved deletions minus53248Barchive
allocation=286720B net280KiB reclaimed, no prior cleanup credited. Partial
attributed retained growth51026B: fixture+21193B,EXE/PDB+29184B,buildlogs-18B,
newZIP replacing186ZIP+667B; excludes hostobjects/buildmetadata/source/Git.
Closing pre-doc free63597277184B (~59.2GiB),drive-wide gain640430080B versus first
62956847104B; almost all is external activity, NOT claimed cleanup. Diagnostics
78095024B,buildlogs420377B/212files; next400KiBoverlap leaves138576B below75MiB,
fresh preflight required.13retainedwindow images10434657B unchanged. All producers
terminal, none queued.421guards/CPU fixture/host build pass; diagnostic question
answered but no additional renderer dependency removed. Goal remains active.

### 2026-09-08 authored eye owner/material connection after5c80bec

Owner push-only turn changed no renderer state; resumed connected authored gaze
producer -> existing native instance entry -> material import/composition. All
three callers audited, including viewer paths; fixed outputs retain late-write
validation and caller scratch export. No source-address cache or new owner index.
Initial scoped free63589031936B; pre-build63589326848B. No live game/build/test
processes. Same cumulative floor62509998080B,3GiB owner exception and protected
failure/raw/image evidence. Buildlogs420377B/212files; fixture10993413B/43files.
422 artifact-free source/scenario guards PASS0.239s. Existing material74/CPU71/
host188 supervisors planned:300s,64/8/192MiB per-command free-drop ceilings,
fixture expected <16MiB peak, host <192MiB; no guest/shader rebuild intended.
Retain73/70/187 receipts until replacement checks pass. Fresh diagnostic-overlap
measurement required before any bounded authored-eye runtime observation.

Material74/PID32232/session99824 build0; CPU71/PID19680 PASS0.12s/CTest0.14s.
Fixture1491968B SHA25681464302C64F9C5F6EF57EE68F5CAC79B9F83FE4FF61D8FE8CC97FEC52300E98;
tree11088335B/43files (+94922B). Host188/PID27880/session14729 build0;
host/version compilation through link step32 after no-op codegen (0writes), no guest objects or
shader compilation. EXE49372672B,PDB113737728B (+65536B combined),EXE SHA256
7369705DEA777D918C232A2FA442D36AED888D96098D5E45EC85C93D0D4F3D8E.
Source/test58d591a committed/pushed; banner5c80becdirty describes that source.
All producers terminal. Six exact73/70/187 superseded build receipts removed
after replacement,4691Blogical;free63590383616->63590391808B,8192B measured
reclaim. Protected runtime effect187ZIP and all unresolved failure evidence
unchanged. New EyeMaterialProbe reuses existing runtime supervisor:60s,400KiB
text,192MiB free-drop, no raw/image/perf/cook. It enables native scene/skin/
deferred consumers and requires checked off-center gaze plus increasing eye
material/native-packet counts after two idle field contexts. It is NOT the
failed UVCON gate, nor a movement/reload/pixel qualification. Full preflight
must fit before profile mutation/launch; exact profile restoration in finally.

Runtime preflight78098318Bdiagnostics,409600Blog reserve,free63590260736B,
floor63388934144B. Host188/PID38012/session61987 starts23:57:53,terminal23:58:48.
Requested EyeMaterialProbe observation reached; wrapper exits1 via diagnostic
stop, NOT a crash/test-suite pass. Eye2077=976exact matches,refused0,off-center216;
owner1747->2047 reads631->931,changed0; idle field1748->2048 eye material
packets27808->41008. Controller95738matching,late6matching,skeleton12813matching,
wrong0. All14settings applied; exact116Bprofile restored with prior hash. No new
raw/window/perf/cache/dumpfiles; no reload/motion-pixel/both-eye qualification.
Full263502Blog hash1D64BA1D1BA65EFE8B4A3391197E817EBA21593E8C2BB3CEF1481DE4A81F3F75
retained in eye188ZIP54079B,hashF10E5244C5BE0663952AC3E85F54359E271952A9CA687637DFCD4F93ED4A26D2.
Sole member/name/length/full hash verified before plaintext removal. Archive
allocation free63589076992->63589019648B (57344B); exact plaintext deletion
free63589019648->63589285888B (266240Breclaim); net208896B. effect187ZIP remains
protected for its missing-driver gate; other protected captures/failures unchanged.

Final source review moved non-flushing FP setup before native eye evaluation,
with subnormal CPU regression and source-order guard. Material75/PID19296 build0;
CPU72/PID38160 PASS0.12s/CTest0.14s;422guardsPASS0.226s. Same300s/64/8/192MiB
supervisors and cumulative budget. Host189/PID37628/session30178 build0 through
link step12,codegen0writes,no guest objects/shaders. This final binary is
NOT live-qualified by relabelling188. FinalEXE49372672B/PDB113737728B;
EXE SHA2562CB159E945C45BF3A93D9FDCAEBCEC9F4151A0CD0D7D540DDAD92803EE0F3BA1.
Fixture1491968B SHA256F05904CD831F572AFBC27218FE40E43FF6DD74ED8C4A859B54D9B0F796230861;
tree11081699B/43files. Six74/71/188 receipts7985Blogical removed after replacement;
free63454375936->63454392320B,16384Breclaim. Retain75/72/189 and runtimeeye188.

Total13files276178Blogical deleted;290816Bmeasured deletions minus57344BZIP
allocation=233472B net228KiB cleanup. No old cleanup credited. Partial retained
growth207901B:fixture+88286B,EXE/PDB+65536B,ZIP+54079B,buildlogs net0. Excludes
hostobjects/buildmetadata/source/Git. Final pre-doc free63450087424B (~59.1GiB),
drive-wide138944512B loss fromfirst63589031936B; not attributable to the listed
task files. Scoped runtime/cache/dump inventory contains only new54079BZIP;
all game/build/test producers terminal. Do not claim external activity as cleanup.
Diagnostics78149103B,buildlogs420377B/212files; next400KiBoverlap has84497B below
75MiB stop, fresh preflight mandatory.13window images10434657B unchanged; no new
raw allowance or budget reset. No next producer queued; goal remains active.

### 2026-09-09 shared animated-material ownership afterdd6a2dc

Owner-requested dd6a2dc WIP commit/push preserved the unfinished source-only
refactor, with failed old-API guards and pending C++/live qualification disclosed.
Resumed the connected controller -> shared instance material lease -> late eye
patch -> next controller -> actual material composer, retaining outgoing adapters.
Migrated fixture APIs and added sparse slots, signed-zero identity, pinned-byte
budget/backpressure/retirement tests and source-read-prohibiting consumer tests.
423 artifact-free Python guards PASS0.232s; C++/host/live evidence still pending.
Initial measured free63542591488B; fixture11081699B/43files, aggregate buildlogs
420377B/212files. No game/build/test producers live. Same original cumulative
floor62509998080B and owner3GiB exception, no new raw/image allowance. Previous
drive-wide fluctuation is not credited as cleanup; scoped outputs are unchanged.
Reuse material76/CPU73/host190 supervisors,300s and64/8/192MiB free-drop caps;
fixture peak expected <16MiB, host <192MiB. Retain75/72/189 receipts until verified
replacement. Runtime requires fresh diagnostic-overlap preflight and a new
owned-scroll-input/material-packet observation, preserving eye regression gates.

Material76/PID25344/session99135 build0; CPU73/PID29188 PASS0.12s/CTest0.13s.
Fixture1537536B SHA2561FA2EF3A8D9E0D74697877A52F94A7F725D44FA6D5B40EED44F90DE315858DC6;
tree11245373B/43files (+163674B). Host190/PID35952/session69911 build0 through
link step32; bannerdd6a2dcdirty, codegen0writes, no guest objects or shaders.
EXE49382912B/PDB113823744B (+96256B combined); closing build free63549243392B.
Extended existing ignored supervisor with AnimatedUVProbe: requires the existing
EyeMaterialProbe and positive fresh owned-scroll-inputs with no owner refusal,
plus increasing generic UV scopes/composed values/packets in the same two idle
field contexts. No threshold change to prior eye/controller/late/effect gates.
Same60s/400KiB/192MiB/no-capture limits and exact profile restoration. Syntax
check passes; one purposeful new runtime remains pending, not repeated189.

Host190 EXE SHA256829B80D48E8126D14F069F5873F72F049F4323339E151F74A3D7AAAE8ECC1CC7.
Six exact75/72/189 receipts4691Blogical retired after passing replacements;
free63558856704->63558864896B,8192Breclaimed. No protected failure data removed.
Runtime preflight78152397Bdiagnostics,409600Blogreserve,noimage allowance;
free63559176192B,floor63357849600B. PID38184/session94844,00:36:06..00:37:02,
reached requested AnimatedUVProbe with original Eye/Controller/Late checks.
Intentional diagnostic-stop exit1, not crash/CTest pass.1918native scroll inputs,
1922publications,refused0;959exact eye updates,225off-center. Two fresh idle
contexts1662->1962:scopes632->932,packets27808->41008 (+13200). Shared owner
changed0;controller94106matching,late6matching,skeleton12585matching,material
19333checks/wrong0. Live preserved non-eye slots/joint UVs/queued transitions0;
CPU coverage does not qualify missing live routes. No full desktop/pixel gate.
All14settings applied;116Bprofile restored with unchanged SHA256. No new raw,
window,perf,cache or dumpfiles; all producers terminal and none queued.

Full253093Blog hash0AEAAC79E344B9DA6F9D32C113287DFAA2EA3B8DA39D2AE8CF3391A8CC5D53BE
retained in animated-uv190ZIP51516B,hash5CF0DDB4FABA6AEA3850951360417EF4583F415CAC3D2EF31C84CBA9AAC0D7C8.
First compressor command lacked the ZipArchive assembly and created no archive;
corrected assembly import, then full member/name/length/hash validation succeeded.
Archive allocation free63559389184->63559335936B,53248B. Verified plaintext and
now-superseded eye188ZIP307172Blogical removed;free63559335936->63559647232B,
311296Breclaimed. Current complete log recoverable from newZIP; old188log is
no longer retained, with historical report preserved. effect187/placement988/
root985 and earlier pixel/reload failures remain protected. Retain76/73/190
receipts and animated-uv190 until the same purpose is meaningfully replaced.

Total8files311863Blogical deleted;319488Bmeasured deletion minus53248BZIP
allocation =266240B net260KiB cleanup, no prior savings credited. Partial retained
growth260661B (fixture+163674,EXE/PDB+96256,ZIPreplacement-2563,buildlogs+3294);
excludes hostobjects/metadata/source/Git and unrelated drive-wide activity.
Post-cleanup free63559647232B (~59.2GiB),drive-wide+17055744B fromfirst;
only measured deletion/allocation is claimed cleanup. Diagnostics78149834B,
buildlogs423671B/212files; next400KiB overlap leaves83766B below75MiB, requiring
fresh preflight.13window images10434657B unchanged; no new raw/budget exception.
Production source is dd6a2dc; only precondition comments changed after host190.
Connected tests and meaningful README/queue/report updates close this bundle;
authored binding/image/late-writer/source-adapter and full desktop work remain.

### 2026-09-09 bind-owned material descriptors after2691882

Previous bundle made verified progress; current source adds binding-time authored
UV programs to the existing instance registry, consumed by controller/eye math
and existing material values/packets. Original name resolution and late-writer
validation stay explicit; no second index/framework. Reused audited parser and
binder contracts; reread the complete binder plus unchanged pso/frame TOMLs.
424 artifact-free guards PASS0.229s. New CPU chain forbids descriptor/unit reads
inside evaluation; source guards remain separate. Adds descriptor/UV shared-budget,
pinned retirement, late descriptor/unit writes and generation invalidation tests.
Initial measured free63561998336B,buildlogs423671B/212files; no live producers.
Same cumulative floor62509998080B/3GiB exception, diagnostics78149834B before new
receipts; no raw/image allowance. Reuse material77/CPU74/host191,300s and64/8/192MiB
free-drop caps. Estimated fixture peak <16MiB and host <192MiB. Retain76/73/190
receipts and animated-uv190 until corresponding replacements qualify. Runtime
must add positive bind-owned descriptor consumption to existing UV/eye/controller/
late gates, with fresh cumulative diagnostic preflight before any launch.

Material77/PID35780/session66425 build0; CPU74/PID17696 PASS0.12s/CTest0.13s.
Fixture1577984B SHA256C998BD095F1AB1771D01FEC02AB8952C906B1A529929E85C2991766DC8F9A721;
tree11388904B/43files (+143531B). Host191/PID22648/session80746 stopped with
compile error: missing REX_EXTERN original declaration for the new binder hook.
No game run, no guest objects or shader compilation; codegen0writes. Added the
declaration and source regression; host192 is the scoped retry under the same
300s/192MiB/cumulative limits. Existing MaterialProgramProbe extends the ignored
supervisor: positive bound/read descriptors without change/refusal and increasing
effect-slot/eye descriptor consumption after fresh field contexts, AND all prior
UV/eye/controller/late observations. Syntax passes, no old gate is weakened.

Host192/PID36348/session80639 build0 through link18 after the declaration fix;
codegen0writes,no guest objects/shaders. EXE49397760B/PDB113934336B (+125440B);
EXE SHA256FBE640F3A6275CC0A5AD985B7818F6C6DE7826D8D398F4821C9101CF27356229.
424guards pass again0.227s. Source/test checkpoint is build/CPU verified with
live descriptor consumption pending; original program-binding/body and late
source validation remain explicit. Next existing supervisor run requests
MaterialProgramProbe plus all prior animated-UV/eye/controller/late observations,
60s/400KiB/192MiB,no capture/perf/cook,exact116Bprofile restoration. Pre-run
profile hash unchanged; never label host190's evidence as the new binary.

Source/test f6e254e committed/pushed before the changed-code runtime. Eight
superseded76/73/190/191 receipts13487Blogical removed after passing192;
free63555710976->63555735552B,24576Breclaimed. Failed191's missing declaration is
resolved and documented, not an unqualified runtime/pixel failure to preserve.
Runtime preflight78146305Bdiagnostics,409600Blogreserve,noimages,free63554990080B,
floor63353663488B. Host192/PID30080/session13730,01:03:47..01:04:41, requested
MaterialProgramProbe reached WITH previousUV/eye/controller/late gates. Intentional
diagnostic-stop exit1, not crash/CTest pass.5binding publications,1878reads,
changed0/refused0;1920effect-slot/957eye descriptor evaluations;1914owned scroll
inputs,231off-center eye controls. Fresh idle contexts1638->1938 material packets
27808->41008. Missing joint-driver/cue/non-eye-preservation live coverage remains0.
All14settings applied,exact116Bprofile restored; no new raw/window/perf/cache/dump.
Original name resolution and per-update descriptor comparison remain source
adapters; no full-frame, motion-pixel, reload/stereo or Quest acceptance claimed.

Full254910Blog hash20E47B916951C82DBB0BCD0BC4E01B7EF52675CDA02138C29034F9EC9657C3F7
retained losslessly in material-program192ZIP51869B,hash88080D12C4BA4DF855E2BA77B18BB1272E66D345B608464E5D0053A69DC2C3FF.
Full member/name/length/hash verified; archiveallocationfree63553740800->
63553687552B,53248B. Plaintext plus supersededanimated-uv190ZIP306426Blogical
removed:free63553470464->63553781760B,311296Breclaimed. Current log recoverable
from newZIP; old190log no longer retained, historical report preserved. Keep
effect187/placement988/root985 and earlier unresolved pixel/reload evidence.

Final explicit descriptor wrong-generation/reload/count/missing-input regressions
change tests only. Material78/PID4232 build0;CPU75/PID31084 PASS0.12s/CTest0.14s,
same64/8MiB/300s/cumulative caps. Fixture1580032B,tree11399067B/43files;
fixture SHA256039F960E4808088AEE7F40046A2EB9C1798BD943746B434E7875D572FD6565E9.
Four77/74receipts2331Blogical removed after replacement;free63556743168->
63556747264B,4096Breclaimed. No host rebuild/run restamp for test-only additions.

Total14files322244Blogical deleted;339968Bmeasured deletions minus53248Barchive
allocation=286720B net280KiB cleanup, no old savings credited. Partial retained
growth275790B (fixture+153694,EXE/PDB+125440,ZIPreplacement+353,buildlogs-3697),
excluding hostobjects/metadata/source/Git and unrelated drive activity. Post-cleanup
free63556747264B (~59.2GiB),drive-wide5251072Blower thanfirst63561998336B;
only measured deletion/allocation is task cleanup. Diagnostics78146490B,
buildlogs419974B/212files; next400KiB overlap87110B below75MiB, fresh preflight
mandatory.13windowimages10434657B unchanged, no new raw/budget exception.
All producers terminal, none queued. Retain final78/75/192 and material-program192.
README/queue and research/20260909_0104_native-material-uv-programs.md record the
connected outcome, remaining writers/validation and unchanged full desktop goal.
