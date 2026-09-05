# Explicit native post image flow

2026-09-05, Windows Vulkan desktop, EDT. Base `d9a3785`.

Previous goal turn made progress: direct scene handoff removed both guest
temporary-container lifecycles and passed flat/both-eye normal verification.
Guest-source and devloop skills read completely. No delegation or device work.

## Source and implementation scope

Trace `native_scene_pass_bridge.cpp::End`, `resolve.cpp::PublishSceneOutput`
and `CanAliasResolveLocked`, `native_post_bridge.cpp::RunEffectSequence` /
`RenderPostPlan`, and `post_chain.cpp` input, atlas, composite and compatibility
consumers. Scene publication can either alias unscaled colour with exposure
(.25 for HDR), or materialize an already-scaled image (MSAA/size/format cases).
Each post root previously followed that link, rendered, detached it and
republished its output to the scene getter; the next root rediscovered it.

Introduce explicit sampled colour/depth/exposure for the native renderer,
import the current scene/getter boundary once, and pass actual completed
outputs between roots. Exposure applies only to the first stage. Publish to
the remaining UI/getter adapter once after all roots finish. Separate native
atlas production from the retained compatibility pyramid/slot path. Do not
change shaders, authored focus ordering or shadow/asset recipes. Borrowed
inputs are synchronous; caller-owned scene images and RAII-held native output
targets remain alive throughout submission, with no cross-frame pointer cache.

Scene-end publication, initial resolve-link import, UI/presentation getters,
engine properties and the remaining parent scheduler are still migration
boundaries, not a completed native frame. The explicit input API is also the
required next boundary for direct native scene completion records.

## Cumulative storage gate

Initial actual free: 54,376,407,040 bytes (50.64 GiB). Reuse existing host/test
trees; planned peak build/link growth <=1 GiB, expected reserve >49 GiB,
required reserve 20 GiB. No hook/codegen input, asset, dependency or shader
change planned. Capture-free diagnostic timeout 75 seconds, explicit delay
zero and effective configuration audit. Cumulative logs/perf/small exports
cap 100 MiB. No incoming raw allowance until fresh eligible cleanup; previous
turn's recovery is fully spent and cannot be credited again. Current baseline
is `native_scene_handoff_flat` / `_vr`; preserve it and protected evidence.

## Build and low-storage checks

Native explicit inputs, a separate direct atlas producer, direct optical/noise
sampling and one final publication are implemented. Compatibility DoF retains
its original getter cleanup identity and optional non-atlas slot pyramid;
normal post never enters that path. Input exposure is finite/positive checked,
the scene import issues no GPU work, and complete list preflight / ordered
focus publication / no partial replay remain intact.

All 30 CTests and 42 source guards pass (29 post, 10 scene, 3 reflection).
The sequence test covers counts 1..64, alternating image reuse, distinct eye
values and once-only HDR exposure against an already-exposed reference.
The first focused build used the CTest name instead of its executable target;
it produced no output. The corrected restricted-runner Ninja process stalled
without compiler children. Inspected the live process tree, deliberately
stopped only owned Ninja PID 7532, observed session termination, and then
rebuilt `host_post_sequence_test` outside that restriction. No duplicate tree
or concurrent retry. CPU tests are not multi-root GPU/pixel qualification.

Host build succeeded, codegen wrote zero files (module up to date); no guest
TU, shader, asset or dependency rebuild. Binary 47,526,400 bytes, linked
19:34:31 EDT, embedded `d9a3785b9` plus local changes, SHA-256
`e35a531f2be497edddccd3965120ab3fae00cff6e6ecfafd624b77cd66cde322`.
Post-build actual free 54,376,042,496 bytes, net growth 364,544 bytes from
preflight, within the peak plan. Incoming raw produced zero. Next: bounded
capture-disabled diagnostic, then fresh eligible cleanup before images.

## Fresh image budget and retention decision

The current handoff normal pair supersedes the older sequence normal pair for
the same field test. Reviewed both retention records. Remove only 120 raws
each from `native_sequence_flat` (`frame_1788649133_0.raw` through
`frame_1788649137_119.raw`, 995,330,400 bytes) and `native_sequence_vr`
(`frame_1788648965_0.raw` through `frame_1788648973_119.raw`, 2,189,724,000
bytes), plus their exact automatic hard links. Preserve all eight PNGs, logs,
reports, current handoff controls, readiness startup, effect previews/shared
probes and unresolved failures. These historical raws cannot be recovered.

Reuse the exact-path/NTFS-link/ancestry/reference validator, with this pair's
names and endpoints; require the renderer stopped. Measure actual recovery
before capturing `native_post_image_flow_flat` / `_vr`, 120 frames each,
3,185,054,400 incoming unique raw bytes total. The new allowance equals only
this fresh removal; no historical credit, retry or archive growth. The same
100 MiB cumulative small-output cap includes diagnostic/perf and endpoint
analysis, with no every-frame PNG export. Runs stop at completion or 110 seconds,
retain original profile and process-local runtime rules. Review the handoff
controls at the next replacement checkpoint, not before inspecting this pair.

