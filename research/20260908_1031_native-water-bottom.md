# Native water-bottom ownership and layered clears - 2026-09-08

Implementation/test source: parent f22e607 dirty; Plume
191c31c0da6e32f891bb0b2f9d6c80a2acabd9ef (pushed before the parent gitlink).
This records the completed implementation session. The subsequent commit/push
request reuses its evidence; no additional build or game run is needed.

## Boundary and remaining adapters

Whole-function begin/end replacements for sub_82187878/sub_82187A00 reuse
HostTargetAcquireNative, NativeSceneCommands, the existing framebuffer/fence
store, native pass nesting and typed NativeImageLease publication. The named
256x256 mono D32_FLOAT_S8_UINT slot is distinct from the sun shadow slot and is
never selected by dimension classification. Eligible execution no longer creates
a console depth surface, invokes D3D clear/resolve, or destroys that surface.
Ordered draws and pending far-depth clears complete before a shader-read barrier;
there is no image copy. Temporary output/resource headers remain adapters.

The completed receipt retains the actual producing camera's world_to_clip and
the depth sampling view. Wrong frame/view, missing/nonfinite camera, pending
clear and rewritten image layout refuse the receipt. Native pass discovery uses
the exact physical attachment pair and nesting. Compatibility begin/end remain
paired when eligibility fails; a failure after native side effects cannot replay
the old lifecycle.

Source contracts recovered from translated C++ (not modified):

- sub_82179528 creates bottom output+12 and VS40 descriptor+148; the global owner
  is `(uint32_t(-32137)<<16)+28452`.
- sub_821795D8 fits the authored top-down camera when owner+8 is enabled, writing
  view+16/projection+80. This guest fit remains. bdBuildViewMatrix publishes its
  result through the existing native camera producer.
- sub_82187878 creates source+36 depth, clears depth/stencil, disables colour
  writes, establishes that camera and frustum. sub_82187A00 resolves depth to
  global output+12, restores colour writes and releases the surface.
- sub_82179A68 still flushes the later descriptor and binds output+12 at slot9
  in phase3/5, including inline linear-filter writes outside native slots0..4.
  sub_82179B68 unbinds it. These sampling adapters are not removed here.

## Causal backend regression

NativeWaterImages now requires actual D32 bottom depth, not the prior fixture's
HDR colour placeholder. GPU water10 failed its unchanged mode4 oracle at eye0,
x1/y1/channel0: actual0.125 versus expected0.0871406, with validation0/0.
Plume held clears by texture alone: switching from a full multiview depth
framebuffer to individual layer views could merge a later layer's clear over an
earlier layer, or consume another framebuffer's pending clear.

Plume191c31c flushes a conflicting pending clear through its original framebuffer
before switching overlapping targets; merging and render-pass consumption now
require matching framebuffer ownership. No shader, oracle or threshold was
relaxed. The production fixture deliberately uses full-layer deferred clears,
not an explicit-rectangle workaround. New empty-bottom mode14 samples the native
far clear; distinct per-eye written depths remain covered by mode4.

Water10 failure log SHA256:
BDB9521DCFD8FC7D1C708E98F6419DD8D674519127FCD164523C781C57AC34D2.
An earlier GPU62 compilation failed converting an ambiguous empty handle in a
braced array; explicit NativeTargetImageHandle fixed both call sites. That log:
0428EFFC4DA3BDBEB71E107AB75B0EDBD8F86F1D01439505F40C96B548918F5A.
Both failure logs remain retained at this handoff. No claim about historical
game flicker/tree gaps follows from this focused backend fix.

## Verification

- CPU build58/PID35100 and output34/PID31948 pass; output test0.49s. Tests cover
  clear/transition/no-copy behavior, frame/view/matrix qualification, shared
  layouts, owned sampling views and framebuffer/image retirement.
