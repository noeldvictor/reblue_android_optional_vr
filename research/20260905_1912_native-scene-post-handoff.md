# Native scene-to-post handoff

2026-09-05, Windows Vulkan desktop, EDT. Base `6231c4c`.

## Scope and source evidence

Previous goal turn made progress: cumulative storage rules were updated and
pushed. Guest-source and devloop skills read completely; no delegation/device
work. Current normal baseline remains `native_sequence_flat` / `_vr`.

Read the complete post setup/cleanup/epilogue region of `bdRenderViewSubmit`
(`generated/reblue_recomp.16.cpp`), its entry and main scene descriptor setup,
and the complete `sub_8221C9A0` constructor (.57.cpp). The view input r18 holds
colour/depth getters at +0/+4; each getter's texture is at +4. The scene begin
adapter consumes those same fields. The post constructor copies that handle
to +0 and allocates surface-level wrappers and binding-index vectors, all
destroyed after the effect sequence. The scene owner already retains the
image for the synchronous submission, so those temporary wrappers are redundant.

Hook before 0x821865B8, after final focus store 0x821865B4. Import the explicit
scene/depth images once and run the native sequence, whose core no longer
accepts container addresses. On success jump to 0x821867E8, beyond both full
destructors. That block reloads saved flags and their owner before its calls;
it consumes neither skipped temporary data nor condition-register state. The
skipped GPR-allocation restore is already `BD_NOOP` in `gpu/hooks/device.cpp`.
Refusal leaves PPC registers and temporary stack memory unchanged; complete
list preflight and no partial replay retain the existing sequence contract.

This removes two guest container lifecycles and the normal wrapper invocation,
not scene-output getters, resolve-link/exposure publication, the engine list /
camera/focus producers, UI or the remaining parent rendering scheduler. Those
are still migration boundaries; full desktop/game/both-eye gates remain open.

## Cumulative storage ledger / initial gate

Starting measured free: 54,395,469,824 bytes (50.66 GiB). Reuse the existing
desktop/test trees. Planned peak build/link/test additional space <=1 GiB;
expected reserve >49 GiB, minimum 20 GiB. Hook TOML is a legitimate codegen
input change: expect only affected partition/header/metadata regeneration,
not an unrelated full guest rebuild. Inspect emitted build work.

No new raw capture allowance: historical archive remains frozen over budget;
previous cleanup savings were fully spent. Early diagnosis explicitly disables
captures, stops owned processes at 75 seconds, and shares a cumulative 100 MiB
cap for logs/perf/small reports. No asset cooking/download/copy/duplicate tree.
Reconcile actual free space and outputs before another large producer. Fresh
eligible cleanup is required before any new image sequence.

## Source/build verification

All 30 existing CTests and 39 source guards pass (26 post, 10 scene, 3
reflection). The new optional owned-code test maps every PPC comment and
checks all branch labels, exact entry/focus/exit addresses and both skipped
constructor/release calls. The runtime-independent hook guard verifies its
configuration and the post-epilogue GPR no-op. This is not pixel qualification.
The first test attempt found Python lacks `tomllib`; the guard now uses only
the standard-library text/regex helpers already supported here, no download.

Build succeeded: codegen wrote one partition, 218 files unchanged, no deletes.
Only guest partition 16 rebuilt; no other guest TU or shader/asset generation.
Generated output shows the hook before the depth-getter load, jumping directly
to `loc_821867E8`. Host target remains the configured Vulkan/OpenXR executable.
Binary 47,522,304 bytes, linked 19:15:17 EDT; embedded base `6231c4cfb` with
local changes. SHA-256:
`8adfecfb06d530ebb662d18910aaf0380f48cbf1432196a48e9bf13f2390b6a1`.

Post-build free 54,394,572,800 bytes; net volume growth 897,024 bytes from
initial preflight, within the 1 GiB peak plan. Raw output remains zero.
Next is a bounded no-capture diagnostic using the existing runner with Count=0;
its older cumulative log cutoff is stricter than this checkpoint's 100 MiB cap.
Runtime/image verification and final retention pending.

