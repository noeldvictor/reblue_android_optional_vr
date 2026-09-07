# First direct native rigid caster

2026-09-07 EDT, parent b9d3076. The selected real field asset now reaches the
native shadow shader and shared submission backend. This is an opt-in caster
checkpoint, not complete object/scene ownership or the full renderer goal.

## Contract and development order

The guest-source skill kept investigation in existing translated bodies,
hook metadata and producers. `bdSceneTreeDraw` builds the traversal context;
the shadow walk has phase1, while `NativeObjectTextureScope` intentionally
owns phase0 colour/texture semantics. The depth-only native VS consumes only
`object_data.world` and `pass_data.world_to_shadow`. It needs neither the scene
texture-view fix nor fog, albedo, selected lights or shadow receiver colour.
Those remain explicit scene-path work, not fabricated defaults for a caster.

`host_walk.cpp` now publishes checked object-pass policy once per traversal,
then passes its owned instance pose and model-local node to direct submission
after existing culling and before `bdSceneNodeDrawSingle`. The consumer has no
source addresses, resource lookups, guest callbacks or template access.
Source tree discovery, policy import and original pose production remain
transitional producers. This does not claim source-free GPU asset loading.

`bd_native_rigid_shadow` defaults **false**. When enabled, geometry identity
258694267A8DBAEE selects the acceptance family. Whole-node admission requires
the sole primitive, matching owned material63B8D67932573E51, technique0,
explicit zero declaration bones, canonical vertex input, solid known routing
and finite affine world/pass matrices. Unknown texture-dependent routing,
skinning, alpha/deferred primitives and extra siblings refuse before submission.
A recognized refusal throws visibly instead of interpreting/capturing/replaying
that node. Other families continue through their existing path.

The renderer reuses `CreateNativeRigidPrograms`, `GetOrCreatePipeline`,
`GraphicsBindings`, native pass commands and `DrawQueuePush`. Only set0's two
dynamic constant bindings are required by the caster. Native matrix uploads use
the existing bounded host upload arena, now with CONSTANT usage and the device's
uniform offset alignment. No mapped upload data is read back on the CPU.
The device retains at most8 shader/input pairs and4096 per-slot descriptor/geometry
records; the matching frame-slot fence drains those records. No implicit
translated instance-record ABI: native batching/indirect remains follow-up work.

Camera publication belongs to the exact active native framebuffer scope and
frame/view. World-only updates cannot bootstrap a camera. Both camera components
must have been explicitly produced; suppression, fallback, nonfinite values,
derived overflow and unobserved cache changes invalidate ownership. A nested
scope cannot borrow its parent's camera. Direct submission rechecks the active
camera before enqueue. The existing source camera producer is still an adapter.
Unqualified early receiver-colour scaffolding was removed from this checkpoint:
the real receiver callback can observe later authored values.

## Verification

The devloop skill selected the existing fixtures first, then one coherent field
run. Output16/PID27088 exposed a Windows `max` macro in the new overflow test;
fixed with the parenthesized numeric-limits call before retrying. Output17/
PID29516 and CPU2/PID24408 pass:0.34 s assertions/0.36 s CTest. Coverage includes
partial camera updates, world-only/late/suppressed writes, new frame/view/nested
scope, NaN/Inf/overflow, whole-node rejection and retained geometry after model
source destruction. Native geometry's header now uses standard fixed-width
types without a ReX header; layout and persisted formats are unchanged.

GPU fixture24/PID19644 and rigid03/PID31484 pass. The production shadow shader
binds only its matrix set, then the scene shader binds its complete descriptors.
Four8x8 two-eye colour/depth cases pass on RTX3060 in1.07 s/1.08 s CTest,
maximum error0.0000404567, Vulkan validation errors0/warnings0. A known missing
GOG overlay manifest generates one loader message, not a validation warning.
No raw/image files are written by this fixture. These are fixture pixels,
not live-game two-eye qualification.

