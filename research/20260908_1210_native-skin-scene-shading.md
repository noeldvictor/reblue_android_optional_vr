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
Live skin scene reachability, game pixels, independent reload and stereo are
pending. Host160's cold native skin-shadow emission remains historical evidence,
not proof for the changed binary. Host159/run984 remains the last full strict
regression. Prior visual failures remain unqualified.

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
