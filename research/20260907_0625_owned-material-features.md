# Load-owned ordered material features

2026-09-07, EDT; parent `892ef66`. This removes the selected direct object's
dependency on captured diffuse/specular/normal-map/reflection/fog decisions.
It does not complete direct object submission or the full host renderer.

## Producer, owned data, consumer

The existing model-load decoder folds commands into one feature recipe per
primitive range. It does not retain a second command stream or create a cache.
Control-table import is confined to loading; range storage participates in the
existing model byte budget. Object publication now includes the final diffuse
enable. Composition uses that value, the object's shininess permission and the
fresh native lighting pass. Packets retain the resulting named booleans by value.

Source reference: complete relevant control flow in `bdSceneNodeDrawSingle`,
`generated/reblue_recomp.40.cpp`; E000 branch around line12905, 0300 at
loc82280D50, 0400 at loc82280CC0, 0500 at loc82280F40. The material-control
dispatcher is `sub_8228AB40` in generated94; its first switch handler is
`sub_8228AAB0` in generated79 and second payload handler `sub_8228AB18` in
generated67. `sub_821739F0` in generated7 applies the live specular gate.
Generated code and original assets were not edited.

- A null control table is a no-op. A present table whose record's present-bit0
  is absent restores defaults; it is not the same no-op.
- E000 disables diffuse or restores the object's entry value. Both specular
  disable and entry restoration reset specular to zero in this ordinary phase0
  path. The second handler writes output+4, not the four switch bytes.
- Repeated 0400 power commands skip the entire update, even after E000 resets
  specular. A changed power can request specular again, subject to object/pass gates.
- 0300 explicitly changes diffuse. 0500 requests normal mapping unless its value
  is255; the pass gate remains live. Existing ordered reflection recipes supply
  reflection enable; fog comes from the fresh pass. Unknown imports refuse
  consumption until later explicit commands make the required decisions known.

For VS `B5C88BB6295138CC` / PS `FB83DD3F5E67CEB7`, interpreted capture compares
the five packed switches before marking ownership. Replay preflights all siblings
and recomposes these bits from current owners before submission. Missing or
conflicting values refuse the whole node before issuing draws. The temporary ABI
adapter changes only PS bits3/4/6/7/8; shadow receiving and unrelated bits keep
their separate paths. No captured boolean history supplies these five decisions
once the owned path is selected. Shader selection, templates and the interpreter
still exist; no entire compatibility consumer can be deleted yet.

## Verification

- Material27/PID28560 builds; CPU25/PID26240 passes in0.11 s (CTest0.12 s).
  Coverage includes1,280 composed feature combinations, control ordering and
  null/default distinction, repeated power, unknown/truncated imports, source-free
  retained packets, live object gates, control bounds and preservation of unowned bits.
- All261 Python source/scenario tests pass in0.058 s. `--material-features`
  requires fresh post-event comparisons and actual consumption, rejecting stale,
  reset, unused, wrong-scene and mismatched evidence. An initial source-guard
  spelling error was corrected; no build/run was accepted from that failure.
- Host88/PID22220 passes: host objects/link only; codegen up to date, no guest
  compilation or shader regeneration. Existing build trees reused.
- Run930/PID30484,06:22:52-06:23:53, passes. Feature windows2009/2309, after
  observed opening event1409, add1,326 comparisons and41,061 owned-input draws,
  wrong0. Shader-layer/colour checks have the same deltas. Lighting-pass windows
  following1709/2009 add1,271 comparisons/39,093 draws, wrong0.
- Existing field gates also pass:125,401 pose comparisons,90,888 object-input
  comparisons,14,299 selected-light publications/1,326 draw checks,2,400 fog
  publications/1,326 draw checks with2,652 active layers,15,590 shadow checks.
  Observed movement adds21 samples/27.457556 units in the same walk. These are
  scenario observations, not a speedup measurement.
- Inspected the1920x1080 renderer-owned147,837 B JPEG: running Shu by the fence,
  terrain, vegetation and shadows coherent; known cliff marks/distant blur remain.
  This single flat sanity image is not sequence, reload or both-eye qualification.
  Activation of every optional feature in authored content is not established.

All15 temporary settings audited; exact116 B profile restored. Raw capture trigger,
perf and persistent cache/cooking disabled. Post-run inventory finds zero new
raw/perf/cache/dump files. No live `CreateNativeRigidPrograms` game draw or
source-free GPU load is claimed. The next dependencies are sampler recipes,
remaining shadow/vertex-pass values and correctly timed per-node lights, then
whole-node direct scene/shadow routing and interpreter-disabled cold-load/reload.
Reuse the persisted162-vertex target; no bulk recook or new renderer framework.

SHA-256 evidence (host88 plus these source changes):

- Exe48,406,016 B: `7B848A57C945AA8FADA503D7EA06E9BAE1713FE42977C83EF5F3B62926DEF9B7`.
- Run930 text234,945 B: `3B79741DADF8DDD11E7DF16B8F4734FD2CFCC745951F8E81FFBDABE2D8122672`.
- `out/verification/native_material_features_window.jpg`: `142E4094E6BEFCB750E062A76651DFD337E13575CCDE8537AA64E28DF62E9376`.
- Restored profile: `2F1BC38D763A1B7BDBA31F560684FD4AA7E42A714600B8D344F19DA7F38E23B0`.

## Storage

Same original3 GiB exception and62,509,998,080 B floor, no budget reset; raw0.
Planned <=192 MiB link overlap; existing build256 MiB/runtime192 MiB free-drop
stops,400 KiB text/160 KiB image and10 MiB aggregate logs/images remain enforced.
One wrapper patch-order error changed no file and started no producer.

After replacement passed, removed eight exact superseded agent files:
host87, material26/CPU24 stdout/stderr, run929 text and lighting-pass JPEG.
364,432 logical B removed. Actual reclaimed bytes cannot be isolated: immediate
free62,939,676,672 ->62,938,685,440 B decreased991,232 B during cleanup, so no
positive measured recovery is claimed. Build logs are reproducible; exact retired
runtime text/image are gone, with hashes/observations retained in the prior report.
Keep current fixture/host88 logs,930/image, selected asset and distinct protected
GPU, flat/VR, movement, unresolved-failure and raw evidence. No producer remains.

Cleanup-end58.62 GiB free; drive-wide use+26,255,360 B (25.04 MiB) from this
continuation's first62,964,940,800 B, not wholly attributable to task files.
Known retained components grow109,084 B: material fixture+46,638, build logs+3,988,
replacement image+18,705, runtime text+2,377, exe+4,608/PDB+32,768. Growth covers
ownership/test code and a slightly larger replacement image, not another retained
verification set. Other objects/source/Git/metadata deltas are not fully attributed.
See the cumulative scene-state ledger; replace equivalent evidence next checkpoint.
