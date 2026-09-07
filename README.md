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

**The full host-renderer transition is not complete.** Gameplay stays statically
recompiled; the local generated executable contains 18,777 function bodies, not
the original high-level source project. There is no defensible conversion
percentage based on function or host-draw counts.

Latest desktop checkpoint (2026-09-06): **2,973 primitive shadow-receiving policies
are load-owned**, removing control-table reads from their native draw adapter.
The material fixture, host65, 157 source guards and 39 scenario tests pass.
Field run913 adds 15,019 matching receiver checks and 43,037 composed replays,
alongside canonical geometry, native pulling and matching material/pose checks.
One full-resolution image was inspected after movement. No new raw captures,
performance CSVs or asset-cache files; superseded sanity outputs were removed.
This is material ownership within the transitional draw path, not direct native
scene/shadow submission. [Evidence](research/20260906_2344_load-owned-shadow-policy.md).

Canonical geometry owns named values and immutable vertex inputs independently
of the imported declaration. Unsupported layouts still use transitional packed
data. **Source-free GPU loading was not exercised; shader-register ABI, source
lookup and retained draw templates remain. This is not direct static drawing,
complete sequence/both-eye qualification or a measured speedup.** Float4 storage
is an initial checked representation, not the final compact headset format.
[Format and remaining work](docs/NATIVE_MESH_FORMAT.md).

The desktop test loop now starts walking after verified field readiness instead
of waiting a fixed 150 seconds. A roughly 61-second run observes actual player
displacement, fresh native-component checks and three inspected motion images;
walking starts on observed readiness, not a guaranteed boot time. This improves short
field coverage, not renderer ownership. Reloads, longer sequences, cliff-edge
artifacts, distant blur and both-eye qualification remain open.
[Test-loop evidence](research/20260906_2120_readiness-driven-autoplay.md).

The preceding texture-table checkpoint publishes native image
leases after completed synchronous/asynchronous loading, with atomic image/table
publication and generation-safe replacement/retirement. The field has 5,736
tables /2.07 MiB. Comparison adds 22,326 matching lookups and 22,006 matching image
checks; the new pulling-enabled field check adds 22,333 normal lookups with zero
original comparison or fallback calls. Source selection/return ABI, dynamic
overrides and remaining resource consumers are still adapters; the direct native
reflection consumer was not exercised. **Broader movement/reload sequences, both eyes and
full-game qualification remain open.**
[Texture-table implementation and evidence](research/20260906_1955_native-texture-tables.md).

Native instance IDs and immutable render poses feed host traversal/replay after
the final handoff, including late edits. The pulling-enabled field check adds 118,987
matching pose reads with no misses/refusals. Original pose calculation/copy,
secondary palettes, source lookup and retained draw templates remain.
[Instance implementation and evidence](research/20260906_1850_native-instance-render-poses.md).

The preceding load-owned geometry path supplies 2,973 primitive geometries and
their material associations. Its earlier field check added 51,785 native-handle
draws and 2,700 matching geometry checks. Canonical rigid layouts are now exercised
above; complete native object/texture/pass records, source-free GPU loading and
direct scene/shadow submission remain next.
[Geometry implementation](research/20260906_1743_load-owned-model-geometry.md).
The [corrected loader/field observations](research/20260906_1638_field-state-observations.md)
remain the scenario gate; water activity alone is not a field identifier.

The prior [Toon checkpoint](research/20260906_1505_native-toon-materials.md)
retains a normal flat standing-scene image and desktop XR parameter comparisons.
Its water/camera activity markers do not independently prove interactive-field
execution; full-game and both-eye pixel qualification remain open.

Native mesh persistence now has independent 256 MiB /16,384-file limits, a
20 GiB free-space reserve and a non-waiting writer lease. All 3,510 existing
mesh files load unchanged in a source-free, read-only check; host/storage tests
pass. This closes the disk-growth prerequisite for load-time geometry work,
not geometry/instance ownership. [Evidence](research/20260906_1701_native-mesh-storage.md).