- GPU build64/PID35784 passes. Water11/PID35692:17 two-eye cases,1.13s;
  rigid21/PID34168:55 cases,1.22s; snapshot20/PID34676:8 mono/stereo MSAA cases,
  1.04s. All report Vulkan validation0 errors/0 warnings, one known missing GOG
  overlay manifest loader notice, and zero retained raw bytes. The fixture
  retains producer command framebuffers until their fence; queued image leases
  cannot permit snapshot-pool overwrite. It is not a game queue-admission test.
- 372 artifact-free Python source/scenario checks passed during implementation.
- Host140/PID31868 and final141/PID31140 pass. Codegen0 writes/deletions; no guest
  objects rebuilt. Host140 compiles the new bridge;141 relinks the Plume fix.
- Host141/run970/PID23820 terminal0, 2026-09-08 10:23:35..10:25:41 UTC. All22
  temporary settings were audited. Existing strict receiver/light/caster/cutout/
  native-input/effects checks pass in cold and reloaded bg41_01 windows. Old
  generation93/instance144 retires; generation207/instance384 produces900 fresh
  scene and shadow emissions in each epoch. No native-water-bottom report or
  bottom-target allocation is observed: the new lifecycle is NOT live-qualified.
  The116B original profile was restored byte-for-byte. No new game pixels,
  full-frame stereo, performance measurement or Quest test was produced.

| Artifact | Bytes | SHA256 |
| --- | ---: | --- |
| host141 reblue_vk.exe | 48,897,024 | 222369FEF7AC62395CA8A6A761386142F513352E70B386DF043F8EF90BF672F2 |
| host141 reblue_vk.pdb | 110,755,840 | 00090927E7535DC7B7F34145C8AA32970C56A7402C8B373DD8095CCCC5613951 |
| GPU64 native_scene_snapshot_test.exe | 1,135,104 | D312AD5CEA9EBDFE70FDE1D367D29BA58D7091CE29102010662800E601671D6C |
| CPU58 host_post_output_test.exe | 1,098,752 | 813A193305DC94839A751D45F8DFE4AE677872EA61367881C62D73241A2C6D9D |
| reblue_970.log | 496,521 | 05F6D5C71F057D1C1888DCACAC8FCB9DD27D195B03777812E91E2A6888139B6E |

Restored profile SHA256:
2F1BC38D763A1B7BDBA31F560684FD4AA7E42A714600B8D344F19DA7F38E23B0.

## Next connection, not another unchanged boot

SubmitNativeWaterScenePackets still has no game caller. Establish an authored
bottom-active scenario or its scheduling gate before another bottom probe.
Publish completed per-primitive material values after the ordered resource/light
writer into native instance/generation owners, with bottom depth/projection,
bump slot12, environment slot5, planar reflection, snapshot and shadow leases.
Late sampler writes, live pre-cull wave bounds and cutout/ATOC policy still need
connection. Do not freeze late values, gather translated draw constants, infer
an owner from a texture slot, or weaken unknown-writer refusal. Preserve the
existing queue, image stores, fences and material/update helpers. Camera fitting,
caster/sampling adapters, all authored controls and game sequence/both-eye
acceptance remain open; no fully host-owned frame or measured speedup is claimed.

## Storage

The first run970 preflight refused before profile changes or launch because the
existing diagnostic cap lacked room. Nineteen explicitly named completed runtime
logs (817,818,822,828,845,850,852,860,861,865,868,879,886,887,888,905,907,908,911)
were losslessly archived, with every entry's length/SHA verified before deleting
its source plaintext. Total4,961,074B ->598,041B, net4,363,033B reclaimed.
Contents are recoverable from ignored
out/build/win-amd64-release/logs/retained-native-runtime-817-911.zip, SHA256
D7215B13DB852D8912B864ACA38DFB1510AE0A05D358681A5D2EED7C7F3FDBFD.
No originals, profiles, assets, captures, unresolved evidence or active build
trees were removed. Cumulative accounting remains in the existing ledger;
budgets did not reset. No build/run/capture was repeated for the commit request.
