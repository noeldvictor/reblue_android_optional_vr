# Native scene resolve ownership

2026-09-05 EDT. Root base `45210ef`; local Plume `465c2ad`.

Previous goal turn made progress: native sampled scene/post inputs were built,
CPU/source checked and exercised in capture-disabled desktop runs, then committed.
Only the unpublished Plume gitlink is dirty at resumption. Publishing is still
paused after auto-review rejection; no owner approval or push retry. No agents
or Quest work. Complete devloop and guest-source skills reread, along with current
transition requirements and the prior native-image worklog. No producer is live.

## Source evidence and direction

Scene begin owns persistent source attachments; scene end still calls
PublishSceneOutput for its emulated MSAA/scale copies. The new sampled contract
allows post to consume separately owned native resolve images instead. Give that
pair bounded device residency, generation-safe identity and descriptor/image
retirement after both CPU readers and a submission fence, using the existing
FencedAssetCache mechanism rather than allocating new guest resource wrappers.
Framebuffer attachments must be the exact source pair and native resolve pair.

The existing source images can be recreated (log 817 switches 1920x1080 to
1280x720); raw pointer identity alone must not identify a new allocation as an
old framebuffer. Native generations belong to the host slot, not guest headers.
Default flat post currently samples 1920x1080 FP16 input with exposure 1.
The emulated MSAA shader forces alpha 1 after RGB exposure, whereas the ordinary
copy shader preserves sampled alpha. Native post must carry that distinction;
removing copies is not permission to apply HDR exposure twice or invent opacity.

Khronos states that multiview subpass begin invalidates non-render-pass state:
https://docs.vulkan.org/refpages/latest/refpages/source/VkRenderPassMultiviewCreateInfo.html
Plume begins lazily at draw/query time. Live frame_ring binds native descriptor
heaps once per list; draw_queue then deduplicates pipeline/viewport/index state.
Therefore opening a pass before only the pipeline is insufficient: descriptor,
dynamic, buffer and push state must be established for each multiview subpass.
The standalone fixture's explicit begin does not qualify live ordering. This
remains part of scene integration, not a claimed solved or completed host frame.

## Storage ledger

At 22:06: 65,400,877,056 bytes free (60.91 GiB). Continue the original 20:47
attachment-resolve budget: start 65,462,788,096, max 2 GiB cumulative peak growth,
supervisor stop at 1.75 GiB or 21 GiB free, 250 ms polling and 256 MiB headroom.
About 59 MiB is already consumed; no reset or repeated cleanup credit. Incremental
host/test build and link overlap planned <= 512 MiB additional, using existing
trees. Diagnostic retained cap remains 100 MiB, including the one reused local
validation layer, test outputs and logs; cumulative build logs stop at 10 MiB.
No downloads, new build trees, asset cooks or game raw captures are authorized
by this plan. Historical raw archive is still over budget and protected current
baseline/failure evidence remains untouched. Reconcile before any producer.

## Resume at 22:33 and implemented integration

The preceding documentation turn made progress (`4120729`, stronger storage
defaults). Revalidated the dirty renderer work and terminal build session 99838:
attempt 03 failed because the new bindless-using TU was in `reblue_common`.
Added it to `reblue_backend_only`, preserving the guard. No producer was live
before retry. Devloop and guest-source were reread completely. The vrsim skill
was inspected, but no VR run or device work occurred.

Native MSAA scenes now create their exact colour/depth resolve destinations,
array sampling views, descriptors and resolve framebuffer without guest resource
allocation. A 512 MiB payload/64-entry device cache bounds residency. Full source
keys include allocation generations, dimensions, layers, samples, formats and
density map. The source adapters remain host-owned; new native resolve images
are independently owned. Descriptor retirement precedes framebuffer/view/image
destruction after the recorded slot's next proven fence, not at CPU release.
The native owner CPU test uses actual owner/cache types and interface doubles
to assert that ordering, live layout sharing, generation separation and reuse.
Physical Vulkan image-view creation is checked before descriptor publication;
density-map creation/lookup follows the video-lock serialization.

