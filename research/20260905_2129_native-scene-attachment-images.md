# Native scene attachment images

2026-09-05 EDT. Root base `df89878`; local Plume `a8b3c15`.

Previous goal turn made progress: implemented and GPU-qualified native layered
attachment resolves, committed the dependency/tests and reclaimed obsolete
small outputs. Root and dependency pushes remain rejected pending explicit
owner approval. No retry or alternate publication route is authorized by the
automatic goal continuation; do not commit the unpublished parent gitlink.

At resumption only the changed Plume gitlink was dirty; no producer was live.
Read current AGENTS, transition acceptance/current status, prior worklog,
complete devloop and guest-source skills. The initial combined read truncated
AGENTS' tail and guest-source, which were reread fully before changes. No agents
or device work. Main scene/pass, framebuffer/cache, resolve publication, native
post inputs, texture binding and fence-lifetime sources were inspected.

## Evidence and direction

`native_scene_pass_bridge.cpp` creates FP16 colour (`0x1A2201BF`) and combined
D32_FLOAT_S8_UINT depth (`0x2D200196`, via format.cpp). The existing GPU fixture
covered only R8G8B8A8_UNORM/D32_FLOAT. First exercise actual scene formats,
including depth MIN with stencil NONE, before introducing them to live frames.

The current completed result pins native sources but still receives the outputs
of PublishSceneOutput's emulated sample/scale copy. Native render-pass completion
must instead own persistent single-sample images at the source extent, attach
them at scene drawing, and carry them into post with explicit exposure and
lifetime. The remaining UI/getter adapter is not the native image owner.
Bindless descriptors and framebuffers must outlive queued GPU work; a plain
CPU-scope unique_ptr destruction is insufficient. Native post's actual image
interface still uses GuestTexture wrappers, which must not become new guest
resource allocations or EDRAM identities for the resolved images.

## Storage ledger

Start 21:29:29: 65,424,044,032 bytes free (60.93 GiB). Reuse the existing
Plume/main and CPU-test trees and the 21 MB validation tool. Carry the original
attachment-resolve checkpoint cap forward: 65,462,788,096 starting volume bytes,
2 GiB peak total incremental growth, supervisor stops at 1.75 GiB or 21 GiB free,
250 ms checks and 256 MiB headroom. The prior checkpoint consumed about 37 MiB
net; this turn does not reset that allowance. Cumulative diagnostic outputs
remain capped at 100 MiB (about 30 MiB retained tool/test bytes plus small logs).
Reuse `out/verification/build_attachment_resolves.ps1`, one target at a time,
unique attempt logs, bounded test timeout and owned process shutdown. GPU
fixtures stay 8x8 with in-memory readbacks, no images written.

No new game-capture allowance. The historical archive remains over budget and
`native_scene_result_flat`/`_vr` remain protected. No download, asset cook or
new dependency tree is planned. Reconcile actual growth before a main build or
new producer; full-game capture needs a fresh explicit retention/space plan.

## Resumption at 21:48

Previous turn made progress by committing the owner's additional tool-download
storage discipline as root `24002fa`. Pushes remain paused; no approval was
supplied by the automatic continuation. The unfinished native-image edits and
Plume scene-format test remain present and are being completed, not restarted.
No reblue/build/test producer is live according to Get-Process. Complete devloop
and guest-source skills were reread; current AGENTS overrides their older recipes.

The last actual GPU fixture report (21:36, `pixels_06`) passes FP16 HDR colour and
D32_FLOAT_S8_UINT MIN/SAMPLE_ZERO with stencil NONE, mono/two eyes, including
LOAD and held clears: zero API errors/warnings, one existing GOG loader message,
1.08 s, no images written. This is format coverage, not live scene integration.

Free space is 65,408,172,032 bytes: about 52 MiB below the original cumulative
starting point, with no new producer since the previous ledger. Source/Git and
unrelated volume activity are included in that net measurement. The next work
finishes the explicit native sampled-input contract and checks invalid samples,
dimensions, descriptors, eye counts, exposure and physical-image feedback before
any GPU work. It does not change the existing exposure/alpha/extent filtering.
Reuse the main and native_texture_test build trees. Planned additional peak
growth <= 512 MiB (incremental host objects/PDB/link overlap and a small CPU
test), within the original 2 GiB budget and >59 GiB expected free reserve.
The supervisor will retain its original 1.75 GiB growth/21 GiB stop floors,
250 ms checks, cumulative 10 MiB log cap, 300 s build timeout and owned cleanup.
No new game captures, asset output, downloads or duplicate toolchains.

## Native sampled-input implementation and first checks

