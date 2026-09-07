---
name: devloop
description: Run focused reblue host checks, incremental desktop builds and readiness-gated verification while preserving existing build trees, profiles and storage budgets.
---

# Host development loop

[AGENTS.md](../../../AGENTS.md) owns scope, storage limits and Git authorization.
[The transition queue](../../../docs/HOST_RENDERER_TRANSITION.md#active-work-queue)
owns current work. Desktop host rendering and its complete acceptance gate come
first; historical device experiments are not the default loop.

## Select the smallest useful check

- Status/docs: source, existing evidence and `git diff --check`; no build or run
  merely to restamp evidence.
- Rigid-object source work: `python -B tools/host_checks.py`. Repeat `--area` with
  `model`, `geometry`, `material`, `instance` or `scenario` for focused checks.
  `--list` explains the selection without importing tests. Default is fail-fast;
  no logs/caches, C++ builds or renderer launches.
- Broader integration: `python -B tools/host_checks.py --all-boundaries`. These are
  Python source guards and scenario-parser tests, not behavior/pixel qualification.
- C++ changes: build/run the relevant existing fixture. Shader changes also need
  affected shader compilation and GPU/pixel checks; CPU math does not prove GPU parity.
- A coherent renderer change: incremental host build and a bounded, targeted run.
  Do not run the full matrix per scalar edit, or weaken required sequence,
  reload, authored-effect and both-eye acceptance.

## Reuse the configured tree

Verified local tree: `out/build/win-amd64-release`, D3D12 off, OpenXR on, PCH on.
Inspect its actual CMake cache when configuration may have changed.

```powershell
$env:PATH = 'C:\Program Files\LLVM\bin;' + $env:PATH
$env:VCPKG_ROOT = 'C:\vcpkg'
cmake --build --preset win-amd64-release --target reblue -j 4
```

Target **reblue**, executable **reblue_vk.exe**. The second target `reblue_vk`
exists only in dual-backend builds, which OpenXR does not use. Never build the
default target, wipe a tree or rebuild guest objects for host-only edits.
Unexpected guest compilation requires inspecting codegen inputs. Keep PCH for
small incremental changes; do not incidentally switch to compiler caching.

| Changed behavior | Existing tree | Target / test |
| --- | --- | --- |
| Material/model/instance ownership, policy, light/fog math | `out/native_material_test` | `native_material_test` / `native_material_data` |
| Canonical mesh data/persistence | `out/native_mesh_check` | `native_mesh_test`; inspect its CTest list |
| Textures, bindings, scene/post owners | `out/native_texture_test` | Select the relevant target/test in its CMake files |

For example, after the current checkpoint's storage preflight:

```powershell
cmake --build out/native_material_test --target native_material_test -j 4
ctest --test-dir out/native_material_test -R '^native_material_data$' --output-on-failure --timeout 30
```

These commands select work; they are not storage supervisors. Reuse the current
bounded wrapper/ledger when available or enforce the AGENTS limits in a scoped
supervisor. Ignored wrappers may be machine-specific: inspect actual defaults and
do not assume they exist on a fresh clone. Check the producer's real exit code,
not a pipeline's final command. A sandboxed Ninja stall is not permission to
launch duplicates or wipe a tree; inspect owned processes and use the available
execution-approval route when needed.

## Readiness-gated desktop verification

- Settings are flat TOML in `<InstallRoot>/profiles/default/reblue.toml`, not
  unverified command-line overrides. Inspect before launching: the local owner
  profile can enable 120 raw frames. Disable captures/perf/dumps for text
  diagnostics and verify effective `[config]` values. Enforce timeout and
  aggregate byte limits, restore exact original profile bytes and stop only
  owned processes in guaranteed cleanup. Retries share the checkpoint budget.
- Windows install root holds the executable and game mount. Registry record:
  `HKCU\Software\Zolaware\reblue\Install`, `InstallRoot`, `SchemaVersion=3`.
  Do not alter a working install just to verify source.
- `bd_xr_autoplay` uses observed interactive-field readiness.
  `tools/native_instance_scenario.py --movement` checks fresh post-event samples
  and displacement. Fixed delays, water activity and stick input alone are not
  coverage. Its current field gate is not a reload test.
- Inspect renderer-owned pixels. On Windows, use `PrintWindow`, not foreground
  screen copying. One image is sanity evidence, not a stable sequence or both-eye
  qualification. Budget compressed exports and raw sequences separately.
- For desktop OpenXR, read [vrsim](../vrsim/SKILL.md). Use the existing headless
  runtime with an absolute manifest library path; it cannot qualify Quest
  performance, comfort or device-only foveation.
- `other_ms` includes `xrWaitFrame` in XR. Vsync-locked FPS and tiny fence waits
  do not prove speedup; use the actual CPU/wait breakdown and GPU timers.

## Fresh checkout / another platform only

Use [README prerequisites](../../../README.md#building), checked-in presets and
the SDK version in `reblue_manifest.toml`. Supply legally owned
`assets/default.xex`; `tools/extract_xex.py` can extract it without copying a disc.
Reuse installed dependencies before downloading. Generated code is ignored, not
source to edit/commit. Do not bootstrap/codegen an already configured tree merely
to prove it is available.

Windows needs a Windows SDK slice and vcpkg/DXC; cached Android SDK paths cannot
link a Windows executable. Standalone fixtures may need explicit Clang and RC
paths with forward slashes in CMake arguments.

Android needs host-native codegen/shader tools and an ARM64 SDK. Historical
research is setup context, not current qualification. No Quest deployment or
optimization before the full desktop gate; AYN Thor is not a target. Never
uninstall an existing game package to update it.
