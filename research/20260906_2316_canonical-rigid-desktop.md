# Canonical rigid geometry: desktop component verification

2026-09-06 EDT. Rendering source is parent `1e4a72d`; Plume stays `3094b35`.
This resumes the earlier CPU/source checkpoints after the owner's explicit
3 GiB cumulative-cap approval. It does not complete native static-object drawing.

## Builds and scenario gate

Used the devloop skill and the existing configured host/mesh trees. No download,
guest-object rebuild, shader regeneration or asset-library recook. The original
storage ledger remains `20260906_0333_native-scene-state-bridge.md`; only its peak
cap increased. The three ignored supervising helpers now enforce the approved
62,509,998,080 B operational floor; other limits remain unchanged.

- Mesh build09 /PID31092 and CPU08 /PID30632 pass. CPU test0.11 s, ctest0.14 s.
  This is the rebuilt fixture including the pull-default correction, not the
  earlier executable. Numeric/schema/identity, source-destroyed IA/pulling,
  lifetime, persistent round-trip and disk-limit/lease fixtures all pass.
- Host64 /PID27696 passes. CMake reconfigured in place for the new source file;
  codegen reports zero files written and its module up to date. Ninja's final
  edge ends at19.162 s; this is not total wall time including CMake/wrapper.
  The linked game exe is48,212,992 B, PDB106,475,520 B.
- 156 source-boundary tests and34 scenario tests pass. The new canonical mode
  reuses the existing post-event consecutive observation windows and requires
  positive canonical mesh ownership and at least32 fresh canonical draws, plus
  native pulling at the CLI gate. It rejects startup-only, wrong-scene, missing
  event, stale/reset/missing counters, nonconsecutive windows and oversized logs.
  Zero source-free loads are reported as zero, not silently called covered.
  The full checker passes on run912 and correctly returns Pending on the old
  run911 log with canonical mode requested; earlier native draws cannot qualify it.

## Desktop run912

PID27288, 23:12:47..23:13:58, about71 s, normal flat1920x1080/MSAA. The wrapper
enforces75 s,400 KiB run log, the cumulative diagnostic/free-space thresholds,
zero raw bytes and at most one300 KiB JPEG. All10 profile settings took effect:

```toml
bd_xr_autoplay = true
bd_perf_csv = false
bd_capture_after_s = 0
bd_capture_min_draws = 600
bd_capture_frames = 120
bd_native_materials_verify = true
bd_pso_precache = true
bd_native_instances = true
bd_native_texture_tables = true
bd_native_texture_tables_verify = false
```

The original116-byte owner profile was restored byte-for-byte in guaranteed
cleanup; every owned producer is terminal. No runtime/config/Vulkan error,
pose drift, table refusal or matching-check failure was reported. Read-only
post-run inspection found **zero new raw/perf/cache/HLSL-dump files**. Mesh
cache remains3,510 files /36,510,144 B; mesh persistence reports zero writes.

After an observed opening event, field contexts at frames1726 and2026 identify
`FieldActive`, state0, `bg41_01`, player present, event/movie/loader/icon clear.
Fresh samples following those windows establish:

| Observation | Result |
| --- | --- |
| Canonical geometry | 2,206 meshes;104,787 fresh draws; zero source-free disk loads |
| Native vertex inputs | 117,515 fresh pipeline/decode/pulled records;2 owners /3,824 B |
| Load-owned geometry | 35,324 fresh draws;380 matching source checks;782 unavailable replay lookups remain |
| Material checks | 15,053 diffuse and14,341 specular, all matching; reflection not exercised |
| Instance poses | 96,857 fresh matching reads/checks; no unavailable/refused observations |
| Normal texture tables | 20,919 fresh lookups; zero original fallback/comparison calls |
| Observed walking | Same episode;90 nonstationary observations and129.694684 world-unit distance delta;9.805 s walking |

Instancing groups and indirect calls are active; e.g. the last sampled queue
reports234 input draws,230 issued, a four-draw instanced group,268 pulled, and
272 indirect draws in175 calls. These are diagnostic queue observations, not
performance results or whole-frame native-ownership counts. Autoplay begins
this walk about57.1 s after initial pad polling; the earlier42 s observation
was not a guaranteed boot time.

## Pixels and remaining ownership

Inspected `out/verification/native_canonical_mesh_window.jpg`,1920x1080,
210,325 B (JPEG85). Character/gait, foreground terrain, foliage, fence and cast
shadows appear coherent; distant scenery remains blurred. Also inspected the
prior standing native-pulling image. Framing and pose differ, so this is not
same-camera pixel equality or proof that earlier thin cliff-edge artifacts
are fixed. The three prior motion images and standing/failure baseline remain.
One new sanity image does **not** qualify a new sequence, reload or both eyes.

The GPU geometry arena reserves64 MiB. Of3,186 resident owners,2,206 are canonical;
the other980 are still noncanonical imports. This one field's mix is not a
whole-game conversion percentage. Expanded float4 vertex bandwidth, compact
packing and speedup remain unmeasured. No Quest work or full desktop gate claim.

`LoadNativeGeometry(content_id)` still has zero observed source-free GPU loads.
The current game producer imports source wrappers, while shader-register ABI,
source-index selection and captured templates remain consumers. Next, complete
one rigid object's native shader/material/texture/pass records and native
instance-to-direct-scene/shadow path, including source-free loading/lifetimes
and reload qualification; do not bulk-recook the library merely to get files.

## Reproducibility and retention

- Game exe SHA256: `632c0f26468f7fb462922e8a476963f59b45966e92d5779e4c21055cf43bf952`.
- Rebuilt mesh fixture SHA256: `d49e7b7dc03acbd9c25c369b9a1df81bb51a1620e105cc3fcb3d46dc798490e3`.
- Run912 log202,300 B SHA256: `468f77b6f900fa5a6522b65b2a9284c76bc500d161a04d1378cbb2652a5d0533`.
- New JPEG SHA256: `33dd7c6e508774bac8fd37648ba3e644b69fb44e05b80bd430580bf3dd63fec8`.

Keep run912/new JPEG as current canonical component evidence, run911 and its
three images as the movement baseline, and protected prior failure/flat/XR
evidence. Replace by equivalent verification purpose, not each commit. After
validation, removed only six exact superseded build/CPU stdout/stderr logs
(mesh08, CPU07, host63):2,675 logical B,8,192 B measured reclaim in two operations.
Keep mesh09/CPU08/host64 and unchanged cache-verify04. No protected data removed.
Complete retained-growth/free-space accounting is in the original ledger.