Diagnostic log 810, PID 26344, 19:37:43-19:39:00: all five settings audited,
delay zero and zero raws. Last sample 3,601 native sequences/roots, image
imports and final publications, zero original container/wrapper/post scopes
or refusals. Maximum one root; direct inter-root edges zero, so no multi-root
GPU claim. First three startup atlas samples show explicit exposure 1; they
do not establish later HDR-exposure coverage. Checked runtime error markers
absent. Owned process and runner terminal; no restart from a polling timeout.

Fresh cleanup completed: 240 unique raws / 480 exact paths, 3,185,054,400 raw
bytes removed. All eight PNGs/reports retained. Actual free 54,378,684,416 ->
57,564,717,056; recovered 3,186,032,640 bytes (2.97 GiB). Incoming raw consumed
zero; remaining allowance exactly 995,330,400 flat + 2,189,724,000 VR, no
retry. Final net disk change will include those replacements and small outputs.

## Capture receipt and documentation handoff

VR runner session 35542 completed successfully; owned PID 26676 terminated at
19:42:33. Log 811 audits all 16 temporary settings. Existing isolated set
`out/verification/native_post_image_flow_vr` contains 120 raws totaling
2,189,724,000 unique bytes; automatic paths are hard links, not another payload.
Runner-reported ending free space was 55,371,874,304 bytes. Fresh raw allowance
consumed 2,189,724,000; remaining 995,330,400 bytes, reserved for the planned flat
set, not a retry allowance. No flat run has started. Pixel analysis, both-eye
inspection and final runtime qualification remain pending; completion of capture
alone is not a visual pass. Preserve the existing set for analysis, do not rerun.

At the owner's storage-instruction update, checked the completed session and
confirmed no running `reblue_vk` process. Restored the original five-setting
profile (capture delay 60, minimum draws 600, count 120; autoplay/perf enabled).
No new build, capture, analysis export or deletion was needed for this docs edit.
`AGENTS.md` now explicitly covers interrupted-job storage accounting, duplicate
producer prevention and guaranteed cleanup of temporary capture overrides.

## Resumed VR inspection and flat preflight

Previous goal turn made progress: `8c45d12` strengthened storage rules, recorded
the completed capture and restored the profile. Resume the same checkpoint and
remaining allowance, not a new budget. Existing VR analysis session 99763 is now
terminal; no renderer or capture was relaunched to replace the existing set.

All 120 VR frames were analyzed in bounded memory. First
`frame_1788651740_0.raw`, last `frame_1788651750_119.raw`, render frames
7708..7827, stacked 1440x3168: 0/119 changes above 6%, maximum 0.475698829%
(pair 81); cyan 0/120, maximum 0%. First and last stereo bands both show far
-1 px / near -9 px, correctly crossed, spread 8 px. Inspected all four full
eye endpoints: coherent village, stairs, rocks and moving windmill shadows;
existing distant blur remains. This framing does not qualify character/shadow
alignment. Six endpoint/overview PNGs total 6,024,124 bytes, no every-frame export.

VR last sampled counters: 7,801 native scopes, sequences, roots, direct handoffs,
scene imports and final publications; zero original scopes/container wrappers,
sequence or input refusals. Maximum roots 1, direct inter-root images 0; CPU
multi-root exposure tests are not GPU qualification. Logs 810/811 both mount
1673 archives / 119346 names; 811 confirms the 1440x1584 OpenXR session and eye
offset distinct from the game camera. Checked error/critical/device-loss/fatal/
exception/assertion markers absent. Binary hash unchanged from the build above.

Before the flat run, actual free 55,353,761,792 bytes. Logs/perf from 19:29
onward total 2,676,243 bytes before flat, plus the six PNGs and small helpers,
well within the shared 100 MiB allowance. Remaining raw budget 995,330,400 bytes
exactly covers one 120-frame 1920x1080 set; peak incoming raw plus the entire
small-output cap leaves >50.5 GiB free. Rechecked original profile: five settings,
delay 60/minimum 600/count 120. Reuse the bounded runner and existing binary;
flat stops on a complete set or 110 seconds. No additional raw retry allowance.

## Flat qualification and superseded-control review

Flat session 30157 / owned PID 26780 completed, 19:49:58-19:51:05, log 812.
All five original settings audited; 1673 archives / 119346 names mounted.
Last sample 3,001 native scopes, sequences, roots, direct handoffs, scene imports
and final publications; zero original scopes/wrappers/container paths, input or
sequence refusals. Maximum one root, no direct inter-root edge GPU coverage.
All three logs have zero checked runtime error markers. Thirty CTests and all
42 source guards rerun successfully without rebuilding. A first reflection
test invocation used a nonexistent filename; corrected to the discovered
`tools/reflection_lock_order_test.py` (three tests pass), no generated outputs.

