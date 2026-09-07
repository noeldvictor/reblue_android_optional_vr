# Owned primitive shader inputs

2026-09-07, parent04490b1 plus the source/test changes in this checkpoint.

## Outcome and boundary

The selected rigid object's texture-layer count, vertex-color enable and
declaration bone count are load-owned values, retained by the native primitive
packet after model/source retirement. Ordinary scene replay now packs its three
texture-enable bits and vertex-color bit from those values, not captured bool
values. Its interpreter, template, shader-register ABI and source alias lookup
remain. No native rigid game draw, source-free GPU load, cold-load/reload,
sequence/both-eye completion or measured speedup is claimed.

This removes a concrete material dependency from the direct-object packet.
Other lighting flags, sampler behavior and live pass composition remain; do not
expand this into a generic register import or another renderer framework.

## Source contract

Read all of generated `bdSceneNodeDrawSingle` (reblue_recomp.40.cpp:9901–14217)
and complete `sub_82198138`, `sub_821981E0`, `sub_8228AB40`, `sub_8228AAB0`,
`sub_821739B0` and `sub_821739F0`. No generated code was edited.

- Scene phase0 starts texture mode1. Command0100 modes0..3 select that many
  texture layers. Larger modes leave the enable values unchanged, despite
  updating the compatibility mode/serial. Decode preserves this ordering.
- The selected declaration slot's byte2 is its bone count; halfword8 bit0 is
  vertex-color enable. The checked load-only reader imports both transactionally
  from host-endian boundary words. Missing reads stay unknown. An attribute's
  presence or an omitted0200 bone command cannot stand in for these values.
- Owned fields live in `NativeMaterialRange::shader`, so existing model byte
  accounting includes them. `BuildNativeObjectPrimitive` copies them by value.
  No new disk format, cooker, cache or source index is introduced.
- Current replay packing is restricted to VS`B5C88BB6295138CC` /
  PS`FB83DD3F5E67CEB7`. Capture compares against the original draw, refuses a
  mismatch, and replay changes only VS bit4 / PS bits0..2. Other bits remain
  untouched; unsupported families are not silently converted.

Remaining producer map from the same source read: staging356 (PS diffuse) starts
from visual3052, is affected by0300 and material-control bit0; staging360 (PS
specular) has global gating and0400/control-bit1 ordering. Control commands reset
these to node-entry values when not disabled, so a final control record alone
cannot replace ordered command semantics. Staging340 is normal-map enable,
344 reflection,348 shadow. Visual3440/3444..3456 are UV overrides, not ambient.
The existing NativeLightingPass owns ambient/camera/color/sampling values but
does not yet expose the complete fresh direct-pass packet. Reuse that owner.

## Exact selected shader and UV evidence

Target remains bg41_01 node64, sole primitive, technique0/view3; geometry
`258694267A8DBAEE`, material`63B8D67932573E51`. Its existing17,572 B cooked asset
was neither changed nor recopied.

Run927's two bounded actual-draw observations at05:32:22.694/05:32:24.594 identify
VS`B5C88BB6295138CC`, PS`FB83DD3F5E67CEB7`, VS bools`40000010`, PS bools`046000C1`,
with owned-input match true. These observations precede the post-event windows;
they establish the selected pair, not invariance of every pass flag. In those
observations texture0, diffuse and fog are active; normal mapping, reflection,
specular and shadow receiving are off. Shadow casting is a separate pass.

The matching cached vertex shader is `hlsl_dump/bd_mirror_vs_norm.hlsl`, whose
first line names that exact hash. The checked-in shader-hash table maps both
`bd_mirror_vs_norm.vso` and `bd_normal_vs_norm.vso` to it. XenosRecomp deduplicates
by payload hash; the dump filename alone is not material-family identity.
The CS-normal variant actually has a different hash`BB6AC0229237A4CB`.

Read the complete matching VS main. It computes `(uv+1)/512+offset`, passes
vertex RGBA when enabled, and otherwise supplies white. The selected canonical
UV0 range16383..16895 therefore gives32..33 before the live offset. Preserve it:
do not guess a−32 asset bake. For the native shader's UV scale/offset fields,
the equivalent scale is1/512 and offset is1/512+the authored offset. Sampling
address modes still need their named producer; this does not prove a sampler.
This VS also emits clip position and world-to-light projection for shadow input.

The fresh selected packet at05:32:47.882 owns one layer, vertex color enabled,
and zero declaration bones. Other observed candidates have three layers and
must not be routed through a one-texture shader without an explicit conversion.

## Verification

- Material25/PID28736 and CPU23/PID16220 pass; behavior0.10 s, CTest0.11 s.
  Covers all256 mode values, default/ordered mode inheritance, BE declaration
  fields, null/alignment/overflow/missing reads, transactional refusal, known
  false vs unknown, all four layer counts, untouched compatibility bits, and
  retained packet use after source/model/instance retirement and key reuse.
- 257 Python source/scenario checks pass (0.071 s final check). New parser gate
  requires fresh comparisons and actual owned-input draws in consecutive ready
  windows after an event; missing/stale/reset/wrong-scene/mismatch logs fail.
- Host85/PID25316 passes: host objects/link only; codegen reports all modules
  up-to-date. No guest objects or shaders rebuilt. Exe48,398,848 B,
  SHA256`5862A27E0FC22F32FA9B6B7D7C268992899F691C4359ED80142AC51C2728FB79`.
- Run927/PID31504,05:32:03–05:33:02, all15 temporary settings audited and exact
  owner profile restored. Full existing native image/model/geometry/instance/
  table/pulling/movement/shadow/texture/policy/lit/binding/node/object/light/fog
  gates plus `--primitive-shader`. Raw/perf/dumps/cook/cache persistence off.
- After event frame1415, ready frames2015/2315 add1,332 matching primitive
  shader checks and42,166 owned-input draws; zero mismatches. Existing gates
  include1,332 matching light/fog draw checks,2,400 fog publications,2,664 active
  fog layers,100,326 pose comparisons,1,761,600 bounds comparisons, and38.479884
  units of observed movement. Source-free GPU load count remains0.
- Viewed actual1920x1080 renderer-owned image: running Shu beside fence and
  blue bell/support, terrain, foliage and shadows coherent; known cliff marks
  and distant blur remain. One flat image is not sequence or both-eye acceptance.

Log`out/build/win-amd64-release/logs/reblue_927.log`,231,185 B, SHA256
`055C9DA362C3A6EC6AC1ED9E3AD3DA1E1B3D02E615822776BDC68950AA30294C`.
Image`out/verification/native_primitive_shader_window.jpg`,135,419 B, SHA256
`2ABFA53031E9D44647D2D3B562C6997770B8AF4D34F8A458927E448D0D532AC4`.
Owner profile116 B SHA256
`2F1BC38D763A1B7BDBA31F560684FD4AA7E42A714600B8D344F19DA7F38E23B0`.
Storage uses the existing cumulative ledger in
[native scene state](20260906_0333_native-scene-state-bridge.md); replacement
retention is by verification purpose, not one set per commit.
