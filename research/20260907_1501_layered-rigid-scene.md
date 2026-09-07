# Layered native rigid scene connection

2026-09-07, EDT. Source parent9a9c1d2. Previous goal implementation delivered
the wider caster family; the intervening push/status check made no renderer
progress. This bundle connects ordinary multi-primitive scene materials to the
same native owners/queue. It is CPU/shader-fixture verified, **not game-qualified**.

## Recovered contract and implementation

Reused the guest-source/devloop skills and existing material/lighting findings.
The native consumer needed the three-layer material contract, not another replay
cache or a per-console-helper rewrite. Relevant sources:

- `bdSceneNodeDrawSingle`, generated file40,0x8227FEE8; its census entry is in
  `config/hooks/guest_census.toml`. The active `scene_node.cpp` replacement guards
  legacy entry, while `host_walk.cpp` selects direct submission before calling it.
- Ordinary initialization at generated40:10324-10490 copies the object's initial
  four UV offsets to both UV01 and UV2B. The default-object branch does the same.
  The complete body's direct writes to offsets48/52/56/60 are these eight stores.
  Later channel0/1 overrides and resets update UV01 only. Layer2 must not inherit
  layer0's final animated offset. Modes6/7/8 and special callbacks remain excluded.
- Ordinary vertex shader contract (recorded B5C88BB6295138CC): `(UV+1)/512+offset`;
  TexCoord0.xy/zw feed layers0/1, TexCoord2.xy feeds layer2. Canonical v2 assets
  already retain named float4 attributes. Large authored wrapping UVs are not
  an origin to subtract. No recook or new asset format was necessary.
- `bd_normal_lit.hlsl` retains the ordinary translated texture frontend
  (FB83DD3F5E67CEB7). Layer0 supplies RGBA; each detail performs
  `rgb += (detail.rgb-rgb)*detail.a` in order. Base alpha survives both overlays,
  then object/vertex RGBA modulation applies. With reflection off, negative U
  selects transparent black, not sampler wrapping; zero texture layers use white.
  Reflection's different fallback, normal mapping, skin/wind and alpha remain
  explicitly unsupported by this family.

`MaterialTextureValues` now owns the independent initial secondary UV vector.
The real packet builder consumes that value. `NativeGeometry` resolves separate
four/five-input signatures at its existing upload boundary: the three-layer VS
requires an actual TexCoord2, never a substituted attribute. The shared native
shader implementation consumes three textures and three independently owned
samplers, plus the existing mono sun shadow. The explicit object/instance GPU
records are208/816 bytes; structure-offset assertions and upload placement use
that actual ABI, not translated constants or per-draw register files.

Scene family admission reuses opaque caster participation, then checks every
authored sibling before pose fallback. Technique0/phase0, known rigid inputs,
zero-to-three layers, ordinary nonreflective/non-normal-mapped materials qualify.
Unknown owners/resources after admission refuse; they cannot select a legacy
warm-up. Whole-node CPU plans and pipeline/image/sampler preflight finish before
the first sibling enters the shared queue. The material-ID restriction and
primitive0-only scene producer are removed. Native batching retains all active
images and sampler identities through the existing recording-slot fence.

Inactive image slots legally bind the already-owned shadow image but are never
sampled; this does not count as a material image. Ready/admission checks require
each active layer's own image. No extra white-image allocation or template fallback.
Selected-asset source/GPU reload counters remain isolated; wider scene submissions,
layered submissions, aggregate emissions and retirements have separate counters.
Those counters are instrumentation, **not fresh game evidence**. A future runtime
gate must distinguish actual layered/multi-primitive emission, not infer it from
unrelated submission/aggregate counters alone.

## Verification completed

-311 artifact-free Python source/scenario checks pass (0.138 s initially).
- Material33/PID31468 builds17 steps; CPU31/PID23768 passes0.12 s (CTest0.13 s).
  Covers independent UV lifetime after source destruction, override/reset order,
  explicit0..3 layer ABI and invalid layer/finite-input refusal.
