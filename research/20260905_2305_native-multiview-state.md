# Native Vulkan multiview command-state ordering

Date: 2026-09-05. Root base `bbf027d`, existing uncommitted native scene resolve
integration, Plume base `465c2ad`. No guest-code changes or Quest work.

## Preflight and storage

Reuse `out/build/win-amd64-release`, the existing 8x8 attachment-resolve fixture,
installed DXC and retained Khronos validation layer. No downloads, game captures,
new build trees or asset conversions. Planned additional peak <=512 MiB, within
the original 20:47 checkpoint's shared 2 GiB budget, not a new allowance.
Starting measured volume free space: 65,404,219,392 bytes; original checkpoint
start: 65,462,788,096 bytes. Expected reserve after the planned peak >60 GiB.
Retained diagnostics <=100 MiB; aggregate build/test logs <=10 MiB. Existing
baseline/failure captures, logs 821/822 and the one scene-resolve PNG stay protected.

Use `out/verification/build_attachment_resolves.ps1`, whose supervisor enforces
the original cumulative growth budget, free-space reserve, bounded logs and
owned-process timeout/cleanup. Tests use tiny in-memory readbacks, not captures.
Record completed attempts and actual storage below; do not duplicate producers.

## Defect and regression plan

The fixture previously called backend-only `checkActiveRenderPass()` before
binding graphics state, hiding the ordering used by ordinary Plume callers.
The public draw method starts the native render pass lazily after those binds.
With multiview, starting a subpass makes non-render-pass state undefined:
[Khronos multiview specification](https://docs.vulkan.org/refpages/latest/refpages/source/VkRenderPassMultiviewCreateInfo.html).

Remove the fixture's private begin workaround and refuse submission after any
record-time API validation error. First establish the regression, then correct
native command-state handling without disabling multiview or weakening validation.
Keep the distinction between this small backend test and full game/stereo gates.

Publishing remains denied pending explicit owner approval. Do not retry pushes
or commit the parent dependency gitlink before the dependency is published.

## Implemented and committed locally

Plume `81bdca8` preserves current native command-list bindings across its internal
multiview begin. Graphics, compute and ray-tracing pipeline/descriptor bind points,
owned dynamic offsets, vertex/index bindings, viewport/scissor ranges, depth bias
and incremental push values are retained; mono begin does not reissue them.
Cache reset at recording boundaries prevents reuse of prior command-list state.
No guest addresses, register images, Xenos commands or per-draw replay history
were added. This is native RHI binding lifetime across a backend-owned boundary.

Descriptor disturbance follows prefix layout compatibility, including push
ranges, binding flags and immutable sampler handles; equal definitions need not
be the same layout object. Layout definitions own their immutable sampler arrays.
Push storage keeps only last-written word/stage owners, preserves the original
stage masks and coalesces contiguous replay writes. Its size is bounded by the
device push-constant limit and stage count, not draw count. Static pipeline binds
invalidate cached dynamic depth bias. Layouts must outlive recording, documented
in Plume's README. Ray-tracing binding restoration is implemented but not GPU-
exercised by this fixture; overlapping multi-stage masks also need broader tests.

Native API references:

- [Descriptor compatibility and incremental push constants](https://docs.vulkan.org/spec/latest/chapters/descriptorsets.html).
- [Dynamic-state lifetime](https://docs.vulkan.org/guide/latest/dynamic_state.html).
- [Pipeline binding and render-pass scope](https://docs.vulkan.org/refpages/latest/refpages/source/vkCmdBindPipeline.html).
- [Definition identity, including referenced handles](https://docs.vulkan.org/spec/latest/appendices/glossary.html#glossary-identically-defined).

## Verification

All attempts below reuse `build_attachment_resolves.ps1`; names are its Target
and Attempt arguments. Each producer reached a terminal state before replacement.

| Attempt | Outcome |
| --- | --- |
| `native_attachment_resolve_test 10`, PID 20244 | Built red regression, exit 0. |
| `pixels 07`, PID 26500 | Expected failure, CTest exit 8: mono passed; first two-eye draw reported VUID 08606 (pipeline undefined). Pre-submit guard refused invalid work. Exception cleanup also exposed a leaked test debug messenger, fixed with RAII. |
| `native_attachment_resolve_test 11`, PID 27376; `pixels 08`, PID 14712 | Initial native-state fix built and passed, zero API errors/warnings. |
| `native_attachment_resolve_test 12`, PID 22912; `pixels 09`, PID 25212 | Expanded native descriptor/VB/IB/indirect/restart test built and passed. |
| `native_attachment_resolve_test 13`, PID 27248 | Compile failed on a test-only descriptor enum spelling; no test ran. Corrected to the actual writable structured-buffer type. |
| `native_attachment_resolve_test 14`, PID 26956; `pixels 10`, PID 23216 | Compute/graphics/compute and MV/mono/MV test built and passed. Both Vulkan storage-buffer enum variants map to the same descriptor type; final source uses the writable API spelling. |
| `native_attachment_resolve_test 15` | First preflight refused for disk budget before launching or creating logs; resumed only after terminal/no-output and storage checks. Owned PID 18980 built final source, exit 0. |
| `pixels 11`, PID 22020 | Final strict suite passed in 1.06 s, validation enabled, zero API errors/warnings; one separately reported existing GOG loader-manifest diagnostic. |
| `reblue 07`, PID 27296, session 64542 | Desktop host build linked successfully, exit 0. Build plan dry-run stopped at CMake regeneration and was not complete dependency proof; actual guarded build rebuilt host objects, no recompiled guest objects. |
| `cpu 03`, PID 23020 | Existing 30 CPU CTests passed, 3.52 s, no rebuild. |

The final GPU test uses only 8x8 images and in-memory readbacks. It checks mono
and both eyes, RGBA8/FP16 HDR, D32/D32S8, depth MIN/SAMPLE_ZERO, colour averaging,
resumed LOAD, DISCARD, pending/held zero-draw clears and eighteen attachments.
Every sample draw forces a lazy pass restart after bindings. Graphics state is
bound once before the first framebuffer, with copied dynamic uniform offsets,
separately created compatible layouts, vertex slot 2, nonzero VB/IB byte offsets,
direct/indexed/indirect draws and overlapping partial pixel pushes. Additional
checks cover both descriptor-disturbance directions, static/dynamic depth bias,
MV/mono/MV, recording reuse, and compute pushes at the same byte offsets as pixel
pushes. A compute counter equals 62 after two adds of 31 around the graphics
passes without a compute rebind. These are real readback checks, not just counters.

Final test executable: 743,424 bytes, linked 23:26:30, SHA256
`98976d7100faf1234e5b6bebfaab98ae8629aa6ecfed781e768c7f420808db63`.
Shader sizes: VS 640, PS 1,716, CS 920 bytes. Existing Khronos validation layer
from the 20:47 checkpoint was reused, with core and synchronization validation.
Builds retain pre-existing Windows CRT deprecation warnings, not API warnings.
The 31 post + 15 scene source guards also pass; they are source boundaries, not
pixel qualification. The final test log is retained in the existing CTest tree.

Desktop executable: 47,579,136 bytes, linked 23:29:28, SHA256
`4b676df86cf27bdc8bc6b7ff69462936a5134e15e694549f1fc69ebfcac3c563`.
It contains root base `bbf027d` plus the existing uncommitted resolve integration,
using Plume `81bdca8`. The parent renderer/gitlink remain uncommitted until the
dependency can be published; a docs-only commit does not change the tested binary.

## Desktop diagnostic

Capture-disabled desktop XR: owned PID 22576, session 47996, 23:30:19-23:31:34,
log 823 and `perf-20260905-233021.{csv,meta.txt}`. Existing desktop xrsim, diorama,
1440x1584 per eye, two-layer swapchain, multiview enabled, mirror off, native post
and DoF enabled. All 16 profile settings took effect; full 1673 archives/119346
record names mounted. Last census: 8,700 native scene results, all consumed;
8,700 initial colours deferred, zero recovered; 8,701 native post scopes/final
publications, zero scene-image imports/original scopes/refusals. Last allocation
census: three native resolve pairs created, two retired, one resident with
72,990,720 payload bytes, no refusal/failure. This is not fully host-owned frame
count: scene association, animation/engine inputs and other adapters remain.

The wrapper enforced its 75-second timeout and zero-frame allowance, stopped
only its own renderer and restored the owner profile byte-for-byte. Zero raw
files were produced. No main-game Vulkan validation or new eye-image inspection
was performed; the tiny GPU fixture is not broad game/stereo visual qualification.

Capture-disabled flat/non-MSAA: owned PID 2324, session 38726, 23:33:04-23:34:21,
log 824 and `perf-20260905-233307.{csv,meta.txt}`. All six profile settings took
effect (`bd_msaa=0`). Last census: 3,600 completed/consumed scene results, zero
native attachment-resolve results or deferred colours; 3,601 native post scopes,
completed native inputs and final publications, zero imports/original scopes/
refusals. Source materialization counters were colour 1/depth 0 (including startup),
not a claim of absolutely zero scene publication work. No checked runtime/config
errors, zero raw frames, owned process terminal and owner profile restored exactly.
Neither diagnostic measured headset performance or qualified current game pixels.

## Storage interruption

Before build 15, volume free space unexpectedly fell to 62,999,302,144 bytes
(2,463,485,952 below the original checkpoint start). The producer was refused
before launch. CIM showed no renderer/build/test producer; scoped logs, captures,
Plume outputs and cache checks found no new game captures or multi-GiB output
explaining the change. Windows paging allocation was 2,048 MiB, usage 8 MiB at
inspection, but no before/after paging evidence establishes the cause. Do not
attribute the unexplained volume change to a particular process or cleanup.

Free space recovered to 64,754,196,480 without any deletion by this agent. The
planned <=512 MiB peak again fit the original budget and reserve, so build 15
resumed under the unchanged supervisor. Final host build ended at 64,718,913,536
free bytes; XR diagnostic ended at 64,716,865,536. No budget was reset or expanded,
and none of the free-space recovery is credited as reclaimed project storage.

## Completed retention review and remaining work

Retain the red regression (`native_attachment_resolve_test 10`, `pixels 07`),
final build/test (`native_attachment_resolve_test 15`, `pixels 11`), host build
07, CPU 03, final CTest log, and runtime logs 823/824 with their perf metadata.
These supersede only this turn's intermediate build/test diagnostics. Protected
flat/VR raw baselines, unresolved-failure evidence, the previous single scene
PNG, logs 821/822, tools and active build trees remain untouched.

Removed exactly fourteen regular `out/verification/attachment_resolve_*.log`
files: stdout/stderr pairs for `native_attachment_resolve_test` 11, 12, 13, 14
and `pixels` 08, 09, 10. Their 7,902 logical bytes are superseded by the results
above and final tests; scenarios can be regenerated, exact old logs cannot.
No recursive deletion or reparse traversal. CIM confirmed no build/test/renderer
producer before cleanup. Immediate volume free space rose from 64,719,220,736 to
64,719,228,928: **8,192 measured bytes reclaimed**, not a large disk saving.

Ending free space: **60.27 GiB**. Net volume use is 743,559,168 bytes above the
original checkpoint start, including unexplained external fluctuations; it is
not all attributable to this work. Cumulative retained runtime logs: 1,681,443
bytes; perf files: 4,555,424; build/test logs: 153,205; prior PNG: 3,196,857.
No newly modified runtime caches or new raw images. Those diagnostics plus the
existing 41 MiB tool/inspection reservation fit the 100 MiB retained cap, and
build/test logs remain far below 10 MiB. No producer remains live.

The full renderer goal remains active. Next ownership work includes removing
depth/getter and final output/UI publications by converting their consumers,
native source attachment ownership, scene/animation producers and parent frame
scheduling. Multiview native binding is now tested, but full game validation,
both-eye pixel sequences, HDR/exposure, multi-root/nested/early-return cases and
the existing distant VR blur are not qualified by these diagnostics. Ray-tracing
and broader push-mask/layout combinations need additional backend coverage.
No Quest optimization before the complete desktop gate. Root renderer integration
and parent gitlink stay uncommitted while publishing approval remains outstanding.
