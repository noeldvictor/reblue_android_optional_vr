# Owned ordinary material samplers

2026-09-07, EDT; parent `35c0448`. The selected direct object's ordinary 2D
sampler contract no longer requires captured fetch-state recipes. This is one
dependency removed, not direct native object submission or a complete host frame.

## Contract and integration

`native_material_sampler.h` holds address-free filtering/addressing values.
The existing load-time material decoder folds ordinary phase0 node entry and
ordered 0700/0800 commands into each primitive range. Channels0/1/2/4 begin with
wrap; channel3 inherits and remains unknown. Encoding3 preserves each axis
independently; repeated address commands apply again. No second command stream
or new cache is retained; range bytes share the existing model ownership budget.

The existing host sampler producer publishes filters from scene settings, scoped
to the current frame/shared scene render-view identity. Late known setters update
that publication; unavailable/foreign production invalidates it. Unknown slots
cannot invent their other fields. Retained packets own copied values across later
updates/retirement. Setting2 maps to linear min/mag but nearest mip, preserving
the original setter's two-bit mip behavior instead of assuming all three match.

Source: `bdSceneNodeDrawSingle`, `generated/reblue_recomp.40.cpp`, entry address
resets and loc8228112C through loc82281250; complete `sub_82184A88` in generated70
and existing `SceneSamplerDefaults`/sampler setter reference. Hook metadata has
no instruction-site replacement of these sampler operations; the current host
whole-function hooks are in `native_sampler_bridge.cpp`. The host scene begin
calls defaults after pass dispatch publishes the shared render-view ID. Generated
code and game assets were not edited. The exact ordinary PS uses 2D channels
0/1/2/4; cube5 and shadow6 are not this material-sampler contract.

For VS `B5C88BB6295138CC` / PS `FB83DD3F5E67CEB7`, capture compares eligible
recipes before marking ownership. Replay preflights all primitives and current
bindings before consuming fresh values through the existing sampler cache.
Normal owned slots skip fetch reads and captured sampler-history copies. Missing
or conflicting inputs refuse the whole node before submission. Only unused W
addressing/border color are normalized for known non-border 2D sampling; every
other descriptor field and unsupported sampler retains its full comparison.

The uploader creates `TEXTURE_2D_ARRAY` views for ordinary images. Array-layer
selection does not use sampler W addressing. Both plain2D and array2D are eligible;
cube/volume and compound companions are refused. No new GPU cache/shader code.
Native object packets now retain the same recipes for direct submission work.
The template/interpreter branch and its unconverted consumers cannot yet be deleted.

## Verification and failed attempt

- Sampler1/PID24184 and CPU1/PID28244 pass: 112,000 independent PPC publication
  comparisons, semantic filtering and 216 scene-setting combinations; 2.44 s
  behavior /2.46 s CTest.
- Material28/PID29820 and CPU26/PID27088 pass: 512 address payload cases,
  truncation, repeated commands, independent inheritance, late/reset/wrong-view
  publication and retained source-free packets; 0.11 s behavior /0.13 s CTest.
- Binding20/CPU18 and corrected21/PID28988, CPU19/PID24168 pass. The final fixture
  covers 72 sampler combinations, complete descriptor differences and both
  production-compatible ordinary view types versus unsupported dimensions and
  companions. Behavior0.05 s /CTest0.07 s.
- All263 Python source/scenario checks pass in0.068 s. `--material-samplers`
  requires fresh post-event comparisons and consumption, rejecting unused,
  stale/reset/wrong-scene/oversized/mismatched evidence.
- Host89/PID30404 and corrected90/PID27020 pass; host objects/link only, codegen
  up to date, no guest compilation or shader regeneration. Host90 takes6.66 s
  including the bounded wrapper, not an overall development-speed measurement.
- Run931/PID31668 timed out06:47:28 with **zero eligible sampler checks/draws**.
  The first gate accepted only plain2D; the uploader emits array2D. No mismatch
  was hidden, and this run was not counted as a pass. A focused boundary fixture
  was added before retrying. No image was written; profile restored.
