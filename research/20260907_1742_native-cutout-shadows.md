# Native phase1 cutout shadow connection

2026-09-07, EDT; based on8a1bf9a. CPU and production GPU fixtures pass;
host compilation and game acceptance are pending. No complete host frame,
game stereo, performance gain or Quest qualification is claimed.

## Ownership bundle and recovered contract

The guest-source skill followed the existing translated node function and active
hook seam, not a new binary/decompiler investigation. `bdSceneNodeDrawSingle`
in generated40 was read through its complete PPC control flow. The new native
consumer reuses the load-owned model/material program, object texture scope,
instance pose, pass camera, sampler publication, shader/program cache and shared
indirect queue/fence owners. There is no new asset cache, recook or renderer.
Admitted phase1 cutouts no longer require the per-node rendering interpreter,
translated shader/register state, per-draw texture setter or replay template.
Source object/pass/image import and authored pose production remain adapters.

- Node entry initializes phase1 texture mode0. Loc82280F14 substitutes
  visual+3068 for nonzero01xx commands. Loc822810A0/822810E4 selects that same
  mode or zero for09xx commands. The load recipe retains this independently of
  phase0's final layer count. Modes above3 preserve old enables and remain an
  unrepresented input, not a guessed layer count.
- Loc82281490 skips90xx colour commands in phase1. The ordinary colour update
  at8228076C..82280870 preserves object alpha from visual+3416; RGB modulation
  does not alter alpha. The shadow producer copies just alpha and the complete
  visual+3068 texture mode; it does not consume phase0 material/light/fog values.
- Loc822816B0..82281938 processes early image/UV overrides before the shadow
  gate. In phase1, non-base channels and base commands issued while authored
  alpha is off skip table/special/late binding. An earlier image override still
  binds; a first UV match skips its own image. The recipe therefore records
  alpha at each texture command, not at the eventual primitive. Existing ordered
  UV initialization/reset and image leases are shared with the scene composer.
- Phase1's07xx/08xx force the command value to zero; base addressing stays Wrap
  on both axes. Filtering comes from the current owned pass publication. The
  existing ordered cutoff/default/object-override contract is reused; comparison
  is copied from the non-bootstrapping alpha owner, without requiring blend data
  for a depth-only draw.
- Hard-off route classification and actual submission use the same whole-node
  shadow admission. Deferred, skin/wind and texture-dependent effect siblings
  remain explicit unsupported families. Missing admitted inputs refuse before
  any sibling reaches the queue. Phase0 cutout casting is not relabelled phase1.
  Legacy texture/shader/light consumers cannot see the new phase1 scope.

Three new production shaders provide the alpha-only vertex stage and textured /
zero-layer fragment variants. The latter declares no sampled resources; the
former binds the retained base image with a wrap sampler. Native comparisons
operate on base*object*vertex alpha, including negative-U absent-layer behavior.
Discarded fragments leave depth untouched. No colour attachment, scene lighting,
receiver read or active-depth sampling is involved. Inactive descriptors reuse
the base image, not the shadow attachment. Object/instance GPU sizes remain
208/816 bytes; no new shader record ABI. Opaque casting keeps its position-only
program, and textured leases survive through the existing frame-slot fence.

## Verification

The devloop skill selected existing fixtures and bounded supervisors. All jobs
below are terminal; no main game process, capture, profile override or cook ran.

- 315 artifact-free Python source/scenario checks pass (last0.201 s). These
  check wiring, not native consumer reachability or pixels.
- Material36/PID24192 builds14 steps; CPU34/PID34112 passes0.13 s/CTest0.15.
  Tests cover command-time alpha versus later primitive state, ordered texture
  modes, early image/UV exceptions, late-image suppression, reset semantics,
  full object mode/alpha, bounds/nonfinite values and source destruction.
- Output36/PID32580 builds; CPU19/PID31912 passes0.40 s/CTest0.42. Mixed opaque /
  textured / zero-layer siblings, required packet count, missing/invalid image,
  UV/sampler/comparison/alpha, retained geometry/image and batch descriptor
  admission are exercised. No scene material or lights supply caster coverage.
