# Native rigid scene/shadow shaders

2026-09-07, parent `d98b980`, Windows Vulkan-only desktop tree. This checkpoint
implements the actual shader/input dependency of the direct rigid-object path;
it does not yet route a live game object or delete a compatibility consumer.

## Implementation and sources

- `src/gpu/scene/native_rigid_inputs.h`: explicit C++/HLSL layout, 176-byte
  object and 608-byte pass buffers. Row-vector matrices have explicit rows;
  inverse-transpose normals support nonuniform scale. Semantic light/fog inputs
  are explicitly packed, with finite/flag/range validation and reserved zeros.
- `native_rigid_program.{h,cpp}`: canonical named position/normal/UV/color to
  native locations 0..3, owned scene VS/PS and position-only shadow VS programs,
  shared explicit layout. Two dynamic UBOs, two textures and two samplers.
  Exact input schema is checked. No global cache growth or import-source reads.
- `native_rigid_shader.h` and three `native_rigid_*.hlsl` files: actual scene
  transformation, per-eye views/cameras, single albedo and vertex color, three
  named lights, specular, two ordered fog layers and native shadow casting/
  comparison sampling. No translated shader common/register ABI.
- Reuses `native_lit_shading.h`, the existing normal-lit shader's scalar core
  with independent CPU references. Existing `bd_normal_lit.hlsl`, transform
  helpers and ignored `bd_normal_cs_vs_norm.hlsl` dump informed the contract;
  no new extraction or generated/guest source edits. Original asset UV conventions
  and actual model/material eligibility still need producer-side association.
- Reuses the existing pipeline-program, binding and Plume backend code. The
  Vulkan fixture calls the production factory, pipeline-description application
  and command binding core; it does **not** exercise `GetOrCreatePipeline` or its
  background cache. That path has the preceding checkpoint's CPU coverage.

The first family is opaque rigid single-albedo geometry. Normal/detail maps,
reflections, skin, wind, alpha test/translucency and special overrides are not
silently accepted as converted. Art-style and live family equivalence are not
established by the synthetic fixture. The new shaders are not selected by any
game producer. Named light/fog packing is not ownership of their live producers.

## Verification and corrected failures

Repository guest-source/devloop skills guided source-first investigation and
reuse of focused fixtures, not another game boot or build tree.

Material18/CPU16 passes: new object/pass packing and refusal checks, plus the
existing independent lighting/fog reference tests; behavior 0.10 s, CTest 0.12 s.
234 Python source/scenario checks pass in 0.051 s; eight runner tests in 0.007 s.

Fixture build21 failed before any GPU run: cross-directory shader generation
was not ordered before production C++ compilation, and Windows `max` collided
with `std::max`. Explicit generated-header ordering and `(std::max)` fix those.
Build22 passed and compiled only three new host shaders. Its log remains.

Rigid GPU01 then failed mode1/right-eye/left-edge color: actual 0.176633 versus
expected 0.134101. Eye translation put the sample exactly on the shadow-frustum
edge; interpolation could place it just outside, where the shader intentionally
returns lit. The test now uses a half-pixel eye separation with all samples
strictly inside. Shadow input is position-only, fixing six unused-attribute
warnings without suppressing validation. Fixture23/PID30020 passes, 3.877 s.

Rigid02/PID30332 passes on RTX 3060, 1.21 s, Khronos synchronization validation
enabled: zero validation errors/warnings. One unrelated missing GOG overlay JSON
loader diagnostic is reported separately. Each case reads all 128 pixels in two
8x8 float color layers, both scene depth layers and an 8x8 native caster depth:

| Case | Maximum color error |
| --- | ---: |
| Directional + secondary point, unshadowed | 0.0000293851 |
| Same lights, native caster/receiver | 0.0000129938 |
| Primary point + radial/ordered fog | 0.0000365227 |
| Primary spot + planar/ordered fog | 0.0000404567 |