Flat 120 raws: `frame_1788652260_0.raw` through `frame_1788652264_119.raw`,
render frames 2845..2964, 1920x1080. Streaming check: 0/119 changes above 6%,
maximum 2.847608025% (pair 0); no cyan hits, maximum .022665895%, median
.011598187%. Full first/last images inspected: recognizable Shu with cast
silhouette, foliage, ground and changing windmill shadows. Existing distant
DoF remains. This qualifies only the normal short field window, not full-game
coverage, authored multi-root events, title artwork or the unresolved late scene.
Runner/profile/process checks are terminal/restored; no Quest/Thor run.

Before further retention cleanup, exact scoped NTFS inventory is unchanged:
29,565 raw paths / 28,331 unique payloads, 263,324,814,100 logical bytes and
233,564,594,672 allocated bytes. The first fresh removal was fully spent on
equal-sized replacement captures; it is not credited again. The new normal
pair has passed streaming, full endpoint and both-eye review, so it is now the
baseline. Reviewed the complete scene-handoff worklog and retention condition:
its normal pair is now superseded for the same short field question, not needed
as complete raw sequences. Preserve its eight PNGs/reports/logs and all distinct
startup, authored/synthetic previews/shared probes and unresolved failures.

Remove only `native_scene_handoff_flat` raws (`frame_1788650556_0.raw` through
`frame_1788650559_119.raw`, 995,330,400 bytes) and `_vr` raws
(`frame_1788650414_0.raw` through `frame_1788650422_119.raw`, 2,189,724,000
bytes), with their exact automatic hard links. Reuse the reviewed validator
in `out/verification/cleanup_verified_post_controls.ps1`: dry run first,
exact endpoints/counts/sizes, no reparse ancestry, exactly two NTFS links per
payload, all same-name references checked, renderer stopped. Historical pixels
will be unrecoverable; small visual evidence remains. This further cleanup
funds no new producer in this checkpoint; measure recovery and final net usage.

## Final retention and checkpoint result

Second cleanup completed after a successful read-only validation, then full
revalidation with deletion enabled: 240 unique payloads / 480 exact paths,
3,185,054,400 logical bytes removed. Measured volume free
54,351,921,152 -> 57,537,953,792 bytes: recovered 3,186,032,640 bytes (2.97 GiB).
Both this pair and the earlier removed sequence pair now have zero raws and
retain all 16 PNGs. Reports/logs retained; historical removed pixels unavailable.

Retained current baseline `native_post_image_flow_flat` / `_vr`: 240 unique raws /
3,185,054,400 bytes, eight endpoint/stereo PNGs / 9,780,919 bytes, three app logs /
972,952 bytes and six perf files / 2,462,032 bytes. Runtime small evidence totals
13,215,903 bytes; helpers, CTest logs and research are small text, together well
below the shared 100 MiB cap. No assets, downloads, full backups, new build trees
or every-frame PNG export. No active renderer/analysis session remains; original
five-setting profile and the tested executable hash were checked unchanged.

Final scoped NTFS inventory: 29,085 raw paths / 28,091 unique payloads,
260,139,759,700 unique logical bytes, 230,379,540,272 bytes reported by
GetCompressedFileSizeW after identity deduplication. Relative to the initial
archive, exactly 240 fewer unique raws and 3,185,054,400 fewer logical bytes.
This still exceeds the 10 GiB target; no historical blanket exemption or new raw
producer is scheduled. The original capture allowance is fully spent. Further
capture plans must reconcile the current archive and storage ledger first.

Keep the new baseline's complete normal sequences until equivalent verified
replacement allows cleanup; preserve their small reports/endpoints afterward.
Readiness early-startup, grading startup controls, all distinct authored/synthetic
effect previews/shared probes and unresolved late-scene/failure evidence remain
protected, pending equivalent qualification or resolution. Review these named
categories before future captures; this checkpoint does not grant indefinite
retention or extra capture allowance for the entire historical archive.

Ending actual free 57,537,957,888 bytes (53.59 GiB). From the original preflight
54,376,407,040 bytes, net volume usage fell 3,161,550,848 bytes (2.94 GiB).
Both cleanup receipts are gross; equal replacement raws consumed the first one.
Net change also includes build/small-output and unrelated volume activity, not
an exact repository artifact sum. No further large producer under this budget.

The guest-source skill anchored the existing instruction/lifetime boundaries;
devloop reused the built binary/test trees and restored the profile; vrsim and
local raw analysis qualified the two desktop eyes without any device access.
This is progress, not completion: next replace the initial scene resolve/getter
import with explicit native completed-scene ownership and downstream consumers,
then remove final UI publication and the remaining parent scheduler/engine
producers. Multi-root/unknown-callback GPU cases, HDR image flow, authored effect
events, full native animation/material/light data, title artwork, VR blur,
late-scene failures and representative fields/battles/cutscenes/menus/transitions/
reloads in both eyes remain required before Quest qualification.
