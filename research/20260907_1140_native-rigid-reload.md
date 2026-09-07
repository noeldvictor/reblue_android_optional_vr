# Selected rigid asset: actual title teardown/reload

2026-09-07 EDT. Extends42a47c0; no new renderer or asset conversion.
Full desktop/both-eye acceptance remains open.

## Connected lifetime and game flow

`bd_native_rigid_reload` defaults false, requiring hard-off and autoplay.
The existing model loader reports selected generations; its complete original
graph destructor reports source retirement only after returning. Native batch
items carry host generation/instance metadata, leaving shader ABI unchanged.
Submission precedes possible immediate flush; emission follows the actual GPU
command; retention retirement follows the existing slot fence. Different
generations cannot merge into one attributed indirect command.

The fixed eight-epoch diagnostic never evicts proof or adds an asset cache.
Zero/stale identities, missing loads, duplicate retirement, bad views,
over-counts and full capacity refuse. Pending commands/fences may finish after
source destruction. No forced registry retirement or fence drain fabricates it.

After the original SequenceControl update, the game-thread diagnostic resolves
the actual Title ID, validates dispatcher/child UIDs and uses the game's deferred
shutdown request. Sources: bdGameTaskUpdate0x820C4930/file82, complete
SequenceControl_vf02/file53, FindSequenceByName/file36 and GameTask destructor
sub_820C3CB0/file81. Generated code stays untouched. The existing autoplay pauses
until source retirement, all old fences, a fresh title child and cleared game/
field roots are observed, then resets its policy/clock for reentry. Fresh game
task, model and instance identities are required. Walking readiness is specific
to bg41_01 and expires after250 ms. Input/GPU callbacks never call game functions;
evidence is copied before guest calls, avoiding inverse model/video lock order.

## Run939: real round trip, failed sampling acceptance

Host102/PID24732,11:32:38-11:34:51: selected generation93 was destroyed after
1379 scene/1379 shadow submissions. Two retained draws per consumer were still
pending at source destruction; all1379 retired before reaching title. Reload
created generation206, instance384 instead of144. Both new consumers emitted
1365 draws, including600 after interactive-field readiness.

The harness did **not** accept939 or capture an image. Its600-emission cold
window lacked two complete fresh caster/hard-off reporting windows. These
reports precede their same-frame context: context1345 still had an event,
so guards1645/1945 cannot prove two post-event windows. The retained cold prefix
reproduces Pending for shadow/hard-off while movement, scene and batching pass.
Gameplay later retired206 before the wrapper refused extra lifecycle events.

A focused regression reproduces that ordering and requires report2245. The
driver now waits for900 actual scene and shadow emissions after readiness,
three300-frame reporting periods, for **each** generation. Freshness is not
weakened and939 is not relabelled a pass.

Retained939 log:520,428 B, SHA256
72C339635DF32E6242F1058D388720CA5F91555E884E8E640DD3F77976952C8C.
The host102 binary was replaced; its exact hash was not recorded. Exact profile
restored; no owned producer or raw/perf/cook/dump output remains. Keep this
failed-sampling log until the corrected runtime passes.

## Current verification and storage

293 Python checks pass. Post-output25/PID27012 and CPU9/PID27176 pass,0.40 s
CTest: lifecycle overlap/capacity/identities, batch barriers and900-output gate.
Host103/PID26000 passes one-source build/link in3.32 s, codegen0 written/up to
date, no guest objects. Exe48,547,840 B, SHA256
1F65616313ED81BEF6B98E0CE14E494D3375B3D89F8929FAB94D1681DBACEB40;
PDB108,756,992 B. **Corrected runtime/pixel acceptance pending.** Unchanged
shaders retain GPU26/rigid05 evidence, not a new GPU qualification.