All 2,048 color-channel comparisons pass tolerance 0.003; expected eye depths
0.5/0.25 and caster depth 0.25 pass tolerance 0.00001. Nonuniform object transforms,
UV stripe sampling, different cameras and two dynamic UBO offsets are exercised.
The CPU oracle shares scalar math with HLSL, backed by the separate independent
reference tests; it independently reconstructs pixel positions and texture values.
It does not qualify shadow filter-edge behavior, every flag combination, arbitrary
geometry, MSAA shading, the full shared cache, game reloads or animated effects.

Snapshot19/PID31048 passes the reused fixture's eight 1/2-layer x 1/2/4/8-sample
snapshot/resolve combinations, 1.04 s, zero validation errors/warnings.
Both tests use bounded in-memory readback, 5 s fences and 30 s CTest limits;
no image files/raw captures or profile changes. These are actual GPU pixel/depth
comparisons, not inspected game imagery or complete desktop/VR qualification.

Host73/PID28680 terminal success, 9.882 s: up-to-date codegen probe, host C++ and
link only; no guest objects or translated shaders rebuilt. Output:

- `reblue_vk.exe`: 48,325,632 B, SHA256
  `F7827E8DD5666ACBC60D71164582EF6451DED4A7AA9A46CE324E6DE1871815A0`.
- `reblue_vk.pdb`: 107,266,048 B.
- `native_scene_snapshot_test.exe`: 794,112 B, SHA256
  `6A894CE657A8CFB97E3C5F890FE07A471E7A72474DD907F0F62A3C64A7C9FCBA`.
- Fixture PDB: 7,647,232 B. Persistent profile remains 116 B, SHA256
  `2F1BC38D763A1B7BDBA31F560684FD4AA7E42A714600B8D344F19DA7F38E23B0`.

No new game run was warranted: existing engine behavior does not select these
programs. Run918/image remain the previous engine regression, tied to host72's
different binary, not this build. No game FPS or development-speed multiplier
was measured; short focused test durations describe only these tests.

## Storage and next consumer

Same cumulative ledger in `20260906_0333_native-scene-state-bridge.md`, original
3 GiB exception/floor and zero-new-raw gate unchanged. Known comparable retained
fixture/shader/log growth is 2,301,486 B; host exe/PDB plus new factory object add
666,544 B. These add real shader GPU coverage in reused trees; replace evidence
by purpose on the next equivalent run. Other object/metadata/source/helper deltas
are not fully measured. Peak overlap was budgeted at 128 MiB and supervised.

After replacements passed, removed 14 exact superseded build/test log files:
material17/CPU15, snapshot03, snapshot builds01/04/21 and rigid01 stdout/stderr.
20,826 logical B; immediate free 63,285,940,224 -> 63,285,972,992 B,
**32,768 B actually reclaimed**. Reproducible logs are removed, not recoverable
in place. Failed build21 stdout SHA256 was
`692391BD5301E4927C4C2E885603978EF389FFE8D0E5B1CBA6B7DE43CD918EE9`;
rigid01 stdout was
`2502809542F20B0BC7C533E69484EC4FA0D6C2FCB70A69A54729377290666454`.
Their causes/results above remain. Prior distinct failure logs, run918/image,
baseline/motion/protected raw evidence and game data stay untouched.

Cleanup-end free is 58.94 GiB, drive-wide usage +48,009,216 B from current first
63,333,982,208 B; do not attribute that whole delta to this task. Aggregate build
logs now 175,763 B, 929 B below the initial set. No owned producer remains.

Next: select/record an actual `bg41_01` model/content identity, publish its owned
model/instance-to-primitive association and live light/fog/material inputs, then
route scene and shadow submission through the shared queue/cache before the
interpreter/replay branch. Whole-node preflight must preserve unsupported
siblings. Only the selected family's interpreter/capture/replay-disabled cold
load, updates, teardown/reload and pixels can establish a direct game object.
