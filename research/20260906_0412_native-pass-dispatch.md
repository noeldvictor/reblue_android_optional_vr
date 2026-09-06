# Host pass lifecycle dispatcher

2026-09-06. The complete desktop host-renderer goal remains active; no Quest work.
Current root f07d853 plus the preserved local renderer changes; dependency
publication remains unapproved. This work uses guest-source/devloop. The original
checkpoint storage ledger continues in `20260906_0333_native-scene-state-bridge.md`.

## Trace the actual parent before replacing its shared lifecycle

Read the complete `bdRenderViewSubmit` at 0x82184E90, size 0x19AC, in
`generated/reblue_recomp.16.cpp`, including all reflection/cube/main/post branches.
Read the hook maps first: render_list, guest_census, output_resolution and the
native post boundary in render_tweaks. The existing frame_interp hook owns camera
composition and completed scene-image scope, but still calls the original parent.

Correction to the preceding report's final paragraph: `bdRenderViewSubmitAllPasses`
at 0x8213C160 is an object-to-view-list insertion policy, NOT the parent all-pass
wrapper. Its complete body in generated file 101 inserts into views 0/2/1/4/5/6
according to object type and settings. Replacing that would not replace frame
scheduling. The actual parent contains:

- Indexed auxiliary/shadow views (420-byte records), primary and six-face shadow
  modes, the last auxiliary view, and the separate shadow-volume pass.
- Primary planar reflection and ten secondary reflection candidates, including
  transformed bounds, frustum/direction checks and thread-indexed eligibility.
- A six-face environment pass (plus a legacy debug export), another auxiliary
  view, and the main scene with motion-blur scheduling.
- Camera/focus publication, optional native post handoff, and saved effect-flag
  restoration. The existing native post hook skips complete temporary containers.

The shared lifecycle must be preserved across these branches. Read both complete
dispatchers: `bdShaderSystemBeginFrame` (0x821869F0, generated file 1) and
`bdShaderSystemFlush` (0x82186B10, file 0). The former publishes phase/light-space,
calls descriptor slot +12, snapshots the participant count, calls each participant
slot +16 and marks byte +6 only for a low-byte result of exactly one. The latter
snapshots its own count, calls participant slot +20 only for byte +6 equal to one,
clears the live slot afterward, and finally calls descriptor slot +20 even when
the registry is empty. Both reload the registry table around callbacks; count is
signed and its snapshot is not changed by later callback mutations. Also read
the descriptor destructor `sub_82184980`; it releases +36 before +28. It is not
the pass-finish dispatcher and must not replace that lifecycle.

## Actual host execution change

Added an allocation-free native ordered dispatcher and complete raw replacements
for both shared entry points. Existing callers now execute those loops in host
code; the parent inline starts still execute in the guest, but their shared
finishes use the native dispatcher. No parent branch or pass is dropped.

Engine phase/flag decoding is isolated from the scheduling core. The bridge
retains imported descriptor callbacks, registry slots and byte flags; it does NOT
claim host ownership of that scene registry or zero guest participant execution.
The core's live-slot rules prevent stale cached identities when a callback changes
the registry. Preflight refusal can call the original only before any effects;
invalid imports after a callback fail explicitly, never replay a half-executed
pass. The inherited PPC caller stack is restored by RAII, without a typed import
re-rooting it. No native GPU resource, shader, target or post algorithm changes.

CPU tests use the actual dispatcher and import decoding: exhaustive active-byte
and representative upper-result-bit combinations, all normal phase classifications,
exact start/finish ordering, unchanged refused flags, empty/negative counts,
repeated finishes, begin-created registries, table/count mutations, nesting and
callback faults. Tests and runtime results are not yet qualification claims.

## Verification on the current renderer

Focused `host_scene_pass_test_02` (PID 22064) built in two steps, exit 0.
The existing CPU suite `cpu_15` (PID 27172) passed 31/31 in 3.31 s, including the
expanded dispatcher cases; 24 scene and 36 post source guards also passed.

Host `reblue_18` (PID 27368, session 80356) exited 0, final displayed link 12/15.
The new source entered reblue_common's OBJECT library. Expected glob reconfiguration
occurred; codegen reported its module up to date, no guest objects rebuilt.
Build stdout/stderr are 2,532/36 B. The binary is 47,642,624 B, linked 04:14:33,
SHA256 `334621c82e44ea42ddd44e40e318d3d225e8ee1efdc48643acc4d2b5155d1342`.
Root f07d853 plus local changes, Plume 81bdca8; ending build free 64,594,579,456 B.

Both normal runs used that binary, 75 s bounds, zero raw captures, supervised
log/cache/free-space limits and byte-for-byte profile restoration. Wrapper exit 0
means the bounded check succeeded; the wrapper stopped its own game process.
This is not natural-shutdown or game Vulkan-validation qualification.

| Check | PID / session | Time | Log / bytes | Native starts / finishes | Participant begin / end calls |
| --- | --- | --- | --- | --- | --- |
| Flat MSAA | 25432 /62834 | 04:15:29--04:16:45 | 856 /249,796 | 3,601 /9,940 | 64,818 /159,040 |
| XR MSAA | 23264 /27373 | 04:17:12--04:18:28 | 857 /553,285 | 9,301 /22,428 | 167,418 /358,848 |

Both report zero dispatcher compatibility starts/finishes, refusals and faults.
The larger finish count is expected: parent-inlined starts still end through
the replaced shared routine. Participant calls are not counts of guest execution
or fully host-owned frames; the callback import can dispatch existing host hooks.
Accepted/cleared totals are 57,616/159,040 flat and 148,816/358,848 XR.

Scene and shadow scopes also report zero compatibility/refusals/ownership errors.
Scene native clears are 3,600/9,300; compatibility clears and state-308 calls
remain zero. Native post records 3,601/9,301 direct handoffs and completed inputs,
zero original container scopes, imported image scopes or sequence refusals.
All 5/16 respective settings took effect. XR used the existing absolute xrsim
manifest, multiview, 1440x1584 per eye, height 0, scale 1 and no mirror/preview.
No new stereo pixels were captured; desktop timings do not establish Quest speed.

Inspected `out/verification/native_pass_dispatch_window.png`, captured from owned
flat PID 25432 at 04:16:31: 1920x1080, 3,276,731 B, SHA256
`a7fabfd76ef9cc679e6e53efc75a8e7d18dba46bc05bfecddc215996a591d640`.
Shu, terrain, foliage, structures, cast shadows and distant DoF are visible with
no obvious full-frame corruption. This replaces the prior normal-flat sanity PNG,
not sequence, event, stereo or full-game evidence. Current perf CSVs are
041532 (602,112 B) and 041715 (1,609,728 B), each with 112-B metadata.

Cleanup and the cumulative byte reconciliation are in the continuing ledger.
No unapproved push or parent Plume gitlink commit is made. This dispatcher has no
new Plume dependency and can be committed independently of the pending renderer
integration. Parent branch selection/inlined starts, native registry/callback
ownership, materials/animation/shadows/effects/UI/frame ownership throughout,
asset cooking and full desktop both-eye/game gates remain required. The complete
parent trace above is the map for continuing that conversion, not completion.
