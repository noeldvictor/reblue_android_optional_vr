# Cutout integration: reachable consumers and the light-space cutoff fix

2026-09-07, EDT. Verification/gates pushed as8a1bfbc; the cutoff correction is
committed with this report. Goal turn: progress, not complete. No whole host frame,
full-game stereo, performance gain or Quest result.

## Connection and causal correction

The devloop skill selected the existing bounded host/CPU/GPU/scenario tools.
The guest-source skill reused the prior node audit, then reopened the precise
contradicted branch and its current pass-dispatch producer. No decompiler, new
renderer, recook or generated-source edit.

- Existing queue records now count cutouts separately at submission, actual
  draw emission and recording-slot fence retirement. The emitter receives the
  complete admitted instance span; the first packet's flags cannot classify a
  mixed scene batch. Textured emission/retirement is counted separately.
- `--cutout-family` requires fresh scene AND shadow textured cutout emission and
  retirement in each independent reload epoch. Opaque/startup/stale/missing/reset/
  impossible counts cannot pass. The normal acceptance switches remain off.
- Run953 failed before scene rendering. Host114's failure-only context identified
  generation93/instance144/node65, phase1, three ranges: alpha-reference producer
  unavailable. Interpreter/capture/replay stayed disabled for the admitted node.
- The former `ReadMaterialAlphaInputs` rejected a nonzero byte at
  `(uint32_t(-32035)<<16)-26711`. The existing pass dispatcher names it
  `kLightSpace`; `ImportPassLightSpace` makes it true for phases1/4/8.
  In `generated/reblue_recomp.40.cpp`, loc82280740 branches to82280880 around
  colour writes. Default cutoff selection at82280738..3C precedes that branch;
  reference setters at82280988..D0 still execute afterward. Rejecting the cutoff
  publication on that colour flag was wrong, not a missing shadow texture.
- The importer no longer reads that unrelated flag. The C++ regression verifies
  defaults and explicit object override with light-space set, unreadable unrelated
  flag, blocked special override and missing actual cutoff data. No rendering
  comparison, cutoff value or tolerance was weakened.

Important remaining contract: light-space skips the original object-colour writes,
while the native caster currently multiplies by copied object alpha. This change
proves cutoff ownership independently; it does NOT prove authored shadow-alpha
equivalence. Recover that multiplier/shader contract and a real textured-shadow
participant before qualifying the paired material family. Do not manufacture
coverage by changing authored texture enables or loosening the new gate.

## Verified jobs and live evidence

All producers below are terminal; no retained profile override or active capture.

- 320 artifact-free Python source/scenario checks pass, final1.027 s. These are not
  proof of native pixel consumption.
- Host113/PID25168/session6384 passes:19 scheduled host steps, codegen0 written,
  no guest objects rebuilt. Host114/PID30120/session97473 passes8 scheduled steps.
  Both build logs were retired after host115; no binary restamp implied.
- GPU32/PID32464/session48551 builds the expanded existing fixture. Rigid10/
  PID35752 is stopped by the32 MiB free-drop guard before results. After the
  process audit showed concurrent builds absent, rigid11/PID31932 passes41 modes
  in1.65 s/CTest1.67 on RTX3060, validation0 errors/0warnings, one absent GOG
  overlay JSON loader diagnostic. The previous37 cases/tolerances remain.
  Four new8x8 two-eye modes cover textured caster holes, a zero-layer rejected
  caster, two different caster instances, and a blended cutout receiver. Native
  receivers sample the mono D32/S8 image with linear comparison and four PCF taps.
  An independent analytic caster-depth/bilinear oracle checks every pixel.
  Lit/shadowed/filtered pixels:48/56/24,128/0/0,80/24/24,48/56/24 respectively.
  Maximum error3.39895e-05, unchanged colour tolerance.003/depth1e-5. Readback
  stays in RAM, with no raw/image exports.
- Material37/PID22788 and CPU35/PID22244 pass the causal cutoff regression and
  existing material suite,0.17 s/CTest0.20. No shader change for this correction;
  reuse rigid11's production-shader evidence.
