# Native water queue ownership connection

2026-09-08, EDT. Source base333ce83, dirty at build. The preceding Git-only
turn was no renderer progress; this continuation changes native mesh load and
the shared scene batch/emitter. Goal remains active, all-rendering scope intact.

## Implemented connection and boundary

`native_mesh.cpp` now resolves the native water tangent signature at load and
derives indexed maximum `abs(COLOR0.r)` from checked little-endian CPU mesh data
before mapping/upload. Signed base vertex, negative/HDR weights and unused
vertices are handled explicitly. Existing content identity, persistence and
geometry arena are reused; no recook or second GPU geometry store.

`native_water_scene.h` defines completed semantic water submissions with owned
geometry, bump/environment native assets and planar/snapshot/bottom/sun native
target leases. Views live in the same owners as images. Explicit dimensions,
layers, formats, sampling layouts and sampler roles are checked; no current
translated slots/descriptors are used as missing-input defaults.

`SubmitNativeWaterScenePackets` preflights the whole submission, reuses the
existing program/PSO cache and native scene scope, and stages into the existing
draw queue/store. It does not import a source object or call legacy material
callbacks. The shared emitter selects the976B water ABI versus816B rigid ABI,
packs CPU values into the existing upload arena, binds six images, records the
indexed indirect command and retains records/bindings until the existing fence
drain. Rigid/cutout counters remain separate. Fixed256-instance/4096-record and
eight-program limits remain; water data is allocated only for water records.

Consecutive compatible water items can coalesce without changing primitive
order. They remain barriers to queue reordering; a different image publication,
either-eye camera, geometry, generation, pipeline or intervening non-native item
breaks the batch. The shader, material semantics and prior pixel oracles are
unchanged. No translated instancing/gather ABI is introduced.

Final queue audit found older opt-in depth/eye-major sorters ignored the native
`reorderable` flag, and legacy depth prepasses were hoisted over the whole queue.
`draw_order.h` now supplies the same run boundaries to both sorters and prepass
scheduling; ordered native items are singleton barriers. Blended gathering also
stops at them. Sorting/prepasses still operate inside each compatible run, not
disabled globally. CPU tests cover stable ties, leading/trailing and adjacent
barriers plus the exact run ends used to schedule prepasses. This also protects
ordered native cutouts; it does not prove a cause/fix for prior game pixel defects.

Bounds include the affine-transformed indexed positions plus world-Y displacement
`1.5 * abs(amplitude) * maximum_abs_red`, with outward rounding/error envelope.
Unknown or unrepresentable displacement refuses rather than culling on stale
rigid bounds. This feeds the existing current-depth visibility mechanism.
`NativeSceneCommands::WritesImage` rejects feedback against all source AND
ordinary MSAA resolve attachments, including depth; this is not emulated EDRAM.

**Not live water conversion:** the game has no caller of the new submission
entry. `host_walk`, material-writer hooks and deferred family admission are
unchanged. The GPU fixture executes the production packer/binder, not the global
`DrawQueueFlush`/`SubmitNativeWaterScenePackets` or real water callbacks. Their
source builds, but reachable live water consumption is still unqualified.

## Verification and causal correction

- 370 artifact-free Python source/scenario checks pass; two string guards were
 updated for the equivalent typed packer/Cutout access, retaining their meaning.
- GPU build58/PID36552 passes; rigid20/PID29716 passes55 unchanged cases in1.23s,
 validation0errors/0warnings. This is regression evidence, not game coverage.
- Water6/PID32492 fails validation with120errors, despite its pixel comparisons
 passing. Cause: the new fixture released setup framebuffer/render-pass objects
 while their clear commands were still recorded. Image owner leases alone do
 not pin those independent command resources. No threshold/check was relaxed.
 Full failed stdout64,985B, SHA256
 `BFF817F6CAA0258057AD28F2389E7DC889000B7B9A85CB67426FAB348A5FB466`.
- Corrected fixture releases producer image references before drawing, while
 setup framebuffer/views survive their own fence. Weak leases prove geometry
 metadata and images survive producer retirement and release after the fence.
 Actual geometry buffers remain in the fixture/production arena's independent
 fence lifetime; this is not proof of per-mesh GPU arena eviction.
