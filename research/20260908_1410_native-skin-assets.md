# Native joint-local skin assets

2026-09-08. Source f04aaa8 plus this checkpoint. A prerequisite implementation,
not a new live renderer qualification. Previous turn pushed both water commits.

## Contract that changes the implementation

Read the existing skin-binding and instance-pose findings, model ownership
frontier, current mesh import/cooker, shader input adapter, pose producer and
rigid scene/shadow admission. Inspected the existing translated shader output
in `out/build/win-amd64-release/hlsl_dump/bd_normal_cs_vs_env.hlsl` (skin branch
around lines956..1147) and `bd_shadowmap_vs_env.hlsl` (around lines918..1064).
These are existing shader artifacts, not newly generated output. Their active
constrain branches have this contract after the shared attribute decoder:

| Influence | Joint-local position / weight | Joint-local normal / palette slot |
| --- | --- | --- |
| 0 | POSITION0.xyz / .w | NORMAL0.xyz / floor(.w * 64) |
| 1 | POSITION1.xyz / .w | POSITION2.xyz / floor(.w * 64) |
| 2 | POSITION3.xyz / .w | POSITION4.xyz / floor(.w * 64) |

Each active joint transforms its own local position. The shader divides weighted
position/normal sums by the active weight sum; zero-weight lanes contribute zero.
The palette maps joint-local coordinates directly into world space, so applying
the node's world again is wrong. Normals use each palette's linear transform and
are normalized after blending; this is not the rigid inverse-transpose path.
The original tangent path uses the first influence rather than three supplied
tangents. The cooker preserves an authored Tangent0; future normal-mapped native
shaders must retain that distinction or make an explicitly verified asset change.

This is not an ordinary single-position/four-weight vertex. Converting it to one
bind position requires the skeleton/inverse-bind contract, not an assumption that
all joint-local positions coincide. The existing exact per-primitive binding in
`native_skin.h` supplies model-local IDs, without matrix-equality discovery.
`native_instance_bridge.cpp` publishes owned pose matrices at the completed
render-copy boundary. `host_draw.cpp::ImportSkinPose` still reads the source
palette for replay; the native rigid programs still reject all skinned ranges.

## Implementation

- Reuse `native_mesh_cook.cpp`'s checked numeric/endian decoder; the private
  staging representation may contain the secondary source positions, but cannot
  be serialized. `CookSkinMesh` requires an explicit1..3 influence count and
  exact binding; resolves fractional palette slots into model-local uint16 IDs;
  splits positions, normals, IDs and normalized weights into semantic fields.
- BDMESH v3 uses little-endian float4 interleaving with explicit paired
  `SkinPosition`/`SkinNormal` indices0..2 and `SkinJoints`/`SkinWeights` fields.
  Integer IDs are exactly representable floats for portable vertex fetch.
  Padding is zero; schema/ID/weight/finite/count/stride validation is transactional.
  Unsupported negative/zero-total weights, invalid active slots and missing
  fields refuse rather than invent identity data. IDs do not inherit the49-slot
  import ABI ceiling. No game assets or per-vertex payloads are committed.
- Existing v1/v2 identity/encoding stays unchanged. v3 cannot masquerade as v2.
  All versions use the same64MiB payload bound and `NativeMeshDiskCache` budget,
  stable names, writer lease, restart inventory and unchanged-output reuse.
- `native_skin_mesh.h` provides source-free weighted deformation and indexed
  animated world bounds from native mesh bytes and a supplied owned pose span.
  Signed base vertices are respected; unused vertices do not enlarge the bound.
  Skin assets have no rigid Position semantic, so rigid bounds and the legacy
  rigid shader-input adapter refuse them. Do not substitute bind-pose bounds.

## Verification and limits

Mesh build14/PID37340 and CPU12/PID33928 passed. After adding shared disk-budget,
packed-pair, normal-transform and indexed-outlier regressions, build15/PID29008
and CPU13/PID36612 passed (CTest0.12s,0.14s total). The fixture covers1/2/3
influences, reordered source declarations, all position/normal pair swaps, exact
joint mapping up to65535, zero-weight inactive slots, missing bindings/attributes,
source destruction, persistence/checksum corruption/truncation/version confusion,
transactional refusal, rotation/nonuniform scale, separate local normals,
single world translation, changed poses, missing joints and non-affine matrices.
v1/v2/v3 share one exact-budget disk fixture, including reuse and process-style
cache reopening; temporary fixture directories clean themselves up.

All378 artifact-free source/scenario guards pass, including23 geometry guards.
They are not GPU evidence. No shader was changed, no game was launched and no
asset was cooked from game data. The host binary remains host155/run980; no
additional live rendering dependency is claimed removed and no speedup claimed.

Next connect the load-owned primitive's binding/influence count to `CookSkinMesh`
and the existing geometry library. It must then feed native GPU palette storage
from the instance pose, native scene/shadow shaders and existing queues with
animated bounds, ordering and fence retirement. `ImportNativeMesh` still invokes
only `CookRigidMesh`; simply relaxing admission or calling the old translated
shader with v3 inputs would be incorrect. Secondary/interpolated pose producers,
skeleton/animation ownership, scene/material families and the complete desktop
field/battle/cutscene/menu/reload/both-eye gate remain open. Quest stays gated.

Storage accounting and cleanup are in the cumulative scene-state ledger; this
does not create another budget or duplicate the prior GPU/game artifacts.
