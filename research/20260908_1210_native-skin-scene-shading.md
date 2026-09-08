# Native ordinary skin scene shading

2026-09-08 EDT, source07c4623 plus this connection. Full desktop host rendering,
native animation and Quest qualification remain incomplete.

## Delivered connection

The guest-source/devloop skills directed reuse of the translated shader contract,
load-owned BDMESH v3 geometry, instance poses, material/light/image owners and
the existing indirect queue, depth visibility and frame fences. No new renderer,
palette cache, asset format, persisted cook, guest edit or shader translation.

Ordinary technique0 phase0 nodes can now use native skin scene shaders. Each of
1..3 influences transforms its own local position and normal with the retained
joint-to-world matrix. Normals use the palette linear transform and normalize
after blending, not rigid inverse-transpose. The node world is not applied twice.
TexCoord0/2 and colour come from checked native asset semantics. Existing scene
lighting, fog, shadow receiver, zero-to-three layers, cutout and sorted consumers
are reused. Exact pose/model/influence checks and animated bounds precede native
submission; deferred plans and batch items retain the pose through retirement.
Scene/shadow skin counters stay separate. No silent unsupported sibling omission.

Contract: existing generated bd_normal_cs_vs_env.hlsl skin branch956..1147 and
the prior20260908_1410 asset audit. Technique1's distinct scene shader, wind,
normal-mapped/reflection families and texture-effect routing remain unconverted;
they are not relabelled ordinary. Original pose/animation production remains.
The shared native scene consumer still requires mono live targets; two-eye GPU
fixtures do not establish a live stereo camera producer or game acceptance.

## Verification

- Output72/PID32380 and CPU47/PID32320 pass, CTest0.55s. Exact identity, missing
  pose/layout, one/three-layer plans, influence mismatch, singular unused node
  transform, pose replacement, deferred retention and source/fence lifetime.
- GPU71/PID8168 compiles both new scene shaders and the shared skin shadows;
  GPU72/PID30468 builds the corrected fixture. Rigid27 passes154 cases,
  including69 new skin scene cases (23 modes times1/2/3 influences), CTest1.95s.
  Real indexed indirect draws, nonzero storage offsets, distinct instanced poses,
  lights/materials/fog, layer-specific UVs, all8 alpha comparisons, depth and both
  eye colours. Poisoned object-world/normal data catches double transforms or
  rigid normal reuse. Independent scalar oracle retains its tolerances.
  Vulkan validation0errors/0warnings, one loader message; no raw/image output.
- Host161/PID32304/session30288 passed; codegen0written, no guest objects rebuilt.
-384 Python source/scenario checks pass; these do not replace runtime evidence.

Two fixture defects were corrected without weakening checks: CPU46's old rigid
setup lacked owned influence metadata and asserted at1191 (30s CTest timeout);
GPU26's new scene asset placed Color before TexCoord2 and failed canonical
schema validation before the new pixel cases. CPU47/GPU27 replace these failures.
At source checkpointa83d076, live skin scene reachability, game pixels, independent
reload and stereo were pending. Host160's cold native skin-shadow emission remains historical evidence,
not proof for the changed binary. Host159/run984 was the last full strict
regression before the integration below. Prior visual failures remain unqualified.

SHA256:

- Host161 EXE49078272B:949F50817051747200D9DEFBB807523D79EAED10CAAF17516F3404DA05FE5C41
- Host161 PDB111747072B:27D3CC32979246F6C44D2CCF1AFB4C00B2F79898AEC9AE556A7C91C04DA7C7C3
- CPU72:CD2C2C5C9DB04ACD7AA0D26D2DF1DF194514825FCE8B7C292DCE0C396F3D2F40
- GPU72:E1597A845E2520B4A634309887210719C4D16F15A3BF4E2D2C00B1C5684D3518

Storage uses the same cumulative20260906_0333 ledger and unchanged limits.
Initial11:57:02 free79040143360B;12:08:02 free78997954560B. Task outputs are
separately inventoried; this drive-wide difference is not all attributed to them.
Previous game GLCache entries remain429851081+410176B at12:08, unchanged from
the prior handoff. No global cache cleanup or limit increase. Runtime checks
must still reserve their own overlap and stop at the existing192MiB free-drop.

Removed16 superseded agent build/test logs after replacement passes:98827B
logical, free78997434368->78997544960B (+110592B observed). Current compiler,
CPU47, GPU27 and all required runtime/visual failure evidence remain. The removed
fixture setup failures are explained above and reproducible from their tests.

## Live integration after a83d076 was pushed

An initial preflight rejected diagnostic overlap before changing the profile or
launching a process. The existing hash-verifying2MiB-bounded archive operator
then losslessly archived941/945/948/956 and977/978, preserving complete failed
visual/causal/baseline logs. Savings1553725+535077B logical; exact entries and
archive SHA256 identities are in the cumulative ledger. No cap was increased.

Host161/PID22748/session6707 ran12:15:08..12:17:09, terminal success. Its log
filename is reused969, NOT the earlier969:570640B, SHA256
8EA740240A774B8F126D3F2C9C06804A87FECAE45AE139BB10276DD168749E91.
All24 settings took effect; raw/perf/persistence off, exact116B profile restored
(SHA2562F1BC38D763A1B7BDBA31F560684FD4AA7E42A714600B8D344F19DA7F38E23B0).
The complete strict cold/reload/mixed/receiver/material/lighting chain passed:
old generation93/instance144 -> new207/385, actual source and GPU retirement.
Fresh native skin-shadow windows add13200 submissions/emissions/fence retirements
per300frames independently in each epoch; reloaded4601..4901. This closes the
previous disk-stopped shadow reload gate, not every scene's shadow acceptance.

Native skin scene consumption is now real: frame876 first29 submissions;
frame1176 has5661 submitted/emitted/retired, during the opening event. After
reload frame3701 totals11322, another5661 emitted/retired in the opening event.
Counters remain flat in later interactive-field windows. Consequently the new
`--rigid-reload --skin-scene` verifier returns Pending, not a false pass from
startup/shadow counts. Three new tests check separate scene/shadow consumption,
freshness, fences, resets, failures and independent epoch requirements;387 total
Python tests pass. No per-character identity or ongoing scene coverage inferred.

The requested PrintWindow JPEG (native_skin_shadow_window.jpg) is1920x1080,
105571B, SHA256F46CE583DC3BA3BCAA26BCC47E0AC54DBCCE451EC79EE69B65DBE75FA373B574.
Inspected: player, ground shadow, terrain, fence and foliage are visible, without
a blank/cyan frame. The background remains dark with some black speckling;
attribution/parity is unqualified. The picture was after the scene skin counters
stopped, so the visible hero is NOT proved to use the new ordinary scene shader.
This is single-frame sanity evidence, not stable motion, authored skin/cutout
parity or stereo. Existing941/948/956 visual failures remain protected.

Next: use the real ordinary event consumer as regression coverage while moving
ongoing character/technique1 shading and animation to these same owners. Keep
the separate interactive scene gate; do not rerun unchanged code to pass it.

Ending12:18:59 free78979936256B (73.56GiB),60207104B less drive-wide than the
11:57 baseline. Total actual logical cleanup2187629B, counted once. Selected
retained files net-1057601B after fixture growth, log archival,570640B new log
and105571B image. New shader headers/other objects/source/Git/system activity
are separate. Existing game GLCache grew69389B measured (429920374+410272B
ending); no new game cache/HLSL/raw payload. Image aggregate10408682B/12files,
77078B left under10MiB. No owned producer remains. No new capture fits by
assuming another110KiB allowance; reclaim eligible image overlap first.