- Output31/PID30772 builds3 steps; CPU14/PID7612 passes0.34 s (CTest0.36 s).
  Covers named fifth-attribute indices/extents/duplicates, missing active images
  and samplers, source/GPU image leases, whole-node classification, unsupported
  siblings, pre-pose/legacy hard-off, and batching across all retained layers.
- GPU27/PID26616 compiles all four production SPIR-V programs and the existing
  fixture. GPU28/PID26900 rebuilds the fixture to match real inactive descriptors.
  Final rigid07/PID26432 passes all12 cases in1.08 s (CTest1.09 s), RTX3060,
  two8x8 eyes/128 pixels per case. Maximum RGBA error3.39895e-5, tolerance.003.
  Zero Vulkan validation errors/warnings; one unrelated GOG loader diagnostic.
  Cases0..4 retain prior lighting/fog/shadow/instancing coverage;5/6 add two/three
  layers;7/8/9 exercise negative U separately for each layer;10 is untextured;
  11 is a two-instance three-layer draw with distinct transforms, UVs and lighting.
  Scalar texture/addressing/alpha oracle is independent of the production shader.
  Actual indirect scene/caster commands, nonzero structured-buffer/command offsets,
  per-eye colour/depth and caster depth are checked. Readbacks stay in memory.
- The actual host commands/PCH were reused with `-fsyntax-only`, removing all
  object/dependency output flags. All six real consumers pass: native_rigid_draw
  PID25168, material_texture_bridge23732, native_mesh29284, instance_bridge31428,
  host_walk24140, draw_queue31048. These are not host-link/runtime qualification.

Fixture exe SHA256:
`ACF89F7C69B61D2A5A93F8FBDAB23E5D7A086B0011F824416ADC5FFEC94220E9`.
No game/guest build or game launch. Host107 remains48,622,080 B, SHA256
`663715CBFD7E69C378A7F50C8F219149CAAE2899CAD128D9001ECA48A5F6757A`;
PDB109,068,288 B. Exact owner-profile SHA256 remains
`2F1BC38D763A1B7BDBA31F560684FD4AA7E42A714600B8D344F19DA7F38E23B0`.

## Pending acceptance and storage

Host integration and representative live scene/reload pixels remain required.
Run945 is still the last accepted game result, not evidence for these new scene
materials. Run941's strict dirty-bit failure remains unexplained and preserved.
Object/tree/pass/receiver-visibility adapters, inherited light bindings, explicit
game per-eye production, all other material/character/effect/UI/frame ownership
and the full desktop gate remain open. No speedup, whole scene, full host frame,
full-game stereo or Quest claim. Normal acceptance switches remain off.

Reuse the cumulative ledger in20260906_0333_native-scene-state-bridge.md: the
original3 GiB exception/floor is unchanged. The192 MiB host integration estimate
does not fit; a requested4 GiB allowance is pending, not approved. Small fixtures
ran sequentially under32/64 MiB caps, tests/syntax under8/16 MiB; no new tree,
raw/image/cook/perf output. The first wrapper parameter attempt failed before
launch/output creation; the missing ignored-wrapper edit was then applied.

Deleted28 exact superseded/empty fixture logs,15,568 logical B. The first eight
reclaimed12,288 measured B. The second cleanup's PowerShell table formatting hid
its before/after values; no additional physical savings are claimed. These logs
are reproducible; final CPU/GPU logs replace their verification purpose. Keep
945,940/941 and all protected captures/game/profile/build data unchanged.

Known fixture/log net growth806,314 B: material+29,859 (8,159,345 B/41files),
texture+678,570 (70,274,593 B/129files), GPU+97,780 (10,585,571 B/10files),
logs+105 (191,725 B/136files). The new layered shader header adds50,885 B;
all four current shader headers total326,650 B; other header/CMake/source/Git
deltas are unallocated. First free62,550,061,056 ->post-cleanup62,608,248,832 B:
58,187,776 B drive-wide gain,58.31 GiB free, **not cleanup credit**. Only98,250,752 B
remain above the original floor. Recheck before further producers; no jobs live.