Scene framebuffer binding attaches both resolve images (colour average, depth
MIN, stencil NONE), prepares write layouts after the outgoing draw queue, and
finishes the pair into sampled layouts before post. Post carries scene opacity
separately from RGB exposure: MSAA's former shader forced alpha one. The native
atlas and 240-byte composite constants preserve that intent; subsequent roots
consume their predecessor's alpha instead of forcing it again.

Normal final-scene native post now omits the initial colour publication/copy.
An accepted scoped result pins the source and output until either native post
publishes the final image or explicitly recovers the initial colour. The sequence
reports actual publication separately from successful empty/no-op handling.
Disabled/refused/empty post recovers colour before compatibility consumers;
normal view return and clear-before-reuse recover an unconsumed result. Exception
unwinding releases ownership without recording new GPU work in a destructor.
**Initial depth publication remains** for unmigrated external getter readers;
final colour UI/getter publication also remains. This is not all-copy removal.

Rechecked the exact generated final view tail and `bdShaderSystemFlush`: the
earlier vtable +16 call is not scene end. Flush calls +20 after camera motion
blur, then the caller does teardown, view/focus updates and the existing post
hook. The prior owned-XEX vtable check is recorded in
`20260905_1958_native-scene-image-result.md` (+20=0x82187010). No generated edits,
new disassembly tools or guest object rebuilds.

## Verification

All producers ran under the original shared storage budget and owned-process
supervision; every listed process/session is terminal, with actual exit codes.

- Host build attempts 04/05/06 passed. Attempt 04 compiled the two changed
  shaders; subsequent incremental builds compiled only changed host TUs. Codegen
  reported zero writes, and no guest objects rebuilt.
- CPU target `host_post_images_test` attempt 04 rebuilt and linked. CTest attempt
  02 passed all 30 tests (3.61 s), including the new real-owner/double lifetime
  assertions. Post/scene source guards pass 31+15 checks; these are source guards,
  not independent GPU equivalence tests. `git diff --check` passes.
- Initial wiring diagnostic log 819, 22:35:01–22:36:18, PID 25692/session 35015:
  3,300 native resolve results, 3,301 native post sequences, no imports/original
  scopes/refusals. Initial colour copies still existed in this intermediate build.
  Three resolve pairs created, two retired, one resident (33,177,600 payload bytes).
- Colour-copy removal log 820, 22:42:55–22:44:11, PID 14964/session 9637:
  3,600 native results/deferred colours, zero recovered; 3,601 native sequences,
  zero imports/original scopes/refusals. Three pairs created, two retired.
- Post-off recovery log 821, 22:45:38–22:46:54, PID 25288/session 69602:
  temporary `bd_native_post=false` audited (all six settings). 3,600 deferred
  colours recovered 3,600 times before the compatibility post path. Zero sequence
  refusals; original/settings-disabled counters are expected here, not native work.
- Final normal log 822, 22:47:23–22:48:39, PID 768/session 67415:
  3,600 native results/deferred colours, zero recovered; 3,601 native sequences,
  completed inputs and final publications, zero imports/original scopes/refusals.
  Three pairs created, two retired, one resident (33,177,600 payload bytes), zero
  allocation/capacity failures. Full 1673 archives/119346 records mounted.
  Both final runs use the binary linked 22:45:10, 47,569,408 bytes, SHA256
  `9b34248339ad6700ff2486f182289f466e2569e2b601996d72dcd9811096328e`.

All four runs disabled automatic captures, produced zero raw frames, found no
checked config/runtime errors, stopped only their owned process and restored
the five-key owner profile byte-for-byte. Diagnostic timeout is 75 seconds;
wall-clock endpoints include supervision/cleanup. No main-game API validation
was enabled; standalone Plume validation from the prior checkpoint is not proof
of the live multiview ordering noted above. Only one post root was exercised.

