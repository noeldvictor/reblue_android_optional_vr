# Native effect preparation and cleanup

2026-09-06, desktop Vulkan; continuing the full host-renderer transition.

## Source and ownership change

Read the complete translated/PPC bodies before implementation: sub_8221DB00
(generated file 43), sub_8221D548 (95), sub_8221DBE0 (74), sub_8221DCA0 (71),
sub_8221D5E0 (14), and the thin sub_8221D530 wrapper (98). The hook map has no
instruction-site hooks for these addresses. Confirmed deferred_consumer.cpp
calls the group-1 pair using the same registry address. native_material.cpp's
ModelOwnsReflectionBinding also identifies the group-0 resource begin slot.

The address-free native_effect_lifecycle.h now executes both preparation groups,
paired cleanup and three-array teardown. Six whole-function hooks in the existing
effect bridge cover both model entry points, model finish, visual prepare/finish
and global registry destruction. They use the existing bd_native_passes switch.
The host deferred consumer reaches these native hooks without a second list walk
in translated code. No generated source, configuration hook, shader or gameplay
change is required.

Preparation snapshots the signed count and reads each current slot. Results 1
and 2 mark the post-callback live slot active; 1 becomes 2 and continues, 2 stops,
and 3 returns immediately. Other results remain exact signed values; the final
result is not an accumulated acceptance bit. Model preparation reads the input's
resource after participant callbacks, publishes it even when null, and invokes
its begin callback only when nonnull. Result 3 leaves the prior resource intact.
Resource callback results do not replace the preparation result.

Model finish invokes the current resource's finish callback, then clears the
published resource. Only afterward does it snapshot the participant count.
Both groups finish only exact active byte 1, and clear the live slot after each
callback. Array teardown reads/frees/clears groups 2, 1, 0 in order and resets
capacities/counts. It does not destroy participants or finish the prepared
resource: those are separate lifetimes. It uses the same temporary allocator
as native insertion/removal; no allocator mismatch or redundant host mirror.

The boundary checks mapped/aligned stack, array and model-input ranges before
work, retains a 256-B callback ABI frame, and restores the caller's stack through
RAII. Invalid initial imports may use the original path; faults after any native
effect propagate without replay. Callback destinations are validated at the call
and record last_indirect_target. Lifecycle counters are thread-local, including
per-group prepare/finish, participant/resource callbacks, teardown/free and
compatibility/refusal; fault count is explicitly shared with the effect bridge.

This removes preparation/cleanup/teardown execution from translated rendering,
not all registry dependencies. Participant callbacks, metadata/flag producers,
shared allocation storage and imported object identities remain conversion work.
There is no claim of complete native scene/resource/frame ownership.

## Verification

The CPU fixture exercises all 512 three-status combinations for each group,
including 0/1/2/3, negatives, signed extremes and 0x101; empty/nonpositive counts;
live slot/count/resource mutations; all 256 active bytes; null publication;
resource-before-count cleanup; repeated teardown; and callback failures without
replay or premature clearing/free. This is algorithm evidence, not visual proof.
Seven effect source guards plus 5 view, 24 scene and 36 post guards pass (72).

Focused target host_scene_pass_test_05: PID 22992, exit 0, two compile/link steps.
CPU suite cpu_18: PID 23732, exit 0, 31/31 in 3.32 s. Host reblue_21: PID 24860,
session 74453, exit 0, linked at 05:40:59 EDT. Expected GLOB reconfiguration;
codegen reports the module up to date, no guest objects or shaders rebuilt.
The final build display is 12/15; six build/test logs total 6,319 B.

Binary reblue_vk.exe: 47,683,584 B, SHA256
`0478717c13eed6645a531d194f08c34e47ada1b73ddadae8f84f095a5f7d7d21`.
It records root 61e6b2f with the pending renderer integration and Plume 81bdca8;
this is not a clean-parent-only build claim. The native lifecycle change itself
uses previously committed APIs and can be committed independently of the
unpublished dependency integration. Storage remains on the original cumulative
ledger in research/20260906_0333_native-scene-state-bridge.md.

