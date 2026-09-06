# Native scene write commands and scope-owned clears

2026-09-06; unfinished desktop renderer transition, no Quest qualification.

Previous goal turn made progress: `21d1c99` / `893b0eb`, native framebuffer
integration, CPU/host/source verification, four bounded diagnostics, inspected
flat PNG and measured cleanup. Rechecked current worktree and read AGENTS,
guest-source/devloop/vrsim skills, current transition scope and latest evidence.
Root/Plume publication still requires explicit approval after the earlier denial;
no push or parent gitlink commit is attempted.

## Change and boundaries

Scene scopes now prepare explicit native source/resolve write layouts, first-use
discards, framebuffer binds and typed colour/depth/stencil clear values. Scene
begin no longer calls Video::RequestClear or stores its clear on binding headers.
The native bind skips generic target aliasing, seed copies, compatibility layout
selection and the scene-bind tile-chain-head update. Source/resolve owners retain
the exact images/layout records. Rebinding a scope does not discard or clear it
again; an empty scene binds and clears before exposing outputs. No asset copies,
extra GPU images, generated-code edits or shader changes.

Inspected the scene begin's translated colour/clear/state sequence in
generated/reblue_recomp.53.cpp, the scene-post hook map, bdSetRenderState in
generated/reblue_recomp.24.cpp, and frame opening/binding/clear/resolve code.
State-308 calls remain explicit compatibility work, not claimed eliminated.
Frame-opening/getter cleanup remains in the boundary; other legacy clear producers
remain supported and counted. Sampled-input/resolve-link consumers, complete draw
state/pass execution, engine scene traversal, remaining getters/scaling, UI/frame
scheduling and the full fields/battles/cutscenes/menus/transitions/reloads/both-eye
desktop gate are still open.

## Original cumulative storage plan

Original checkpoint stays 2026-09-05 20:47, 65,462,788,096 B free, 2 GiB peak growth,
100 MiB retained diagnostics including tools, 10 MiB aggregate build/test logs,
20 GiB reserve. Current turn's 02:55 free reading: 64,631,410,688 B. Prior closing
diagnostics total 55,379,622 B including the unchanged 41 MiB tool/inspection
reservation; two current flat PNGs total 6,606,341 B. The prior unattributed volume
growth stays charged, not credited away. No new raw-capture allowance.

Reuse existing CPU/host trees and guarded wrappers, <=512 MiB additional build/link
overlap and <=16 MiB diagnostics. Focused CPU/source tests first. Plan normal
MSAA/non-MSAA flat and XR plus MSAA post-disabled recovery, with up to two bounded
flat PNGs in the unchanged 4 MiB individual / 10 MiB aggregate reservation. Inspect
each before retiring its matching predecessor. Retire only superseded successful
small diagnostics after equivalent verification and exact file/producer checks.
No downloads, new trees, guest rebuild, raw capture, or Quest run.

## Verification

23 scene + 36 post source guards pass (59 total); diff checks pass. CPU command
tests added to existing post-output target cover mono/stereo, 1/2/4/8 samples,
exact native layouts, barrier/discard/bind/clear order, zero-draw clear commands,
resumed LOAD without a repeated clear, next-scope persistent-image clears and
malformed/feedback/layout-alias recipes.

Focused CPU build `host_post_output_test_09`, PID 23756, failed on a test-recorder
type typo (`RenderBarrierStage` instead of flags type `RenderBarrierStages`), before
GPU testing. Fixed it; `host_post_output_test_10`, PID 19644, exited 0 (two steps).
`cpu_13`, PID 24596, exited 0: 31/31 in 3.33 s. Free after CPU tests
64,633,802,752 B. These are real recipe/command classes with CPU interface doubles.

Host `reblue_15`, PID 27100/session 18455, exited 0. Final displayed step 17/20
links the renderer; codegen reports the module up to date and no guest objects
rebuilt. Existing CRT warnings/new-header glob recheck only. Free after build
64,633,565,184 B. Binary linked 03:07:15, 47,636,992 B, SHA256
`19f6207255a29b3be6940906fde96fc58909f87017a87ff5126a1f7ddcbd841a`;
root `893b0eb` plus dirty renderer work, local Plume `81bdca8`.

Window wrappers now admit explicit MSAA/single-sample command-check filenames,
with the same configuration matching, no-overwrite and byte limits. First PNG
must fit the remaining 3,879,419 B aggregate space. Inspect/retire its matching
predecessor before recording the second. Five runs and two PNGs share the original
16 MiB diagnostic plan; no new raw allowance or full pixel qualification implied.

## Runtime and bounded images

Normal MSAA flat: PID 24308/session 52932, 03:08:39--03:09:55; wrapper exit 0,
five effective settings, zero new raws and exact profile restoration. Log 847:
235,774 B. Final reported native command binds/clears 3,300, compatibility clears
0; native depth/deferred colours 3,300, compatibility depth/recovered colours 0.
Native post 3,301 with imports/original scopes/refusals 0. Ending free
64,628,649,984 B. This is bounded owned-process termination, not natural exit/VVL.
Build 15 logs are 5,468 B stdout / 18 B stderr.

