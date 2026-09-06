# Project instructions

This is the canonical, shared instruction file for coding agents in this
repository. `CLAUDE.md` imports it. Keep enduring rules here, current progress in
[`docs/HOST_RENDERER_TRANSITION.md`](docs/HOST_RENDERER_TRANSITION.md), and dated
evidence in `research/`. Updated 2026-09-06.

**Storage-first default:** produce the minimum new data needed for the task.
Reuse existing tools, build trees and evidence; do not start builds, game runs
or captures for status or documentation-only requests. Choose the smallest
sufficient verification: existing evidence and focused text/source checks first,
then a targeted test or incremental build when needed. Before a larger run,
capture, download or asset conversion, explain what it must establish that the
existing outputs cannot. This does not replace required visual qualification.
Before a space-producing job, follow
[Disk-space discipline](#disk-space-discipline): budget peak overlap,
count all retries together and enforce output limits while the job runs. Clean
up verified obsolete agent-created outputs at each checkpoint, while preserving
game data, active builds and required evidence. Report measured savings, not
proposed cleanup. Free disk space is not an allowance to accumulate artifacts.
Storage discipline also applies to the code and tools we add: bounded retention
and reuse must be designed in, not left to occasional manual cleanup.

**Repeated verification should target no net growth in retained artifacts.**
Replace equivalent evidence instead of accumulating a set for every run or
commit. Validate the replacement before retiring its predecessor, budgeting
their temporary overlap. Retained growth needs a specific new coverage or
unresolved-failure reason and a cleanup trigger; free space alone is not a
reason. Never meet this target by deleting protected data or skipping required
qualification.

**Quick storage limits:** preserve at least 20 GiB free; cap each checkpoint at
2 GiB peak additional disk use, 100 MiB of newly retained diagnostics (including
tools), and 10 MiB of aggregate build/test logs within that diagnostic budget.
These ceilings include retries and continuations, not just the latest run.
Stop space-producing work and ask before exceeding them; continue low-storage
source or documentation work where possible. Unknown output size is not zero:
inspect the producer's defaults and set an enforceable bound before launching.
Implementation checkpoint summaries must include a short storage line: measured
net disk change, ending free space, bytes actually reclaimed and any retained
growth with its reason. Report unavailable measurements as unknown, not zero.
See the detailed rules below for the separate raw-capture archive gate and
protected evidence.

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
2. Read the transition document for renderer work and the relevant dated research
   for the subsystem. Do not load every old experiment as standing guidance.
3. Use the repository skills when applicable, reading the entire `SKILL.md` first:
   - [devloop](.claude/skills/devloop/SKILL.md): builds, runs and tests.
   - [guest-source](.claude/skills/guest-source/SKILL.md): guest/render-loader investigation.
   - [vrsim](.claude/skills/vrsim/SKILL.md): desktop OpenXR verification.
   These files contain historical setup/status passages. Current owner scope,
   CMake definitions and verified local configuration take precedence over those
   passages; they do not authorize a Quest run before the desktop gate.
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

## Verification rules

- Inspect actual pixels, not just counters or build success. Capture sequences
  catch intermittent defects; a single image is not a stability qualification.
- Desktop settings live in `<InstallRoot>/profiles/default/reblue.toml` (flat
  TOML). Preserve/restore temporary overrides. Check `[config]` audit output:
  malformed TOML can discard the whole file; command-line flags are not the
  verified desktop settings route.
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

The owner explicitly requested disk cleanup and space-conscious work on
2026-09-05 and reaffirmed it on 2026-09-06. Treat storage as a budget, not an
unlimited experiment archive.
Minimize retained and peak temporary bytes even when the drive has free space;
available capacity is not a reason to keep unnecessary outputs.
Storage cleanup is part of completing each checkpoint, not a separate future
task. Leave only the outputs needed for continued work and required evidence;
apply the protection and cleanup rules below before removing anything.
Ignored files still consume disk: a clean `git status` is not a storage check.
Count build outputs, caches, captures, logs, asset intermediates and Git history,
including temporary outputs that exist only while a job is running.

Default to **no new artifacts unless the current request needs them**. For every
output-producing command, including a retry, follow this order:

1. Inspect and reuse existing evidence, tools and outputs where sufficient.
2. Choose the smallest necessary output; set its location, aggregate size limit
   and stop condition before launch. Include automatic caches and temporary files.
3. Budget replacement overlap and validate new evidence before retiring the old.
4. Remove verified superseded agent-created outputs, then report actual retained
   growth and reclaimed bytes. Do not start another run with an unreviewed backlog.

These checks should be lightweight for small commands; they do not require a new
report or inventory file. Reuse existing outputs, build incrementally and disable
captures for nonvisual diagnostics. Documentation-only work needs focused
text/diff checks, not a new build or game run. Do not create diagnostic directories,
screenshots or dated research reports merely to record an instruction-file or
other text-only edit; update the relevant existing documentation and report read-only disk checks
inline. Reuse an open checkpoint's storage ledger instead of creating a new
per-turn accounting file. Automatic goal continuations and unattended runs obey
the same limits; they do not authorize more storage or reset a checkpoint's budget.

Use these defaults unless the owner approves a different budget:

| Storage measure | Default limit |
| --- | --- |
| Peak additional disk use per checkpoint | 2 GiB |
| Newly retained diagnostics, including downloaded diagnostic tools | 100 MiB per checkpoint |
| Aggregate build/test logs, also counted in diagnostics | 10 MiB per checkpoint |
| Free space to preserve throughout a job, including temporary overlap | At least 20 GiB |
| Total retained unique raw capture payloads, including historical sets | Around 10 GiB; exceptions and the incoming-capture gate below apply |

These are ceilings, not allowances to spend; keep smaller jobs smaller.
Existing tighter limits and the free-space/capture gates below still apply.
Count all attempts and continuations together, including unfinished prior work;
do not rename a checkpoint to reset its budget. If the work cannot fit, pause
the space-producing step and ask before increasing these limits.

Before storage-heavy work, state a minimal-output plan: what existing output can
be reused, what new output is necessary, its peak byte estimate, and what will
be retained or cleaned up afterward. Prefer a focused test over a full rebuild
when it answers the same question. Do not launch inherited build/capture jobs
for a status or instruction-file request; documentation-only verification is
text and diff checks.

The latest request controls what work resumes after an interruption. A status,
documentation or cleanup request does not authorize continuing a queued renderer
verification matrix. Check any already-running agent-owned producer, account for
its outputs and ensure temporary profiles are restored when it stops; do not
launch the next build, game run or capture merely because an earlier handoff
listed it. Keep unfinished verification explicitly pending.

- Check actual volume free space before builds, asset cooking, downloads and
  captures. Estimate peak additional space first: final outputs plus overlapping
  temporary, extraction, conversion and linker files. For a large job, record
  free space, the estimate and the expected remaining reserve in the worklog.
  If free space cannot be measured, resolve that before launching the job. Plan
  to keep at least 20 GiB free throughout the job, including its peak overlap.
  If the estimate would breach that reserve, reduce the batch, safely reclaim
  eligible outputs or ask the owner before proceeding. Below 10 GiB free, do
  not start a large job until space has been reclaimed and the estimate fits.
  Give each large producer an explicit output location, byte or file-count
  limit, stop condition and retention/cleanup plan. Retries share the original
  job's cumulative storage budget; failed runs do not reset the allowance.
  Enforce those limits in the producer or its supervising wrapper, not only in
  prose. Monitor output growth and free space while long jobs run, and stop the
  agent-started producer before its limit or reserve is breached. Allow for bytes
  it can write between checks; a final disk check alone is insufficient.
- Keep one cumulative storage ledger in the checkpoint worklog for all large
  producers: starting free space, planned peak growth, bytes produced, measured
  bytes reclaimed, retained outputs and ending free space. Include concurrent
  jobs, retries, automatic captures, caches and temporary files in the same
  budget. Do not credit the same cleanup savings twice or carry an already-used
  allowance into a new checkpoint. Reconcile the ledger with actual free space
  before starting another large producer; investigate unexplained growth first.
- On interruption or handoff, record each agent-started producer's session/PID,
  output location, enforced limit, completion state and consumed/remaining byte
  budget. Before resuming, inspect the existing process and outputs; a quiet
  poll or lost tool session is not permission to launch a duplicate job.
  Finish accounting for completed outputs before retrying or replacing them.
  Do not treat an unused allowance as a reason to generate more evidence.
- Reuse configured build trees, dependencies and installed game data. Do not
  make full backups/copies of builds or assets for a small change. Check for
  junctions and hard links: logical directory sizes can count the same bytes
  twice. Measure actual free-space change before claiming savings.
  Never commit large generated outputs as a temporary backup: deleting them
  in a later commit leaves their payloads in Git history. Review staged file
  sizes as well as paths, and keep disposable outputs outside version control.
  Inspect existing evidence before producing more. Documentation-only changes
  do not justify rebuilding or recapturing merely to stamp a new commit hash;
  record the actual tested binary and source revision instead.
  Run focused, low-storage checks before storage-heavy verification so simple
  failures do not consume another capture/build budget. Produce only the outputs
  needed for the current question, without weakening required qualification.
- Before downloading tools, inspect installed toolchains and existing caches.
  Reuse a suitable installation; prefer the required component over a full SDK
  or duplicate toolchain. Budget both the download and extracted files, extract
  only what is needed, and verify the extracted tools before removing disposable
  agent-created archives. Keep only one required representation unless a specific
  reproducibility need is recorded. Do not clear shared/global caches or tools
  used by other projects without the owner's explicit approval.
- Prefer streaming analysis and bounded in-memory batches over additional
  on-disk copies. Analyze complete sequences when required, but do not export
  every frame to PNG just to inspect a sample. Keep only the representative and
  failure frames needed for visual review and reports.
  Check tools' default cache, temporary and automatic output locations before
  running them; include those bytes in the job budget, not just named outputs.
  New diagnostic helpers must default to captures off and bounded logs; require
  an explicit per-run opt-in for raw frames, verbose dumps or bulk image exports.
  Prefer summaries and selected failure evidence over redundant successful-run
  output. Do not change the owner's persistent profile merely to set defaults.
  Bound aggregate log/cache retention as well as individual file sizes: repeated
  small runs can still fill the disk. Rotate agent-owned disposable diagnostics
  within the checkpoint budget, preserving required baseline and failure evidence.
- Cook or convert assets in bounded batches, reusing unchanged native outputs.
  Avoid extracting the whole game or retaining every intermediate format for a
  small test. Budget overlapping source, temporary and final representations;
  remove only agent-created disposable intermediates after validating their
  replacements. Preserve original game data and assets needed to reproduce them.
- Build storage limits into new disk-writing code and tools. Caches, asset
  cookers and diagnostic producers need explicit aggregate byte limits,
  retention/invalidation rules and a safe full-budget behavior. Reuse unchanged
  data; do not retain a duplicate representation for every run, commit or retry.
  Keep disposable outputs separate from originals and protected evidence, and
  remove stale agent-created partial outputs after a failed job when safe.
  If meeting the limit would require evicting protected data, stop the producer
  and report the constraint instead of silently growing the output or deleting
  that data. Verify limit and cleanup behavior with small fixtures, not a
  disk-filling test.
- Recheck free space between batches and after large jobs. If growth exceeds
  the estimate or threatens the reserve, safely stop the agent-started producer
  before it fills the disk. Do not launch another batch until the budget fits.
  Start storage investigations with scoped output/cache inventories, not a
  recursive scan of the entire drive or the user's unrelated directories.
- Bound captures explicitly. A 120-frame RGBA sequence costs about 0.93 GiB
  at 1920x1080 or 2.04 GiB for stacked 1440x1584 eyes. Keep total retained raw
  capture evidence around 10 GiB, with documented exceptions for unresolved
  regressions or required qualification. Historical and superseded sequences
  still count; moving or relabeling a directory does not reclaim its bytes.
  Include both the automatic `out/build/win-amd64-release/logs/capture/` output
  and isolated `out/verification/` sets in that inventory, deduplicating shared
  hard-linked payloads. An unfiled capture still counts against the budget.
  Before a new capture, budget retained unique raw bytes plus the incoming
  sequence and analysis exports. If that exceeds the budget, clean up eligible
  superseded outputs or document the required exception before launching.
  Each exception must name the retained sets, their unique byte count, the
  verification or unresolved failure requiring them, a maximum additional byte
  allowance, and a concrete review/cleanup trigger. An over-budget historical
  archive is not a blanket exemption for new captures. Recheck exceptions at
  the next checkpoint; do not let every checkpoint become a permanent raw archive.
  If the retained archive is already over budget, reclaim at least the incoming
  retained raw bytes before another capture. If that cannot be done safely,
  pause new captures and ask the owner before increasing the archive's unique
  byte count. A new per-run exception or a new turn does not bypass this gate;
  continue source work and low-storage checks while capture growth is paused.
  For diagnostics that do not require images, explicitly disable automatic
  captures and verify the effective configuration before launch; do not trust
  a profile left by an earlier run. If capture cannot be disabled, its delay
  must exceed an enforced run timeout. For image verification, set bounded
  frame counts and output locations before launch. Bound diagnostic dumps and
  logs too, especially verbose shader/frame dumps; a no-capture run is not an
  unlimited logging allowance.
  At run completion or interruption, stop only agent-started jobs that are no
  longer needed and restore the owner's profile after temporary overrides.
  Put producer shutdown and temporary-profile restoration in guaranteed cleanup
  paths where possible; do not rely on reaching the final step of a successful
  run. On resumption, check for leftover capture overrides before another launch.
- Keep the current baseline, current flat/VR verification and evidence needed
  for unresolved failures. Retain evidence by verification purpose, not by
  commit or timestamp: normally keep one passing capture set per required
  configuration, plus the minimum evidence needed for each unresolved failure.
  Verify a replacement before retiring its predecessor; do not accumulate a
  full archive for every small source change. For superseded experiments,
  retain small reports, logs and representative images; losslessly compress or
  remove redundant raw outputs once their investigation no longer needs the
  complete sequence.
  Storage limits must not silently reduce the renderer's verification gate.
- At each experiment checkpoint, identify which artifacts remain the baseline,
  current verification or unresolved-failure evidence, and which are superseded.
  Give retained large artifacts a reason and a cleanup condition. Avoid keeping
  several copied representations of the same capture.
  Once a cleanup condition is met, perform the safe, in-scope cleanup before
  generating another replacement set; do not only document an ever-growing
  backlog. Preserve protected evidence and ask if its value is uncertain.
- Prefer lossless compression when a complete historical sequence is still
  useful. Validate hashes before/after and avoid compression work during GPU
  timing measurements. Hard links isolate a run without duplicating payloads,
  but deleting one link alone may reclaim no space.
  Budget compression's temporary overlap with the originals; do not start it
  unless both fit within the reserve. Keep only the validated representation
  needed for retention, removing originals only when the cleanup rules allow.
- Cleanup may remove identified, reproducible temporary/verification outputs,
  not game data, discs, saves, profiles, source, dependency checkouts or active
  build trees. Inspect exact targets, references and running processes first.
  Never recursively delete a workspace/build root or follow junctions into
  other data. Record what was removed, whether it can be regenerated, and the
  measured bytes recovered; note when historical raw evidence is no longer
  available. Ask before deleting anything whose ownership or value is unclear.
  An inventory or cleanup proposal is not reclaimed space. Report completed
  cleanup separately from candidates still awaiting review or removal.
- After storage-heavy work, report ending free space and the measured net disk
  change, plus any large retained outputs and their cleanup condition. If the
  budget cannot fit without deleting protected or uncertain data, stop the
  space-producing work and ask; do not fill the disk to finish a checkpoint.

## Git and documentation

- **Commit and push often**, as requested by the owner on 2026-09-04: small,
  coherent, verified checkpoints during implementation, not one giant final
  commit. Stage explicit paths and review the staged diff. Do not sweep up
  unrelated changes, local settings, binaries, logs, disc data or cooked assets.
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
