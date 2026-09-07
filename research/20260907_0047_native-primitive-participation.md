# Native primitive participation and compound template invalidation

2026-09-07, EDT. Parent `cf923aa`; this checkpoint advances the first native
static-object outcome, not the completed renderer goal. Plume remains `3094b35`.

## Ownership change and source evidence

The existing load-owned material program now retains ordered alpha/texture
policy steps and per-range winding. The pure native evaluator consumes these
steps plus one object/pass publication, without source memory, device state or
captured draws. It composes cull, alpha-test, shadow eligibility and ordinary
direct/deferred/suppressed participation. All owned capacities are charged to
the existing model and 4 MiB object-scope budgets; no asset format/cook change.

Exact source traced after `config/hooks/pso_predictor.toml` and
`config/hooks/render_list.toml`:

- Generated `bdSceneNodeDrawSingle`, `reblue_recomp.40.cpp`: 1000/2000/3000
  winding, 0900 alpha and technique/wind/pass rules, texture-dependent routing,
  and the separate direct and deferred emission paths.
- `sub_8227FDC8`, `.25.cpp`: pass-mode overrides for alpha/sorted participation.
- `sub_82287738`, `.85.cpp`, and native raster import: unsigned source cull byte,
  reverse winding and two-sided handling.
- Texture tail and early-image branch: base table image, not the final sampled
  override, controls volume analysis. An early image replacement skips that
  analysis; a later zero-alpha command does not reset prior shadow rejection.
- `D3DResource_GetType_hook` and native texture ownership: 3D images identify
  volume resources. Their authored effect-count/fade producers remain missing;
  the policy marks their routing unknown rather than inventing opaque-only work.

The native consumer uses already-owned image-table leases and range bindings.
Fresh policy cull replaces captured cull at replay dispatch; the native pipeline
intent still prevents engine state history from overwriting it. The whole-node
preflight rejects missing or no-longer-direct known policy before any draws.

Before either direct replay or deferred append, a changed policy stamp expires
both template halves under the same store lock, including stale empty/never
state. An interpreted node contributing no deferred entries retires its old
list recipe. Winding is part of the stamp because deferred compatibility entries
still carry imported winding. This is a safety boundary for transitional
submission, not the removal of those templates or original material callbacks.

## Verification

Existing native-material fixture15 build and CPU13 pass (behavior0.10 s,
CTest0.12 s). Tests destroy source commands before consuming owned policies,
exercise mixed direct/deferred nodes, alpha/cull/pass/technique/wind rules,
unknown-volume transitions, changed-plan conditions and transactional bounds.
Atomic erasure of both template halves is source-guarded; this fixture does not
execute the GPU template store. All167 boundary guards and49 scenario tests
pass. Host67 linked successfully with32 actual build edges, module up to date,
no recompiled guest objects or shader regeneration.

Run915 /owned PID31620,00:44:15..00:45:14 EDT: normal flat1920x1080, native
MSAA path, precache/pulling enabled. All13 profile settings effective:

```toml
bd_xr_autoplay = true
bd_perf_csv = false
bd_capture_after_s = 0
bd_capture_min_draws = 600
bd_capture_frames = 120
bd_native_materials_verify = true
bd_pso_precache = true
bd_native_instances = true
bd_native_shadow_inputs = true
bd_native_material_textures = true
bd_native_primitive_policies = true
bd_native_texture_tables = true
bd_native_texture_tables_verify = false
```

Fresh consecutive post-event `bg41_01` windows at frames2040/2340, field-state0,
player1, event/movie/loader/icon0:

| Check | Observed delta |
| --- | --- |
| Primitive plans | 45,313 known,0 unknown;74,785 direct/14,419 deferred/0 suppressed candidates |
| Policy consumption | 75,985 reads;15,493 checks,0 wrong;59,292 native-cull replays |
| Live policy changes | 0 cull changes,0 compound refreshes; not runtime-qualified |
| Material image/UV | 33,600 object publications,300 with overrides;58,631 matching checks;62,063 image slots/59,292 UV draws |
| Object-scope budget | Peak111,300 B;0 refusals;167,654 cumulative unsupported scopes |
| Canonical geometry/pulling | 2,206 canonical owners,+137,587 draws;150,558 pulled records;0 source-free GPU disk loads |
| Load-owned geometry | 45,544 draws,1,416 matching checks;942 cumulative unavailable lookups |
| Material values | Diffuse15,493/specular14,753 matching;reflection0 |
| Poses | 109,733 reads/checks;0 unavailable/refused/drift |
| Normal image tables | 20,912 lookups,0 original fallback/comparison calls |
| Shadow receivers | 15,493 checks,14,878 receiving;2,973 policies,209 disabled,0 unknown |
| Movement | Same episode,+31 observations,+30.749320 world units;12.033 s walking |

Policy/material lookup unavailability is72,281 cumulative and remains explicit
adapter coverage debt. Candidate counts are not issued native direct/deferred
draw counts. The old native-image hook has0 reads; the newer object lease
consumer is exercised above. Instancing/indirect activity is not a timing result.

Inspected one full-resolution quality60 JPEG: Shu running beside the fence and
building; terrain, foliage, rocks and cast shadows coherent. Thin black cliff
artifacts and distant blur remain. This is a component sanity image, not visual
parity, an animation sequence, reload proof or both-eye qualification.

Bounded runner exit0; no remaining owned renderer/build processes. Original
116-byte profile restored exactly. No new raw/perf/cache/dump files; existing
3,510 mesh-cache files/36,510,144 B are unchanged.

SHA-256 evidence:

- Host exe48,266,240 B: `82bfd98ad6d9633706c52b4b600a24db9b39aa85b7c75261633731d41c602cc9`.
- Material fixture exe: `9c46a0e647fcbcac56a9c10f8c881f6eb50e8853339601868d28876269af4786`.
- `out/build/win-amd64-release/logs/reblue_915.log`,216,297 B:
  `94cc7bb6e8ad61eda37539ea8eedbdb1b7e8b2034461176a594a80fca46c40b55`.
- `out/verification/native_primitive_policy_window.jpg`,133,740 B:
  `56ab99f5fc7a0075d5d9c5b41d2a828999af4ecebfb52cb7b44476053a0bf529`.
- Restored profile: `2f1bc38d763a1b7bdba31f560684fd4aa7e42a714600b8d344f19da7f38e23b0`.

## Remaining outcome and storage

Current publisher supports phase0 only. Phase1/2 semantics have pure-test
coverage, not live publication. Original pass/shader/material setup, alpha-state
binding, volume-effect routing, callbacks, source lookup and captured templates
remain. No full-host frame or speedup is claimed. Next: one ordinary rigid
family's named shader inputs and direct scene/shadow consumer, cold-load/reload
tested with its interpreter/template capture disabled. Reuse existing owners;
do not turn every remaining console helper into a separate adapter milestone.

Same cumulative ledger: `20260906_0333_native-scene-state-bridge.md`, approved
3 GiB exception only, operational floor62,509,998,080 B and raw0 unchanged.
After replacement validation, removed run914 log/material image and six exact
material14/CPU12/host66 logs:369,175 logical B. Immediate free increased249,856 B
(244 KiB); concurrent volume writes mean this is the observed recovery, not a
file-allocation estimate. Old reports/hashes remain, exact old runtime files do
not. Build/test logs are reproducible. Baseline/failure/motion images and the
protected historical raw archive are untouched; no prior cleanup recredited.

Comparable fixture/log/image set grows127,452 B: fixture+139,008, aggregate build
logs-540, runtime log-10,673, image-343. This retains new policy fixture/live
consumer coverage; replace by purpose at the next equivalent qualification.
Exe/PDB grow171,520 B. Source/docs/Git/helpers/objects/build metadata lack complete
byte baselines, not zero. Aggregate build logs150,998 B; images10,240,140 B,
245,620 B remaining under10 MiB. At cleanup free63,181,299,712 B (~58.84 GiB),
drive-wide use201,752,576 B since this continuation's first measurement, not all
attributable to identified task outputs. No producer remains active.