Inspected `native_scene_commands_msaa_window.png`: 1920x1080, 3,321,923 B,
SHA256 `6dbd974a6978d5c880d466a55fff4bfada8f094dedddd7a4d1296a7e6c7f8fed`,
owned PID at 03:09:40. Shu/field geometry, foliage, shadows and distant DoF visible
without obvious full-frame corruption. Unaligned single-image sanity only, not
temporal/event/stereo/full-game qualification.

After checking old/new hashes, expected old length, regular-file status, exact
resolved workspace path/no reparse ancestors, and elevated CIM confirming no live
producer, removed `native_scene_source_allocation_window.png` (3,250,318 logical B).
Immediate free 64,628,494,336 -> 64,631,746,560 B: **3,252,224 B measured reclaimed**.
The old report/hash remain, not the PNG verbatim; an equivalent bounded diagnostic
is reproducible. No raw/failure/game/save/profile/source/build data removed. This
frees the existing PNG reservation for the planned non-MSAA replacement.

Normal non-MSAA flat: PID 25612/session 25054, 03:10:43--03:11:59; wrapper exit 0,
six effective settings, zero raws and exact profile restoration. Log 848:
244,524 B; 3,600 command binds/native clears/depth handoffs/deferred colours,
compatibility clears/depth and recovered colours 0. Native post 3,601, imports/
original scopes/refusals 0. Source store 6 / 7,194 / 4 / 2, 33,177,600 payload B;
native framebuffers 3 / 3,597 / 2 / 1; post pool 6 / 3,594 / 4 / 2, same payload
bytes; all refused/failed 0. Ending free 64,627,081,216 B.

Inspected `native_scene_commands_single_window.png`: 1920x1080, 3,334,156 B,
SHA256 `198bd91244699b157bb73d499c20fb518635f878a4f13bf854b879aa0a91e472`.
Shu, terrain/foliage, shadows and distant DoF visible without obvious full-frame
corruption; again only unaligned flat sanity, not temporal/stereo/game evidence.
After the same exact path/regular-file/no-reparse/hash/producer checks, removed
its superseded `native_scene_framebuffer_single_window.png`, 3,356,023 logical B.
Immediate free 64,617,631,744 -> 64,620,990,464 B: **3,358,720 B measured reclaimed**.
Old report/hash remain; the old PNG is no longer available verbatim, but an
equivalent check is reproducible. Protected data untouched. The two current PNGs
total 6,656,079 B; no more are planned this turn. A further roughly 9 MiB volume
decrease between the run and cleanup is not credited away; it remains in the
original cumulative budget and is investigated with scoped output inventories.

Scoped native-test/install inventories found only expected runtime logs/perf and
the temporary profile, no new raw/cache/dump outputs explaining that full volume
change. It remains unattributed and charged. Flat perf files: MSAA
`perf-20260906-030842.csv` 598,016 B and non-MSAA `perf-20260906-031045.csv`
614,400 B, each with 112 B metadata.

Normal MSAA XR: PID 23732/session 84695, 03:12:40--03:13:56, wrapper exit 0,
16 effective settings, 1440x1584/eye, multiview, zero-height xrsim, render scale 1,
mirrors/previews off. Log 849: 548,572 B. Native command binds/clears, resolves,
depth handoffs and deferred colours 9,600 each; compatibility clears/depth and
recovered colours 0. Native post 9,601 with imports/original scopes/refusals 0.
Source store 6 / 19,194 / 4 / 2, 291,962,880 payload B; resolve store 3 / 9,597 /
2 / 1 and post pool 6 / 9,594 / 4 / 2, each 72,990,720 B; refused/failed 0.
Zero raws, exact profile restoration, free 64,618,876,928 B. This is runtime
execution evidence, not new stereo pixel qualification or game VVL.

Normal non-MSAA XR: PID 5352/session 64852, 03:14:14--03:15:30, wrapper exit 0.
After the instruction-file edit (`da1e3cf`), resumed this exact handle and obtained
its terminal result; no duplicate run was launched. All 17 settings took effect;
XR geometry/settings match the MSAA run except `bd_msaa = 0`. Log 850: 574,103 B.
Command binds/native clears/native depth/deferred colours 10,200 each, compatibility
clears/depth and recovered colours 0. Native post 10,201, imports/original scopes/
refusals 0. Sources 6 / 20,394 / 4 / 2, 72,990,720 payload B; framebuffers
3 / 10,197 / 2 / 1; post 6 / 10,194 / 4 / 2, same payload bytes. Refused/failed
0. Zero raws, exact profile restoration, ending free 64,616,087,552 B.
MSAA XR perf `perf-20260906-031242.csv`: 1,609,728 B; non-MSAA XR
`perf-20260906-031416.csv`: 1,753,088 B; each has 112 B metadata.

Before the remaining recovery run, rechecked the actual binary hash, clean local
Plume `81bdca8`, idle producers, original profile and cumulative outputs. Free
64,613,707,776 B; retained build/runtime/perf logs 163,076 / 4,871,311 /
13,542,944 B, plus the unchanged 41 MiB reservation. Reused the original five-run
plan and guarded wrapper, with no rebuild, capture, download or budget reset.

