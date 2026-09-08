<img width="1480" height="662" alt="Untitled-1" src="https://github.com/user-attachments/assets/1779fdfd-bc3a-416d-8b6c-38874d8eae93" />



> [!CAUTION]
> **This is a personal, vibe-coded fork. It is AI-driven experimentation for my own amusement.**
>
> I am not looking for users, testers, bug reports, feature requests, support questions, or
> Discord pings about it. Nothing here is supported, nothing here is promised, and most of it
> is written by an AI under loose supervision and pushed without ceremony. It will break. It
> will stay broken for a while. That is fine, because it is a toy.
>
> **Please do not bother me about it.** If you want something out of this: fork it and do the
> work yourself. That is genuinely the intended workflow, and the license permits it.
>
> If you want a real, working, supported build of re:Blue, go upstream:
> **[zolaware/reblue](https://github.com/zolaware/reblue)**. All the credit for this project
> belongs there. Issues and pull requests on *this* repo may be closed unread.

> [!IMPORTANT]
> re:Blue is an unofficial project, not affiliated with or endorsed by Microsoft, Xbox, Mistwalker,
> Artoon, or Sega. It ships no game data. You supply that from your own discs.


# re:Blue (personal fork)

re:Blue is a static recompilation of *Blue Dragon*: the original PowerPC program
is translated ahead of time into C++ and compiled as a native application. This
personal fork is moving the game's rendering out of the Xbox 360 model and into
a host-native Vulkan renderer, while gameplay remains recompiled.

## The goal: desktop host renderer, then Quest 2 VR

Move **all rendering** to the host: scene and material data, animation and GPU
skinning, shadows, reflections, particles/effects, post-processing, UI, frame
scheduling and presentation. The finished frame must no longer depend on guest
rendering execution, per-draw Xenos/D3D state translation or EDRAM emulation.

Preserve Blue Dragon's recognizable art style and readability, with freedom to
change assets, materials, lighting and geometry. Desktop asset conversion,
generated LODs, merged statics, impostors, offline texture mips and compression
are part of the work. The renderer is being built around multiview stereo,
occlusion/frustum culling, instancing, indirect draws and modern Vulkan features.

**Desktop comes first.** Fields, battles, cutscenes, menus, scene transitions,
reloads and both eyes must be verified before Quest 2 qualification and
optimization resume. The eventual Quest target is **72 Hz at 1440x1584 per eye**,
with shadows and foveation; it is not a result this fork has achieved. AYN Thor
is not an active test target.

Optional character cel shading and tourist mode remain side features. They do
not replace the host-renderer goal.

## Current state

**This is not yet a fully host-owned renderer.** We have the statically
translated game executable (18,777 function bodies in the local census), not
the original high-level source project. That source lets us trace exact behavior
and replace complete rendering paths; it does not make ownership automatic.

**Latest connected checkpoint (2026-09-08, host155/run980):** native water now
gets its planar reflection directly from the completed native pass. Refraction
snapshots also return their exact image lease directly, without callback/getter
readback in the water consumer. Late outgoing-image aliases are checked, not ignored.
The prior native node producer, sorted queue and visual/material/draw connection
remain in place; admitted water no longer executes the node command interpreter
or allocates source render-list entries.
CPU behavior tests, 378 source/scenario checks and strict desktop cold/reload checks
pass. Both ready-field epochs advance native reflection publications and reads;
the last sample has 3,900 publications and 3,171 reads. The GPU sample reports 3,158
submissions, 3,069 emitted draws and 3,156 fence-retired packets, with no admission
refusal. The previous water generation retires before the reloaded one submits.
These are repeated draws, not unique assets or a speedup measurement. The unchanged
20-case two-eye GPU water fixture remains shader evidence, not game-stereo proof.
Outgoing mixed-frame image/state/world/sampler exports, late parameter descriptors,
bump-image table selection and snapshot timing/getters remain adapters; special-image
and other unadmitted families still use the legacy producer. Game water/reflection
pixels, HDR art parity and authored shore/refraction/stereo coverage remain unqualified.
[Completed image connection, verification and remaining work](research/20260908_1345_native-water-images.md).

**Characters (2026-09-08, native skin shadows now emitting in-game):** owned skin
geometry, animated bounds and pose palettes feed the shared native shadow queue
and frame fences. Cutout image/UV/sampler support and checked technique-1 texture
participation are connected; missing or volume-effect inputs remain refused.
Mesh/ownership tests and the host build pass. All 85 Vulkan pixel cases pass,
including 30 skin cases covering one-to-three influences, cutouts and overlapping
instances; Vulkan validation reports zero errors/warnings. All 383 Python checks pass.
Host160's cold-field window emits and fence-retires 13,200 native skin primitives
over 300 frames. Its run stopped during reload at the disk-growth guard, before
the requested game image. Full skin reload, game-pixel and stereo acceptance
remain pending; host159/run984 is the last complete strict regression run.
Skinned scene shading, native animation production and remaining rendering
adapters are still open. No measured speedup or full native character/frame
completion is claimed.
[Connection, live evidence and remaining gates](research/20260908_1143_native-skinned-shadow-consumer.md).

### How much is left?

**Substantial implementation and qualification remain.** Live-qualified native
rigid rendering covers one scene object and a wider shadow-caster family. New
multi-primitive scene shading and water pass targeted cold/reload checks, but
scene-wide pixel and stereo acceptance remain incomplete. No complete
host-only frame or whole native game scene has passed the full acceptance gate.
The foundations below are reusable, but remaining work spans scenery/material
families, characters/skinning, effects/UI, frame/pass ownership, asset streaming
and full desktop stereo coverage. Quest 2 optimization has not resumed. There
is no defensible completion percentage or delivery estimate yet.

Last **accepted live/pixel checkpoint** (2026-09-07, host107/run945): native
opaque rigid **shadow casting now covers a material family and multi-primitive
nodes**, not one hard-coded asset. Every sibling is preflighted before submission;
supported nodes bypass the old interpreter/capture/replay path. In a fresh
300-frame reloaded-field window, the wider family emitted and fence-retired
12,532 additional native primitives, including 2,096 multi-primitive node visits.
These are repeated runtime visits, not a count of unique converted assets.

The original object's native scene/shadow path remains a separate regression:
cold load and real title teardown/reload pass, including old-source/GPU retirement.
Both epochs also pass the receiver and owned-lighting gates. The inspected mono
image shows coherent terrain, trees, fences, Shu and shadows; known cliff marks
remain. Batches are still singletons: no draw-call reduction or speedup is claimed.
[Caster-family implementation and evidence](research/20260907_1420_native-caster-families.md).

**New implementation, built but pending game acceptance:** the native scene route now
handles whole opaque rigid nodes with zero-to-three texture layers, independent
UVs/samplers and retained image lifetimes. It no longer selects one material ID
or only primitive0. Unsupported siblings and missing active resources fail closed
before submission. Its production shaders pass 12 two-eye GPU cases, including
layer composition, alpha, UV addressing and instancing; C++ behavior fixtures and
six real host-consumer syntax checks pass. Host110 links the whole connection and
requires only the material channels enabled by owned object/pass features.
[Layered scene implementation and pending gates](research/20260907_1501_layered-rigid-scene.md).

Ordered native light values now support authored "keep the previous lights"
bindings, with stale-ticket checks and an outgoing mirror for remaining legacy
consumers. Missing inputs and unknown intervening writes still refuse; no default
lights or old shader values are guessed. The new C++ fixtures pass; inherited
game draws and representative layered-scene pixels still need live verification.
[Ordered light ownership and current build](research/20260907_1544_ordered-scene-lighting.md).

Earlier layered integration, **host110/run948**: a fresh 300-frame cold-field window emitted
and fence-retired **1,265 additional native scene primitives**, beyond the selected
regression object. Its cold title teardown also completed. The reloaded field
then hit a strict **UV mismatch in a remaining legacy draw**, so full reload and
pixel acceptance remain open. This run took no images and sampled zero layered
or inherited-light draws; no speedup is claimed. The failure is preserved, not
hidden by disabling its comparison.

Host111 adds bounded UV-failure provenance, not a rendering fix. Follow-up
diagnostics stopped at time/storage limits without reproducing that mismatch;
the prior failure remains open. The local loop now checks full diagnostics
headroom before booting. [Current UV investigation and evidence](research/20260907_1638_uv-boundary-provenance.md).

**Scene and corrected shadow cutouts pass live reload; pixels still open:**
direct alpha-tested rigid materials now have owned, ordered model cutoffs,
object/pass overrides and blend inputs feeding the native scene shader and queue.
This preserves blended cutout ordering and depth writes; it does not assume a
fixed cutoff or treat translucent siblings as opaque. Host115/run955 adds **1,198
textured scene-cutout emissions** in a fresh 300-frame cold-field window; existing
cold-field gates and old-generation teardown pass, but reload and pixels remain
unqualified. Its zero-texture shadow counts do not prove authored cutout coverage.

Recovered shadow shader/control flow now shows why textured casting was absent:
ordinary textured phase1 casters take the deferred route, which native admission
rejected. Source now connects that depth-only family to the native queue and uses
the authored **fixed texture-alpha cutoff of 0.6**, without scene/object/vertex
alpha or generic cutoff state. Zero-texture casters use the solid program; the
unnecessary alpha-only shader is removed. Host116 contains the correction.
All46 GPU cases and the full cold/reloaded-field checks pass in run956: each
fresh300-frame epoch emits and retires **300 textured shadow casters**, alongside
textured scene cutouts. Old source/GPU generations retire correctly.

The inspected image shows gaps between nearby tree-trunk sections, so visual
acceptance remains open. The prior baseline has a different camera position;
the cause needs a matched-state investigation, not a passing-counter inference.
No speedup claim; real batches are still singletons.
[Cutout contract, verification and remaining gates](research/20260907_1713_native-scene-cutouts.md).
[Shadow connection and current verification](research/20260907_1742_native-cutout-shadows.md).
[Cutoff fix, live cutout counts and pending gates](research/20260907_1838_cutout-integration.md).
[Corrected contract, live reload and open pixel issue](research/20260907_1927_corrected-shadow-coverage.md).

**Earlier native-bounds/query checkpoint (now superseded):** Host120 derives
indexed bounds from owned mesh data before GPU upload and transforms them with
the exact native object matrix. This removes the walk's radius-scale handoff
from native occlusion; primitive identities keep sibling visibility independent.
Legacy-only nodes still consume no query/history capacity. The old translated
query shader/upload and address-keyed draw filter remain removed.

Run960 records24,011 submitted/collected queries and **1,026 native primitive
skips**, but times out before reload qualification. **Host121/run961 now passes
the full strict cold/reload checks with those native bounds**, including900 fresh
scene and shadow emissions in each epoch and old-source/GPU retirement. Its
bounded readiness logging sees no reset; run960's timeout is not explained away.

330 Python checks and the expanded CPU fixture pass; the unchanged query shader
retains its eight two-fence Vulkan cases with zero validation errors/warnings.
Run961 records304 skips, but none advance in its sampled interactive-field
windows. The separate image observation therefore does not qualify, and the
overall run exits1 for the missing requested image. **Game pixels, moving-view
visibility, tree-gap investigation and stereo culling remain open.** No timing
gain or complete host-owned frame is claimed; no safety checks were relaxed.
[Indexed bounds, strict reload proof and remaining visibility gates](research/20260907_2107_native-indexed-bounds.md).

**Current-depth GPU foundation now verified:** maximum-depth pyramid shaders and
GPU-generated indexed commands pass72 Vulkan cases, including perspective views,
1x/2x/4x/8x MSAA, changing occluders/cameras in one recording, source retirement
and malformed-result rejection. Generated commands execute before any CPU
readback; fence-delayed receipts distinguish culled, drawn and merely generated
instances. Shared work-buffer limits and ordered scratch reuse are implemented.
46 existing rigid-shader cases and8 query regressions also pass; GPU fixture
validation reports zero errors/warnings.
[Current-depth implementation and GPU evidence](research/20260907_2209_native-depth-visibility.md).

**Host123/run962 connects current-depth GPU visibility to real native rigid
draws and removes production temporal query history.**339 Python checks, the
expanded C++ fixture and the host build pass. Both strict cold/reload epochs
pass with900 actual visible scene/shadow emissions each and source/GPU retirement.
A fresh reloaded300-frame window generates3,248 commands and fence-classifies
3,236 visible plus11 culled instances; pending work is accounted separately.
GPU-zeroed commands do not count as visible output.

**Visual acceptance failed:** the saved window image shows the title logo, despite
the field/camera/movement log context. Keep it as failure evidence; do not infer
whether this is stale window capture or a presentation defect yet. Controlled
culling pixels, the tree gap, full scene/
stereo coverage and speedup remain unqualified.
[Runtime connection, checks and image discrepancy](research/20260907_2256_current-depth-runtime.md).

**Frame-provenance follow-up (2026-09-08):** screenshot buffers now retire at the
real GPU slot fence, retaining frame/request/source identities. Eight Vulkan
pixel cases pass. Host125/run963 passes strict cold/reload checks and records
frame5070, but its truncated JPEG is rejected rather than accepted as pixels.
Host126 fixes bounded JPEG output-length tracking and colour subsampling;
350 Python checks and the full-resolution encode/decode fixture pass. Run964
passes the strict reload checks, but the bounded encoder/export refuses its
1920x1080 probe; no new image is written. The proposed larger per-image limit
still needs approval and room under the unchanged aggregate image budget.
Valid post-gamma pixels and the original title-logo diagnosis remain pending.
[Frame ownership, encoder correction and remaining gate](research/20260908_0006_native-frame-provenance.md).

**Host127 removes dirty-flag dependence from immediate GPU geometry binding.**
After queued/native/instanced work, immediate draws now bind their complete
pipeline, vertex streams and index view without rebuilding cached pipelines or
importing guest state. The C++ fixture,351 Python checks and52 two-eye Vulkan
cases pass with zero validation errors/warnings. Three independent stale-input
controls lose geometry; production rebinding restores the expected pixels.
The desktop build passes without guest object compilation. Live game acceptance
is still pending; this is not a proven fix for the earlier title-logo/tree gaps.
[Handoff contract, causal fixtures and remaining gates](research/20260908_0033_immediate-geometry-bindings.md).

**Earlier Host128 deferred-packet groundwork:**
Native plans now retain sorted depth, cutoff and depth-write policy; mixed-order
metadata preserves submission ties. Lighting captures an owned Bind/Keep action
and resolves inherited values at the final draw position. The existing direct
lighting path shares that implementation. Three C++ fixtures,351 Python checks
and55 two-eye GPU cases pass, including overlapping blend-order controls.
At that checkpoint the deferred producer/consumer still used the compatibility list: its
participant callbacks and late material/pass updates must be accounted for before
native submission is enabled. No new guest path removal or game-pixel acceptance
is claimed by this checkpoint.
[Deferred contracts, validation and the remaining connection](research/20260908_0109_deferred-packet-order.md).

**Host129 connects an object-scope-independent native scene consumer.** The
current direct route hands over owned geometry, images, materials, matrices and
native identities; submission no longer needs the producing pose/object scope.
Bind/Keep lights are finalized together at consumption, with stale publication
and partial-sibling guards. The same consumer retains packets through the existing
indirect queue/fence path and honors their depth-write policy. Two C++ fixtures,
352 Python checks,55 two-eye Vulkan cases (validation0/0) and the host link pass.
The sorted producer was still unconnected at that checkpoint. No new game run, accepted pixels, speedup or full native frame
is claimed. [Owned consumer and remaining integration](research/20260908_0135_owned-scene-consumption.md).

**Host130 connects ordinary rigid sorted packets to the native consumer.**
Owned packets now merge with legacy entries in one stable depth order, without
allocating guest render-list entries for native work. Their native branch skips
per-entry material callbacks, resource/world/constant imports and D3D draws.
Lights resolve at actual submission; blend and receiver colour come from native
publications after the remaining visual transitions. The live callback contract,
queue capacity, frame and single-consumption rules are checked explicitly.
Two changed C++ fixtures and the host build pass. **Host130/run966 now passes
the strict cold-field and real title/reload checks with this route enabled.**
Independent300-frame windows stage and consume10,815 native sorted packets in
the cold field and3,825 after reload, alongside continuing legacy draws. Old
generation93/instance144 retires; the new field uses207/388. These are repeated
packet visits, not unique assets or deferred-specific GPU retirement counts.
The reusable scenario guard passes357 Python tests and rejects stale/one-sided
windows, counter drift, fallback and runtime failures. The route remains opt-in
(`bd_native_rigid_deferred`): mixed-order game pixels and both eyes are still
unqualified. Visual-transition and outgoing compatibility adapters remain;
this is not a complete host frame or a measured speedup.
[Connection, callback evidence and next acceptance gate](research/20260908_0206_native-deferred-connection.md).
[Live cold/reload evidence and retained logs](research/20260908_0225_native-deferred-live.md).

**Host132 removes callback dispatch from native-opened ordinary visual scopes.**
Setup/cleanup runs directly through the existing host receiver, blend and effect
owners; native packets consume retained, frame-checked effect values. Late receiver
colour and legacy material restore are preserved without feeding compatibility
staging back into native materials. Two rebuilt C++ fixtures,32 CPU tests,360
Python checks and the incremental host build pass. **Run967 passes the strict
cold-field/title/reload chain:** sampled windows consume10,957/2,758 native packets
through3,383/234 balanced native visual scopes alongside legacy draws. Source and
GPU lifetime checks pass independently in both epochs. Pixels/both eyes remain
unqualified; no speedup is claimed. In host132, authored-input sidecars, outgoing legacy state
and legacy-opened scopes remain; the route is still opt-in.
[Connected effects, contracts and verification](research/20260908_0251_native-deferred-effects.md).

**Host134/run969 pass writer-ordered input cold/reload checks:** deferred packets
now use native instance/generation identities instead of a per-entry guest-address
sidecar. The existing instance index publishes bounded blend/class values at the
late list handoff; receiver publications use the same native keys. Run968 exposed
a rejected water resource callback before field readiness. The existing water
writer now republishes the native inputs after its complete effects, preserving
late indirect writes instead of freezing the batch. Native omitted callbacks
still require no-ops; unknown writers still refuse.32 CPU tests,364 Python checks
and the incremental host build pass. Run969 independently passes both field
epochs:10,865/3,053 native packet reads with175/139 completed-writer refreshes,
zero pending native packets and correct source/GPU retirement. No new crash dump
or captures; the profile is restored. Pixels/full-frame/both-eye acceptance and
speedup remain unproven. Source-input imports, receiver descriptors and outgoing
state remain. Water was still a legacy draw family at that checkpoint; the current
native draw/material connection is described above.
[Writer-ordered connection and causal failure](research/20260908_0337_native-visual-inputs.md).

**Native water queue and bottom-depth foundation:**
the existing native draw store/emitter now supports water's own instance format,
six retained image/view owners, consecutive ordered instancing and fence
retirement. Load-owned meshes supply tangent inputs and indexed wave bounds;
sampling an active attachment or MSAA resolve is refused. Depth/eye sorting,
blended gathering and legacy prepasses respect ordered native barriers. No second
queue or translated constant-gather path was added.

Live scene, shadow, post and snapshot publishers now retain their own sampling
views in the shared native image lease. The water queue accepts those existing
target/post/resolve owners directly; no duplicate GPU images or adapter-owned
views are needed. The GPU fixture uses the real post-image pool and verifies
that a queued snapshot reader prevents pool overwrite.

The water-bottom pass now has a native depth/framebuffer/clear/publication path
without console surface allocation or resolve. It retains the actual producer's
world projection; camera fitting and caster/sampling adapters remain. Real D32
sampling exposed a layered deferred-clear bug, now fixed in the Plume fork.

The latest shader evidence covers 20 two-eye water cases, 55 rigid cases and eight
snapshot cases, with zero GPU validation errors/warnings. Game water now reaches
this queue, but the tested field has not exercised the authored bottom/snapshot
path. Authored water pixels, sequences, both-eye game coverage and speedup remain
unqualified. [Queue ownership](research/20260908_0509_native-water-queue.md),
[image handoff](research/20260908_0940_native-water-image-leases.md), and
[bottom pass, backend fix and current verification](research/20260908_1031_native-water-bottom.md).

The preceding host119/run959 passed the full strict cold/reload checks and
supplies fresh **multi-instance batching evidence**: its reloaded
300-frame window emits 9,515 native scene instances in 7,893 indirect calls, with
2,813 instances participating in merged groups. Earlier checkpoints saw only
singletons. This is real command batching, not whole-scene or pixel qualification
and not a controlled timing comparison; sampled layered scene draws remain zero.
[Occlusion implementation, evidence and next decision](research/20260907_2007_native-occlusion.md).

The handoff-owned lighting publication contains 2,898 node bindings; these are
not all verified native scene draws. Skin/deformation, phase0 cutout casting,
sorted/translucent materials,
normal/reflection and texture-dependent effects remain unconverted here.
Source tree, object/pass and other producer adapters also remain.
[Owned-lighting contract](research/20260907_1342_owned-scene-lighting.md).

Run941's separate legacy light-selection dirty-bit failure remains unresolved;
this passing run does not explain it. Its strict comparison and failure evidence
are preserved. Authored update producers, live inherited-binding acceptance, broader
material/object families, visual sequences and both-eye game checks remain.
[Preserved regression](research/20260907_1224_native-receiver-setup.md).

Earlier cutout verification:320 Python source/scenario checks, both expanded C++
fixtures and46 GPU modes pass. That GPU run takes1.30 s with zero validation errors/warnings,
including exact cutoff boundaries and overlapping casters in reversed order.
Earlier37/41-mode shadow results tested a superseded alpha model; scene modes0..22
remain unchanged. Host116/run956 passes both strict field epochs, but its new
image needs investigation; host107/run945 remains
the last accepted live/pixel result. These fixtures are not full-game stereo
or lifecycle acceptance. All acceptance switches remain off in the normal profile.

| Area | Reusable foundation | Still to finish |
| --- | --- | --- |
| Assets | Versioned native meshes/textures/materials, canonical rigid vertices, load-owned associations, bounded caches | Source-free consumers, remaining layouts, compact formats and bounded streaming |
| Scene and materials | Owned instance/primitive/light packets, multi-primitive native casters, live scene/textured-shadow cutouts through reload, native indirect draws and selected-object lifetime proof | Tree-gap pixel investigation, layered/inherited-light coverage, representative multi-instance groups, remaining source producers/material families and lifecycle coverage |
| Characters | Explicit joint bindings and current palette gathering | Native skeleton/skin assets, animation/pose production and full GPU skinning ownership |
| Frame, shadows, reflections | Native scene/post images, primary shadow lifecycle, pass scheduling and ordinary MSAA resolves | Remaining camera/light/participant producers, receivers, secondary shadows and reflection recipes |
| Effects and UI | Native post effects, effect lifecycle and sorted/deferred/immediate submission | Authored data/vertex producers, remaining callbacks, UI ownership and event coverage |
| Desktop VR | Layered multiview presentation and headless OpenXR runtime | Complete host frame and representative both-eye/animated-effect qualification |
| Quest 2 | Earlier APK/OpenXR/controller foundations | Desktop acceptance first, then device qualification, foveation and optimization |

Plume integration is at `2d206ee`. Remaining source lookups, register/resource
adapters, retained templates and compatibility scopes prevent claiming removal
of all Xbox 360 rendering machinery. Historical checkpoint detail belongs in
the [transition document](docs/HOST_RENDERER_TRANSITION.md) and linked research,
not a second chronological worklog here.

### How we finish faster

Work in **connected subsystem bundles**: producer, owned data, native consumers
and removal of the replaced interface. The owned-lighting connection is in place;
next are broader rigid scene ownership, material/character paths and remaining
frame producers.
Reuse the existing assets, owners, math, shaders and backend. The first object is
a regression case, not a permanent limit on scene-level development. Delete
compatibility at last use and complete the full desktop gate before Quest work.

The [active queue](docs/HOST_RENDERER_TRANSITION.md#active-work-queue) owns the
dependency order, concrete files and acceptance gates. Do not start another
renderer framework, bulk recook or unrelated adapter campaign. Use translated
source to recover behavior, not to reproduce every console helper one-for-one.

Use the smallest test that can falsify the change:

1. Define the connected deliverable and interface being removed. Reuse completed
   source findings; revisit changed or uncertain contracts, not every old helper.
2. Exercise their real representations in existing CPU/GPU fixtures. A failed
   runtime admission becomes a focused regression before another game boot.
3. Group related edits before an incremental host build and targeted live/pixel
   run. Every diagnostic needs a decision it can change. Reserve broad sequences,
   reloads and both-eye checks for integration milestones; do not omit them.
4. Commit and push each coherent verified checkpoint, replace superseded
   evidence and report the remaining boundary.

The capture-free source/scenario loop is:

```powershell
python -B tools/host_checks.py
```

Use repeatable `--area material`, `--list`, or `--all-boundaries` as appropriate.
These Python checks do not replace C++ fixtures or GPU pixels. The existing
`native_rigid_pixels` CTest exercises the production shader programs without
booting the game; build its `native_scene_snapshot_test` target only when its
code or shaders change. See the [dev-loop guide](.claude/skills/devloop/SKILL.md)
for the configured trees and storage-supervised build/run rules.

The current 310-check source/scenario suite takes 0.146 seconds. Automated menu
tests temporarily disable mouse hover as well as owning the pad, and title-menu
Exit fails immediately instead of being treated as a pending field. Reuse the
existing binary and logs when they answer the question; an intermittent failure
needs a targeted writer/ordering observation, not repeated boots until one passes.

### Evidence limits and performance

There is **no defensible overall conversion percentage or measured speedup**.
Host-call counts and fewer imported words are not FPS gains. The recorded
desktop field median of 16.667 ms (~60 FPS) was frame-limited; it does not
establish the benefit of the conversion or predict Quest performance.
[Measurements and limitations](research/20260906_1323_native-visual-schedule.md).

The desktop gate still includes fields, battles, cutscenes, menus, transitions,
reloads, animated effects and both eyes. Later-scene scenery/text failures,
cliff artifacts, distant blur, VR character shadows, title artwork, per-eye
optics and special effects remain unresolved or unqualified. A single field
image or tiny stereo fixture does not supersede those requirements.
Experimental native sun-camera fitting remains disabled by default.

## Project documentation

- [Host renderer transition](docs/HOST_RENDERER_TRANSITION.md): active scope,
  ordered work queue, completion requirements and checkpoint history.
- [AGENTS.md](AGENTS.md): canonical instructions, storage budgets and standing
  approval for frequent scoped commits/pushes. [CLAUDE.md](CLAUDE.md) imports it.
  Detailed [disk-space policy](docs/DISK_SPACE_POLICY.md) is required before
  artifact-producing work or cleanup, not a mandatory read for status checks.
- [Native material format](docs/NATIVE_MATERIAL_FORMAT.md),
  [native texture format](docs/NATIVE_TEXTURE_FORMAT.md) and
  [host upload arena](docs/HOST_UPLOAD_ARENA.md): native data contracts.
- [Research](research/): dated evidence, including unresolved failures; historical
  observations are not current promises.
- [Original VR plan](docs/VR_PORT_PLAN.md) and
  [archived notes](docs/archive/CLAUDE_2026-09-04.md): historical context,
  superseded wherever they conflict with the current transition.

## Desktop verification

The main desktop loop uses the Vulkan executable and
[the repository's headless OpenXR runtime](.claude/skills/vrsim/SKILL.md).
It exercises the VR path without a headset; it cannot prove Quest performance,
device-only foveation or comfort.

Desktop settings go in `profiles/default/reblue.toml` under the install root.
Use `bd_xr_autoplay` for readiness-driven field walking and `bd_capture_after_s`,
`bd_capture_min_draws` and `bd_capture_frames` for capture sequences. Verify the
live settings in the log and inspect the actual images.

Autoplay's bounded `[autoplay]` records distinguish readiness, stick activity
and observed displacement. `tools/native_instance_scenario.py --movement`
requires movement during fresh post-event native-instance verification windows;
an enabled setting or a stationary character does not pass. Keep raw captures
off for text diagnostics and enforce the storage limits before image sequences.
The opt-in `bd_native_rigid_reload` extends that same runner with a real title
round trip. `tools/native_instance_scenario.py LOG --rigid-reload` checks both
field epochs independently and requires fresh model/instance identities plus
actual old source and GPU-fence retirement. Pixels remain a separate check.

- `tools/capture_seq.py` flags neighbouring-frame changes.
- `tools/capture_cyan.py` checks a known visual artifact.
- `tools/stereo_check.py --raw <capture> --stacked` examines layered stereo.
  Featureless black bars/sky are inconclusive, not proof of depth.

Multiview is the target stereo path. Do not enable legacy side-by-side
`bd_stereo` alongside `bd_stereo_multiview`. Capture the final presented eyes
when qualifying presentation; `bd_mv_capture_array` selects a scene target
instead.

## Table of Contents

- [Renderer Goal](#the-goal-desktop-host-renderer-then-quest-2-vr)
- [Current State](#current-state)
- [Project Documentation](#project-documentation)
- [Hardware Requirements](#hardware-requirements)
- [How to Install](#how-to-install)
- [Features](#features)
- [FAQ](#faq)
- [Building](#building)
- [Credits](#credits)
- [License](#license)

## Hardware Requirements

Requires all three retail Blue Dragon discs or their disc images. The desktop requirements below are inherited upstream baselines, not a fresh qualification of this experimental renderer. Android and Quest remain unsupported development targets.

### Minimum

- OS: Windows 10 version 1909 or later, Ubuntu 24.04 / Fedora 40 / SteamOS 3.6 or later, or macOS 13.3 Ventura or later
- Processor: Intel Core i5-4460 3.2 GHz 4 Core or AMD Ryzen 3 1200 or Apple M1, or equivalent
- Memory: 8 GB RAM
- GPU: Nvidia GTX 1050 Ti or AMD RX 570, or equivalent performance & VRAM. DirectX 12 with Shader Model 6.0, or Vulkan 1.2, or Metal
- Storage: 15 GB available space

### Recommended

- OS: Windows 11, SteamOS 3.6, or macOS 14 Sonoma or later
- Processor: AMD Ryzen 5 5600X or Intel Core i5-12400 or Apple M2, or equivalent performance, 6 physical cores minimum
- Memory: 16 GB RAM
- GPU: Nvidia RTX 2060 or AMD RX 5700, or equivalent performance & VRAM. 8 GB VRAM for 4K with MSAA
- Storage: 15 GB available space

## How to Install

This fork publishes no releases. [Download the latest upstream release for your platform](https://github.com/zolaware/reblue/releases/latest) or [build yourself](#building).

1. Blue Dragon shipped on three DVDs, and you will need a disc image of each one from your own copy of the game.

2. Run the executable. A setup wizard will guide you through the rest. You will be asked to point it at each of the three disc images in turn, and it will check each one before letting you continue. Once you pick where to install, the program copies itself there and restarts from that location, so you can delete the folder you extracted the zip into.

3. Pick a graphics quality preset. The wizard copies the game files out of the discs, and you are done. You may also install DLC from this installer or from the main menu under the config menu

The wizard only needs to run once. If something later goes missing from your install, launching with `--repair` reopens it on your existing install and copies back only what it needs.

## Features

These features are inherited from upstream re:Blue. The host-renderer transition is still in progress; this list is not a claim that every feature has been reverified in this fork.

### Graphics

- Resolutions up to 4K, windowed or fullscreen, on whichever monitor you pick
- Aspect ratios 16:9, 4:3, 16:10, 21:9, 32:9, plus auto and stretch
- Four quality presets, Low through Ultra
- MSAA up to 8x or SSAA up to 4x
- Anisotropic filtering
- Shadow quality and draw distance
- Depth of field adjustment
- Unlocked FPS with optional caps and VSync

### Quality of Life

- Unlocked frame rate, with optional caps at 30, 60, 90, or 120
- Save from the camp menu anywhere instead of only at save points
- Field of view adjustment, 45 through 120 degrees
- Skip the in-game tutorial pages
- Full area map on the world map screen, with zoom, floor switching, and a legend
- Optional map markers for the hidden items, chests, and barriers a floor still has, plus per-floor counts, carried onto the field compass
- The field HUD can fade out once you stop pressing anything, or stay off entirely
- Achievement list viewable in game, with eight new re:Blue achievements alongside the original ones
- Master volume control
- Separate center, rear, and subwoofer levels for 5.1/7.1 tuning
- Fully native keyboard and mouse support with cursor and look modes supported by mouse
- Every controller button rebindable to a key, with mouse sensitivity and cursor opacity of your own
- Menus take the mouse directly: hover a row to move the cursor, click to confirm, wheel to scroll
- Custom input based icons/glyphs for hud elements, following the device you last used or pinned to Xbox, PlayStation, Switch, or Steam Deck
- UI language and voice language chosen separately


### Mods and DLC

- Built-in mod manager
- Official DLC is supported

### Platforms and Languages

- Windows on DX12 or Vulkan
- Linux AMD64 and ARM64, including the Steam Deck and other handhelds
- macOS AMD64 and ARM64
- Custom menus in English, French, German, Italian, and Spanish

## FAQ

### Where is my save data and configuration stored?

Everything lives under the folder you installed to:

- Saves and settings: `profiles\default\`
- Your configuration file: `profiles\default\reblue.toml`
- Game files copied from your discs: `game\`
- Mods: `mods\`

### I want to update the game. Will I lose my save data?

No. Copy a newer build over your existing installation and your saves, settings, and mods are left alone. You do not need to reinstall or point the wizard at your discs again.

### How do I install mods?

Use the mod manager in the config menu. It accepts a mod folder or a zip file and puts everything in the right place for you

### Can I keep more than one set of saves?

Yes. Each profile is its own folder under `profiles\`, holding that profile's saves, settings, achievements, and DLC toggles. Launch with `--profile <name>` to pick one, and anything but `default` starts out fresh.

## Building

The build needs CMake, Ninja, a C++23 Clang toolchain, the
[ReXGlue SDK](https://github.com/rexglue/rexglue-sdk), the checked-out submodules
and `assets/default.xex` from your own game disc. Windows also needs vcpkg
(including DXC). Game executables, generated guest code and cooked game assets
are not distributed in this repository.

Read the [dev-loop guide](.claude/skills/devloop/SKILL.md) for SDK/bootstrap
guidance and [AGENTS.md](AGENTS.md) for current rules. The guide now starts with
the focused desktop loop; a Vulkan-only build's target is `reblue`, not `reblue_vk`.

For the **already configured workspace** used by this fork:

```powershell
$env:PATH = 'C:\Program Files\LLVM\bin;' + $env:PATH
$env:VCPKG_ROOT = 'C:\vcpkg'
cmake --build --preset win-amd64-release --target reblue -j 4
```

That tree is configured with `REBLUE_D3D12=OFF`, `REBLUE_OPENXR=ON` and PCH on;
its output is `out/build/win-amd64-release/reblue_vk.exe`. These are local
configuration choices, not the untouched preset defaults. Reuse an existing
tree rather than rebuilding the guest to test host changes.

For a fresh desktop configure, bootstrap the SDK/codegen and dependencies first,
then select a Vulkan-only preset and configure OpenXR headers/loader as described
in the [vrsim guide](.claude/skills/vrsim/SKILL.md):

```sh
cmake --preset win-vk-release -DREBLUE_OPENXR=ON
cmake --build --preset win-vk-release --target reblue
```

[CMakePresets.json](CMakePresets.json) also includes Linux/macOS and
`android-arm64` presets. Android requires a cross-built SDK plus host-native
codegen/shader tools; `tools/build_apk.sh` packages the APK. Their existence is
not a claim that this revision has been qualified on each platform. Quest runs
remain deferred until the complete desktop host-renderer gate passes.

Standalone checks for the current mesh, material, texture/lifetime and stereo work:

```sh
cmake -S tools/native_mesh_test -B out/native_mesh_check -G Ninja
cmake --build out/native_mesh_check
ctest --test-dir out/native_mesh_check --output-on-failure
cmake -S tools/native_material_test -B out/native_material_test -G Ninja
cmake --build out/native_material_test
ctest --test-dir out/native_material_test --output-on-failure
cmake -S tools/native_texture_test -B out/native_texture_test -G Ninja
cmake --build out/native_texture_test
ctest --test-dir out/native_texture_test --output-on-failure
python tools/stereo_check_test.py
python tools/reflection_lock_order_test.py
```

Use the configured Clang toolchain (on Windows, supply `CMAKE_CXX_COMPILER` and
`CMAKE_RC_COMPILER` if needed). The Python stereo tests require Pillow.

## Credits

Huge thanks to everyone who has put time into this. re:Blue would not be where it is without you.

**None of these people work on this fork, and none of them should be contacted about it.** The
credits below are upstream's, kept because they earned them and because the license says to keep
them. Everything re:Blue actually is came from [zolaware/reblue](https://github.com/zolaware/reblue);
everything broken in this repo came from me and a language model.

### re:Blue Development Team

- **[crack](https://github.com/tomcl7)** project lead and developer

- **[rcold](https://github.com/RC0ld)** developer and has done an absurd amount for this project. A lot of re:Blue looks the way it does because of him.

### Playtesting and Support

- **[infernozotza](https://github.com/Zotza)** - Playtester 
- **baus.98** - Playtester
- **[wolfaeterni](https://github.com/Zolawolf)** - Playtester and French Translations 
- **[griever666.](https://github.com/grv666)** - Playtester
- **[fungus](https://github.com/fungoid-creature)** - Playtester
- **[graine25](https://github.com/Graine25)** - macOS and Linux Development Support
- **[zhyxeryz](https://github.com/Zhyxeryz)** - Playtester and German Translations
- **[Azar42](https://github.com/Azar42)** - Playtesting
- **[ZolaKluke](https://github.com/ZolaKluke)** - Playtester
- **[emersed](https://github.com/RaphyEmersed)** - Playtester
- **[mrcmunir](https://github.com/mrcmunir)** - Spanish Translations
- **[mystixor](https://github.com/mystixor)** - German Translations
- **[toby](https://github.com/TbyDtch)** - Graphic Design

### Special Thanks

- The **[ReXGlue SDK](https://github.com/rexglue/rexglue-sdk)** team, for the toolchain this project is built on.

- The **[hedge-dev](https://github.com/hedge-dev)** team, for [XenosRecomp](https://github.com/hedge-dev/XenosRecomp) and for blazing the trail for Xbox 360 recompilations with [Unleashed Recompiled](https://github.com/hedge-dev/UnleashedRecomp).

- The wider **Xbox 360 emulation scene**, and the [Xenia](https://github.com/xenia-project/xenia) project in particular. A lot of the hardest problems were solved long before this project started.

## License

See [LICENSE](LICENSE).
