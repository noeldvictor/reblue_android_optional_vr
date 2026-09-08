# Native scene occlusion connection — 2026-09-07

## Delivered boundary and limitations

The native scene end now queries owned world bounds against its actual depth
attachment, after queued scene draws and before attachment resolve/retirement.
The walk supplies native instance/model-generation/node identity, owned bounds
and the scope-owned current camera. Persistent history contains no source VAs.
The query shader uses an80-byte native push packet; the old256-register shader,
252-sphere translated uploads, guessed framebuffer-size trigger and address-keyed
legacy draw filter are removed. Legacy-only drawing is no longer a cull consumer.

The native rigid consumer preflights every sibling and commits ordered lights
before omitting GPU work. Limits are2048 queries/frame slot,8192 history entries
and four sample-count pipelines. Current-frame observations, unchanged camera,
depth generation/extent/sample count and two consecutive zero results are
required; stale, ambiguous, near/eye-clipped or unsupported inputs keep drawing.
This remains temporal culling, not same-frame visibility proof. Mono only:
explicit per-eye observations and stereo qualification are still required.
Animation, effects, UI, remaining scene families/frame ownership and the full
desktop gate remain open. No Quest work or measured speedup is claimed.

The near guard uses homogeneous world-to-clip space. The old world-origin
distance guard was wrong, but the native path never emitted its old queries in
run956, so this does not explain or fix the retained tree-trunk gaps.

## Verification

- 325 artifact-free Python source/scenario checks pass, including five new wiring
  guards. They do not replace behavior/GPU/live verification.
- Output38/CPU21 passes in0.40 s (CTest0.43): camera translation/rotation, near/w
  clipping, nonfinite inputs, originating query frames, missing/current-invalid
  observations, identity/generation/depth changes, late camera changes,
  contradictory duplicates, stale/out-of-order results and capacity/retirement.
- The actual shader/program runs in the existing8x8 Vulkan fixture on RTX3060.
  Eight translated/rotated1x/2x/4x/8x cases distinguish front/hidden/intersecting
  cubes and compare every colour/depth pixel to unchanged contents. No raw files.
- Occlusion1 failed with two validation errors: Plume began the first query
  outside its deferred render pass, and the fixture copied both depth/stencil
  aspects into a buffer. The first is a production backend defect; the second
  was corrected to the existing depth-only readback pattern.