MSAA post-disabled flat recovery: PID 8784/session 84549, 03:23:49--03:25:06,
wrapper exit 0; all six settings took effect. Log 851: 255,542 B. Native command
binds/clears/depth/deferred colours 3,600; all 3,600 colours recovered. Compatibility
clears/depth 0. The explicitly disabled native post runs 3,600 original scopes;
its setting refusals are expected, memory/effect/input refusals 0. Sources
6 / 7,194 / 4 / 2, 132,710,400 payload B; resolves 3 / 3,597 / 2 / 1,
33,177,600 B; refused/failed 0. Perf `perf-20260906-032352.csv`: 622,592 B,
metadata 112 B. Zero raws and exact profile restoration, ending free
64,612,261,888 B. All five checks use the same binary; bounded wrapper termination
does not establish natural shutdown, Vulkan validation or full-game correctness.

Re-ran the 23 scene / 36 post source guards with Python bytecode disabled: all
59 pass. Rechecked the existing 31/31 CPU result and source diff; no new host/CPU
build is warranted for these evidence/documentation updates.

## Completed retention review and cumulative ledger

After equivalent replacement checks, validated exact paths, expected byte lengths,
Git ignore status, regular files and no reparse ancestors for 23 superseded small
diagnostics. Elevated CIM confirmed no renderer/build/compiler/linker producer.
Removed only these identified agent-created outputs:

- Build logs (stdout/stderr pairs): `attachment_resolve_reblue_14`, `cpu_12`,
  `host_post_output_test_08` and `host_post_output_test_09` in `out/verification/`.
  The last pair records the fully explained, fixed CPU recorder type typo above,
  not an unresolved renderer failure.
- Runtime logs in `out/build/win-amd64-release/logs/`: `reblue_840.log`,
  `842`, `843`, `844`, `846`. Replacements are respectively `849`, `851`,
  `848`, `850`, `847` for the same diagnostic purpose/configuration.
- CSV/metadata pairs under that log directory's `perf/`:
  `perf-20260906-022004`, `022333`, `024351`, `024547`, `024928`.

These 23 files total 7,134,110 logical B. Immediate free space
64,612,261,888 -> 64,619,413,504 B: **7,151,616 B measured reclaimed**.
Their old reports remain and equivalent diagnostics are reproducible; the exact
deleted files are no longer retained. Alongside the two previously completed PNG
replacements above, this native-command checkpoint removed **25 files /
13,740,451 logical B / 13,762,560 B measured reclaimed**, counted once.
No game data, saves, profiles, source, dependencies or build trees were removed.
Keep log 845/non-MSAA recovery, log 828/optical coverage, tiny GPU-fixture evidence,
the current raw baselines and all unresolved-failure evidence; none was replaced
by this checkpoint's short checks.

Closing scoped inventory at 03:28: 74 build logs / 128,606 B; 10 runtime logs /
3,262,461 B; 20 perf files / 8,930,400 B; zero new cache/dump files. Including the
unchanged 41 MiB tool/inspection reservation, retained diagnostics total
55,313,083 B. Both current PNG hashes still match and their 6,656,079 B total is
inside that reservation. Zero automatic raw files were created since the original
20:47 checkpoint. The historical unique raw archive remains over budget, with
zero incoming allowance; it was not relabelled or credited away.

Ending free 64,619,413,504 B (60.18 GiB): net growth 11,997,184 B from this
native-command work's 02:55 reading, and cumulative growth 843,374,592 B from the
original 65,462,788,096 B checkpoint. Both remain charged, including unattributed
volume movement; these figures are volume deltas, not precise producer attribution.
No producers remain and the original five-line owner profile is restored. Current
small runtime/PNG evidence can retire only after an equivalent verified replacement;
protected raw/failure sets keep their existing review triggers. Commit metadata
may cause a small subsequent volume change; it does not reset the ledger.

## Remaining source lead and publication boundary

Read the scene clear sequence at `generated/reblue_recomp.53.cpp` (0x82186E3C
through 0x82186E90), `bdSetRenderState` in `reblue_recomp.24.cpp`, its host hook
and native raster bridge, and `config/hooks/render_tweaks.toml` before the scene
post callback. State 308 still dispatches through the device table at
`device + 56 + 308` after an engine-cache comparison. Offset 308 alone does not
prove the callback or all getter/dirty-state consumers, so it was not guessed
away as an MSAA toggle. The next conversion needs that exact binding/consumer
trace and a native state contract, not a renamed compatibility call. No generated
code, persistent profile or guest data was edited for this investigation.

Only the independent native command header, its CPU test and README/transition/
evidence documentation are checkpointed. Renderer integration, source guards and
the unpublished Plume gitlink remain uncommitted, as before; there is no push
retry or claim that the dependency publication blocker is resolved. Complete host
draw/pass execution, scene/UI/frame ownership and the original desktop/Quest
acceptance requirements remain active, not redefined around this checkpoint.
