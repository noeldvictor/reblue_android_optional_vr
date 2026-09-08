# Ordered native water draw connection

2026-09-08. Runtime source: efa8c7b plus this checkpoint's source changes;
host143/144, GPU66/67, CPU60/61, game972. This is an intermediate connected
water draw, not completion of the all-host desktop/VR goal.

## Delivered boundary

`native_refraction_material_bridge.cpp` publishes final parameter destinations
only after `PrepareWaterMaterial` completes factor/flush/clamp/state/image/snapshot
side effects. `NativeWaterMaterialPublication` is a single scoped copy keyed by
native instance/generation/frame, not a VA cache. Bump and planar images are captured
at their producers; snapshot's explicit output getter is copied after its writer.
The explicit sorted environment image is captured after possible parameter aliases.
Missing/null inherited inputs never borrow another draw's texture slot.

`deferred_consumer.cpp` closes old compatibility capture for water, creates the
publication scope around ordered material begin, and attempts native submission
before `BindEntry`/`SubmitSurface`. Admitted water skips legacy IB/VB/declaration,
copied VS/PS material constants and translated draw calls. It exports the original
depth-write end state for subsequent compatibility callbacks/draws. Model/visual
begin/end callbacks and the source list still execute; their removal is next work.

The existing load-owned model index resolves an exact native node and original
buffer/range association; duplicate matches refuse. Its aliasing lease pins the
generation. Native pose equality is checked against the temporary entry matrix.
The real queue receives only owned geometry, native identity, final GPU values,
image leases and native pipeline/sampler policy. It reuses current lighting/fog,
receiver and ordered light tickets, typed image owners, shared instance arena,
indirect draw and fence retirement. No second asset cache or renderer framework.

Legal unused descriptors: inactive bottom uses the retained native sun depth;
inactive snapshot uses the retained planar HDR image. Inactive environment uses a
24-byte black cube asset with a content-derived ID through the existing bounded
GPU texture store, without a disk cook. Active inputs must be available and all
sampled roles are checked against active attachments/resolves. This remains
incomplete admission for unavailable image-role combinations, not a final fallback.

Native water sampling is explicit: linear/wrap animated normals, linear/clamp
screen and cube images, nearest D32 shoreline depth, comparison sun sampling.
No draw-time fetch-state translation or ordinary five-slot sampler assumption.
Recognizable-art/pixel qualification of this native policy is still required.

The water GPU layout reuses reserved words for alpha comparison/threshold and
the shared native predicate; no layout-size change. Coverage intent reaches the
MSAA pipeline. The walk cannot know amplitude finalized by a later alias, so known
water scene nodes bypass early undisplaced frustum/distance rejection; the queue
uses their finalized conservative world-Y wave bounds. Other scene gates remain.

## Contract provenance

- Sorted entry: `bdSceneNodeDrawSingle`, generated/reblue_recomp.40.cpp, especially
  0x82280A68 onward; entry+252 node, +268 palette, +272 visual, +280/+284 strip
  triangle/start range, +376/+380/+384 declaration/VB/IB. Native canonical geometry
  is triangle-list data; association checks the original strip count (triangles+2).
- Model/resource dispatch: `sub_8221DB00`, generated/reblue_recomp.43.cpp; input+4
  is entry+244 visual, then its vtable+32 resource begin. Both visual identities
  must agree at this intermediate boundary.
- Water writer: existing native replacement `sub_82454720`; decoder offsets and
  final-destination semantics in `native_water_material_source.h`. Resource end
  `sub_824548A8`, generated/reblue_recomp.62.cpp, still clears bindings7/12 and
  conditionally13. Those callbacks are explicitly not claimed removed here.
- `config/hooks/render_list.toml` documents replay insertion and visual switching;
  known water callbacks remain checked by `native_deferred_contract.h`.

## Verification

All producers terminal. CPU60/PID34016 and test36/PID38712 passed; CPU61/PID37792
and test37/PID27716 supersede them with generation reuse/retirement/ambiguous-node
cases. Final CPU test0.47s. Existing source-lifetime/clear/resolve tests still pass.

GPU66/PID24480 compiled both water shaders. GPU67/PID32032 rebuilt the fixture.
Water13/PID33804 failed the NEW discard oracle: actual red9, expected0.125, Vulkan
validation0/0. The existing fixture explicitly clears the live target to(9,8,7,1)
after taking its retained snapshot. Discard correctly preserves that live colour,
not the old snapshot. Corrected only the new discard expectation to(9,8,7,1);
preserved the failing log, old cases and their exact tolerances.

