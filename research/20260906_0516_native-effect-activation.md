# Native effect activation and ordered participant registration

2026-09-06, root 34aaa7f plus preserved renderer integration, Plume 81bdca8.
Previous goal turn was progress: whole-view scheduler committed and four bounded
checks completed. This turn uses guest-source/devloop, with desktop XR verification
planned through vrsim. No Quest work; the complete desktop host-renderer goal is
unchanged. The existing cumulative storage ledger remains in
`20260906_0333_native-scene-state-bridge.md`.

## Source and conversion boundary

Read the complete effect activation sub_82173DF8 (0x82173DF8, generated file 12),
registration sub_8221D678 (file 98) and removal sub_8221D9A8 (file 10), after checking
the hook maps and native scheduler/dispatcher callers. Read the complete array
helpers sub_8221DD10 (file 46), sub_8221DDA8 (file 65), sub_8221DE20 (file 100),
sub_8221DF78 (file 59) and bdEffectSlotArrayResize (file 67). Generated files and
hook addresses are not edited. Current hooks live in the common OBJECT library.

All twelve selector cases are represented: All invokes 1..7 in order; shadow,
eleven reflections, shadow volume, the two selector-4 participants, two plain
flags, post enable, the two mode encodings, indexed views and auxiliary. Values
remain exact bytes, not normalized booleans. Eligible participant flags publish
before membership callbacks; cached effect flags publish after the group. An
unchanged cached byte short-circuits exactly as before. Indexed view counts are
refreshed only after a membership callback; the remaining tail up to eight slots
is deactivated from the last observed count. Invalid indexed bounds refuse before
activation or fail explicitly if a callback makes them invalid later.

Registration visits three mask groups. Mask callbacks are re-queried for each
group, priorities for each comparison, and equal-priority entries keep insertion
order. Each group's scan count is a snapshot, but entry lookup follows the new
participant's priority callback and append uses the then-current count. Duplicate
registration is preserved; removal erases only the first matching occurrence per
selected group and never queries priority. Later import faults cannot replay a
partially executed operation through the original.

The address-independent array core now performs insertion, shifting, append,
erase and checked growth. First append allocates exact required capacity;
subsequent growth and insertion growth double the required size. Removal retains
capacity and stale unused tail values. Append growth zero-fills new spare slots;
insertion's new unused tail stays unspecified. Growth copies the prior capacity
contents for compatibility readers; no object reference counts are invented.
An explicit capacity check precedes mutation. CPU tests exercise the same core
used by the bridge, not just a separate vector model.

Remaining imports are authored flags/objects, callback-provided priority/masks and
the shared array storage allocator/deallocator. The native bridge does not call
the old registry/container algorithms. Existing engine readers still address
the arrays, so this is host policy/order/mutation, not a claim that the registry
storage or participants have native asset identities. Removing those imported
readers and storage ownership remains required. No GPU resources, asset formats,
console state packets, tiles, seed copies or resolves are introduced here.

## Verification

Four new source guards plus the five view, 24 scene and 36 post guards pass
(69 total). Expanded tests cover all 256 activation bytes, cached no-ops, all
selector families, readiness changes, publication order, live indexed bounds and
tail cleanup; signed/equal priorities, masks, duplicates, first-only removal,
callback mutation/failure and actual array growth/shift/capacity-failure paths.
Focused scene build `host_scene_pass_test_04`, owned PID 24896, exited 0 in two
steps. The existing CPU suite `cpu_17`, PID 25616, passed 31/31 in 3.29 s.
Their stdout/stderr are 139/0 B and 3,626/0 B respectively.

Host build `reblue_20`, PID 23792 / session 15345, exited 0, displayed final link
14/17. Expected glob reconfiguration added the bridge to reblue_common's OBJECT
library; codegen reported its module up to date and no guest objects rebuilt.
Build stdout/stderr 2,734/36 B. Executable 47,677,952 B, linked 05:17:58,
SHA256 `eb30468abafa0b962f4d1c046454835db797e313952e005ab07f63fa958ee024`.
The binary records 34aaa7f plus local renderer changes, Plume 81bdca8; this is not
a committed-only clean-tree build. Ending build free 64,574,910,464 B.

