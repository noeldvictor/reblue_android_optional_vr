# Native render-pose live consumption

2026-09-08. Host165 (the C++ connection committed as a0c6428), scenario tooling
b45dbc0. This verifies the existing connected implementation, not a new build.
EXE SHA256 `09ADF4E2269BB08D2A09F739BBD77585FFE23415865ADDE5A05727FAAF19C791`.

## Result

Run 973 / PID 36304 / session 23957 completed successfully, 15:00:19..15:02:42.
The existing bounded desktop operator used normal mono/MSAA/native post,
temporary flat-TOML `bd_fps_limit = 60`, captures/perf off, mouse-menu isolation
and the full strict cold/title/reload/mixed/skin scene+shadow chain. Its new
RenderPoseVerify switch invokes `--render-poses` independently in both epochs.
The log confirms all 25 temporary settings took effect. The original 116 B profile
was restored exactly; no renderer remains active.

| Fresh field window | Host pose reads | Blends | Shared reads | Snaps |
| --- | ---: | ---: | ---: | ---: |
| Cold 1870..2170 | 52,361 | 1,287 | 37,961 | 13,113 |
| Reload 4187..4487 | 51,929 | 951 | 37,529 | 13,449 |

Refusals remain zero throughout. Startup-only/snap-only updates, counter resets,
malformed outcomes and missing ready contexts cannot qualify. 394 artifact-free
Python guards/scenario tests pass. CPU timing/bounds/consumer/lifetime/budget
fixtures were already verified for this exact C++ connection; no shader changed.

Native skin scene 1809..2109 emits/fence-retires 13,198; reloaded 4135..4435 adds
13,200. Skin shadows add 13,200 in each corresponding window. Model 93 / instance 144
fully retires before 207/419. All existing source comparisons, native geometry,
materials/lighting, indirect submission, mixed deferred and reload checks pass.
These are repeated draws/pose reads, not unique assets. Merged instances remain zero.

## What this does not establish

No pixels were captured. The 971/962 title-window versus logged-field discrepancy
remains unresolved; do not repeat unchanged PrintWindow capture. Use the existing
renderer-fence observation only after its named per-image and aggregate storage
gates fit. Fresh pose consumption does not prove character motion/art parity,
stable sequences, achieved 60 FPS, speedup, game stereo or a fully host-owned frame.

Original animation evaluation and its collision/effect side effects, conditional
source-palette copying, secondary palettes and unconverted families remain.
Native animation ownership and the full field/battle/cutscene/menu/transition/
reload/both-eye desktop gate remain required before Quest qualification.

## Storage and retained evidence

Same cumulative ledger / 3 GiB exception and 62,509,998,080 B floor; no budget reset.
Run 972 first stopped on its unchanged 192 MiB free-drop guard during the opening
event, before qualification. Its 169,284 B log produced no raw/perf/cache/hlsl files.
Three stable-space checks preceded 973; no acceptance/storage threshold changed.
Run 973 also produced no such files; its only new runtime output is 550,521 B of log,
SHA256 `CB6B3511FC826F1A215F8BB624A943C5FAA1FE29EE4D9A0EB3E884A29F09B23D`.

Logs 971/972 were losslessly archived after both entry hashes were validated:
706,706 B plaintext becomes 144,117 B in
`retained-native-runtime-971-972-20260908-poses.zip`, SHA256
`D5EF7766701C04BFA7BD0A90970D02E5CDA2064400E2E0A8C748B889D7C60848`.
Only their exact plaintext copies were removed; all failure evidence is
recoverable. Current 973 stays plaintext; all 13 images (10,434,657 B) are unchanged.
Net selected retained log/archive growth is 157,216 B for the new timing gate.

Logical cleanup savings: 562,589 B; measured deletion-phase free-space gain: 483,328 B.
Concurrent unattributed drive allocation means the full archive operation instead
shows a 5,107,712 B net free-space decline (67,559,735,296 -> 67,554,627,584 B). Do not claim
that drive-wide change as this task's output or a net cleanup gain. The cumulative
ledger records drive-wide changes and source/Git activity separately. At 15:06:57,
free space of 65,378,684,928 B is 2,864,136,192 B below the 14:54:49 sample. Read-only process
inspection identifies a separate Gradle/Ninja rpcsx-android link under
Documents/ps3-thor; it was not stopped or altered. That is evidence of concurrent
activity, not an explanation for every earlier allocation. No new reblue
output-producing job should start without a fresh stable-space check.
Push remains security-blocked pending exact owner approval; no retry this turn.