Water14/PID37580 passes20 two-eye cases in1.15s, including GE/Less alpha discard,
unchanged depth for discarded pixels, immutable material/identity publications,
image retention, reflection/bottom/snapshot producers and real indirect drawing.
Rigid23/PID36096 passes55 cases in1.22s; snapshot22/PID38280 passes8 cases in1.03s.
All three have validation errors0/warnings0 and one known missing GOG overlay
manifest loader notice. GPU readback stays in memory; no raw/image files retained.
Coverage intent is wired but these water pixel cases are single-sample, not MSAA
alpha-to-coverage qualification.375 Python source/scenario checks pass (exit0).

Host143/PID35168/session68957 and host144/PID35932 exit0; codegen writes/deletes0,
no guest objects. Host144 includes the point-depth sampling policy. No full rebuild.

Game972/PID35416/session9141 runs07:44:01..07:46:08 Eastern (11:44..11:46 UTC),
exit0 under180s/800KiB/192MiB caps. Existing wrapper's full mixed/deferred-input,
lighting/material/geometry/hard-off/caster/cutout cold/reload chain is unchanged.
All22 temporary settings took effect; capture/perf disabled; exact116-byte profile
restored. Strict selected rigid reload changes generation93/instance144 to207/389.

Actual water is a distinct instance: initial145/generation94, reloaded383/208.
First new-generation sample: submitted1679, emitted1635, culled43, retired1678;
all preceding water packets had retired before that new submission. At the last
sample(frame4604): submitted3179/emitted3084/culled93/retired3177. Admission log
prints during material begin, before that entry submits: candidates3179/consumed3178,
unavailable0. These are runtime packet counts, not unique assets or full native frames.

Fresh post-event windows, each after interactive bg41_01/event0 readiness:

| Water window | Submitted delta | Emitted delta | Culled delta | Fence-retired delta |
| --- | ---: | ---: | ---: | ---: |
| Cold1577..1877 | 300 | 300 | 0 | 300 |
| Reload4304..4604 | 300 | 271 | 29 | 300 |

The mixed ordinary deferred window4500..4800 consumes2820 packets, pending0,
223 paired visual scopes,299 input batches/refreshes. These counts differ from
older runs/scenes and are not speedup measurements. Reflection still publishes
3900 native outputs with compatibility/faults0;10 initial camera misses stop
increasing. No bottom/snapshot call observed. Full native water-family ownership,
game/HDR pixels, animated sequences, all desktop scenes and game stereo remain
unqualified. No Quest run or optimization.

## Artifact identities and retention

- Host EXE48952832B SHA256 FB4D011D8CFEF151DEAE4B77D381818E27EDCFF8C1997F05EC17BB9C65013E67
- PDB111058944B SHA256 CC1C6C0BF17B4B80D8DA04720E99C53820A1303845CCC153E23D1B811F7C64F9
- GPU EXE SHA256 91C89D7A5B963394F9423D459C4AADBF09B8407B53E8AA6C0C0A09165EF582F6
- CPU61 EXE SHA256 E9ECEE4A4BEA46BBCC7A4FD86BDCB967EBD9A494EBD2896FA358005D2DDA560B
- Run972541835B SHA256 BEF7A0C6B9261FD915C5F8F12DA241B1B257061C9E8764A09A95FFD98B6B2620
- Restored profile SHA256 2F1BC38D763A1B7BDBA31F560684FD4AA7E42A714600B8D344F19DA7F38E23B0

Same cumulative ledger:20260906_0333_native-scene-state-bridge.md. Retain current
builds, run972 and water13 failure; protect previous unresolved failures/captures.
Removed18 superseded successful attachment logs (CPU59/test35,GPU65,water12,
rigid22,snapshot21,host142,CPU60/test36, both streams),41701B, and hash-verified
run971524518B after972 replaced its strict mixed/reload purposes.19 files566219B
logical; measured cleanup interval free80146726912->80147308544B,581632B gain.
Full old log text is retired; checked-in reports preserve results and hashes.

Selected retained CPU/GPU trees now78238320B/132files and19063572B/19files;
attachment logs298537B/186files. Against the preflight, those trees plus EXE/PDB,
attachment logs and replacement runtime text grow373280B net. Retained for current
connection/failure evidence; replace rather than accumulate at the next checkpoint.
No new game assets, native asset disk cache, profile content, raw or image files.
Drive-wide movement is separate: first80170917888B, post-cleanup80147308544B
(~74.64GiB),23609344B less free. Scoped runtime cache/perf checks found no new
files; host objects/metadata/Git and other processes are not all attributed here.