Flat MSAA: owned PID 7812/session 18853, 05:41:43--05:42:58, wrapper exit 0.
Log 864 is 267,329 B; all 5 settings took effect. Render thread 27540 records
381,870 model prepare/finish pairs, 681,379 visual pairs, 2,126,498/2,114,391
participant begin/end callbacks and 369,763 resource begin/end pairs. Activation
thread 25984 has zero lifecycle work and the same 28 registrations/9 removals/
40 insertions/18 erasures as before. Both threads report zero compatibility,
refusal and shared faults. Parent native views 3,601, dispatcher begins/ends
9,941/9,940 at its sampled boundary; scene results/clears 3,600, post 3,601, zero
ownership errors, compatibility clears/depth publications, state-308 adapters,
parent/dispatcher fallback/refusal/faults or native-post imports/refusals.

Inspected owned-window PNG native_effect_lifecycle_window.png at about 60 s:
1920x1080, 3,278,821 B, SHA256
`b8b4ff98d91cb5fcbb8c8e040e8b838d4ce253e5f51844c6c9d2f8e84341e7c8`.
Shu, terrain, foliage, structures, cast shadows and distant DoF are visible
without obvious full-frame corruption. This unaligned sanity image does not
prove pixel equivalence, temporal stability or either-eye correctness. Perf
054145 is 602,112 B plus 112-B metadata; ending free 64,533,057,536 B.

Desktop XR MSAA: owned PID 22472/session 75656, 05:43:53--05:45:10, wrapper exit 0.
Log 865 is 616,049 B; all 16 settings took effect. Existing absolute xrsim manifest,
1440x1584/eye, multiview, height 0, scale 1, mirror/preview off. Render thread
25744 records 723,492 model prepare/finish pairs, 1,151,643 visual pairs,
3,750,270/3,731,735 participant begin/end calls and 704,957 resource pairs.
Activation thread 26136 again has zero lifecycle work, with 28 registrations,
9 removals, 40 insertions and 18 erasures. Both report zero lifecycle/activation
compatibility, refusal and shared faults. Parent views 9,601; dispatcher begins/
ends 23,270/23,269 at its sampled boundary. Scene results/clears 9,600, post 9,601,
zero ownership errors, compatibility clears/depth publications, state-308 adapters,
parent/dispatcher fallback/refusal/faults or native-post imports/refusals.
Perf 054355 is 1,650,688 B plus 112-B metadata; ending free 64,529,760,256 B.

Both runs added zero raw/cache/dump files, stopped only their owned renderer and
restored the profile byte-for-byte. Final whole-log error/config/VK_ERROR scans
were empty. They cover startup/field preparation, not the full-game/scene-change/
both-eye gate or Quest performance. Teardown/free counters are zero: global
destruction has CPU coverage but was not exercised by these timed stops. No
natural-shutdown or Vulkan-validation qualification is claimed.

Thirteen superseded small diagnostics were removed after equivalent checks and
PNG inspection: 6,275,409 logical B, 6,283,264 measured B recovered. The original
ledger records exact purposes and totals. Closing free space 64,535,580,672 B
(60.10 GiB); net volume growth from preflight 4,329,472 B. No protected evidence,
game/save/profile/source/dependency or build tree removed. Required current
evidence replaces predecessors; no new per-commit raw archive. Push remains
unapproved; no attempt or unpublished Plume gitlink change is made.

## Next conversion boundary

Participant/resource callbacks, stable native registry identities/storage and
scene production remain. Further source review read all of sub_8221D2C8 (file
38): for phase 3 it conditionally resolves a current scene region, for phase 5
it resolves a 256x256 region, then binds an imported texture to slot 13. Its
direct callers include sub_82454720 (file 38, guarded by field +4700) and
sub_82455150 (file 89, after bdShaderConstantFlush). Call-site excerpts were
inspected; the full callers and native image/sampler ownership must be mapped
before replacing this remaining console-style path. Do not just move its D3D
resolve call into another host wrapper. The neighboring sub_8221D028 is a
40-byte record-array growth routine, not the effect participant registry;
sub_8221D4D8 and sub_8221D460 concern a separate object/counter lifecycle.
