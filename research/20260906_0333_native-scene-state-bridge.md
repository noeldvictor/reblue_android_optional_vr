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
