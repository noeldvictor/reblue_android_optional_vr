# Native planar reflection ownership - 2026-09-08

Runtime implementation: parent a5d66b91857d55561b29dc0e7902913b99ceb2cb.
This continuation adds production-representation GPU coverage; host142 was built
with those test-only worktree changes (dirty version stamp). Plume remains
191c31c0da6e32f891bb0b2f9d6c80a2acabd9ef. No generated source was changed.
The preceding goal work made progress by installing and pushing the connection;
this checkpoint supplies its first build, behavior and live-lifecycle evidence.

## Removed interface and reused owners

Whole-function hooks sub_821875F8/sub_821877C8 replace eligible planar-reflection
attachment creation, pass entry/clear/completion, console resolve and attachment
release. Colour comes from an exclusive NativePostImagePool HDR16 lease; depth
uses the explicit native ReflectionDepth slot. NativeSceneFramebufferStore and
NativeSceneCommands now accept that colour lease with native depth, retain the
attachments through fences and publish the exact producing image/view/layout.
No colour copy or emulated resolve is needed. A persistent shared reflection
colour slot would allow a later plane to overwrite an earlier retained output;
the existing exclusive pool prevents this without a second production cache.

The resource header is only an adapter for remaining binding consumers. It does
not allocate a GPU image or enter SurfacePool. The normal resource-adapter
release path, native framebuffer store and pool retain their existing fence
responsibilities. Active-pass discovery checks physical attachments and nesting;
the existing native scene snapshot core can discover reflection scopes too.

Eligibility refusal precedes effects; faults after native side effects cannot
replay the original lifecycle. Empty passes consume their native colour/depth
clear before shader-read publication. This changes the reflection target from
the original colour format to HDR16: visual/art parity is not inferred.

Recovered contracts reused from the prior source work:

- generated/reblue_recomp.14.cpp, sub_821875F8: source+28/+36 attachments,
  source+40 plane, source+12 camera, authored clear and camera setup/order.
- generated/reblue_recomp.55.cpp, sub_821877C8: output plane+12, extent+168/+172,
  resolve/publication then restored pass and released attachments.
- generated/reblue_recomp.9.cpp, bdPlaneReflectUpdateTexture: scale-driven
  dimensions/getter replacement. Its existing instruction-site resolution hook
  is in config/hooks/render_tweaks.toml. That helper still executes here.

Authored camera calculation, extent/getter update, state/receiver exports,
scheduling and legacy draws remain adapters. The game still does not call
SubmitNativeWaterScenePackets. No full host frame or speedup is claimed.

## Tests and live observation

- CPU59/PID35632 builds; output35/PID32268 passes in0.47s. Reflection tests cover
  mono/two-layer clear ordering, exact output identity, no copy, invalid leases,
  retained-reader overwrite refusal and framebuffer/image fence retirement.
  Existing scene commands include1/2/4/8-sample regressions; MSAA write images
  remain writable even though they are not valid single-sample leases.
- GPU65/PID24556 builds. Water12/PID17760 passes18 two-eye cases in1.15s. The
  fixture now renders the planar input through the native leased-colour scope,
  transitions it with FinishNativeReflection and samples that same retained
  owner in the real indexed indirect water shader. Distinct per-eye colours,
  snapshot independence and empty-reflection mode15 have fixed pixel oracles.
  Producer framebuffer and queued water lifetimes are checked at their fences.
  It does not execute the game's mixed queue or reflected-object draw scheduler.
- Rigid22/PID32140 passes55 cases in1.22s; snapshot21/PID37028 passes8 mono/stereo
  MSAA cases in0.51s. All three GPU groups report validation0 errors/0 warnings,
  one known missing GOG overlay manifest loader notice, and zero raw output.
- 373 artifact-free Python source/scenario checks pass. The earlier cache-key
  guard failure was an obsolete exact source expression: the new key adds the
  colour owner while preserving source attachments and density-map identity.
  The guard now requires all three and colour lifetime before framebuffer death.