- Host115/PID37360/session50752 passes15 scheduled host steps, codegen0 written,
  no guest object rebuild. Embedded revision is8a1bfbc dirty with the cutoff fix.
  Exe48,703,488 B SHA256
  `A7A0BBAD1B7F46F3B573DCD09BE463402C969624F650D9C7E833004185D0118C`;
  PDB109,481,984 B SHA256
  `46FCD3994742B76D8BC56852424A7FD2B0F4C7F7CA44D4D92C428266AD5ADEC1`.
  Fixture exe831,488 B SHA256
  `43156A8ADBA5BB4E6E651E837E787F3EEEB78CB752002BF718673A2306AB3F88`.

Run952/PID34196 stops on the192 MiB free-drop guard18:11:19..24, before useful
render evidence. Run953/PID34072/session21773 stops18:21:11 on the generic caster
refusal. Run954/PID35800/session3840 stops18:25:50 on the specific cutoff failure.
Retain954,55,505 B SHA256
`4A9B3B0375122E3B019FC5FEDA59A7521AF8DBB3D2D86A235A090A3942E1B7E3`.

Host115/run955/PID27696/session70140 runs18:33:19..18:35:14 with all21 effective
settings, raw capture/perf output off and temporary mouse-menu isolation. It
clears the causal refusal. Fresh cold-field frames1842..2142 have cutout deltas:
scene submitted/emitted1,198, retired1,196 (all textured); shadow submitted/emitted
8,778, retired8,766 (textured0). Prior-slot work explains unequal fence deltas.
Read-only `verify_rigid_epoch(cold, True, True, True)` passes all preexisting cold
rigid/receiver/scene-light/caster checks. Total native scene emitted+2,744, retired
+2,742; non-regression caster family emitted+24,673, retired+24,660. These are
repeated instances, not unique assets or measured speedups. The separate strict
cutout gate correctly remains Pending because textured shadow counts are0.

At18:34:37.920 old generation93 is fully fence-retired and the title is reached.
The run stops on the unchanged disk guard during the reloaded opening event,
before post-event reload verification. No image was written; no raw, perf,
cache or shader-dump file was added by955. Log364,742 B SHA256
`D730F172620BAFDE11C0CDBA9895BED1D702BCDFDD86A74236FFBBA530EA7EDF`.
The116 B owner profile is restored exactly, SHA256
`2F1BC38D763A1B7BDBA31F560684FD4AA7E42A714600B8D344F19DA7F38E23B0`.
Keep945's accepted mono image and948/941 UV/light failures. No non-reproduction
here explains those failures. Full reload/ordering/pixel/inheritance/both-eye
qualification remains pending. The local image supervisor reserves a distinct
110 KiB cutout JPEG only after its full text gate; the reservation was unused.

## Same cumulative storage ledger

Original3 GiB exception,62,509,998,080 B floor,100 MiB diagnostics/75 MiB stop,
10 MiB logs/images and no new raw allowance remain unchanged. Host/game peaks
192 MiB, fixture peaks32 MiB. First free78,924,582,912 B. Other project builds
were visible but not modified/stopped; later drive changes are not attributed.

After replacements passed, removed16 exact superseded build/test logs: host112,
host113/114, GPU31, rigid09/aborted10 and material36/CPU34 stdout/stderr.25,787
logical bytes. Also retired952's startup-only log9,126 B and953's generic refusal
55,241 B, replaced by954's specific failure and955's progress:18 files/90,154 B
total. Original text is gone; tests are reproducible and failure954 is retained.
No protected raw/image/game/save/profile/build-tree data was deleted.

Immediate free-space observations for the four deletions, respectively:
77,380,018,176 ->77,387,395,072;
76,252,446,720 ->76,252,467,200;
74,371,796,992 ->74,371,813,376;
71,608,049,664 ->71,608,119,296 B. These are not isolated recovery measurements;
the first includes far more unrelated activity than its4,701 B of removed text.

Retained material tree8,331,276 B/41files (+2,144), texture tree70,636,050 B/129
(unchanged), GPU tree10,670,060 B/10 (+15,272), build logs199,175 B/136 (-1,644).
Exe/PDB growth8,192/28,672 B; new retained954/955 logs420,247 B. Counted retained
growth472,883 B; other objects/CMake/source/Git remain unallocated. Retire fixture
logs after equivalent replacement; retain954 until its causal coverage is no
longer needed and955 until cold/reload/pixel evidence supersedes its purpose.
Inventory free71,608,135,680 B (66.69 GiB), down7,316,447,232 B drive-wide since
the first sample, not explained by or attributed to the roughly462 KiB subtotal.
