# Owned scene lighting reaches native rigid draws

2026-09-07, parent63c5c75. A connected producer/owner/consumer change, not another
legacy callback translation. The full host-frame/desktop/Quest goal remains open.

## Contract and source provenance

The guest-source skill kept investigation on the existing source map and the
handoff. `bdMainGameStep` (`generated/reblue_recomp.27.cpp:2177`) waits on DrawEnd
at the0x82126C60 block, then completes scene preparation/draw-task updates.
`bdFrameSubmitAndDebugHUD` (file26:2493) invokes transfer callbacks before its
`bdLightListUpdateSnapshot` call at2929, then signals DrawStart at4579.
Snapshot body is file54:4204. Existing interpolation hooks were read; no generated
code, hook TOML, guest object or shader was changed/rebuilt. Snapshot and gameplay
producers still execute; this does not claim their original bodies are removed.

The existing whole-function snapshot hook publishes only after the original
snapshot and held changed-list restoration. Rendering is not running across this
handoff. Native publication copies semantic records, scoring/mode, render-thread
priority and special-scene inputs, then object/node class and spatial values from
the existing native instance index's completed render-side model/poses. A frame
number alone is not the synchronization mechanism. The loader/source adapters
remain explicit; broadened/late authored families still need runtime coverage.

New `native_scene_lights.h` owns at most300 lights and65,536 native node bindings
(4 MiB binding-capacity cap). It is one current publication, not another instance
registry/history cache. Access is serialized by the existing bridge's publication
mutex; selection returns copied values. Queued packets/GPU inputs cannot be
mutated by source writes, new publications or instance retirement. Replacement
overlap is bounded to two binding buffers plus fixed light sets; invalid producers
clear eligibility, not borrow the preceding scene. No new disk cache/asset format.

The source adapter imports object default selection visual+3132 or its non-null
per-node override through visual+3376/+3380. It copies class+0, centre+200 and
radius+212, **not** dirty+4, selected slots+8 or category+216. Null inherited entries
are unsupported, not guessed default data. Native keys are instance/model
generation/node. The lighting pass captures its actual light-selection view and
update; render view3 is not light view0. Stale updates (including same-frame
replacement) and retired/reused model/instance identities cannot borrow data.

`PrepareNativeRigidSceneForObject` now selects/composes from this owned publication
and copies the result into its existing primitive/pass packet. It no longer reads
per-node source selection bindings or previews legacy descriptors/cache.
`PrepareNativeSelectedLightValues` and `PreviewSelectedLightValues` were deleted;
compatibility selector/publisher/comparators remain for unconverted consumers.
Native category is not needed by these semantic shader inputs; selection math
preserves scoring, stable order, priority and exclusion. Unsupported hold mode
refuses; no stale cache value is invented. Existing GPU programs/layouts, image
leases, native instance storage, queue, indexed indirect draws and fences remain.

## Verification

Devloop grouped connected source/consumer/fixture edits before integration.
Material32/PID25736 builds; CPU30/PID29820 passes in0.15 s. The existing fixture
adds source-destruction consumption, same-ID changed colours, directional/point/
spot, render priority/exclusions/special-scene rules, distinct per-node overrides,
missing inheritance, address/count/identity limits, same-frame stale update,
reload identity and production GPU packing after publication retirement.
The existing128,000-candidate ranking and strict16-view dirty-output tests remain.
305 artifact-free Python source/scenario checks pass in0.133 s, including the new
two-epoch `--scene-lights` gate and negative stale/missing/bounds cases.

Host106/PID28988 builds in the existing Vulkan/OpenXR desktop tree. CMake glob
regeneration reports0 generated modules written and no guest/shader compilation.
Run944/PID28276/session30334 runs13:46:36-13:48:56, then is terminal. All21
temporary settings applied, exact116 B owner profile restored. No raw/perf/cook/
cache/dump output. Independent rerun of the read-only scenario parser passes
`--rigid-reload --receiver-setup --scene-lights`, including both field epochs.

| Fresh observed boundary | Result |
| --- | --- |
| Native source/model reload | generation93/instance144 ->207/384; old draws fully fence-retired before title |
| Ready native scene/shadow emissions | cold776->1676; reload769->1671; >=900 in each epoch |
| Reloaded owned scene lighting window4359->4659 | +300 publications, +300 native reads, matching updates4360/4660 and pass IDs, light view0, no refusal/missing reads |
| Current imported node bindings | 2,898;465,212 cumulative unavailable imports (repeated missing poses/inherited/invalid bindings), not claimed converted |
| Reloaded host receiver window | +74,574 native setups, +40,531 packets, +300 reads; original/refused/missing0 |
| Native batching | +300 scene/caster instances and calls per sampled window; singleton batches, no draw-call reduction claim |

The1920x1080/135,993 B window JPEG was actually inspected: coherent terrain,
trees/foliage and shadows, Shu partly hidden by vegetation, existing black cliff
marks visible. One mono image is sanity evidence, not animation stability, a
same-pose pixel differential, broad authored-light coverage or both eyes. Reuse
unchanged GPU26/rigid05's five8x8 two-eye shader cases; no new GPU-fixture run.
Run941's strict legacy dirty-bit failure remains unexplained and preserved;
944 passing does not identify its writer or fix it. No full host frame or speedup
claim. Next is representative rigid scene ownership using the same owners, then
remaining material/character/frame paths and complete desktop acceptance.

| Retained artifact | SHA-256 |
| --- | --- |
| Host106 exe48,609,792 B | `084A9883CC71C5E639F140B55D1779223B28B13438D64F6A46964C9025E857BC` |
| `logs/reblue_944.log`,490,886 B | `7630E837C613AE116611D4A38FD25DBD2FAB861648FD2786499B531E2710F8A1` |
| `out/verification/native_receiver_setup_window.jpg`,135,993 B | `6424763E163BC784035E7BD21860B9B86D1A023C002B54C084C11713E3B6D2C0` |
| Restored owner profile | `2F1BC38D763A1B7BDBA31F560684FD4AA7E42A714600B8D344F19DA7F38E23B0` |

## Storage

Same original cumulative ledger/floor/3 GiB exception and zero incoming raw gate.
Builds supervised at300 s/256 MiB free-drop; runtime180 s/192 MiB free-drop,
800 KiB aggregate text,160 KiB JPEG/10 MiB aggregate images, no raw. Peak estimate
<=192 MiB; no new tree or bulk recook. All producers terminal/profile restored.

After equivalent replacements passed, removed material31/CPU29's four logs
(2,035 logical B,4,096 B recovered), then host105's two logs and943 text
(487,408 logical B,491,520 B recovered). Total **495,616 B recovered once** from
seven superseded agent outputs. Build logs are reproducible; exact943 text is
gone, with its hash/findings retained in prior reports and stronger944 text/pixels.
Keep940/image as the pre941 baseline and941's unresolved failure; protected raw,
other baseline/VR/failure evidence, game assets, profiles and build trees untouched.

Known retained net growth462,603 B: material fixture+153,473 (8,129,486 B),
exe+45,056/PDB+118,784, build logs+5,052 (192,641 B), replacement runtime text+4,245,
new image135,993 (aggregate10,367,089 B). The old940 image is retained for the open
941 regression; review when its cause/replacement is established. Other source/
host object/Git and drive-wide activity are not fully attributed. Starting free
62,937,022,464 ->cleanup-end62,917,083,136 B (58.596 GiB):19,939,328 B drive-wide
use, not all renderer bytes. Final post-push free measurement is reported at handoff.