- Plume2d206ee is pushed to noeldvictor/plume:main. It starts the render pass
  before an occlusion query and makes failed visibility reads nonzero rather
  than exposing stale zero counters. The latter error branch is defensive,
  not independently fault-injected. The scope requirement is specified by
  [Vulkan query begin/end valid usage](https://docs.vulkan.org/spec/latest/chapters/queries.html#VUID-vkCmdEndQuery-None-07007).
- GPU39/occlusion2 passes8 cases1.09/1.10 s, validation0 errors/0warnings.
  One unrelated GOG overlay manifest loader message remains.

## First integration and causal follow-up

Host117/PID28944/session67898 links successfully, codegen0 written/no guest
objects. Exe48,702,464 B SHA256
`89E2E065B0B4020521C1F3C11BA7B345F6739B0FC52A7CEC7170EC6C3D715D93`;
PDB109,670,400 B SHA256
`6F6CC13E922E4419800BBED6ED529434003C82B10786A1D2F8A86405D1BCA318`.

Run957/PID3732/session47473 stopped with a loading-stage access violation at
20:10:47, before fresh field/query acceptance. All21 temporary settings took
effect; the supervisor stopped its process and restored the116 B profile exactly
(SHA256 `2F1BC38D763A1B7BDBA31F560684FD4AA7E42A714600B8D344F19DA7F38E23B0`).
The87,567 B log is retained. No new raw/image/perf/cook output was requested.

Matching host117 PDB symbolization maps return RVA02444463 to
`VulkanCommandList::setDescriptorSet` (plume_vulkan.cpp4465, inlined bindings3303)
and0244493d to `setGraphicsDescriptorSetDynamic` (3602). The new query scope had
left its push-only layout active for the next descriptor consumer. The existing
queue restores an explicit `EngineGraphicsBindings` snapshot; queries now use the
same binding core through `WithNativeOcclusionBindings`, including exception
restoration. This is a transitional outgoing interop handoff, not query inputs
or a claim to have removed every translated binding from the frame.

Before another live attempt, GPU40/occlusion3 adds a real three-dynamic-offset
descriptor consumer after the query using that exact production scope. All8
cases pass1.31/1.32 s with validation0/0 and unchanged pixels. Runtime correction
was then integrated and exercised as follows; host117 is not restamped.

## Corrected host118/run958

Host118/PID22648 links with no guest objects. Exe48,701,952 B SHA256
`ECE88C333AA6E40CF732238044FD27CED20DCE8012A21392A2EDBB235CE7B8FD`;
PDB109,674,496 B SHA256
`F3682E0E7023FC0702AE18487B15C54D57E117774C7BB7D14F403787848ACA59`.
GPU40 fixture SHA256
`801C040B81DAAA789404ED310E8676F970F8175387F72574CB639525238C3602`.
Run957 log SHA256
`4F3248DB7BA8BABE0B2370FB6A6608A006815143CC2A5743FE0CEA35278E8652`.

Run958/PID36336/session19501,20:18:15..20:20:21, exits0 after the full strict
cold/reload chain. Generation93/instance144 fully retires at title before
generation206/instance384;900 fresh selected scene and shadow emissions occur
in each interactive epoch. The reload4373..4673 window emits/retires900 textured
scene cutouts and300 textured shadow casters. Wider casters emit33,156/retire33,158;
native scene emits2,420/retire2,419. Batches remain singletons. All21 effective
temporary settings took effect and the profile was restored exactly.

Native query samples now advance in both epochs. At frame4674 the cumulative
totals are515,479 noted,450,896 queried,450,679 fence-collected,101,924 zero
results and0 native-skipped draws, with227 history entries. The fresh reload
4374..4674 query window adds69,411 queries /69,287 collected /17,477 zeros,
still0 skips. These query samples are one frame after the material gate samples;
they independently show query execution, not the same sample boundary.
The current501,130 B log SHA256 is
`3006F1ED173A8B19AED7AD62C0132D99F046646F0B2E81CF7E8CAEA64D31E8FC`.

The descriptor handoff correction survives both live epochs. Effective native
culling remains unqualified: queries include nodes that may still use legacy
draws, and the current log does not distinguish missing consumer versus changed
camera/bounds/depth/history. The next decision is to restrict queries to eligible
native consumers and record bounded rejection reasons, then establish controlled
hidden/visible native draws and pixels. Do not relax safety checks to get skips.

The separately attempted110 KiB JPEG found the renderer already terminal and
wrote nothing; no raw or game image was produced. Existing sanity images total
10,437,840 B, leaving47,920 B under10 MiB: the full110 KiB reservation does not
fit. Identify/retire a superseded image before any new image producer. This run
does not replace956's tree-gap question or945's last accepted game pixels.

Storage accounting and producer limits continue in the
[same cumulative ledger](20260906_0333_native-scene-state-bridge.md).
Preserve host107/run945 accepted pixels, run956's tree-gap evidence, and the
unresolved948 UV /941 light-selection failures. Current live reload/query evidence
is scoped above; game pixels and actual native draw skipping remain open.

## Consumer-driven queries: host119/run959 (2026-09-07)

Bundle: remove broad walk-time query publication for legacy-only nodes, reuse the
owned node/pose/pass, native rigid admission and existing query/history/fence path,
and request observations only after ordered light effects and complete sibling
preflight, with at least one actual native draw pending. The walk passes its owned
world sphere by value; no source address or new GPU readback enters the query.
`FindNativePassOcclusionView` and the walk's `OcclusionCullNote` are removed.
The active native command owner supplies the view at consumption. Empty query
passes avoid pipeline creation/layout/descriptor handoff entirely.

`NativeOcclusionTracker::Request` both admits a query and decides culling. A culled
candidate still requests a refresh. Legacy-only/suppressed nodes never call it.
The history predicate is unchanged; zero sample count is additionally refused.
Duplicate conflicts remain invalid until pass end, bounds/camera/scope/identity
and originating-frame age remain exact. Bounded decision counters report even
when no queries emit, without per-node logs. Camera mismatch takes precedence
over history warm-up/visible classification; these are first refusal reasons,
not mutually independent causes. Invalid bounds includes missing/nonfinite/near
clip packet rejection, so its runtime count does not prove one particular subtype.

Verification: 326 Python source/scenario checks (0.156 s); output39/PID29224 and
CPU22/PID37148 pass (0.37/0.39 s). CPU cases cover requests, missing input, duplicate
invalidation, depth/sample/camera/bounds changes, delayed/old/out-of-order results,
history capacity/pruning and refreshing culled candidates. GPU41/PID32580 builds;
occlusion4/PID29660 passes all eight mono1/2/4/8-MSAA × translated/rotated cases
in1.10/1.11 s, validation0 errors/0 warnings. There are now two real command
submissions and fences per case, not duplicate stamps on one result: hidden
warms after one zero, then receives the production Occluded decision; front and
intersecting candidates stay Visible. The existing exact HDR/depth preservation
and descriptor-resume checks pass. The fixture does not prove skipped game draws.

Host119/PID38816/session51328 links0:29 host steps, codegen0 written, no guest
object compilation. Actual binary stamp c9bbc8e dirty; subsequent docs/commit
do not restamp it. Exe48,703,488 B SHA256
`89285D58704A346C093CB4457E37EF961A85D9BAAAFC7BD218F6909558E4566B`;
PDB109,686,784 B SHA256
`80EC3A22FD676E647E8D77748E03CFB3F2780A70C02EAFD3F6F6284198AA5B72`.
GPU fixture SHA256
`4C1C5F1666167267D23D4637CA330BD970B8871104EAC599E93C921FFD6C0D69`.

Run959/PID2480/session20034,20:42:39..20:45:00, exits0 after the full strict
cold/reload chain. Generation93/instance144 fully retires before207/384; both
interactive epochs again emit900 selected scene and shadow primitives. All21
temporary settings took effect, no raw/image/perf/cook/dump output, exact normal
profile restoration SHA256
`2F1BC38D763A1B7BDBA31F560684FD4AA7E42A714600B8D344F19DA7F38E23B0`.
Retained511,958 B runtime log SHA256
`F7CC0D355357FDCF10C618CC1A9BA1817AF465DAAFA3BF0680EA2179C8ABD6C8`.

Last query sample4800: requested31,649, queried8,960, collected8,954, zero3,034,
native-skipped0, history6. Decisions sum to31,649: invalid bounds22,689,
changed camera8,613, no history304, visible43; invalid view/ambiguity/capacity/
changed depth/changed bounds/stale/warming/occluded0. Earlier stationary field
samples contain no new queries because candidate bounds fail preparation.
Camera changes dominate usable inputs. This resolves the admission question;
it is not useful game culling or a controlled performance comparison with958.

Separate fresh reloaded4352..4652 window: native scene9,515 emissions /9,517
retirements, wider shadow39,704/39,748, textured scene cutouts3,334/3,336 and
textured shadow300/300. Scene batching emits9,515 instances in7,893 indirect
calls; `NoteNativeRigidEmission` counts2,813 instances in groups of size>1
(not2,813 saved calls). The corresponding scene-call difference is1,622.
Shadow batch sample39,966 instances/calls remains singleton; its instrumentation
boundary is distinct from the wider-family counters. Sampled layered scene0.
This is the first fresh non-singleton runtime evidence here, not a causal
speedup claim for this edit. Camera movement/scene exposure differ from958;
pixels, representative layered/inherited inputs and both-eye gates remain open.

Next: native canonical geometry currently has no retained tight bounds
(`native_mesh.h::NativeGeometry`); the rigid vertex program transforms native
position by `object_data.world`. Derive conservative load/cook-owned bounds and
consume that exact transform, avoiding the remaining walk radius-scale adapter
where possible. Test containment of actual indexed geometry and near-plane
rejection before live use. Moving-view visibility still needs a safe design;
do not replace exact camera equality with an epsilon to obtain skip counters.
Preserve945 accepted pixels,956 gaps,957 descriptor failure and948/941. Storage
cleanup and retained growth are recorded in the same cumulative ledger.
