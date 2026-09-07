# First direct native rigid scene consumer

2026-09-07 EDT, parent9759a75. Host95/run936 connects the selected rigid object's
scene shader to its existing native caster, owned packet and shared backend.
This is an opt-in mono consumer checkpoint, not a complete native scene/frame.

## Ownership and remaining boundaries

`host_walk.cpp` admits the complete selected node before `bdSceneNodeDrawSingle`.
The scene path binds native shaders/layouts, canonical geometry, retained albedo
and completed primary shadow views, named object/pass constants and native
samplers. It uses the existing pipeline cache, upload arena, scene commands,
sampler cache and draw queue, not a captured pipeline or another renderer.
Geometry/images/descriptors survive through the matching frame-slot fence.
The existing8-program/4096-record-per-slot limits remain shared with the caster.
Native instance batching/indirect support is still missing from this route.

`bd_native_rigid_scene` defaults false. Geometry258694267A8DBAEE selects the
acceptance family; node64/material63B8D67932573E51 has one opaque primitive,
zero declaration bones and one texture layer. Whole-node preflight requires
complete owned material/features/policy/light/fog/camera/sampler inputs. Extra
siblings, unknown/inherited inputs and unsupported families refuse before a
draw; recognized refusal throws, never silently interprets/captures/replays.
Other objects retain their existing path. Missing native identity and other
pass entry routes still need the separate hard-off cold-load/reload audit.

Completed primary shadow publication retains its actual sampled D32/S8 image
and camera, invalidated at the next begin and checked against the current frame
and SHADER_READ layout. Receiver colour is read after the real object callback's
late flush, keyed by visual/frame/view and checked against that exact image.
**The original `sub_82176708` receiver callback still executes.** Its texture/
constant compatibility consumers are not removed by moving this selected draw.
The direct backend never reads source addresses or translated constant arrays;
`PrepareNativeRigidSceneForObject` is the explicit temporary source boundary.

Per-node lights are prepared before the selected node's old shader callback.
The existing native selection and publication cores are evaluated transactionally
against a read-only overlay: no original comparison callback or compatibility
write occurs on this direct path. Unknown unchanged cache slots refuse instead
of borrowing an unqualified previous object. Authored light snapshot/storage,
the compatibility light cache, receiver visibility and source object/camera/
fog publications remain dependencies. This is not independent native loading.

Source investigation used the guest-source skill and existing translated bodies:
`sub_82176708`, `sub_8218B310`, `sub_82142C58`, host parameter projection,
selected-light cores and hook metadata. The selected UV contract stays
`(uv+1)/512 + live offset`; large authored wrap coordinates are not rebased.
Explicit single-layer array views match the previously verified shader contract.
The native four-tap shadow filter uses authored bias,0.4*bias slope strength and
0.65/width UV radius. This is an explicit native policy, not a claim of exact
translated depth-proportional-bias parity. Initial scene acceptance refuses
layered targets; explicit per-eye camera production is still required for XR.

## Verification

The devloop skill kept the inner loop in existing fixtures, then one connected
field run. Output18/PID30216 and material30/PID972 passed their incremental
builds; outputCPU3/PID25628 and materialCPU28/PID30776 passed. A final lifetime
test improvement removes every alternate resource owner, checks that the plan
alone retains each resource, then verifies expiration when the plan retires.
Output19/PID30400 and CPU4/PID29012 pass,0.34 s assertions/0.36 s CTest.
MaterialCPU28 is0.12 s/0.14 s. All280 Python boundary/scenario checks pass.

Fixtures cover complete packet refusal, receiver late updates/frame/view/object
stamps, invalidation, array views/layouts, UV units, source destruction and
read-only per-node light selection/publication. New scenario checks reject
queued-only, stale, reset, wrong-scene/generation and impossible emission/fence
counts. An initial source guard caught producer placement inside the consumer
slice; it was moved outside that boundary, without weakening the guard.

