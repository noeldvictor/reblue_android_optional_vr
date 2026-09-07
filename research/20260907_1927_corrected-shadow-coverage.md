# Corrected native cutout shadows: GPU and live reload evidence

2026-09-07. Source808aa45; host116 embeds808aa4572 dirty because the cumulative
ledger changed. Normal owner profile restored. No complete host frame, visual
acceptance, full-game stereo, Quest result or measured speedup is claimed.

## Recovered contract and removed dependencies

The earlier phase1 implementation used scene-style object/vertex alpha and an
ordered generic cutoff. Its37/41 GPU cases tested that implementation, not the
authored shadow contract. The source investigation recorded in the cumulative
ledger supersedes that assumption:

- `sub_82198138` (generated/reblue_recomp.62.cpp), flushed by `sub_821981E0`
  (generated67), sets texture enable booleans for modes0..3; larger modes retain
  inherited state and remain unsupported by this owner.
- `sub_82174270` (generated64) selects ordinary technique0 shadowmap for the
  phase1 list callback, versus technique24 shadownull VS for the direct callback.
  Both table entries use `bd_shadowmap_ps`; see the checked-in
  `tools/shader_cache/shader_table_descriptors.csv`.
- Retained `bd_shadowmap_ps.hlsl`, shader hash0x3646F81DE849C63C, uses base texture
  alpha with fixed threshold `asfloat(0x3F19999A)` (0.6), no object/vertex alpha,
  generic reference/comparison or negative-U absent-layer sentinel. The normal
  shadow VS consumes position/UV, with `(uv+1)/512 + owned offset`.
- The node phase1 branch (generated40) forces textured alpha casters onto the
  sorted/deferred route. Rejecting all deferred policies structurally excluded
  the textured family; another unchanged zero-coverage run would not fix that.
- The ordinary light-space deferred consumer has depth writes, no scene depth
  sorting and no colour/stencil effects. It can join the existing native
  min-depth queue. Discarded holes do not erase earlier casters. General scene,
  translucent, skin/wind/effect and forced-pass ordering is not covered by this
  substitution and remains excluded where unsupported.

The native shadow packet now owns only the relevant texture mode, ordered image
assignment, UV/sampler and retained geometry/matrices. The shadow path no longer
reads scene alpha/cutoff producers or binds vertex colour. Zero-texture casters
use the solid program; the alpha-only shader/program was removed. Whole-node
preflight and the existing indirect/image/fence owners remain shared.

## Verification

Material38/CPU36 and output37/CPU20 passed in the preceding source checkpoint.
This continuation reran320 artifact-free Python source/scenario checks (0.174 s).
GPU36's already-built fixture was reused for rigid14/PID37760: all46 modes pass
in1.30 s (CTest1.31 s), RTX3060, zero Vulkan validation errors/warnings, one GOG
loader-manifest message. No raw output.

Scene modes0..22 remain unchanged. Corrected shadow modes poison irrelevant
scene-alpha/comparison fields, verify wrapping and the0.599/0.6/0.601 boundary,
and sample actual cutout depth through both-eye native receivers. Modes44/45
overlap complementary near/far casters and reverse their GPU instance order;
both independently match every mono shadow-depth and both-eye scene pixel.
The prior overlap fixture put the far caster level with the nearer receiver;
moving it in front corrected the fixture without changing tolerances.

Host116/PID29224/session48752 exits0 after host compile/link; codegen reported
0 written/module up to date. No guest object compilation. Exe48,695,808 B,
PDB109,486,080 B. This connects the corrected native consumer to the game; it is
not just a fixture binary.

Run956/PID28364/session24363 ran19:21:32..19:23:53 with the full native image,
material/geometry/instance/texture/policy/lighting/receiver/rigid/hard-off/reload/
caster/cutout gate chain and one bounded mono inspection. All21 temporary flat
settings took effect; capture_after_s0, perf off, mouse-menu off, no cooking.
The owner profile was restored byte-for-byte in guaranteed cleanup.

Both independently parsed interactive-field epochs pass the full strict chain:

| Fresh300-frame window | Scene textured cutouts emitted / retired | Shadow textured cutouts emitted / retired | Wider caster primitives emitted / retired |
| --- | --- | --- | --- |
| Cold1863..2163 | 1,000 /998 | 300 /300 | 35,341 /35,358 |
| Reload4259..4559 | 949 /947 | 300 /300 | 33,208 /33,213 |

Retirement deltas may include work emitted before a window. These are repeated
runtime visits, not unique assets. Generation93/instance144 fully retires at
title; generation207/instance435 supplies the independent new scene/shadow
windows. The selected regression has at least900 fresh emissions of each kind
in each epoch. Reloaded wider scene emissions+2,212/retirements+2,207, including
300 multi-primitive node visits; sampled layered draws remain0. Batches remain
singletons, merged_instances0. No draw-call reduction or speedup is established.

## Image verdict and next boundary

Inspected `native_cutout_family_window.jpg` (1920x1080,101,553 B): textured
foliage, terrain, fence and shadows are present, but nearby tree trunks have
visible gaps between lower and upper sections. This is **not accepted scene
pixel evidence**. The prior945 image has a different camera/position, so it is
not a matched correctness comparison. Whether the gaps are authored camera
fading/clipping, native geometry/material participation or retained legacy
interoperation is not established. Preserve the new image/log as the current
visual question and945 as the last accepted baseline; do not simply boot until
a nicer camera makes the issue disappear.

Next visual investigation needs the affected node/material, owned pose/bounds,
participation and camera relation, then a bounded matched-state correctness
comparison if source does not resolve it. It must distinguish the native scene
path from shadow coverage and retained consumers before selecting a fix.
UV failure948 and dirty-light failure941 also remain unresolved; this successful
run neither reproduces nor explains them. No checks or thresholds were disabled.
Independent work on the remaining owned scene/material/character producers can
continue, but full field/battle/cutscene/menu/transition/animated/both-eye gates
still precede Quest optimization.

## Retained identities and storage

SHA256:

- Host116 exe: `81867FE2CD3A58C8DE9289A0EB87C161BBCF8CC28F8CC1994C443F3D164932F1`.
- Host116 PDB: `FC347FA8D33F6A80AE8C7DC4B17BA70040937139DF538BEB408FD0226739CB7B`.
- GPU fixture: `43185D5A085F54F746EF67ED5173D2B0D67965265FC8FF8309179FF0E3C56ADD`.
- Run956 log488,126 B: `8C17A3C0598796A990280E026F3EC5447C933D249AA013C95EF690132748CD5B`.
- New image101,553 B: `3ECD94396D78995DB30C4762E8F1235B005FAC2731E5F43509E9C0937D69B521`.
- Restored116 B profile: `2F1BC38D763A1B7BDBA31F560684FD4AA7E42A714600B8D344F19DA7F38E23B0`.

The original3 GiB exception/floor62,509,998,080 B is unchanged. Rigid14 used the
32 MiB free-drop supervisor; host116 and run956 used192 MiB. Runtime diagnostics
including the reserved800 KiB log/110 KiB image fit72,647,080 B under the75 MiB
stop threshold. No new raw/perf/cache/dump/cooked output; no producers remain.

Removed24 superseded CPU/build/GPU logs54,186 logical B after replacements
passed; run955 log364,742 B after956 replaced its cold-only evidence; and the
unreferenced generated alpha-only shader header31,075 B after the new host
linked. Total26 files/450,003 logical B, recoverable by rerunning/rebuilding
earlier revisions, not restoration of the exact old log text. Historical
reports retain their results. No protected945/948/941 evidence, game data,
saves, profiles or build tree was removed.

Measured cleanup free-space deltas were+8,192,+57,344,+368,640,+32,768 B; other
volume activity is not isolated. Ending measured free81,856,401,408 B is
5,091,328 B below first81,861,492,736 B, not attributed wholly to this work.
Counted retained net growth155,039 B: new log/image589,679, old955 log-364,742,
GPU fixture+414, build logs-35,653, exe/PDB-3,584 and old shader header-31,075.
Material8,333,551 B and texture70,641,489 B are unchanged this continuation;
GPU tree10,659,026 B/10files, build logs201,025 B/136. Other objects/CMake/source/
Git bytes are not allocated in that subtotal. New image/log remain until their
visual question has a verified replacement; the previous allowance is not reset.
