# Native water material and GPU consumer

2026-09-08, EDT. Previous implementation goal turn made progress: source-free
visual identities and writer-ordered publications passed cold/reload in run969.
The intervening commit/push-only turn verified origin/main at ad8e85e; it made
no renderer progress. This continuation adds a native water shader consumer and
production GPU tests. The full host-renderer goal remains active and incomplete.

## Delivered and not delivered

`NativeWaterMaterial` contains named, copied animation, surface, reflection,
shoreline and highlight properties. `NativeWaterInstanceGPU` is a 976-byte explicit
layout, with world/normal transforms, existing native light/fog pass data, two
bottom-projection matrices and explicit mono/per-eye image-layer choices. It has
no source addresses, descriptor indices, translated constants or register masks.
The packer rejects invalid affine transforms, nonfinite values, bad light/fog
inputs, invalid modes and layer counts. The program uses `NativePipelineProgram`
and `NativeVertexInputLibrary`, not a second renderer or draw-replay cache.

The water VS/PS consume Position/Normal/UV/Color/Tangent float4 mesh semantics,
instanced storage and six sampled images: bump, planar reflection, scene snapshot,
bottom depth, environment cube and primary shadow. Mono2D inputs use explicit
array views; planar/snapshot/bottom may select each eye. Cameras and bottom
projection are per-eye values, not a final translated-vertex stereo skew.
Lighting and fog reuse existing native semantic arithmetic. Source-over blending
and independent depth-write policy remain pipeline decisions, not shader hacks.

This program builds into host135 and is executed by the existing real-Vulkan
fixture. **No live water draw is migrated yet.** The source importer is not
installed in a generation/frame-bounded publication; the model/instance producer,
ordered queue, image leases and fence retirement have not been connected to it.
No water admission/fallback rule, resource-writer refusal or game callback changed.
The prior host134/run969 remains the latest live evidence, not host135 runtime
qualification. No fully native scene/frame or speedup is established here.

## Reused source and decisions

Read the guest-source/devloop skills, linked disk policy and relevant hook TOMLs.
Reused the recorded water writer82454720/cleanup824548A8 contract; did not repeat
the whole callback census. Read the complete translated setup
`mcl__water_object__vf04_body` in `generated/reblue_recomp.13.cpp:20896` through its
return at technique5 assignment, and the existing whole host update/setup bridges.
The model ownership-frontier report was read for the upcoming geometry boundary.

Read existing translated shader dump bodies, not a new extraction:
`out/build/win-amd64-release/hlsl_dump/bd_water_vs.hlsl` and `bd_water_ps.hlsl`.
Their identities are 55EF71EE895181B9 and E89EEDF529DDB38D in the checked-in shader
table. Technique5 normal and shadow-view variants are listed separately; the
new native shader does not imply the shadow-view variant is connected/qualified.

The setup body establishes these temporary parameter bindings. Source offsets
and descriptors remain exclusively at the boundary; the new native API does not
expose the old VS/PS50..52 blocks.

| Native meaning | Authored member | Completed destination descriptor |
| --- | --- | --- |
| UV scroll U/V, scale, animation phase |4660/4664/4668/4672|4768/4788/4808/4828|
| RGBA tint |4676..4688|4848 vector|
| Wave amplitude/speed/direction/frequency |4724/4728/4732/4736|4868/4888/4908/4928|
| Reflection distortion, distance fade |4692/4696|4960/4980|
| Refraction mode/distortion |4700/4704|5000/5020|
| Effective reflection choice |4708, phase/settings-dependent|5040|
| Normal blend, highlight exponent/intensity |4712/4716/4720|5060/5080/5100|
| Bottom enabled, opacity scale, brightness/gain |4740/4744/4748/4752|5120/5140/5160/5180|

`ReadNativeWaterMaterial` copies the completed destinations after setup writes
and clamping, not the initial authored fields. It uses checked boundary reads
and accepts no partial result when a final destination is missing. Aliased final
values are re-read while retained copies stay unchanged. Full per-writer frame/
generation ownership must be added before game consumption; this helper alone
does not remove the descriptor dependency from live drawing.

Native material/deformation changes are deliberate, not an exact shader port:
two directional sine waves replace the translated angular-polynomial deformation;
authored phase/speed/direction/frequency/amplitude and vertex red influence it.
The refraction result explicitly includes its sampled background, with opaque
base alpha before highlights, instead of relying on inherited blend arithmetic.
Reflection-none emits no reflection sample. A native four-tap sun filter and
optional material cel mode are present. The owner's permission covers material/
geometry changes, but recognizable art and authored sequences still need actual
game review. The cel branch, all authored controls, wave displacement bounds and
water shadow-view variants are **not qualified by these fixtures**.

