# Native render-pass attachment resolves

2026-09-05 EDT, Windows Vulkan desktop. Base `7d82df0`; implementation baseline
`ca90d3f` is pushed, documentation commit remains local after two explicit
auto-review rejections. Owner approval for that external payload is pending;
do not retry the rejected push or route it through another destination.

Previous goal turn made progress: completed-scene ownership passed 30 CTests,
45 guards and normal flat/both-eye pixel checks, then storage usage fell 7.38
GiB. Current worktree and Plume submodule are clean. No delegation/device work.
Read AGENTS and guest-source/devloop; the combined output truncated the devloop
tail, which was reread through EOF before work. Current desktop scope wins over
historical recipes. Read transition acceptance/current state and prior retention.

## Boundary and implementation direction

Plume's existing `resolveTextureRegion` fixes array layer zero/count one and
uses `vkCmdResolveImage`. Its framebuffer descriptions have no colour/depth
resolve attachments; pipeline/framebuffer render passes use the older creation
structure. Add explicit resolve attachments to the native pass instead of
routing completed native images through the console copy/resolve wrapper.

The existing depth shader resolves MIN (nearest depth), not average or sample
zero. Colour averages samples, scales RGB exposure and forces alpha one; scale,
alpha and resampling semantics must remain explicit at the future native scene
consumer boundary. Hardware resolve alone does not replace those operations.

Primary sources checked:

