# Owned lighting pass inputs at object and draw consumption

2026-09-07, EDT; parent `afe5491`, host-only source changes below. This removes
one named direct-object dependency: ambient/camera/color grading no longer need
an interpreted pixel-pass publication for the exact ordinary shader pair.
It does not complete the native object route or the full renderer goal.

## Contract and remaining work

The existing `sub_82174CE8` host producer already owns the named values. A bounded
single `NativeLightingPublication` now returns copies only for its current frame
and render view; reset/unsupported production invalidates eligibility. Native
object packets retain a value copy, independent of source/model/instance retirement.
Consumers do not import shader registers to create the native pass record.

For VS `B5C88BB6295138CC` / PS `FB83DD3F5E67CEB7`, capture compares all three
pixel vectors before enabling consumption. Whole-node replay preflight requires
the fresh record for every owned subdraw before issuing anything. These draws
skip PS c0/c1 freshness and c2 drift/copy history, composing the three vectors
from the producer. Other shader families keep their existing path. Vertex pass
constants, templates and the rendering interpreter still remain.

The misleading old lighting names are corrected without changing values:
source bytes16/17/18 feed fog/specular/normal mapping, respectively (PS bits6/8/3),
not secondary-shadow/shadow-mode/specular. The temporary staging serializer keeps
the same byte values and serial arithmetic. The exact normal-lit shader is the
consumer reference; this is not a replacement of all ordered material switches.

Next: connect the selected object's ordered switches, sampler recipes, shadow
inputs and complete pass bindings to the existing native shader/queue consumer.
Its per-node light producer still must run at the correct time without an
interpreted warm-up. Cold-load/reload with interpreter/capture/replay explicitly
disabled for the selected family remains the acceptance test. No broad recook.

## Verification and the corrected failed attempt

- Lighting fixture build1/PID21776 and CPU1/PID26532 pass (0.03 s test).
  Tests cover stale frames, wrong views, same-frame replacement, reset and
  retained values. Material26/PID25712 and CPU24/PID25100 pass (0.09 s test),
  including lighting retained after source and model/instance retirement.
- All259 Python guards/scenario checks pass (0.057 s final test execution).
  The new `--lighting-pass` gate requires positive fresh comparisons and draws,
  rejecting mismatches, stale/reset counters, wrong scenes and lost readiness.
- Host86/PID25544 and corrected host87/PID27252 pass. Only host objects/link;
  codegen reports up-to-date, no guest objects or shader regeneration.
- Run928/PID26808,05:56:10–05:57:11, was stopped with zero eligible pass checks.
  It incorrectly tagged publication with lighting texture slot `kSlot`, not the
  render-view ID. No image/raw output; coverage failed, never accepted. Corrected
  both boundaries to shared `kRenderViewIdVa` (`0x8277405C`) and added guards.
- Corrected run929/PID25960,05:58:36–05:59:34, passes. Lighting windows following
  ready frames1748/2048 add1,242 matching comparisons and38,435 owned-pass draws.
  Other fresh field windows2048/2348 also pass: shader1,216 checks/42,291 draws;
  light30,229 publication checks/1,216 draw checks; fog2,400 publications and
  1,216 draw checks/2,432 active layers. Wrong/fallback counts are zero for these
  producers. Movement records153 new displacement samples in the same walk.
- Inspected the129,132 B1920x1080 renderer-owned JPEG: running Shu beside fence,
  terrain/vegetation/shadows coherent; known black cliff marks and distant blur
  remain. One flat image is not sequence, reload or both-eye qualification.

All15 temporary settings took effect; exact116 B owner profile restored after
both runs. Autoplay/native comparison enabled; raw capture trigger, perf and
cache/cook persistence disabled. No new raw/perf/cache/dump files. No speedup
measurement or live game use of `CreateNativeRigidPrograms` is claimed.

SHA-256 evidence (host87 plus this source change):

- Exe48,401,408 B: `B57E2AB880039A0E603A8156E40AB1DB531D5B552A4CFB91392981A18FD2A0F3`.
- Run929 text232,568 B: `ECEAA05899D1A3A2DBF6E0208F5A538D88979F3A8673410724C475F8C3212C64`.
- `out/verification/native_lighting_pass_window.jpg`: `DBEF77E3FD436DCFB6D46A767B66FEBE9A882AF75DFC2149DF2D440DBC87D23E`.
- Restored profile: `2F1BC38D763A1B7BDBA31F560684FD4AA7E42A714600B8D344F19DA7F38E23B0`.
- Retired failed928 text232,593 B: `E36869C7DD1A99A5DE68BC3E834A2CE57CB8AE15675F58D9CE637BD810ED95E8`.

## Storage checkpoint

Same original3 GiB exception/floor and raw0 allowance; no budget reset. Existing
trees reused, <=192 MiB estimated link overlap, enforced cumulative floor and
build256 MiB/runtime192 MiB free-drop stops; logs10 MiB/images10 MiB aggregate.
One wrapper parse error started no producer and created no build logs.

After replacement qualification, removed11 exact superseded agent files:
host85/86, material25/CPU23 stdout/stderr,927/928 text, previous primitive-shader
JPEG.608,941 logical B; immediate free62,767,718,400→62,768,340,992 B reclaims
622,592 B (608 KiB), credited once. Build logs are reproducible; exact retired
runtime logs/image are no longer retained. Keep current fixture/host87 logs,
929/image, selected cooked asset and all distinct protected evidence.

Closing cleanup free58.46 GiB,5,267,456 B (5.02 MiB) drive-wide use from the first
62,773,608,448 B measurement; not all attributable to task outputs. Known retained
components grow10,112 B net (material tree, build logs, latest field text/image,
exe/PDB). Lighting fixture, other objects, source/Git and metadata deltas are
not fully attributed. Growth covers new ownership tests/code; equivalent small
runtime evidence replaces its predecessor. See the cumulative scene-state ledger.