- Run932/PID17888,06:54:27-06:55:25, passes. Consecutive post-event windows2032/2332,
  after event1432, add3,628 matching sampler checks and41,602 owned-input draws.
  Features add1,210 checks/42,451 draws; lighting1,262 checks/38,705 draws.
  Existing gates pass:115,271 pose comparisons,81,532 object comparisons,
  14,355 light publications/1,210 draw checks,2,400 fog publications/1,210 checks,
  15,344 shadow checks. Movement adds31 samples/38.96121 units in the same walk.
- Inspected1920x1080 renderer-owned JPEG: running Shu, ground, fence, vegetation
  and shadows coherent; known black cliff marks/distant blur remain. One flat
  image does not qualify a sequence, reload, all sampler modes or both eyes.

All15 temporary settings audited; exact116 B owner profile restored. No new
raw/perf/cache/dump/cook files. Source-free GPU loads remain0; no live native
rigid shader draw, full-game qualification or measured speedup is claimed.

SHA-256 evidence, host90 plus this source change:

- Exe48,412,672 B: `F2382778D027275E8E7AA1A0F4CA9DD791CEAC81B58B92ABC40369E6FDE99E8A`.
- Run932 text234,233 B: `AF1558CC0DB79D053EE56401E02D860CC99CCBCC7D810DBC73ED1210FBF3B248`.
- `native_material_samplers_window.jpg`,135,574 B: `E6411EF56B1F82B2CCA665FDFFF3EE079872473565D20494365496B1C90955AB`.
- Failed931 text342,750 B, retired after diagnosis/replacement: `AAC574B17D4A59032DAF36455651A1BB5F2EDE77ED03E3D01103CEE6A763C8DB`.
- Restored profile: `2F1BC38D763A1B7BDBA31F560684FD4AA7E42A714600B8D344F19DA7F38E23B0`.

## Development order and storage

README/transition now distinguish completed sampler ownership from remaining
vertex/shadow inputs, correctly timed per-node lights and direct submission.
The active queue is shortened to dependencies and acceptance rather than repeating
the component history. AGENTS requires boundary fixtures with real producer
representations before retrying failed eligibility. Reuse the selected cooked
asset and native programs; no bulk recook or additional renderer framework.
The next integration must lead to interpreter-disabled cold-load/reload acceptance,
not another generic adapter without a named direct-object dependency.

Same original3 GiB exception,62,509,998,080 B operational floor and zero new raw
allowance. Existing build256 MiB/runtime192 MiB free-drop stops,400 KiB run text,
160 KiB JPEG and10 MiB aggregate logs/images bound both attempts and their overlap.

After replacement passed, removed19 exact superseded agent files: host88/89,
material27/CPU25, binding19/20 and CPU17/18 stdout/stderr;930/931 text and the
material-feature JPEG. **738,922 logical B removed;757,760 B actually reclaimed**
(immediate free62,879,055,872 ->62,879,813,632 B), credited once. Build logs can
be regenerated; exact retired runtime text/images are gone, with observations
and hashes retained. Current fixture/host90/932 evidence, selected cooked asset
and distinct protected GPU/flat/VR/motion/failure/raw evidence are preserved.
No owned producer remains.

Ending58.56 GiB free. From this sampler continuation's first62,765,817,856 B,
drive-wide free increased113,995,776 B; most is unattributed concurrent activity,
not cleanup credit. Known retained components grow221,693 B: material tree+28,341,
texture fixture tree+148,023, build logs-1,600, image-12,263, runtime text-712,
exe+6,656/PDB+53,248. This covers ownership/regression code and replacement
evidence, not extra verification sets. Other objects/source/Git/metadata deltas
are not fully attributed. Replace equivalent evidence at the next checkpoint;
the cumulative ledger remains `20260906_0333_native-scene-state-bridge.md`.