- [Render-pass compatibility](https://docs.vulkan.org/spec/latest/chapters/renderpass.html):
  single-subpass resolve references/modes do not require new draw pipelines.
- [Depth/stencil attachment resolve](https://docs.vulkan.org/refpages/latest/refpages/source/VkSubpassDescriptionDepthStencilResolve.html):
  render-pass-2 subpass chaining and device-supported mode constraints.
- [Resolve properties](https://docs.vulkan.org/refpages/latest/refpages/source/VkPhysicalDeviceDepthStencilResolveProperties.html):
  depth/stencil modes and independent/none restrictions must be queried, not assumed.

Implement pass construction, clear/discard variants, explicit layered outputs,
mode validation and held-clear dependencies together. A read of a resolve output
must also complete a held source clear. Preserve density-map attachment indices
and original source indices; new output attachments append after existing ones.
Existing single-subpass pipelines stay compatible. Unsupported backends must
refuse resolve requests explicitly, not silently ignore them. Standalone GPU
pixel/readback checks should cover layers, depth MIN, clears and resumed passes
before wiring the scene producer; CPU/source checks alone are insufficient.

## Storage preflight

Actual free at 20:47:00: 65,462,788,096 bytes (60.97 GiB). Reuse existing Plume,
host and CPU-test trees; allow <=2 GiB peak incremental build/link growth,
expected reserve >58 GiB, minimum 20 GiB. Build one target at a time, monitor
growth/reserve, stop owned jobs if the estimate is exceeded. New diagnostic/
test/log output cap 100 MiB; GPU microtests should retain no raw images and use
small in-memory readbacks. No asset cook/download/new dependency tree.

Current `native_scene_result_flat` / `_vr` baseline stays protected. Last exact
raw inventory: 27,131 unique payloads / 252,177,116,500 logical bytes. The prior
3,185,054,400-byte capture allowance is spent. No new full-game capture producer
is budgeted here; fresh retention review/reclamation is required before one.

## Resumption and first GPU evidence

The intervening documentation turn made progress: `4c1c88e` tightened capture
retention by verification purpose. Both it and `7d82df0` remain local pending
push approval. Resumption found the five Plume edits and this worklog, with no
live game/build producer. Free space was 65,460,396,032 bytes. Read the complete
devloop and guest-source skills and the transition acceptance/current state.

Plume-only incremental builds compiled Vulkan and D3D12, then linked plume.lib.
No guest target or main executable was rebuilt. The optional standalone test
links that existing library from `out/native_texture_test`; 8x8 shader/readback
fixtures produce no raw captures. First compile exposed the Windows `interface`
macro; first link exposed MT-versus-existing-MD runtime mismatch. Both were fixed.
The first build wrapper incorrectly returned zero with a missing ExitCode; its
log showed FAILED, so this was not accepted as success. The wrapper now holds the
process handle and rejects unavailable exit codes; subsequent real codes agree
with the build logs. `out/verification/build_attachment_resolves.ps1` enforces
the original cumulative 2 GiB budget (stopping at 1.75 GiB), 21 GiB free-space
floor, cumulative 10 MiB logs, owned PID tree shutdown, and 250 ms checks.

Strict GPU CTest failed before device creation: no Windows Khronos layer is
installed/registered. Scoped checks found only the existing Android layer.
An explicit `--pixels-only` diagnostic on NVIDIA GeForce RTX 3060 (depth mode
mask 15, multiview supported) passed every mono/two-layer sample reduction,
MIN versus SAMPLE_ZERO, resumed LOAD, pending/held zero-draw clear and invalid
descriptor check. Overall exit still failed on a loader diagnostic about a
missing GOG overlay manifest. Do not alter the owner's registry or count the
pixel-only run as validation qualification. Strict default still requires VVL.

### Small validation-tool download preflight

Public Khronos CI has a 6,229,508-byte Windows-only layer artifact, avoiding a
full SDK installation/build. Metadata from the official GitHub API:
`KhronosGroup/Vulkan-ValidationLayers`, main commit
`ad4ed518c3c9783b9c9ff912c205c987b17d7bf4`, run `33943828741`, artifact
`9963017667`, created `2026-09-05T04:34:59Z`, SHA256
`e63b8cc3a111fc77274d60cf1e5a653ff323773a5a0563c84f55579d70b48d8a`.
Source: https://api.github.com/repos/KhronosGroup/Vulkan-ValidationLayers/actions/artifacts/9963017667

Amend the no-download plan only for this reusable validation tool. At the last
measurement free space was 65,454,387,200 bytes. Allow <=8 MiB download and
<=64 MiB selected extracted DLL/manifest/licence bytes, within the existing
100 MiB diagnostic allowance and 2 GiB total peak budget. Inspect ZIP entries
and sizes before extraction, verify official digest, reject traversal/links,
and extract only needed files under `out/vvl/windows-9963017667`. Use a
process-local layer path, not a registry/system install. Delete the downloaded
ZIP after successful digest/extraction checks; retain this small reusable tool
until superseded. No new game-capture allowance or new dependency build tree.

## Qualified dependency checkpoint

Plume commit `a8b3c151e97d8fb2b3746269b662f94160da614d` contains the API,
Vulkan implementation, explicit D3D12/Metal refusal and regression fixtures.
The fixtures moved into the actual dependency (`thirdparty/plume/tests/`) so
they ship with their implementation. Root CPU-test CMake/source changes were
removed; its two temporary cache options and optional test were unconfigured.
The final executable uses the existing main-tree Plume target, not an imported
stale library or a new dependency tree. Configure options:

```
PLUME_BUILD_ATTACHMENT_RESOLVE_TESTS=ON
PLUME_RESOLVE_DXC=C:/vcpkg/packages/directx-dxc_x64-windows-static/tools/directx-dxc/dxc.exe
```

Build only `native_attachment_resolve_test`. Run CTest in
`out/build/win-amd64-release/thirdparty/plume` with
`-R native_attachment_resolve_pixels --output-on-failure --timeout 30` and
process-local `VK_LAYER_PATH=<workspace>/out/vvl/windows-9963017667`.
The wrapper removes its own optional backend trace variable; no owner profile
or registry was changed. The main game executable was not rebuilt or launched.

The ZIP digest matched official metadata; it contained only a 21,402,624-byte
DLL. Extracted SHA256:
`19ce8c4adefae74434b0cd35d24964459b983bd05b8a5e285e243b0daccf42e9`.
The same upstream commit's manifest template/known_good pin API 1.4.362. The
small local manifest uses its required loader fields; original LICENSE.txt is
retained. No permanent layer installation. The stale GOG overlay loader message
remains visible and is counted separately, not misrepresented as a VVL error.

First strict VVL run caught 24 invalid blend-enum values in the fixture's
uninitialized copy blend state and 10 missing pipeline binds after multiview
pass begin. Set explicit Copy blending and open the native pass before binding
draw state. This is an important integration requirement, not a qualification
of existing callers that bind before Plume's lazy pass begin. Vulkan explicitly
invalidates non-render-pass state at multiview subpass begin:
https://docs.vulkan.org/spec/latest/chapters/renderpass.html

Final GPU run at 21:22 passed in 1.04 s on RTX 3060 with Khronos core and
explicit synchronization validation enabled: **0 API errors, 0 warnings**, one
reported stale overlay loader message. Every pixel in every 8x8 layer is read
back and checked; no images are exported. Distinct per-sample values are written
using fragment coverage masks, and SV_ViewID gives different left/right values.

- Mono and two layers: four-sample colour average, MIN depth versus SAMPLE_ZERO
  (0.2 versus 0.8, with a 0.05 second-eye depth offset).
- LOAD/resumed samples; DISCARD plus complete sample rewrite.
- Zero-draw pending and held source clears reach both resolve outputs before
  their copy/read barriers, including after framebuffer unbind.
- Eight colours plus depth and nine resolve outputs: all eighteen-attachment
  clear indexing and all eight output readbacks, both mono and multiview.
- Refusal of unsupported/invalid modes, nonexistent sources, aliases,
  single-sample sources, multisample outputs, custom source views, mismatched
  extents and out-of-range eye masks.

Final test executable (718,336 B) SHA256:
`ffbc087aa1455e7fa90d3a1d828c0e27f28fae5cc49f151948f7abf2c467ef82`.
Built plume.lib SHA256:
`9cf5d3d79dac6b7b85c9a0080bfcdacd0231bc72593a96260b9b36f94e567da8`.
Existing 29 texture/state CPU CTests and one material CTest also passed at
21:26 (3.17/0.04 s); those unchanged CPU binaries were not rebuilt. The new GPU
test is additional, not a replacement for them or full-game qualification.

### Publication and remaining work

Verified dependency branch main and push URL `git@github.com:noeldvictor/plume.git`;
read-only ls-remote was `eb7b03cff67688b2e9f7eec7e0d8c1b7dbb5cd38`.
The new, separate dependency push was rejected by auto-review as an internal
source/test export to an unverified external destination. It sent no parent
documentation and was not an alternate route for the earlier rejected push.
Do not retry either rejection without explicit owner approval. Plume is locally
committed and clean; **do not commit its parent gitlink before it is pushed**.
Parent docs `7d82df0`/`4c1c88e` also remain pending publication.

This is the native render-pass mechanism, not complete host-frame conversion.
Next wire explicit single-sample images into the native scene producer and
completed-scene result, removing its emulated initial publication copies while
preserving exposure, alpha and resampling/extent contracts. Establish pass begin
before multiview draw state, and carry image lifetime/resolve-write dependencies
through interruptions. Depth/stencil combinations, sparse colour resolves,
density maps and other GPUs remain untested; do not infer headset tile-local
execution or performance. All broader desktop scene/game/UI/scheduler and Quest
gates remain unchanged; no Quest work.

## Storage closeout and handoff

All agent-started build/test processes are terminal. Final owned PIDs were
26584 (target build) and 4616 (strict CTest), both exit 0. Earlier failed runs
were accounted for; no live session or duplicate producer remains. The
supervisor's stop thresholds were never reached. No runtime profile override,
game capture, asset cook, dependency source download or guest rebuild occurred.

After strict replacement qualification, removed exactly six reproducible files:
the downloaded ZIP and the old CPU-tree test exe/PDB/object/two fixture SPIR-Vs.
Verified exact paths, absence of running build/tests, removed old target from
build.ninja, rechecked ZIP/DLL digests; no recursive directory deletion or link
traversal. Logical removed bytes 15,528,727; actual free-space increase
65,411,203,072 -> 65,426,743,296: **15,540,224 bytes reclaimed**. The six files
can be rebuilt/redownloaded; no historical raw evidence was deleted.

Retained reusable validation DLL/manifest/licence and final GPU exe/PDB/object/
two SPIR-Vs total **29,964,736 bytes**. Wrapper logs total **29,211 bytes**;
small CTest logs and CMake/Git changes are additional. Keep the layer until
superseded, reuse this single configured test target, retain small failure and
qualification logs for the next scene integration. Do not keep another full
test-build copy. New raw bytes: **zero**; protected scene baseline unchanged.

At 21:26:38 free space was **65,425,596,416 bytes (60.93 GiB)**, versus the
checkpoint's original 65,462,788,096: measured net disk growth **37,191,680
bytes (35.47 MiB)**, including unrelated volume activity, within the cumulative
2 GiB peak/100 MiB diagnostic budgets. This is net growth, not net reclamation;
the gross cleanup above must not be credited twice. No new raw allowance exists.