## Fresh capture budget / cleanup decision

Read the full readiness worklog and current effect-sequence retention record.
The current sequence normal pair supersedes readiness normal flat/VR for the
same short field scene; readiness's distinct early-startup probes remain
protected. Reclaim only the two older normal sets' 120 raw payloads each and
their exact automatic hard links, keeping all eight PNGs, reports and logs:

- `native_readiness_flat`: `frame_1788647193_0.raw` through
  `frame_1788647196_119.raw`, 8,294,420 bytes/frame, 995,330,400 unique bytes.
- `native_readiness_vr`: `frame_1788647050_0.raw` through
  `frame_1788647060_119.raw`, 18,247,700 bytes/frame, 2,189,724,000 unique bytes.

Validate all exact paths and workspace ancestry, reject reparse points, verify
each NTFS link pair and all same-name references, and require the renderer
stopped before removal. Retain current sequence controls, readiness startup,
all effect previews/shared probes and unresolved failure/control evidence.
Historical raw pixels cannot be restored; new runs are not identical recovery.

Only after fresh measured reclamation: `native_scene_handoff_flat` / `_vr`,
120 frames each, total 3,185,054,400 unique raw bytes, exactly matching removed
payloads. No retry or growth allowance. Full sequences are required for temporal
checks; export only first/last and stereo PNGs. Keep cumulative small outputs
under 100 MiB including diagnostic log/perf; all runs bounded by the existing
110-second capture timeout. Helpers reuse the prior exact-path validator and
bounded runner with this checkpoint's names/endpoints/time cutoff; no large
copies. This is a fresh budget, not reused savings from earlier checkpoints.

Diagnostic log 807, owned PID 24060, 19:15:57-19:17:14: five settings audited,
capture delay zero verified, no new raws. Last sample 3,601 direct handoffs /
sequences / roots, zero original container scopes, original wrappers, original
post scopes or input/sequence refusals; maximum one root. Checked error/critical/
VK_ERROR/device-loss/exception/assertion/fatal markers absent. Process and runner
terminal. No multi-root or pixel qualification from this diagnostic.

Fresh cleanup completed: 240 unique payloads / 480 validated paths removed,
all eight existing PNGs retained. Actual volume free 54,392,713,216 ->
57,578,221,568 bytes; measured recovery 3,185,508,352 bytes (2.97 GiB).
Removed raw payload is 3,185,054,400 bytes. Incoming raw consumed zero;
remaining allowance exactly 995,330,400 flat + 2,189,724,000 VR, no retry.
This gross reclamation is not a claim of net savings after pending captures.

Implementation `f0f96d3` committed/pushed; unchanged 19:15:17 executable used.
VR log 808, PID 18832, 19:19:12-19:20:25: all 16 settings audited; native
sun/shadows, multiview/layered textures, camera mode 2, height zero, scale 1,
mirror/legacy stereo off. Process-local simulator reports 1440x1584 eyes;
absolute runtime DLL path verified, no registry changes. Capture delay 60 /
minimum 450 / count 120. Last sample 7,801 direct handoffs/sequences/roots,
zero original container/wrapper/post scopes or refusals, maximum one root.
Exactly 120 stacked 1440x3168 raws, frames 7621-7740, 2,189,724,000 bytes;
both first/last stereo checks correctly crossed, near-far -8, spread 8.
Streaming analysis and full-eye inspection pending, not yet image-qualified.

Before flat producer: free 55,383,183,360 bytes (51.58 GiB). Freshly removed
3,185,054,400 unique raw bytes; consumed 2,189,724,000, remaining 995,330,400,
exactly the planned flat set. The ~5.3 MB beyond raw growth includes runtime
small outputs and remains within the shared 100 MiB cap; bounded endpoint
analysis shares that cap. No additional large output or fresh retry allowance.
Original five-setting profile restored for flat, delay 60/minimum 600/count120.