Host94/PID30444 failed on a missing `ResolveGuestTexture` declaration. Adding
the owning include fixed it; host95/PID28064 passed. No guest objects rebuilt;
codegen reported0 written/up to date. No shader change this checkpoint: reuse
GPU25/rigid04's production-array-view four-case two-eye Vulkan pixel evidence
from19c9da6, not a newly run GPU fixture. Known CRT deprecation warnings remain.

Run936/PID28504,09:42:32-09:43:43, enables both direct switches from startup.
First scene/caster submissions occur at frame766 during loading. After the
opening event at1366 and ready-field context1666, later windows1966/2266 add
**300 native scene submissions,300 actual draw command emissions and300 fence
retirements**, with the same node64/instance144/generation93. The caster and
completed shadow-image gates add300 each too. All existing field/material/
light/fog/sampler/geometry/movement gates pass. Command recording and resource
retirement are not GPU duration or a speedup measurement.

Inspected1920x1080 renderer-window JPEG: running Shu beside the fence, coherent
terrain/props and character/tree/fence shadows, known black cliff marks and
distant blur. This is one sanity image, not an isolated selected-object pixel
oracle, stable sequence, reload or both-eye game qualification.17 settings took
effect; the owner's116-byte profile was restored exactly. No producer remains,
and there were no new raw/perf/cache/cook/dump outputs.

| Artifact | Bytes | SHA-256 |
| --- | --- | --- |
| Host95 `reblue_vk.exe` | 48,493,568 | `B760AAFE99178778B2FA5AF0007F8E34C8B54E88DE710E4B626FC4CA6C4EB1BF` |
| `out/build/win-amd64-release/logs/reblue_936.log` | 242,091 | `0A2A2023BD95D4ED8928B34D8689E97565C64089565CDA61946DE7AE72B952D6` |
| `out/verification/native_rigid_scene_window.jpg` | 141,437 | `4D1930033642FB01C5C84A45515B706ABE8D968CA84796EAA048D1D162D2AF50` |
| Restored profile | 116 | `2F1BC38D763A1B7BDBA31F560684FD4AA7E42A714600B8D344F19DA7F38E23B0` |

## Storage and next gate

Same cumulative3 GiB exception,62,509,998,080 B floor and raw0 gate; no reset.
Builds used256 MiB free-drop/300 s stops and10 MiB aggregate logs. Runtime used
192 MiB free-drop/75 s,400 KiB log and160 KiB JPEG/10 MiB aggregate image limits.
CPU18's initial under32 MiB estimate was exceeded by a130.52 MiB drive-wide
transient drop; most returned during material30. Attribution/true peak are
unknown. Remaining host/fixture overlap was corrected to under192 MiB with the
existing256 MiB stop; the original cumulative floor was preserved throughout
the sampled runs. No new build tree, download or recook.

After validating replacements, removed18 exact superseded agent files:
host93/failed94, output17/18, CPU2/3, material29/CPU27 stdout/stderr, plus935's
log and caster-only JPEG.431,108 logical B; immediate free63,143,288,832 ->
63,143,739,392 B: **450,560 B (440 KiB) actually reclaimed**, credited once.
Old exact runtime text/image are gone; their hashes/findings remain. Build/test
logs are reproducible. Keep936/image, host95, output19/CPU4, material30/CPU28,
GPU25/rigid04 and all distinct baseline/flat/VR/movement/failure/raw evidence.
Replace equivalent evidence after qualification, not on every commit.

Known component growth1,029,039 B, chiefly strengthened CPU fixtures and host
code/PDB. Replacement logs/image are smaller. Cleanup-end63,143,739,392 B free
(58.807 GiB),106,983,424 B more than output preflight63,036,755,968 B. This
drive-wide gain is not all cleanup; source/objects/Git and other volume activity
are not fully attributed. The existing scene-state ledger owns cumulative
accounting, including the protected historical raw archive and zero incoming
allowance.

Next: reuse native owners and the submission backend for batching plus hard-off
cold load, movement, teardown and reload with fresh generations, for both scene
and shadow. Remove the remaining source producers/receiver callback at their
last consumer. Expand rigid material families only after that gate; character,
effects/UI, complete desktop/both-eye acceptance and then Quest remain required.