`SampledImage` now carries native image/descriptor identity, dimensions, sample
count and a pointer to its owner's live layout record, not a GuestTexture header
or resolve association. `HostPostInputs::CanRenderTo` rejects invalid readiness,
unresolved MSAA, mismatched eyes, invalid exposure and physical-image feedback
before GPU work. Scene completion and inter-root output edges borrow exact native
images. Sampling preparation remains at the explicit compatibility boundary;
normal atlas/composite/directional-bloom scene/depth readers use native records.
Output and optical-asset adapters are still present and explicitly not completed.

30 native_texture_test CTests pass, including the new native-input contract
test (identity, null/dimension/sample/descriptor/layer/exposure rejection,
normalized-UV extent independence, and live layout mutation through copies).
30 post and 13 scene source guards pass; these guards are not pixel evidence.
CPU build attempt 01 failed before launching due duplicate Path/PATH environment
keys. The wrapper now normalizes those process-local keys. Attempt 02 stalled
inside restricted-sandbox Ninja; owned cmake 22384 and children were explicitly
terminated, session 20169 confirmed exit 1, before attempt 03. The same target
built successfully outside that sandbox (PID 25728), then CTest PID 26800 exited 0.

Host build attempt 01 found one remaining compatibility caller passing the old
depth pointer; import now precedes target acquisition and supplies inputs.depth.
Attempt 02, PID 26948/session 27822, completed successfully. Header/glob refresh
rebuilt host consumers only; codegen reported the module up to date and wrote
zero files. No guest TU, cooked asset or shader regeneration. Known unrelated
CRT deprecation warnings remain. The main Vulkan exe linked 21:57:18, 47,547,904
bytes, embeds `24002fa4b` plus local changes; SHA-256:
`59da4eedd98bca3f70810f43d9aecb1d20f8b0af2f311518d025b269b4758d98`.
It includes Plume production `a8b3c15`; scene-format tests/docs are now locally
committed as `465c2ad`, without production changes. No push was retried and no
unpublished parent gitlink is staged.

Flat desktop diagnostic log 817: 21:59:02-22:00:19, owned PID 25260/session 50922
terminal, all five settings audited, 3,601 native post scopes and effect roots,
zero original scopes or refusals; no raw images. The original five-key profile
was restored byte-for-byte. This proves input execution only, not pixels.
Next exercise the direct source-image path with temporary bd_msaa=0 under the
same 75-second diagnostic wrapper and cumulative budget. The wrapper's native
image mode uses the original 20:47 ledger, forbids raw output, includes 31 MiB
retained tools/tests in its 75 MiB diagnostic stop threshold (100 MiB cap), checks
every 250 ms, and always stops its owned renderer and restores the profile.
After the first run free space is 65,402,789,888 bytes; no new budget or captures.

## Diagnostic completion and remaining work

No-MSAA log 818: 22:01:24-22:02:41, owned PID 18328/session 11921 terminal,
all six settings audited. Final samples: 3,601 native post scopes/roots/final
publications, zero scene-image imports/original scopes/refusals. Scene results:
3,600 completed/consumed, one materialized colour and zero materialized depth,
so the direct native source path is exercised. Default-MSAA log 817 records
3,600 materialized colour and depth inputs. Both runs mount 1673 archives /
119346 names and have no checked error/critical/VK_ERROR/preflight-refusal
markers. Both produce zero raws; the five original profile settings are restored.
Neither run enables API validation or proves image correctness. Only one post
root is exercised; multi-root/HDR/nested-view and new flat/VR pixel checks remain.

Keep the full goal open. The next integration must give scene resolve images
native ownership through submission fences, attach them to the actual producer
framebuffer, and feed this new sampled contract without PublishSceneOutput's
initial MSAA/scale copies. Preserve alpha/exposure/extent filtering deliberately;
ordinary output/UI and optical-asset adapters still need removal. Plume's lazy
multiview pass begin must precede draw-state binding; the standalone fixture's
explicit begin does not fix or qualify live caller ordering. No Quest work.

## Storage and handoff

All owned builds/tests/runs are terminal; no producer or temporary profile override
remains. Current native-input CPU exe/PDB/object total 536,111 bytes, reused in
the existing test tree. Keep them while developing/validating native image
ownership; replace in place. Keep logs 817/818 and their small perf records as
input-path evidence until scene integration supersedes these diagnostics. The
existing validation tools and tiny GPU fixture remain the one reusable copy.
There were no downloads, game captures, asset cooks, new build trees or material
deletions this turn. Protected baseline/failure captures remain untouched; no
new raw allowance exists and the historical archive is still over budget.

At 22:02:41 free space was 65,400,786,944 bytes (60.91 GiB), versus the original
cumulative start of 65,462,788,096: net disk growth 62,001,152 bytes (59.13 MiB),
including unrelated volume activity. This continuation used about 7.04 MiB net
since its 21:48 observation, within the original 2 GiB peak/100 MiB diagnostic
budget; no prior cleanup saving is credited again. Commit source/tests/docs
explicitly, excluding the unpublished Plume gitlink. Publishing remains blocked
pending the owner's explicit approval; no alternative route or retry was used.
