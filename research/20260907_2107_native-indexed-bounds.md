# Native indexed bounds and per-primitive occlusion (2026-09-07)

## Connected ownership change

`BuildNativeMeshBounds` derives bounds from checked v2 little-endian indexed
Position.xyz values, including signed base vertex, before GPU allocation/map.
Unused stream vertices and Position.w cannot enlarge the box. Both import and
source-free asset load share `native_mesh.cpp::Upload`; the resulting native
geometry retains values after source/CPU payload retirement. Existing files,
checksums, content identities and disk budgets are unchanged: no recook, new
format, sidecar or duplicate asset payload. V1 has no guessed position schema.

The existing native rigid VS uses `float4(vertex.position.xyz,1)` transformed
by `object_data.world` (`native_rigid_vertex.h`). Bounds use that same packed
matrix, not the walk's radius scale. Affine interval arithmetic supports shear,
reflection and nonuniform scale, with outward FP32 rounding/error padding;
invalid/non-affine/overflow inputs stay visible. Whole-node material/light
effects and sibling preflight still finish before culling. Then each pending
primitive requests/consumes its own query, keyed by instance/model generation/
node/primitive. Missing bounds or one sibling's visibility cannot hide another.

The broad node sphere and `world_bounds` handoff are removed from this native
consumer. The host walk's remaining source discovery/frustum/distance adapters
are not removed or relabelled native. Only recognized native rigid scene draws
use this path; skin/wind/unsupported material consumers remain tracked elsewhere.

The native query push ABI is now96 B: exact camera rows, world box center and
independent half extents. Boxes inflate conservatively per axis, including flat
geometry; the complete proxy must remain beyond homogeneous near/eye planes.
Clip-plane testing adds an FP32 error envelope. Exact camera/depth identity,
bounds, originating-frame age, two-zero warm-up and fence requirements remain.
This does not solve moving-camera or moving-occluder temporal visibility.

## Verification before the live gate

327 Python source/scenario checks pass (0.150 s). Mesh13/CPU11 pass0.12/0.14 s;
the first mesh12 compile exposed only a Windows max macro in the new test,
corrected before retry. Output40/CPU23 pass0.36/0.38 s. Tests cover unused large
vertices, signed base indices, source destruction/file round trip, unchanged
identity/bytes, malformed/nonfinite data, flat geometry, rotated/scaled/sheared/
reflected corners and large translations/overflow. Occlusion cases retain prior
camera/near/lifetime/capacity safeguards and add anisotropic boxes and sibling
identity separation.

GPU42 recompiles the shader and links the actual mesh-bounds producer into the
existing fixture. Occlusion5 passes all8 mono MSAA1/2/4/8 × translated/rotated
cases in1.09/1.11 s, validation0 errors/0 warnings. Indexed native boxes transform
through two real submissions/fences; only the hidden sibling receives Occluded.
All first-submission HDR/depth pixels remain exact and the descriptor-resume
check passes. This fixture is not an oracle for skipped game-object pixels.

Host120/PID33752/session92497 exits0:38 host steps, codegen0 written, no guest
objects. Actual binary stamp edbcd32 dirty; later commits do not restamp it.
Exe48,708,096 B SHA256
`80E06B21649EA0BACE29CBEE31848F413ED2A29E16B64114EC795BDA9109DDA3`;
PDB109,707,264 B SHA256
`33A6B8BA495325680411369542E2C39D4CB7BB9DFDBE6751250EC59C35C729E4`.
GPU fixture SHA256
`7D74CB68C34ACA3ACB964BE5B00EE74D27BEDF11BAC5C2D2021B89EC2C052B74`.

Run960/PID33516/session76067 started21:07:49 with full strict cold/reload flags,
Count0, one110 KiB owned-window JPEG,300 s/800 KiB text/192 MiB free-drop/75 MiB
diagnostic overlap/original floor, and guaranteed profile restoration. Its final
acceptance is not recorded at this source checkpoint. Early152 native primitive
skips prove the live branch is reached, not visual correctness or speedup.
Keep945 accepted pixels,956 tree-gap and957/948/941 failure evidence. The earlier
940 generic mono reload JPEG was retired after checking its supersession by945;
its hash/findings remain in1140. See the [same cumulative storage ledger](20260906_0333_native-scene-state-bridge.md).