## Verification and causal corrections

GPU tree reused: `out/build/win-amd64-release/tools/native_scene_snapshot_test`.
No additional harness/build tree, shader translator regeneration, game extraction,
game launch, profile override, raw capture or image export.

Final fixture build57/PID33388 terminal0. Water5/PID31016 terminal0, 1.26s:
16 real two-eye cases (modes0..5,8..13, plus two phases each of6/7). Production
native program/input leases survive mesh input retirement; nonzero firstInstance
selects two distinct instances. Fixed-source pixel/depth oracles check material
and eye association. The native scene command owner and `CopySceneSnapshot`
create the actual refraction image; later live scene clears must not overwrite
that retained sample. Animation cases require changed pixels in each eye, not
just finite output. Lighting cases use the shared CPU arithmetic with separate
authored fixture inputs; they are not independent original-shader parity proofs.

Cases cover no/planar/cube reflection, scene refraction, bottom opacity, primary
shadow, directional/spot/point and per-instance secondary lighting, two fog
volumes, separate-alpha source-over and independent depth writes. Both the new
water cases and the existing 55 rigid cases (rigid19/PID29656, 1.24s) report
validation errors0/warnings0. The missing GOG overlay manifest is one loader
diagnostic per suite, not a validation warning or a new installed dependency.
Four new architecture guards pass; 368 total Python checks pass separately.

Initial failures were in this new fixture, not hidden or relabeled passes:

- Water1/PID35244: native scene creation refused generic RGBA32/D32 test images.
  Corrected the fixture to production HDR16/D32-S8; strict owner checks unchanged.
  Failed stdout SHA256
  `BB605ED42B4906519ADDB235F54DA77E9D14E3F14AE5C66C77431F0BE8797D48`.
- Water2/PID33984: refraction blue.503418 vs unquantized.504. The sampled clear
  colours were not exact binary16. Fixed-source route tests now use exactly
  representable clear colours; the original4e-4 low-colour bound stayed unchanged.
  Failed stdout SHA256
  `3B52651059BC696D7895A08BFA4189BE407D5A63CC94FC35A7A51187141D222E`.
- Water4/PID21652: a newly added high-alpha lighting case.89013671875 vs.890577,
  within the correct2^-11 binary16 quantum. The new lighting oracle now checks
  exactly the two bracketing half representations, rejecting their outer
  neighbours in a CPU regression. Existing low-colour checks stayed unchanged.
  Failed stdout SHA256
  `6A0D84A3B3B41CE9AAC9507CC905DAC1418CA3CE4A299D08012A45D2C931E76B`.
- Build56/PID33752 failed on Windows' `max` macro. Parenthesized `std::max`;
  build57 passes. Failed stdout SHA256
  `A8358DE58E120724F4875CBEF9051C68BC665CB095676BD1F2A41F35813FE138`.

Host135/PID32596 terminal0 in9.52s. Codegen reports0written/one module current;
no guest object compilation. New water program plus normal build-stamp consumers
link; source base isad8e85e with this bundle dirty at build time.

| Artifact | Bytes | SHA256 |
| --- | ---: | --- |
| Host135 reblue_vk.exe |48,858,112|14DB8A767EE7BDDD8C9A500D026EC5CA783034071A19F55168514E4CFEB106FE|
| Host135 PDB |110,473,216|75F8C86A8787A922AED894EBA599E77B4785AE24366B6BB0B78E651916DABB88|
| GPU fixture57 executable |1,072,640|E2BB131334E14A5FC0045DA147FC3A750FD2E0A16D9ABDB9F16E0AF57CB7431F|

No image was exported for manual art review. Synthetic readback checks are not
a game screenshot, sequence, mixed-family reload or full desktop stereo gate.

## Next connected work

Install completed semantic water values in the existing instance/generation owner
at the ordered writer boundary. Retain the actual native bump/cube/planar/bottom/
snapshot images and model geometry through queue/fence consumption. Extend native
geometry with the validated tangent layout and displacement-aware bounds. Route
water through the existing ordered scene queue/program binding path; remove its
per-entry legacy material/resource execution only when that whole connection is
valid. Preserve late writes, visual transitions, snapshots, blend/depth and
unknown-writer refusal. Then require fresh water consumption/retirement, mixed
cold/title/reload and authored pixel/sequence/both-eye evidence. No Quest work yet.

Storage/cleanup is recorded in the existing cumulative scene-state ledger, not
a new budget. Per-image110KiB and aggregate/raw limits remain unchanged. The
unanswered larger-image allowance is not assumed. Owner profile remains116B,
SHA256`2F1BC38D763A1B7BDBA31F560684FD4AA7E42A714600B8D344F19DA7F38E23B0`.