- GPU30/PID33340 failed because the existing cross-directory shader-header
  ordering list had not been extended for the three new shaders. The list is
  now complete and a guard checks every production rigid HLSL entry. No guest
  target rebuilt. Retired failure stdout3137 B, SHA256
  `6DDE9283C77F9BA17A0EEEE7986F70E305F6D0B5C797D7BEC2AFF805D9C9B52B`.
- GPU31/PID35344 compiles all three new SPIR-V shaders and the production factory.
  Rigid09/PID23348 passes37 modes in1.86 s/CTest1.87 on RTX3060. Each mode has
  8x8 two-eye scene colour/depth plus a mono D32/S8 shadow map. All previous23
  cases remain;14 new cases check eight shadow comparisons, clear depth after
  discard, zero layers, two instances with different cutoffs/alpha, negative-U,
  disabled vertex colour, wrap beyond1 and unsigned cutoff above255. The new
  caster-depth oracle is independent of the production predicate. The new
  cutout shadow map is not yet qualified as a sampled game receiver sequence.
  Maximum colour error3.39895e-05, unchanged0.003 tolerance; depth tolerance1e-5.
  Vulkan validation errors0/warnings0; one missing GOG overlay loader message.
  All readback stays in RAM; raw/image exports0.

Fixture exe830,464 B, SHA256
`626AB19AF5DD387A1475C8C25F4B40E9120CA2259A670312CAF1460E8EF814A6`.
Host112 preflight subsequently failed before compiler or logs were created.
The host remains111,48,669,184 B, SHA256
`295E4FB6B8265DCA01701D411BCCBB9EC4088B0D4DC7BEB143AF2C8F3EFDA655`.
It contains neither this shadow connection nor8a1bf9a's scene cutout connection.
Profile unchanged116 B, SHA256
`2F1BC38D763A1B7BDBA31F560684FD4AA7E42A714600B8D344F19DA7F38E23B0`.

Next: host compilation/link when the original reserve fits, then a changed-code
integration run proving fresh scene/caster cutout emissions, participant ordering,
retained-legacy interoperation and inspected game pixels. Keep layered/inherited
coverage, selected-object cold/reload and both-eye gates. Run945 remains the last
accepted live/pixel result. Runs948's UV and941's light dirty-bit mismatches
remain unresolved with strict comparisons/evidence preserved. Normal acceptance
switches stay off. Full fields/battles/cutscenes/menus/events precede Quest.

## Storage and retirement

Original3 GiB exception and62,509,998,080 B operational floor unchanged. First
free61,869,699,072 B did not fit any producer. After drive-wide recovery to
63,141,998,592 B, sequential32 MiB fixture peaks fitted. Rigid09 ended at
63,116,869,632 B; later activity again crossed the floor, blocking host112's
192 MiB peak. A separate Android-toolchain build was visible, not agent-owned
or modified; its output is not measured or attributed here.

After replacements passed, removed14 exact old logs: material35/CPU33,
output35/CPU18,GPU29/rigid08 and the explained GPU30 failure, stdout/stderr.
17,207 logical bytes removed; immediate free63,146,737,664 ->63,146,766,336 B,
28,672 B observed recovery (not isolated from concurrent volume activity).
Old text is gone; tests are reproducible and results remain in the reports.
Protected game data, profiles, active builds, baseline and unresolved-failure /
game-pixel evidence remain intact. No protected raw or image payload deleted.

Retained fixture sizes: material8,329,132 B/41files (+38,306), texture70,636,050 B/
129files (+197,240), GPU10,654,788 B/10files (+60,486), logs199,039 B/136files
(+3,104). Three newly required shader headers total107,296 B. Counted retained
growth406,432 B; other objects/CMake/source/Git and drive-wide activity are not
allocated in that subtotal. Retire current logs after equivalent replacement;
reuse the fixture builds and generated headers. Ending inventory free
61,580,406,784 B is289,292,288 B below the first measurement, not attributed to
the roughly397 KiB of counted outputs. Same cumulative ledger, no budget reset.