The last run additionally opted into one <=10 MiB window PNG, included in the
100 MiB diagnostic budget (tool/inspection reservation raised from 31 to 41 MiB,
not a new allowance). `PrintWindow` targeted owned PID 768 without foreground
capture. `out/verification/native_scene_resolve_color_window.png` is 1920x1080,
3,196,857 bytes, SHA256
`a42bec4ed84c0601afba653d9e86ab449e8697660fc0fb9d3148f2115eacde63`.
Inspected it and the protected flat baseline's existing `first.png`: character,
terrain, vegetation, shadows and post effects are visible with no obvious whole-
frame corruption. Animation/shadow phase differs; this is not a time-aligned
comparison, sequence stability, both-eye qualification or a new raw baseline.
Retain this PNG until a qualifying replacement covers native attachment resolves.

## Remaining work and publication

The full goal remains active. Live multiview state ordering, depth/getter copy
removal, output/UI ownership, native source attachment adapters, engine producers
and parent scheduling remain. HDR/exposure pixels, multi-root/nested/early-return
GPU paths, new no-MSAA behavior and complete fields/battles/cutscenes/menus/
transitions/reloads/both-eye coverage are not qualified by these diagnostics.
The new image is not evidence that the existing distant VR blur is fixed.

Renderer integration remains an **uncommitted local working-tree change** on
root base `4120729`, using local Plume `465c2ad`. Publishing remains denied by
auto-review pending explicit owner approval; no retry or alternative route.
Do not commit the parent gitlink before the dependency is published, and do not
pretend a root code commit against the older recorded API would be reproducible.
This worklog and current-status documentation can be committed independently.

## Retention review

Final log 822/perf-20260905-224726 and recovery log 821/perf-20260905-224540
are the retained runtime evidence. Logs 819/820 and their two corresponding perf
CSV/meta pairs are superseded agent-produced diagnostics, not protected raw
baselines. Their results are recorded above; exact intermediate logs/timings are
not needed after final validation. Eligible removal totals 1,623,604 logical
bytes in six explicitly named files; measurement and completed cleanup follow.
All protected baseline/failure captures and existing build trees remain untouched.

The first cleanup preflight removed nothing: its expected total used the run
wrapper's cached FileInfo lengths, before the logs' final writes. Fresh metadata
and terminal-process inspection established log 819=221,391 bytes and
820=238,725 bytes; the four perf files were unchanged. The run helper now refreshes
final file metadata before reporting sizes. The strict size guard was retained.

Completed cleanup removed exactly `logs/reblue_819.log`, `logs/reblue_820.log`,
and `logs/perf/perf-20260905-{223504,224257}.{csv,meta.txt}` (six regular files,
no recursive delete, reparse points or renderer processes). Immediate actual
free space rose from 65,409,138,688 to 65,410,768,896: **1,630,208 bytes reclaimed**
(1.55 MiB). Their diagnostic scenarios can be rerun; exact historical timing/log
bytes are no longer retained. Their summaries above and current evidence remain.

Ending measured reserve at cleanup: **60.92 GiB**. Net volume use is 52,019,200
bytes above the original 20:47 checkpoint start, versus the shared 2 GiB cap.
The largest observed snapshot growth was 85,016,576 bytes; that is not an exact
continuous peak measurement. The wrapper enforced its tighter stop threshold.
Free space also changed independently of cleanup, so only the immediate
1,630,208-byte deletion delta is credited as recovered storage, not the whole
volume improvement since resumption.

Cumulative retained checkpoint runtime logs now total 957,160 bytes, perf files
2,462,144 bytes, build/test logs 111,759 bytes, and the one PNG 3,196,857 bytes.
No newly modified runtime cache files or new raw frames were found. The existing
tool/test reservation plus these outputs remains below the 100 MiB diagnostic
cap. Keep the current host/test trees and one local validation layer; retain
logs 821/822 and the PNG until the next relevant qualification supersedes them.
Do not regenerate the removed diagnostics merely to restore an archive.
