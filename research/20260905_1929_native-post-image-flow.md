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