Flat MSAA: owned PID 21908 / session 71419, 05:18:55--05:20:11, wrapper exit 0;
log 862, 257,843 B. All five settings took effect. Effect counters are per thread:
thread 26732 reports 32,401 updates, 28 registrations, 9 removals, 9 membership
changes, 925 metadata callbacks, 11 shared-array allocations, 40 insertions and
18 erasures; thread 26528 reports 25,201 updates and zero registry mutations.
Both report zero compatibility/refused/faults. Parent native views 3,601;
dispatcher 9,946 begins /9,945 ends at its last sampled boundary, zero fallback/
refused/faults. Scene native clears and completed inputs 3,600, native post handoffs
3,601, zero scene ownership errors, compatibility clears/depth publications,
state-308 calls or normal-post imports/refusals. Sampling boundaries differ; these
counts are not shutdown-balance assertions or counts of fully native frames.

Inspected `out/verification/native_effect_activation_window.png`, from owned
flat PID 21908 at the 60-second inspection: 1920x1080, 3,200,734 B, SHA256
`8eddf4e2cd1bf0beeea8cbb51c89dc5e007b78d35b8fb76c33c6a4c85a1fd05e`.
Shu, terrain, vegetation, structures, cast shadows and distant DoF are visible
without obvious full-frame corruption. It is an unaligned single flat sanity
image, not a pixel-equivalence, sequence, event or stereo qualification. Perf
051857 is 606,208 B plus 112-B metadata. Zero new raw captures; ending free
64,570,527,744 B. The wrapper stopped its own game process and restored the owner
profile byte-for-byte; no natural-shutdown or game Vulkan-validation claim.

Desktop XR MSAA: PID 14736 / session 40248, 05:20:39--05:21:55, wrapper exit 0;
log 863 is 585,945 B. All 16 settings took effect: existing absolute xrsim manifest,
1440x1584 per eye, multiview, height 0, scale 1, mirror/preview off. Thread 27060
reports 86,401 effect updates and the same 28 registrations /9 removals /9
membership changes /925 metadata calls /11 allocations /40 insertions /18 erasures;
thread 21604 reports 67,201 updates and zero registry mutations. Both report zero
compatibility/refused/faults. Parent native views 9,601; dispatcher begins/ends
23,269/23,268 at its last sampled boundary, zero fallback/refusal/fault. Scene
native clears/results 9,600 and normal post handoffs 9,601, zero ownership errors,
compatibility clears/depth publications, state-308 calls or post imports/refusals.
Perf 052041 is 1,617,920 B plus 112-B metadata. Zero new raws or stereo pixel
exports; ending free 64,567,808,000 B. Same bounded owned-stop/profile-restoration
conditions, not full-game/both-eye/Vulkan-validation or Quest qualification.

Both runs exercise startup/field registry changes and sustained native activation.
They do not exercise every authored selector event; exhaustive policy/array cases
are CPU evidence, not substitutes for the required full-game visual gate. Final
whole-log scans found no config errors, critical/error lines or VK_ERROR.

Thirteen superseded diagnostics were removed only after equivalent flat/XR checks
and inspection. Measured reclaimed 6,418,432 B; the existing ledger records exact
targets, net volume growth and retained artifacts. No protected raw/failure or
distinct non-MSAA/recovery evidence was deleted. No unapproved push or Plume
gitlink change is made; this source/test checkpoint uses already committed APIs.

## Remaining ownership and next source boundary

The shared arrays still have engine readers and teardown. Read the complete
sub_8221D5E0 (file 14): it frees/clears the three groups in reverse order. The
sub_8221D530 wrapper (file 98) forwards to sub_8221DB00 (file 43), whose complete
body was also read: it is group-0 preparation dispatch, not re-sorting. It has a
snapshot count/live-slot active-byte update, status-1/2/3 early-exit semantics and
publishes a resource at registry+36 before its virtual +32 callback. That consumer
and lifecycle, further participant callbacks and the lazy scene producer remain
conversion work. Merely owning insertion/removal does not establish complete
native registry storage, identities or lifecycle.
