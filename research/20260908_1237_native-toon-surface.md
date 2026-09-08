# Native Toon surface plans and GPU consumption

2026-09-08 EDT, source d7cae75 plus this work. This is an implemented native
surface consumer, **not yet live technique-1 character replacement**. Desktop
frame/animation/stereo completion and Quest qualification remain open.

## Contract and implementation

The guest-source/devloop skills directed reuse of current owned geometry,
completed poses, material/image/light owners, scene plans and frame-fenced
indirect batches. No second renderer, asset format, cook or parameter cache.

Reviewed existing translated shader artifacts: `bd_toon_vs_env.hlsl` 956..1197
(joint-local positions/normals, colour and UV), `bd_toon_ps.hlsl` 951..1048,
1151..1420 (unreflected/non-normal-mapped textures and lighting), and the
corresponding `bd_toon_cs_ps.hlsl` 1297..1566 response. These are contract
references, not copied shader-register code or a new shader translation.

`NativeToonSurface` carries named diffuse/ambient scale and addition, three
texture RGBA multipliers and ignore-texture-alpha. `NativeObjectPrimitive`
explicitly distinguishes Ordinary/Toon; missing/mismatched/nonfinite inputs are
refused by the scene plan, not guessed white/identity values. The Toon plan
retains ordinary image/UV/sampler, pose, animated bounds, light recipe and fence
ownership. Specular material is required even when the ordinary specular flag
is disabled. Shininess is at least10; negative finite source exponents clamp,
NaN/Inf are refused. Unused texture layers create no false dependencies.

`native_toon_shading.h` implements the authored nonlinear surface response.
For each channel, with adjusted ambient A, primary diffuse P, total diffuse L,
visibility V, shadow colour S and albedo C:

    M = A + .65*(.9*L + .1*A) - .65*(.9*P + .1*A)*(1-V)*S
    R = abs(M)^.9
    colour = (C*(1-R))^2*R + C*R + specular

Ambient is saturated after the1.1 boost and before its scale/add. Light remapping
does not alter attenuation, kind or cone parameters. Toon specular is always
added, primary-only shadowed, using the authored RGB tint. Fog/grade and native
primary-shadow filtering reuse current code. This is the game's Toon response,
not optional Cel banding. Existing native filter differences are not art parity.

The shared native pixel program handles per-instance Ordinary/Toon selection,
RGB layering with tinted detail alpha, negative-U sentinels and optional base
alpha replacement before object/vertex alpha. No pipeline permutation or separate
palette upload is required. GPU object layout expands208->320B and complete
instance816->928B (+112B/instance); existing sizeof-based allocation/alignment,
batch limits and byte caps remain enforced. Shadow/water pass payloads unchanged.

## Verified evidence

- Python388 checks pass. The old guard forbidding every alpha assignment was
  refined to allow only the explicit Toon ignore-alpha assignment; all other
  alpha replacement remains forbidden. It does not replace pixel checks.
- Output73/PID25660 builds; CPU48/PID28192 passes0.56s (CTest0.58s). Missing
  family/inputs, all remap vectors, active/unused layer finiteness, transactional
  late failure, always-owned specular, retained values and exact skin owners.
- GPU73/PID35736 rejected HLSL's struct-valued conditional. Explicit if/else
  fixes it. GPU74/PID35852 compiled the shader then rejected a Windows `max`
  macro collision in the fixture; parenthesized std::max fixes it. No production
  eligibility or tolerance was weakened for either compiler failure.
- GPU75/PID35196/session93552 builds, terminal exit0; executable written12:41:44.
  Rigid28/PID24556 runs the new binary afterward, terminal pass12:42:07:
  **345 cases**, including191 added Toon/mixed-material cases,5.72s (CTest5.74s).
  Rigid and1/2/3-influence skin, all layers/cutout comparisons, zero/negative/HDR
  lighting, ignore-alpha, disabled diffuse/ordinary-specular bit, distinct
  per-instance adjustments, mixed Ordinary/Toon, deferred ordering and both-eye
  colour/depth. New response oracle uses separate scalar/double equations, not
  production Toon helpers. Existing light attenuation and fog oracle helpers
  remain shared. Validation0errors/0warnings;1 loader message; raw/images0.
- Host162/PID37484/session19043 builds and links. Codegen0written; no guest
  objects rebuilt. Host162 has **not been run in-game**.

SHA256 identities:

- CPU73 EXE1688064B:95FF8EAE7576C7C220854518E5725612E40C44175ACF8B624E8B0FA6EE6626A4
- GPU75 EXE1242624B:6BA83930F2A955BDA7A01DD7C380E7AE26D05EC95C5C71531CE2164A770BFAAE
- Host162 EXE49085952B:0DDE0B8D7379C2BA757EE18B833F3CB7603CDFDE4FF7BD592DCC9D89229B75B0
- Host162 PDB111755264B:7CC215CDE4E6A068093CC851ED74063AA130B9439D0952AD02305582045DA936

## Remaining producer and acceptance

`PrepareNativeRigidSceneForObject` and whole-node admission still reject live
technique1. No guest execution is removed from ongoing characters by this
checkpoint alone. Next supply this typed API from the **actual authored
producer**, including light remaps, layer multipliers, ignore-alpha and lattice
eligibility; then admit the complete texture-classified sibling set through the
same route and require fresh post-event native skin-scene emissions/reload.
Do not read back shader rows, manufacture defaults, or rerun unchanged code to
pass that gate. Specialized normal/reflection/cube-shadow/fur/outline variants,
native animation and visual setup remain separate missing dependencies.

The source investigation found generic parameter-descriptor dispatch rather
than literal67..70 setters. `bdShaderConstantFlush` in generated recomp38 owns
flags/start/end/source fields at0/4/8/12; it is already host-replaced by
`FlushHostParameterDescriptor`. That is **not** proof of the exact Toon source
or permission to add a second register cache. Broad numeric-constant searches
and original-body callgraph sites did not identify the producer; do not repeat
those scans as though they establish runtime ownership. A bounded producer
observation must identify its authored owner and timing if source tracing cannot.

Host161's12:15 runtime969 remains the last strict live reload/skin-shadow run;
its scene counters stop post-event and the ongoing-character gate is Pending.
All previous visual-failure evidence stays protected. No new profile mutation,
game cache, raw capture, image or Quest run. Storage and cleanup use the same
cumulative20260906_0333 ledger, not a reset budget.

Ending12:46:21 free78760706048B (73.35GiB);10178560B less drive-wide than the
12:37 preflight. Removed16 obsolete build/test logs after replacement passes,
66495B logical (+77824B free observed). Selected retained fixture/EXE/PDB/log
growth295217B after cleanup, for the new surface/ABI tests and current build;
other shader headers/objects/source/Git/driver/system activity separate. No
capture growth. Superseded compiler diagnostics remain explained above; current
passing logs/binaries and every unresolved game/visual failure stay retained.
