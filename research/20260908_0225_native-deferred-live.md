# Native sorted consumption reaches both field epochs

2026-09-08. Source base0e67545. The previous goal turn made implementation
progress by connecting owned sorted packets; this continuation establishes live
reachability and adds a reusable acceptance guard. The full host-frame/desktop/
Quest goal remains active, not complete.

## Current result

The unchanged host130 executable runs the new opt-in connection through cold
field, title teardown and reload. Exact EXE SHA256
`AE12CB1BEAB1219D3F5A65A965B0B458B9BC3B97D426B060E3E05CE57A97BC33`,
48,791,040 B; build stamp f3131a5d5 dirty describes the renderer sources later
committed in0e67545. No new host/guest/shader build or GPU fixture this turn.

Run965/PID28816/session53891 completed0,02:16:19..02:17:32, under the75 s bound.
All20 temporary profile settings took effect. The cold field's fresh1800..2100
window stages/consumes11,468 native sorted packets alongside5,745 legacy draws;
zero pending,11,490 material bridges. The existing native instance/geometry/
material/light/fog/scene/caster/movement/queue checks pass. Log254,875 B SHA256
`85D5C91387BFEDF62151AFD244E10A0006BA3B5D0228F11EB87D212DE4979F7E` was subsequently
retired as superseded by966's complete cold/reload proof. Its exact raw text is
no longer retained; the bounded source/run procedure is reproducible.

Run966/PID35492/session44373 completed0,02:22:16..02:24:20, under the180 s bound.
All22 temporary profile settings took effect. `reblue_966.log` is retained at
`out/build/win-amd64-release/logs/`,505,061 B, SHA256
`7DE6645EA4B7AD72901155DB22E91E9ADD624F6355DE29DD8360F2933DA7E25E`.

| Fresh mixed-consumer window | Cold | Reloaded |
| --- | ---: | ---: |
| Frames |1800..2100|4500..4800|
| Native packets staged and consumed |10,815|3,825|
| Pending native packets at samples |0|0|
| Legacy direct draws |5,570|7,609|
| Material begin/end bridge calls |11,140|15,218|

These are repeated packet visits, not distinct converted assets. Reported total
at frame4800:65,105 staged/consumed,0 pending. The cold scene uses generation93/
instance144. Title request/reach and old source/GPU retirement complete; reload
uses generation207/instance388. The existing regression observes900 fresh scene
and shadow emissions in each interactive field epoch. The complete scenario
chain, including receiver setup, scene lighting, caster family and textured
scene/shadow cutouts, passes independently for both epochs.

In the final scene-counter window,11,884 native submissions,10,905 emissions and
10,905 visible fence retirements are observed, alongside1,018 culled and retired
culled instances. Those scene counters aggregate direct and deferred work; they
do not attribute GPU retirement to each deferred family. Batching remains
singletons in these samples (merged delta0), not a speedup measurement.

## Reusable check / how this changes the next action

`tools/native_instance_scenario.py --rigid-deferred` checks the production
`[native-deferred]` and matching `[host-consumer]` receipts with the existing
consecutive, recent post-event readiness windows. It rejects malformed/missing
paired reports, stale frames, counter regressions/conservation failures, queue
overflow, fallback/refusals and runtime/mismatch/shutdown errors. Both native
and remaining legacy work must advance; native production alone cannot pass.
The CLI also runs the scene emission/fence guard. With `--rigid-reload`, the
native deferred guard runs independently on both split epochs.

Four added test methods cover fresh mixed work, startup/wrong-scene/one-sided
work, stale or missing frames/reports, malformed/over-budget input, conservation,
late errors and independent epoch gating.357 artifact-free Python source/scenario
tests pass in0.166 s. These are parser/source tests, not substitute pixels.
The final retained966 log also passes a direct read-only split and complete
`verify_rigid_epoch(..., rigid_deferred=True)` check for both epochs.

The local existing `run_post_image_flow.ps1` gained an explicit capture-free
`-RigidDeferredProbe` opt-in and scenario flag; its safety limits and exact
profile restoration remain. Run965 requested the existing full cold-field chain
through `-RigidHardOffVerify`. Run966 additionally requested `-RigidReloadVerify`,
`-ReceiverSetupVerify`, `-SceneLightsVerify`, `-CasterFamilyVerify` and
`-CutoutFamilyVerify`. Both use `-NativeImages` for native ownership diagnostics,
not captures; Count/FrameBytes0, no `-InspectWindow`, `-FrameProbe` or VR.

This proves the callback contract is reachable and the owned queue drains across
real generation changes. The next implementation should remove the remaining
ordinary visual-transition dependency through the existing native receiver,
blend/alpha and scene-light/effect owners, preserving late outputs and outgoing
state for remaining consumers. Repeating a cold boot or the callback census now
adds no new decision evidence.

## Storage and remaining acceptance

The original3 GiB exception and floor62,509,998,080 B persist. Each probe enforced
the existing192 MiB free-drop/20 GiB reserve,75 MiB cumulative diagnostic stop
with25 MiB analysis/polling reserve, and400/800 KiB per-run logs. No captures,
CSV, shader dumps or asset cooking; no runtime cache/perf/hlsl files were created
or modified after run965 start. Owner profile restored after both terminal jobs:
SHA256 `2F1BC38D763A1B7BDBA31F560684FD4AA7E42A714600B8D344F19DA7F38E23B0`.

Initial preflight had only191,537 B below the diagnostic stop, so **no game was
started** until log overlap fit. Completed962..964 logs were losslessly archived
as `logs/retained-native-runtime-962-964.zip`,236,981 B, archive SHA256
`20C9B1EFE9F36D7884A203B54D4CAD84A1C1B28D3C094FC10733679DD9DD6359`.
Each expanded entry was hashed before removing the original plaintext:

- 962:521,322 B, `00876D1ACDF8A40306DB32143E2498BDDF1386F1F28F84A6276D1FCD6566156C`.
- 963:535,227 B, `DF96F670913A3F5FEA1AD64332AD9BB4D9E136371A0930D902FF3BDD9B116780`.
- 964:519,448 B, `5434E7DA19DA08652068375C3F4E25F482144B351B97FFC32205F67486FB6980`.

Full failure/provenance text remains recoverable. Latest964 was temporarily
expanded to preserve the logger's next sequence number until965 existed, then
removed again; that temporary removal receives no extra savings credit.
Archive conversion saves1,339,016 logical B relative to its1,575,997 B originals.
965's superseded254,875 B success log was removed after966 passed. New run logs
total759,936 B before that retirement; ending run-log/archive footprint is833,955 B
smaller than this continuation's start. No protected pixels/raws or user data
were deleted. Detailed cumulative accounting remains in the existing ledger.

No new game image, sequence or headset run.945 remains the last accepted mono
image; all unresolved visual/adapter failures remain open, including archived
962..964. Larger image export remains paused on the unanswered budget question.
Cold/reload text success does not qualify authored pixels, complete frame ownership,
all scene/event families, stereo, Quest readiness or a performance improvement.
