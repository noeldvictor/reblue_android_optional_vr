# Direct native water material consumption

2026-09-08. Parent source4c23030 plus this bundle; Plume191c31c unchanged.
This removes a live rendering dependency, not the full game's remaining guest
renderer. Desktop mono main-view water is the connected route; full desktop
scene/event/art/both-eye acceptance still precedes Quest work.

## Delivered ownership boundary

The mixed deferred consumer calls `NativeWaterMaterialScope::Draw` before
`bridge.Material(36)`. Accepted entries bypass that begin and its paired end,
the lights/shader model participants, the water resource callbacks and translated
shader selection. The existing water writer supplies copied semantic values and
image leases to `SubmitNativeWaterScenePackets`; no new renderer, queue, asset
cache or registry was introduced.

`ConsumeNativeWaterMaterial` fixes the sequence: commit owned lights once,
export source model flags, perform the complete ordered water writer, publish
completed values/images and refresh neighboring visual inputs, submit the native
draw, export end-of-entry depth intent, then unbind compatibility slots7/12 and
optional13. Later parameter writes cannot be overwritten by another light commit.
Model active bytes/resource storage stay idle rather than simulating callbacks.
Native image/geometry/value leases survive cleanup until the shared queue retires.

Preflight checks the actual known model/visual registry, idle model/resource state,
regular shader-only model technique and material/stack inputs. Owned light recipes
come from the existing instance/node/frame handoff. Refusal before participation
can leave a compatibility entry; after the first light commit, an invalid writer,
missing final input or failed native submission is terminal, not callback replay.
Live source inputs are rechecked after the light export; the final water values
are read only after factor/clamp/parameter/image/snapshot effects.

## Source provenance and retained adapters

- `sub_8218B310`, generated/reblue_recomp.55.cpp:4018: phase3 model participation
  publishes selected lights then flushes its two descriptors, unless its original
  visual/node cache skips. The existing native light mirror invalidates original
  selection/node caches so following unmigrated consumers must refresh.
- `sub_82174270`, generated/reblue_recomp.64.cpp:3551: phase3 techniques2,4..12,14
  only store input flags at engine+16 then select a translated shader. Other
  techniques have additional writes/suppression and are not assumed equivalent.
- `sub_82286228`, generated/reblue_recomp.90.cpp:9525, and `bdInitDefaultTextures`,
  generated/reblue_recomp.1.cpp:10197: select/bind VS/PS recipes, despite the latter's
  misleading name. They do not reset texture slots. The native water program needs
  neither operation; following compatibility shader selections retain truthful
  caches because this route does not pretend to bind a translated water shader.
- `sub_8221DB00` and the active native effect-lifecycle replacement establish model
  participant/resource ordering. `sub_824548A8`, generated/reblue_recomp.62.cpp:20014,
  only clears slots7/12 and, when the late signed material4700 value is positive,13.
- Existing `PrepareWaterMaterial`, checked descriptor adapters, texture/native-image
  owners, light mirror and native draw queue/backend are reused. Original generated
  source and hook TOML are unchanged; generated files are not edited/committed.

Still temporary: source sorted-entry construction/import, visual callbacks,
per-entry sampler/world/state calls, descriptor/flag/state exports, authored feature
reads, image getters and snapshot scheduling. Other material/view/geometry cases
are not automatically admitted. The thread-local callback publication adapter also
remains to be retired as the next producer/visual boundary is consolidated.

## Verification

No shader/backend changes: reuse GPU67/water14's20 two-eye cases and rigid23's55 /
snapshot22's8 cases (validation0/0). Those are controlled GPU tests, not new game
pixel or stereo acceptance. No fresh image/HDR inspection is claimed.

- `host_deferred_test` build6/PID36620 and `deferred_cpu`1/PID37200: exit0; test0.03s.
  The production lifecycle has exact light/flag/material/output/draw/depth/cleanup
  ordering, one light commit, late snapshot-enable writes, no side effects on an
  unavailable light commit and no replay/cleanup continuation after injected
  failure at each of15 stages. Contract tests cover techniques0..31/UINT_MAX,
  model-active/visual-active byte separation, occupied resource and changed methods.