| Area | Implemented foundation | Ownership still required |
| --- | --- | --- |
| Assets | Persistent, versioned `.bdmesh`, `.bdtex` and `.bdmat`; canonical named rigid vertices, geometry-owned runtime inputs, primitive material associations and texture tables, shared GPU data, mip cooking, generated LOD support and bounded owners | Complete native object texture/pass associations and source-free consumers; remaining packed/dynamic layouts, compact assets and streaming/backpressure |
| Scene submission | Host traversal/replay, native instance identities/render-pose snapshots, packet intent, frustum/occlusion culling, instancing, vertex pulling and indirect draws | Complete native object/update production; replace source lookup, retained guest draw templates and remaining resource dependencies |
| Materials | Native material assets, load-owned primitive recipes and shadow policy, lighting/state producers, parameter storage, pass binders, water and Toon callbacks | Native geometry/texture/lighting associations, live texture overrides and draw routing, all recipes, bool/sampler inputs; remove temporary source index, shader-register ABI, mirrors/getters and remaining callbacks |
| Characters | Explicit per-draw joint bindings and host-owned current palette gathering | Native skeleton/skin assets, animation/pose production and complete GPU skinning ownership |
| Frame, shadows and reflections | Host view/pass scheduling, native scene attachments/framebuffers, ordinary MSAA resolves, image snapshots and sun-shadow lifecycle | Native scene/camera/light/participant producers, secondary shadows, reflection recipes and remaining getter/compatibility scopes |
| Effects, post and UI | Native post images and many post effects; host effect lifecycle, sorted/deferred scheduling and immediate vertex submission | Authored effect/vertex producers and storage, remaining callbacks, UI ownership and event coverage |
| Desktop VR | Layered multiview presentation, native eye extents and headless OpenXR test runtime | Complete host frame, broader both-eye/animated-effect qualification and remaining modern-GPU/VR requirements |
| Quest 2 | Earlier ARM64/APK and OpenXR/controller foundations | Full desktop gate first; then fresh device qualification, foveation and optimization |

Existing native scene/post integration is published with Plume `3094b35`.
Normal supported paths own source/resolve images and their fence-retained
lifetimes without inferred EDRAM sources or seed copies. Unconverted scopes
and consumers still prevent claiming removal of all console rendering machinery.

### Next ownership milestones

The dependency-ordered queue is maintained in
[Host renderer transition](docs/HOST_RENDERER_TRANSITION.md#active-work-queue).
Work is organized around complete producer-to-consumer paths, not isolated
callback counts:

The [static-model dependency map](research/20260906_1531_static-model-ownership-frontier.md)
identifies the existing load-time integration points and remaining template/data
dependencies. The source-index tool now exposes indirect and hook boundaries;
this tooling checkpoint does not itself convert additional rendering.

1. Complete one real native static-object path from cooked geometry/materials
   and instance updates through direct scene/shadow submission. Establish
   native shader/texture/pass contracts on the canonical rigid layouts; remove its
   guest-renderer warm-up, source lookup and captured templates. Verify movement
   and reload behavior, then expand material families.
2. Complete character asset, pose, joint-palette and GPU skinning ownership.
3. Finish dynamic geometry, effects, UI and remaining reflection/pass producers.
4. Remove unused compatibility machinery and complete the representative desktop
   gate before Quest 2 work.

Small, coherent, verified commits and pushes remain the default. Focused CPU
fixtures and incremental builds form the inner loop; rendering changes still
need appropriate GPU/pixel checks. Startup-only counters or an empty effect
queue do not qualify an authored field/effect path.

### Evidence limits and performance

The full gate still includes fields, battles, cutscenes, menus, transitions,
reloads, animated effects and both eyes. Earlier later-scene scenery/text
failures remain unresolved evidence, not superseded by a standing-field smoke
image. VR character shadows, distant blur, title artwork, per-eye optics and
special-effect coverage also remain unqualified. Experimental native sun-camera
fitting remains disabled by default.

The sorted-scheduler check reduced imported float words per native parameter
block from about 13.00 to 0.757 (94% less import work), **not a 94% FPS gain**.
Its desktop field median was 16.667 ms (~60 FPS), with 6.610 ms `other_ms` and
5.677 ms GPU time. No controlled overall speedup or Quest performance result
is established. [Measurements and limitations](research/20260906_1323_native-visual-schedule.md).
The latest vertex-input checks are correctness evidence, not performance benchmarks.

## Project documentation

- [Host renderer transition](docs/HOST_RENDERER_TRANSITION.md): active scope,
  ordered work queue, completion requirements and checkpoint history.
- [AGENTS.md](AGENTS.md): canonical instructions, storage budgets and standing
  approval for frequent scoped commits/pushes. [CLAUDE.md](CLAUDE.md) imports it.
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
details and [AGENTS.md](AGENTS.md) for current rules. Some older setup passages
in the guide are historical; in particular, a Vulkan-only build's target is
`reblue`, not `reblue_vk`.

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