The existing runner/parser support one180 s round trip,800 KiB total log,
each field epoch <=400 KiB, and one <=160 KiB final JPEG. Prior consumer/field
checks run separately on both epochs, never across-load deltas. Global caps,
zero incoming raw and guaranteed profile restoration remain. Sequences,
both-eye game checks, repeated-object batches, remaining scene/receiver source
adapters and other families are still required.

Same cumulative scene-state ledger, no reset. Lossless NTFS compression of64
historical raws preserved all hashes and reclaimed277,176,320 observed B, with
no raw deletion.18 superseded build/test logs (14,264 logical B) were removed
after replacement passed, recovering32,768 measured B; they are regenerable.
Fixed max-macro/include/cvar-link failures remain described in the ledger.
Cleanup-end63,330,783,232 B free,44,118,016 B drive-wide use since output
preflight, not all task-attributable. Keep current builds and failed939 until
corrected runtime validation. No new raw allowance.

## Corrected runtime: run940 passes the mono round trip

Published implementation86f00d4; same host103 binary/hash above (compiled before
the source commit, no restamping build). PID28128,11:44:43-11:46:46, all19
settings audited. Exact profile restored and no owned producer remains.

Generation93/instance144: readiness window774->1675 emissions in **each**
consumer,901 fresh emissions. The game destroys its source with1676 submitted/
emitted and1674 fence-retired per consumer. At title all1676 are retired.
Generation207/instance389: its independent readiness window764->1665 adds901
scene and901 shadow emissions. Native commands reach fresh images after reload.

The cold260,704 B and reloaded221,447 B parser epochs independently pass every
existing field/consumer gate. Each has300 fresh native scene/shadow emissions,
retirements and hard-off admission checks; merged runtime instances0. Movement
samples advance83/92, distances79.102978/116.885012 in the source coordinate
units. There is no across-load counter subtraction or inherited autoplay clock.
The new source loads and instance identities differ from939; IDs are observations,
not persistent asset names or hardcoded expectations.

Final log483,446 B, SHA256
2785A5001BC6DD0EF935269CB322A8E0A266439CB5A7163B4DA850DF574B536B.
Final `native_rigid_reload_window.jpg`:1920x1080,124,696 B, SHA256
6BA8E4AD2BC2F806134D1914B9F0637739AA8677A81D42E66DB58AE3C834277C.
Actually inspected: Shu running by rocks, fence and trees, with coherent terrain
and character/tree shadows. Known black cliff marks and distant blur remain.
This is a whole-scene sanity image, not isolated selected-object shadow pixels,
sequence stability, a full-game lifecycle claim, both eyes or a speedup result.

940 produced no new raw/perf/shader-dump/cache files. Audit of both runs did
find one **unexpected68 B native material cache entry** created at939's late
transition:cache/native_materials/v1/561cb848b5104e0c.bdmat. This contradicts the
earlier zero-cook observation; retain the single versioned reusable entry under
the existing cache policy, no duplicate/archive. Include its68 B in storage.

After940/pixels passed, removed superseded938/939 logs,938 hard-off JPEG and
host98 stdout/stderr:five files900,296 logical B,913,408 measured B reclaimed
(62,970,437,632 ->62,971,351,040 B free). Those old diagnostics are no longer
available; failure findings/hashes and regression remain above. Keep940/image,
host103/output25/CPU9, unchanged GPU26/rigid05 and all distinct protected baseline,
movement, VR, unresolved visual-failure and historical raw evidence.

Across this continuation, measured cleanup/compression recovery278,122,496 B,
counted once. Output-preflight63,374,901,248 ->cleanup-end62,971,351,040 B:
403,550,208 B drive-wide use, not all attributable. Known retained component
growth471,903 B: CPU fixture115,818, exe20,480, PDB106,496, build logs7,
replacement JPEG5,474, longer two-epoch runtime log223,560, cache68. Other host
objects/source/Git and unrelated volume activity remain unattributed. Retained
growth enables the new lifecycle coverage; replace this set at the next
equivalent qualification, not one set per commit. Original floor/caps unchanged.