VR streaming analysis finished: `frame_1788650414_0.raw` through
`frame_1788650422_119.raw`, 0/119 changes above 6%, maximum .392904742%,
zero cyan pixels. First/last bands 44/52/62/72/82/90/95% measure
-1/-2/-3/-5/-6/-8/-9 pixels in both captures. All four full first/last eye
PNGs inspected: coherent orange-sky village, rocks, stairs/ground and changing
windmill shadow. Existing distant blur remains; Shu's shadow is not qualified
in this framing. No new defect identified in this short normal window.
This does not qualify title artwork, late scenes, authored multi-root events,
headset comfort or complete-game rendering.

## Flat pixels, final verification and retention

Flat log 809, owned PID 23916, 19:21:33-19:22:40: all five original settings
audited. Last sample 3,001 direct handoffs/sequences/roots, zero original
container/wrapper/post scopes or input/sequence refusals; maximum one root.
Captures `frame_1788650556_0.raw` through `frame_1788650559_119.raw`, frames
2840-2959, 1920x1080. Streaming analysis: 0/119 changes above 6%, maximum
3.288869599%, no cyan hits (maximum .020592207%, median .011188272%).
Full first/last PNGs inspected: recognizable Shu and cast silhouette, foliage,
ground and moving windmill shadows; existing distant DoF remains. No new
defect identified in this short normal sequence.

All three logs 807-809 mount 1673 archives / 119346 names, with zero checked
error/critical/VK_ERROR/device-loss/exception/assertion/fatal markers. Original
five-setting profile read back. All owned renderer/build/analysis sessions
terminal; no runtime registry edits or Quest/Thor run. Guest-source established
the exact boundary/lifetime; devloop reused existing trees; vrsim exercised
both final desktop eyes. Unknown callbacks, disabled/empty lists, multiple roots
and comparison-setting fallback remain unqualified on GPU; source/CPU guards
are not substitutes for those cases or broader full-game verification.

Retained new evidence: 240 unique raw payloads / 3,185,054,400 bytes, eight
endpoint/stereo PNGs / 9,777,907 bytes, three app logs / 963,202 bytes and six
perf files / 2,441,552 bytes. Small evidence totals 13,182,661 bytes, below
the cumulative 100 MiB cap. Helpers/research are small text; no every-frame
PNG export, assets, dependency downloads or duplicate configure trees.
Both removed readiness normal sets contain zero raws and retain all eight
PNGs. Historical removed pixels are not recoverable.

Current normal baseline is `native_scene_handoff_flat` / `_vr`; retain the
sequence normal pair as previous control, eligible for review at the next
replacement checkpoint, not automatic deletion. Readiness early-startup,
all authored/synthetic effect previews/shared probes and unresolved failure /
control evidence remain protected until equivalent qualification replaces them.
The entire raw allowance is consumed; no retry/growth allowance remains.

Final scoped NTFS-identity inventory across automatic and isolated roots:
29,565 paths / 28,331 unique raws, 263,324,814,100 logical and 233,564,594,672
allocated bytes. All counts and byte totals match the prior checkpoint: the
raw archive did not grow. It is still far over the 10 GiB target, so its
historical review obligation and no-growth/reclaim-before-capture gate persist.

Ending measured free 54,376,480,768 bytes (50.64 GiB). Net volume usage
increased 18,989,056 bytes (18.11 MiB) from initial preflight. Gross recovery
2.97 GiB was spent on equal-sized replacement captures, not saved a second
time. Remaining net growth covers retained small outputs/build changes and
unattributed volume activity; volume change is not an exact repo artifact sum.
No further large producer is scheduled under this spent budget.

Next: replace resolve-link/exposure and scene-output getter associations with
explicit native frame-image ownership and downstream UI/presentation inputs;
move the remaining parent scheduler and engine producers to host scene/frame
data. Native light/visibility/animation/material producers, authored effect
events, title artwork, VR blur, late-scene failures and representative fields /
battles/cutscenes/menus/transitions/reloads in both eyes remain unfinished.
This is removed guest rendering execution, not completion of the full goal.
