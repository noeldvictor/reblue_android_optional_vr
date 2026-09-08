# Owned scene consumption and late light finalization

2026-09-08, parent016d3a9. Host129 is016d3a954 dirty, not a restamp of host128.
The last implementation checkpoint was progress; the intervening commit/push
request confirmed a clean, already-pushed tree and did not advance rendering.

## Connected change

The current direct native scene path now hands a `NativeRigidSceneSubmission`
to `SubmitNativeRigidScenePackets`. It owns primitive plans and native identity
values, not a pose, source model or object-scope pointer. The consumer uses the
existing native programs, descriptors, indirect queue, current-depth visibility
and slot-fence retention. No second renderer, resource registry or deferred ABI
was introduced. It checks the publication frame and both packed camera rows
against the active mono scene scope; packet depth writes feed both PSO and queue.

The object producer captures an owned Bind/Keep light recipe. It does not seed
the GPU record with early inherited values. The consumer resolves the first
recipe at its submission position, checks every sibling's action, packs all
three semantic light slots transactionally, then commits the revision-checked
outgoing compatibility mirror before taking the backend lock. Invalid siblings,
nonfinite lights, wrong publication and repeated finalization refuse; no partial
node is submitted. The stack argument belongs only to that outgoing mirror and
comes from the walk's current frame, not a retained object-scope stack address.

Removed `FindNativeSceneLights` and the scope-dependent
`CommitNativeRigidSceneLights`. Existing direct rendering actually uses the new
capture/consumer connection; this is not an unused alternate draw implementation.
`SetRigidLights` is shared by initial GPU packing and late finalization. Native
light-read counters now count whole-node resolutions, not per-primitive prepares.
No guest call-count reduction or timing improvement was measured.

## Source decision and remaining interface

The guest-source skill reused the existing deferred callback map. One bounded
new upstream trace found the roster producer: `sub_82182DE8`, generated80:4104,
calls `bdRenderInfoInit`, invokes participant virtual+32, and only on success
sets eligible/active bytes and calls the existing native registration hook.
Its sole direct caller is `sub_82182FB8`, generated98:4194. Registration attempts
cover the common shader/light participants and feature families; the reflection
loop has11 objects and the indexed loop8. Registry metadata still decides group
membership. Initialization is not proof that every late ordinary callback is inert.

The sorted producer is **still unconnected**. `ConsumeDeferredList` and its
temporary callback/resource adapters are unchanged; default scene admission still
excludes deferred work. The next connection must account for late authored
material/pass/suppression outputs and visual begin/end order, then feed owned
sorted packets to this consumer. Do not solve that by retaining console entry
images as the native API or assuming the shader-selection participant is the
entire chain. No new game/pixel, inherited-light, full-scene/stereo or Quest gate
is qualified. All prior UV/light/tree-gap/title-logo failures remain open.

## Verification

The devloop skill selected the existing trees and bounded supervisors; all jobs
completed0, with no new game, raw capture, image export, asset cook or profile edit.

- Output build49/PID38380 and CPU32/PID28552 pass. New production-plan tests
  capture Keep before a seed exists, destroy producer inputs, change the preceding
  Bind from red to blue, finalize two owned siblings and verify every light slot.
  Seven refusal controls preserve both siblings' complete GPU pass records;
  object/camera/fog/resources remain unchanged. Existing JPEG tests stay in RAM;
  their incomplete-scanline controls intentionally print diagnostics.
- Material build41/PID35024 and CPU39/PID38140 pass, including selection,
  publication, lifetime and shared GPU packing regressions.
- All352 Python source/scenario checks pass. Wiring assertions follow the new
  consumer and enforce no pose/scope/source lookup, finalization before mirror
  before backend locking, and whole-node preflight before enqueue.
- GPU build51/PID38416 and rigid18/PID25132 pass all55 existing two-eye cases
  in1.26 s on RTX3060. Validation0 errors/0 warnings; one unrelated missing GOG
  overlay manifest message. Shared C++ packing changed, shader behavior did not;
  the header dependency caused native shaders to recompile. Readback stays in RAM.
  These cases do not exercise the live mixed compatibility list.
- Host129/PID33244, session36460, terminal0. Codegen0 written/1 module current;
  no guest objects. EXE48,763,392 B SHA256
  `B0AAB5CE3A68BB855839C4847FBD6BBE153DABE5CD8D3D5A02858D61CD13FCF5`;
  PDB109,981,696 B SHA256
  `5D7C13C237ED47BC09BB3D08A021E39A62367C965546882DB5FB3C50ADA7D9B9`.

The new live connection still needs a fresh targeted game/pixel regression;
fixture success does not restamp run945 or explain the preserved failures. The
pending larger-image question grants no extra budget. Storage is reconciled in
the original `20260906_0333_native-scene-state-bridge.md` ledger.