275 all-boundary Python source/scenario checks pass, including stale/cross-scene/
reset/identity/refusal tests for the new direct-caster gate. Host93/PID29384
passes;104 scheduled build steps, host recompilation/link only. Codegen says
0 written/up to date; no guest object or shader rebuild. Existing settings-row
and CRT warnings remain. Adding a device-owned field rebuilt its host dependents;
this is not the cost of an ordinary one-file edit.

Run935/PID24424,08:43:40–08:44:42, enables the caster from startup. First native
submission: frame787, node64/instance144/generation93/phase1. Fresh native reports
at frames1987/2287 add300 submissions and300 matching fence retirements; both
occur after ready-field context1687 and the opening event at1387. All previous
field/movement gates pass, including300 native shadow-image handoffs and no
image ownership/fallback mismatch. Selected-light publication adds14,401 complete
checks and900 changed slots; sampler checks add3,626/40,074 owned-input draws.
This records submission and resource lifetime, not GPU timing or a speedup.

The inspected1920x1080 image shows running Shu by the fence, coherent character,
tree/fence/prop shadows, and the known black cliff marks/distant blur. It is
sanity evidence, not an isolated per-caster pixel oracle, stable sequence,
reload or both-eye qualification. No live native scene shader, native rigid
instance batching, source-free cold-load/reload or full-game acceptance is claimed.
Sixteen settings audited; original116 B profile restored exactly. No owned
producer remains, no new raw/perf/cache/dump/cooked outputs.

| Retained artifact | SHA-256 |
| --- | --- |
| Host93 exe48,457,216 B, tested by935 | `CEAADAE41F5B4D964C792F469BD1566620606E2EEEB168160603D626F85C8BF1` |
| `out/build/win-amd64-release/logs/reblue_935.log`,242,052 B | `5F069D9EF341815B7A7856FB90A3EF5F6E0451A7234737B96C1D992DA7D5DE2A` |
| `out/verification/native_rigid_shadow_window.jpg`,148,199 B | `08C451B509F0142542B8B2D9C07A4AE7EFC8130FE6B5D13E6B876F4595BA34ED` |
| Restored profile | `2F1BC38D763A1B7BDBA31F560684FD4AA7E42A714600B8D344F19DA7F38E23B0` |

## Storage and next step

Same cumulative3 GiB exception,62,509,998,080 B operational floor/raw0 gate;
no reset. Existing build256 MiB/runtime192 MiB free-drop stops,75 s runtime,
400 KiB log/160 KiB JPEG and10 MiB aggregate log/image limits remain enforced.
Existing trees and cooked asset reused; no new framework, download or recook.

After replacement validation, removed14 exact superseded agent files: host92,
output15/CPU1, failed output16, GPU build23 and rigid02 stdout/stderr, plus934
text and the shadow-image JPEG.386,909 logical B; immediate free
62,832,992,256 ->62,833,393,664 B: **401,408 B (392 KiB) actually reclaimed**.
Build logs are reproducible; exact retired runtime text/image are gone, with
prior hashes/findings retained. All distinct baseline/flat/VR/movement/failure/
raw evidence and game data remain protected. Retain935/current fixtures until
equivalent replacement or a distinct coverage reason permits cleanup.

Output preflight62,877,323,264 B; cleanup-end58.52 GiB free,43,929,600 B
(41.89 MiB) drive-wide use. Known retained component growth2,443,146 B is mostly
the expanded CPU fixture, plus replacement log/image and binary/PDB growth.
Other GPU fixture/object/source/Git and unrelated drive changes are not fully
attributed. The existing cumulative ledger holds the same measurements.

Next: finish the **scene** consumer for this same object. Qualify the uploader's
2D-array image views against the shader, publish receiver values at correct
timing, and move the owned per-node light selection ahead of the callback.
Then native batching plus interpreter/template-free cold-load and teardown/reload
acceptance. Do not turn this caster into another reason to postpone that consumer.