- GPU59/PID576 and water7/PID37792 pass after that fix. GPU60/PID3732 and final
 water8/PID37092 pass after the scene feedback guard,16 two-eye cases in1.23s,
 validation0errors/0warnings. One existing missing GOG overlay manifest loader
 message per GPU suite. No new exported images/raw captures.
- Water fixtures also cover stale frame/slot, generation and second-eye camera
 barriers, snapshot replacement, missing/layer-mismatched images, rigid/water ABI
 refusal, nonzero element/byte-aligned upload offsets,256-instance bounds,
 signed indexed weights and256 deformation phases under reflection/shear.
- CPU output53/PID33924 and30/PID34480 pass; final output54/PID32200 and
 CPU31/PID27540 pass in0.57s. Existing production rigid batch/retirement tests
 and source+resolve write ownership across1/2eyes and1/2/4/8samples are covered.
 JPEG negative-case messages are expected; no retained image output.
- Host136/PID33668,137/PID29092 and final138/PID32948 pass. Codegen0writes/current
 module, no guest object rebuild. Host137 has two existing deprecated getenv/
 fopen warnings in unchanged draw_framebuffer.cpp; host138 has no new warnings.
- CPU build55/PID26132 exposed a fixture-local variable name collision (`barrier`);
 renamed the predicate, not the contract. Failure stdout SHA256
 `603CF6FE25302F393F41FE7DE75C02F90C7C11A7DAF1D65EE387AC6FABEED328`.
 Final build56/PID38044 and CPU32/PID21864 pass in0.50s, including the new queue
 ordering/prepass-run tests. No additional GPU rerun for CPU-only ordering code;
 the water8/rigid20 program/packer results above retain their exact provenance.
 All producers terminal.

Final artifacts, kept in the existing trees (no copies):

| Artifact | Bytes | SHA256 |
| --- | ---: | --- |
| Host138 EXE |48,879,104|009637B6BDCADD3658F03C13780198BA85F46066A1248839DF6B7BF60303C0D4|
| Host138 PDB |110,645,248|3CBA4C5113BEFF365BB7A169933648D784D4134509D08C5829375401A0708843|
| GPU60 fixture EXE |1,103,872|8A7E3DFF7F428375F32F260ED2FBD4EBABC1E8A755525697B76F5363DBF652AB|

## Next connection and remaining acceptance

Publish completed water material values by native instance/generation at the
ordered resource-writer boundary; integrate real named image leases and late
lights/receiver/blend/depth values. Extend the existing mixed deferred packet
consumer/family admission, not a second queue. Use displaced bounds before live
walk culling too. Remove whole-family per-entry legacy resource/material execution
only with this connection complete; preserve unknown-writer refusal, snapshots,
late alias updates, source generation retirement and outgoing mixed-state effects.

Require fresh water submission/emission/fence receipts and mixed cold/reload
qualification. Actual per-eye game cameras/images, water shadow variant, authored
controls, art/pixel/sequence tests and the entire desktop frame remain open.
Latest live evidence remains host134/run969; no new live result or speedup claim.
Quest work has not resumed. Image/raw allowance is unchanged; its blocked producer
does not prevent the next native ownership implementation.

Storage/cleanup shares the existing scene-state ledger. Original profile116B
SHA256`2F1BC38D763A1B7BDBA31F560684FD4AA7E42A714600B8D344F19DA7F38E23B0`
is unchanged. No game process, assets, capture, raw archive or global cache touched.

After replacements passed,30 superseded agent log files were removed,108,094B
logical /126,976B summed immediate physical free-space gains. Corrected water6
and CPU-build55 failure stdout is retired; causes/hashes and regressions remain
above. Keep current GPU60/water8/rigid20/CPU56+32/host138 logs, first shader build52 and
last live host134 evidence. The separately discovered older CPU29 timeout and
associated CPU build52 remain protected; no explanation or fix is claimed for it.
See the cumulative ledger for exact selected retention and drive-wide accounting.