- `python -B tools/host_checks.py --all-boundaries`:376 tests, exit0. Architecture
  guards require native draw before old material dispatch and no second light
  resolve/commit in submission. These are not behavioral GPU evidence.
- Host145/PID35064/session91407: exit0, incremental `reblue` target. Codegen0 files
  written/deleted; no guest object compilation. Existing Vulkan-only/OpenXR tree.
- Run973/PID32068/session54385: exit0,08:09:16..08:11:24Eastern;180s/800KiB/192MiB
  bounds. Full strict mixed/deferred-effect/input/material/geometry/light/receiver/
  caster/cutout/hard-off/cold/reload scenario passes. All22 profile settings audited,
  captures/perf/dumps off,0 raw files, original116-byte profile exactly restored.
- Rigid regression: generation93/instance144 ->207/384;900 fresh scene and shadow
  emissions per interactive epoch, with old-generation retirement before reload.
- Water separately: generation94/instance145 ->208/382. At new-generation frame3048,
  submitted1659, emitted1617, culled41, retired1658: all preceding water packets
  retired before this first new submission. Last sample4548:3159 submitted,
  3071 emitted,86 culled,3157 retired;3159 balanced direct material begin/end pairs
  and candidates/consumed3159/3159, unavailable0.
- Fresh event0 cold1551..1851:300 new direct begins/ends/submissions/retirements,
  293 emissions and7 culls. Reload3948..4248:300 of each,300 emissions,0 culls.
  Counts are repeated runtime draws, not unique assets or a speed measurement.
- Mixed neighboring gate4200..4500:10250 ordinary native packets consumed, pending0,
  5412 legacy draws,10824 material bridges,3176 balanced native visual scopes,
  300 writer-ordered input batches/refreshes. Strict source comparisons remain on.
- Reflection publications3900, compatibility/faults0;10 initial camera misses remain
  unchanged after startup. No bottom/snapshot call observed. Neither those authored
  image roles nor broader scenes/stereo/art parity are qualified by this run.

## Artifact identities and storage

SHA256:

| Artifact | Hash |
| --- | --- |
| Host145 EXE,48,958,464B | `F2B035AFD85F4FA19EB4ED9A7B24B6338B07DDC911C28E33B22334093BFF06F0` |
| Host145 PDB,111,083,520B | `35E6DC7D48D4DCAB703609DD0B8CFE022FCA6BDED7C732994A4D80B9590E9808` |
| Deferred CPU6 EXE | `EB3A2A5CE4883E2C9E7075B320553D8DC567EC3DADB42BD83C07A29923C34E3A` |
| Run973 log,529,864B | `A16389CD9C23253DC781B47D52DAF742449A3E6356CFA8C7050FB57C2C0363B3` |
| Restored owner profile | `2F1BC38D763A1B7BDBA31F560684FD4AA7E42A714600B8D344F19DA7F38E23B0` |

Same cumulative ledger in `20260906_0333_native-scene-state-bridge.md`;3GiB
exception,20GiB reserve and all diagnostic/log/raw/image gates unchanged. No new
assets, downloads, build trees, raw/images or archived binaries. After replacement
passed, retired host143/144 and deferred CPU5 stdout/stderr (6files,3958B), plus
hash-verified run972541835B. Seven files545793B logical; free-space interval
80,142,864,384 ->80,143,417,344B gains552960B. Old full text retired; dated reports
preserve outcomes/hashes,973 is the strict replacement. All unresolved failures,
protected raw/images, historical zip, original game assets and profiles remain.

Selected retained growth96373B: CPU tree+78040B, EXE/PDB+30208B, attachment logs+96B,
runtime replacement-11971B; GPU tree unchanged. Other host objects/metadata/Git/system
activity are separate. First free80,140,918,784B, post-cleanup80,143,417,344B (~74.64GiB),
2498560B drive-wide gain, not all cleanup savings. No fresh cache/perf files found
under the run's scoped output directories. All producers terminal.
