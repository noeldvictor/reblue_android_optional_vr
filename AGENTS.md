# Project instructions

This is the canonical, shared instruction file for coding agents in this
repository. `CLAUDE.md` imports it. Keep enduring rules here and in its linked disk policy, current progress in
[`docs/HOST_RENDERER_TRANSITION.md`](docs/HOST_RENDERER_TRANSITION.md), and dated
evidence in `research/`. Updated 2026-09-07.

**Delivery-first, storage-conscious:** remove complete rendering dependencies,
not one console helper per checkpoint. Use existing source, owners, assets and
tests; group connected changes before integration runs. Keep current priorities
in the transition queue, not a growing instruction-file worklog.

No build/run/capture is needed for a status or documentation request. Reuse
existing artifacts, preserve user data and obey the quick
[disk limits](#disk-space-discipline). Detailed storage rules are loaded when
output-producing work or cleanup needs them.

## Goal and boundaries

Move **all rendering** to a host-native, modern Vulkan renderer on desktop;
remove Xbox 360 rendering paradigms, then optimize the completed host frame for
Quest 2 VR. Gameplay remains statically recompiled; this is not a gameplay rewrite.

- Preserve Blue Dragon's recognizable art style and readability. Materials,
  lighting, geometry, effects and asset formats may change for performance.
- Host ownership includes frame scheduling, scene data, materials, animation,
  GPU skinning, shadows, reflections, effects/particles, post-processing, UI,
  stereo and presentation. Native meshes or a replayed draw list alone are not
  completion. Temporary compatibility paths must stay explicitly tracked.
- The finished frame must not execute guest rendering or translate per-draw
  D3D/Xenos state. Remove EDRAM allocation/tile matching, seed copies and emulated
  resolves; ordinary native MSAA resolves are a separate mechanism.
- Multiview, fixed foveated rendering, frustum/occlusion culling, batching,
  instancing, indirect draws and suitable modern Vulkan features are mandated.
  Build and verify them; do not benchmark them against legacy paths to decide
  whether to implement them, or quietly retain the old path because it is faster.
  Correctness-only on/off image comparisons are allowed.
- Cook assets on desktop into persistent, versioned native formats with stable
  identities, not guest addresses. Offline texture mips/compression, generated
  LODs, merged statics, atlases and impostors are approved. The recorded headset
  asset budget is 1.5 GB. Materials need a lighting-model slot for optional cel
  shading.
- Verify fields, battles, cutscenes, menus, transitions and reloads on desktop,
  including both eyes and animated effects, **before any Quest runs or Quest
  optimization**. The eventual target is 72 Hz at 1440x1584 per eye with shadows
  on. That is a target, not an achieved result. AYN Thor is not a test target.
- Do not buy FPS with unreadably low resolution. Desktop timings do not prove
  headset performance, comfort or device-only foveation support.

The full acceptance checklist is in
[`docs/HOST_RENDERER_TRANSITION.md`](docs/HOST_RENDERER_TRANSITION.md). Its scope
supersedes the older stages in `docs/VR_PORT_PLAN.md` and historical skill recipes.
Optional cel shading and tourist mode do not displace the renderer priority.

## Start work

1. Inspect `git status` and the relevant source before changing anything. Preserve
   unrelated user changes, game data, saves, profiles and existing build trees.
   Before producing large outputs, check the disk-space rules below and the
   retained artifacts from prior checkpoints; a new turn does not reset the budget.
2. Read the transition document's active queue for renderer work and the relevant
   subsystem evidence. Follow source-map links only for the boundary being changed;
   historical sections are references, not mandatory full reads.
3. Use the repository skills when applicable, reading the entire `SKILL.md` first:
   - [devloop](.claude/skills/devloop/SKILL.md): builds, runs and tests.
   - [guest-source](.claude/skills/guest-source/SKILL.md): guest/render-loader investigation.
   - [vrsim](.claude/skills/vrsim/SKILL.md): desktop OpenXR verification.
   Current owner scope, CMake definitions and verified local configuration take
   precedence over historical passages in skills/research. The devloop and
   guest-source skills were refreshed on 2026-09-07; vrsim still needs its
   historical setup claims checked. No skill authorizes Quest before the gate.
4. Make a bounded change, verify it in proportion to risk, and record what was
   actually built, run and inspected. Do not silently reduce the full goal to the
   latest milestone.

## Code and build rules

`src/` is host C++23. `config/functions.toml` names guest functions;
`config/hooks/*.toml` defines hook sites. `generated/` contains the statically
recompiled PowerPC program, not an interpreter or JIT. The rendering command
interpreter mentioned in research is distinct from CPU emulation.

- Never hand-edit or commit `generated/`, `assets/default.xex`, game assets or
  derived caches. Do not weaken their ignore rules. Work from legally owned data.
- Before binary/decompiler work, use the exact translated C++ and PPC comments in
  `generated/`; read the hook TOML before the callback. Recompiled rendering
  functions may be replaced via host hooks; generated source stays generated.
- Function replacements use `REX_HOOK` / `REX_HOOK_RAW`; instruction-site hooks
  use `config/hooks/`. Hook symbols must remain in OBJECT libraries, not STATIC
  archives that can discard them.
- Never rebuild the guest merely to test host changes, and never wipe a build
  directory. Build one target. If guest objects rebuild, inspect the codegen inputs.
- In a Vulkan-only configure (`REBLUE_D3D12=OFF`), the target is **`reblue`** and
  the Windows output is **`reblue_vk.exe`**. `reblue_vk` is a second target only
  when the dual-backend configuration creates it. OpenXR requires Vulkan-only.
- Reuse this workspace's configured desktop tree, `out/build/win-amd64-release`:

  ```powershell
  $env:PATH = 'C:\Program Files\LLVM\bin;' + $env:PATH
  $env:VCPKG_ROOT = 'C:\vcpkg'
  cmake --build --preset win-amd64-release --target reblue -j 4
  ```

  It currently has D3D12 off, OpenXR on and PCH on. Fresh-clone bootstrap is in
  the devloop skill; paths and installed prerequisites must be checked locally.
- PCH for incremental local edits; compiler caching with PCH off for broad rebuilds.
  Check the real build exit code, not a pipeline's last command or a stale log.
- Shader-translator changes require rebuilding the host XenosRecomp tool and
  regenerating the build-tree shader cache. Verify emitted HLSL/SPIR-V, not just
  the translator source. Invalidate only the exact affected generated artifacts.
- SDK/codegen and PCH changes can require explicit regeneration; inspect the
  dependency chain first. A cached Android SDK cannot link a Windows executable.
- Match surrounding formatting, retain license/copyright headers, use PascalCase
  types/functions and snake_case locals. Guest structures are big-endian; swap
  reads at the boundary. New native asset formats must define their byte order.
- Keep CPU copies of data needed by importers/tests. Never read back upload-ring
  `alloc.memory` or mapped write-combined GPU buffers as a CPU data source.
- `xr_math`, camera, culling and settings math must remain testable without
  OpenXR headers. Convert handedness once, in `FromOpenXRPose`.

## Ownership-oriented development

The owner requested a faster approach on 2026-09-07. Work in **connected subsystem
bundles**: producer -> immutable owned data -> real native consumers -> retirement
of the replaced dependency. The active queue identifies the next bundle and
existing files; do not rebuild a second renderer framework.

- Before coding, state the output the bundle will deliver, the guest interface
  it removes, the owners/backend it reuses and the falsifiable acceptance test.
  Group the related data, lifetime, bindings and consumer changes together.
  A helper rewrite, callback count or new counter alone is not that deliverable.
- Keep scope coherent, but do not turn one diagnostic asset into a permanent
  gate on scene-level ownership work. Once its route is established, use it as
  a regression case while extending the same owners to representative consumers.
  Full completion still requires every desktop scene/event/both-eye gate.
- Use translated source to recover contracts, not replicate every console
  helper one-for-one. Preserve authored changes, ordering and lifetime; console
  register/scratch/cache layouts are temporary adapters, not native APIs.
- Reuse the current source index and relevant findings. Reopen source when the
  contract is incomplete, evidence conflicts, or the code changed; do not repeat
  a completed whole-function audit solely because a new turn began. Read the
  active queue and relevant evidence, not the full historical worklog every time.
- Bound each investigation by a concrete decision. Before another probe, name
  what new observation it will obtain and what implementation choice it changes.
  If it gives no useful evidence, change the hypothesis or observation method;
  do not keep booting until a pass or accumulate diagnostic-only checkpoints.
- Triage failures by affected ownership boundary. Preserve the failure and strict
  checks; add a focused causal regression when the cause is known. An unresolved
  legacy-adapter failure is not a fix, but need not stop independent work that
  removes that adapter from the native consumer. Do not disable a comparison,
  change a threshold or relabel evidence to make a bundle pass. The affected
  behavior remains unqualified until investigated and verified.
- Test production representations at their boundaries: view types, units,
  identities, generation reuse, late writes and source/GPU lifetimes. Failed
  runtime eligibility needs a focused boundary regression before a retry;
  helper/source-string tests alone cannot prove reachable native consumption.
- Keep the inner loop in existing CPU fixtures and incremental host builds.
  Group related edits before building; use GPU fixtures when shader/binding
  behavior changes and a targeted live/pixel run for a connected integration.
  Broad reload/scene/sequence/both-eye matrices belong at meaningful milestones,
  not after every scalar edit. Never omit their final acceptance requirements.
- `python -B tools/host_checks.py` selects artifact-free Python guards/scenario
  tests; use repeatable `--area`, `--list` or `--all-boundaries` as appropriate.
  These do not replace C++ behavior fixtures or GPU/pixel evidence. Avoid widening
  shared headers or rebuilding unrelated targets for private changes.
- Commit/push coherent verified bundles and useful intermediate connections.
  A source/fixture-only commit must name its unconnected consumer or pending
  live gate; it does not restamp the last tested binary. Report dependencies
  actually removed, remaining interfaces, acceptance evidence and storage.
  Update the active queue/README once per meaningful change, not after each probe.
- Delete temporary adapters when their last consumer migrates. Preserve full
  goal scope; do not substitute permanent replay or compatibility for ownership.

## Verification rules

- Inspect actual pixels, not just counters or build success. Capture sequences
  catch intermittent defects; a single image is not a stability qualification.
- Desktop settings live in `<InstallRoot>/profiles/default/reblue.toml` (flat
  TOML). Preserve/restore temporary overrides. Check `[config]` audit output:
  malformed TOML can discard the whole file; command-line flags are not the
  verified desktop settings route.
- Automated pad input does not own mouse-menu hover. For unattended menu/reload
  diagnostics, temporarily set `bd_mouse_menu = false`, verify it took effect
  and restore the owner's profile afterward. Treat title-menu Exit as terminal
  failure, not pending field readiness. Keep normal manual-input defaults intact.
- The Windows install registry record must name the directory holding the exe:
  `HKCU\Software\Zolaware\reblue\Install`, `InstallRoot`, `SchemaVersion=3`.
  A full tested install mounts 1673 archives / 119346 record names.
- Use `tools/xrsim/` for desktop OpenXR and read the vrsim skill. The manifest's
  runtime library path must be absolute. Do not infer Quest performance from it.
- `bd_capture_after_s`, `bd_capture_min_draws` and `bd_capture_frames` produce
  field-scene sequences in `logs/capture/`. Analyze only files from the current
  run in an isolated output directory; inspect logs for crashes and capture site.
  `bd_mv_capture_array` captures a scene target, not necessarily the final eyes.
- `tools/capture_seq.py` flags frame jumps; `tools/capture_cyan.py` detects a known
  artifact. `tools/stereo_check.py --raw <capture> --stacked` checks layered eyes;
  black bars, uniform sky and bad near/far framing are **inconclusive**, not proof
  of depth. Inspect both eye images and record the verdict honestly.
- `bd_host_draw_verify` compares replay composition with interpreted draws, but
  cannot prove that later interpreted nodes inherit correct state after replay.
  Use visual sequences and RenderDoc when retained-state errors survive counters.
- Use guest-call/resource counters to track remaining dependencies explicitly.
  A host-issued draw count is not the count of fully host-owned frames.
- Require sampled verification counters to be fresh for the intended scene and
  mode. Matching startup samples followed by a later field marker do not prove
  field correctness; establish the active scene/camera first, then inspect a
  subsequent comparison sample. Keep prior evidence until that replacement passes.
- `other_ms` includes `xrWaitFrame` in XR runs; a near-zero fence wait does not
  prove GPU idleness. Read the `[xr]` CPU/wait breakdown and actual GPU timers.
  Confirm active settings, scene and binary when reporting any measurement.
- Known correctness traps: relaxed guest memory can hang polling loops;
  `non_argument_as_local` miscompiles guest IO; forcing blended depth writes off
  breaks cliffs/fog/DoF; Adreno lacks SSCALED vertex formats. Do not reintroduce
  these as generic optimizations. Historical details are in `research/`.
- After the desktop gate, use the verified Quest deployment scripts; never
  `adb uninstall` (it deletes game data), never run concurrent device measurements,
  and use `MSYS_NO_PATHCONV=1` with adb under Git Bash. Device captures must come
  from the app; `adb screencap` cannot establish compositor-layer correctness.

## Disk-space discipline

Before builds, game runs, asset conversion, downloads, captures or cleanup,
read [the detailed disk-space policy](docs/DISK_SPACE_POLICY.md) in full and
reuse the existing cumulative ledger. Relocating the policy does not reset
any budget, exception, retained-evidence obligation or cleanup trigger.

- Preserve at least 20 GiB free. Default checkpoint ceilings: 2 GiB peak additional
  use, 100 MiB new retained diagnostics and 10 MiB aggregate build/test logs.
  Retries and automatic continuations share those limits. A documented owner
  exception overrides only its named limit; inspect the current ledger.
- Default to no new artifacts. Reuse builds/assets/evidence; bound all producers
  before launch and enforce limits while they run. Unknown output size is not zero.
- Retained raw payloads target roughly 10 GiB including historical sets. When
  already over budget, reclaim at least incoming raw bytes before new capture,
  or pause that producer for owner approval. Never silently skip visual gates.
- Repeated verification targets no net retained growth. Validate replacement
  evidence, then remove only identified superseded agent outputs. Preserve game
  data, saves, profiles, active builds and evidence for unresolved failures.
- Status/docs work uses text/diff and scoped read-only storage checks, not a
  build, capture, new dated report or another ledger entry merely for a restamp.
- On interruption, inspect existing producer handles; never duplicate a job
  because polling was quiet. Stop only owned producers and restore exact profile
  bytes in guaranteed cleanup. A status request does not launch the next run.
- Implementation handoffs report measured net drive change, ending free space,
  actual reclaimed bytes and retained growth with its reason. Distinguish
  drive-wide activity from attributed files; do not double-credit cleanup.

## Git and documentation

- **Commit and push often**, as requested by the owner on 2026-09-04: small,
  coherent, verified checkpoints during implementation, not one giant final
  commit. Stage explicit paths and review the staged diff. Do not sweep up
  unrelated changes, local settings, binaries, logs, disc data or cooked assets.
- **Standing owner approval (2026-09-06):** normal, task-scoped source/test/docs
  commits and pushes to GitHub `noeldvictor/reblue_android_optional_vr:main`
  and `noeldvictor/plume:main` are permanently authorized unless the owner
  revokes that approval. Do not ask again for each checkpoint to these named
  destinations. This does not authorize force-pushes, unrelated changes,
  secrets/game-data uploads or bypassing a genuine tool/security restriction.
- Work on this fork's `main`. Push normal commits to its configured `origin/main`;
  verify success and report the commit. Never force-push, reset user changes or
  rewrite published history. If the remote diverges, inspect it before proceeding.
- Dependency changes belong in the actual forks, not patch files. Plume uses
  `noeldvictor/plume:main`; XenosRecomp uses `noeldvictor/XenosRecomp:reblue`
  (**not main**); the separate SDK checkout uses
  `noeldvictor/rexglue-sdk:android-arm64`. Push dependency commits before a parent
  gitlink that references them. Never commit Windows libmspack symlink repairs.
- Keep README focused on the project, current scope, honest status and setup.
  Update this file for durable rules; update the transition document for progress.
  `CLAUDE.md` remains a thin import, never a second copy of these instructions.
- Findings go in new `research/YYYYMMDD_HHMM_<slug>.md` files with dates, sources,
  settings, evidence and limitations. Do not rewrite old research to hide a
  superseded conclusion. Remove stale conclusions from active documentation.
- The former long instruction file is preserved in
  [`docs/archive/CLAUDE_2026-09-04.md`](docs/archive/CLAUDE_2026-09-04.md).
  It and `docs/VR_PORT_PLAN.md` are historical references, not current priorities.
- This is an unsupported personal fork. Preserve upstream credits and license
  notices; do not add support infrastructure or promises unless asked.