- Host142/PID32328/session1353 links successfully. Codegen0 writes/deletions;
  no guest objects rebuilt. All build/test producers are terminal0.
- Run971/PID37076/session32898,11:05:57..11:08:03 UTC, passes the existing strict
  mixed cold/title/reload checks. All22 temporary settings took effect. Old
  generation93/instance144 retires before new207/384; both interactive epochs
  have900 fresh native scene and shadow emissions. Reloaded mixed-consumer
  window4200..4500 consumes11,200 native packets, pending0,300 input batches,
  978 visual publications and158 completed-writer refreshes. Pixels are separate.

The live reflection depth target is480x270, one layer/sample. Last reflection
sample: begins3901, ends/publications3900, active1 (sample taken during begin),
compatibility0/0, empty clears743, owned cameras3892, missing9, faults0.
After observed field readiness, cold log times07:06:51.039..07:06:57.610 add300
publications; reloaded07:07:51.517..07:07:57.873 add300. Both also add300 cameras,
with no new missing camera, empty clear, fallback or fault. Log times are local
Eastern; the first nine camera misses occur before the first interactive field
window and their cause is not established by this run. Last pool sample has
13 allocations/8 retired/5 resident and36,288,000 payload bytes, no refusals/failures.

There is no native-scene-snapshot report: reflection-phase refraction snapshot
execution was not observed. No new game image/sequence was captured or inspected,
so this is live lifecycle/regression evidence, not reflection pixel or complete
stereo qualification. Existing tree-gap/UV/dirty-state failures remain open.
Original116B profile was restored byte-for-byte; the renderer is terminal.

| Artifact | Bytes | SHA256 |
| --- | ---: | --- |
| host142 reblue_vk.exe | 48,918,528 | C1B7A8F7E1CC3841B0054605CC2EC70FBA14E91E04876B2E5D8B86F1F17FBF2F |
| host142 reblue_vk.pdb | 110,886,912 | 9A2A3F642ED5029ABB198F8D9F9E91B0A04ED2DFE9D35C14D20566ABAFF210E9 |
| GPU65 native_scene_snapshot_test.exe | 1,151,488 | 3A930B48B21F684432C5B1D6C1A14C7F1FE947AA1BB1982F728C3C3B11AFC9A9 |
| CPU59 host_post_output_test.exe | 1,121,280 | C65CC2609688A610E703384EEBED06186496EBA33EC49F322D5D7FB1449BAAEC |
| reblue_971.log | 524,518 | 190B7A90927AB8629ED0429ACC1AFC03D63DAE620285D7CBBD5390D22DAF6434 |

Restored profile SHA256:
2F1BC38D763A1B7BDBA31F560684FD4AA7E42A714600B8D344F19DA7F38E23B0.

## Storage and next connection

The cumulative ledger remains research/20260906_0333_native-scene-state-bridge.md;
no budget resets, new build trees, shader regeneration, assets or raw/image sets.
32 superseded successful attachment logs (100,547B) were removed after passing
replacements. Two hash-verified prior successful mixed-runtime logs969/970
(1,008,228B) were removed after971 passed the same strict chain. Their reports
and hashes remain; exact old plaintext is retired, while971 is retained. All
unresolved failure evidence and the older lossless817..911 archive remain.

Total removed34 files/1,108,775B logical, with1,134,592B immediate drive interval
gains across three cleanup operations, credited once. Selected CPU/GPU/products/
logs/runtime retention grows75,311B net. Ending measured free80,174,153,728B
(~74.67GiB),2,199,552B below the first reading; drive-wide activity also includes
other host objects/metadata/source/Git/system activity. No unrelated gain claimed.

Next: connect completed water material/image/sampler values to native instance
identity at the ordered writer and call the existing water queue from the mixed
consumer. Use this reflection lease and the existing bottom/snapshot owners;
do not rebuild them or keep rewriting individual console helpers. Authored
bottom-active/refraction scenes, initial camera misses and reflection pixels
remain verification work. Full desktop scene/event/reload/both-eye acceptance
still precedes any Quest work.
