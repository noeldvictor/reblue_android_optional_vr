# Native post attachment and optical-image contract

2026-09-05/06; completed local checkpoint, not a full-frame or pixel qualification.

## Scope and decision

The previous turn restated the completed storage-rule change; it did not advance
rendering. Current source still makes `HostPostRender` inspect output and optical
`GuestTexture` headers, even though scene/depth inputs are native. Remove those
dependencies from actual post rendering and carry completed root images directly.
Keep output allocation and final UI/getter publication explicitly at the temporary
boundary, without adding a copy or changing FP16 HDR intermediate semantics.

The initial depth publication is not removed: the prior source investigation
established that normal native post uses independent depth, but did not establish
that outer/other-view getter readers never need the published depth. Likewise,
blindly drawing final post into an RGBA8 getter could clamp HDR and cause physical
feedback on direct-source frames. Neither is justified by the current evidence.

Files inspected: `post_chain.cpp/.h`, `native_post_bridge.cpp`,
`host_post_inputs.h`, `sampled_image.h`, `post_sequence.h`, existing CPU/source
guards, Plume framebuffer interface. Canonical AGENTS and transition requirements
apply. The devloop and guest-source skills were read in full; no generated source,
hook/codegen input, shader, profile or game asset is changed by this checkpoint.

## Cumulative storage preflight

This continues the original 20:47 native attachment checkpoint; no budget reset.
Original free: 65,462,788,096 B. At 23:51: 64,717,680,640 B. Existing net volume
growth: 745,107,456 B (includes external/unattributed fluctuations recorded in the
23:05 worklog). Existing reserve is about 60.27 GiB. Plan at most 512 MiB additional
peak overlap for an incremental host link, the existing CPU target and bounded
capture-disabled diagnostics, under the original 2 GiB ceiling and 20 GiB reserve.
No new raw frames, downloads, build trees or shader/codegen outputs are planned.

Reuse `out/verification/build_attachment_resolves.ps1`: original free-space floor,
256 MiB growth headroom, aggregate 10 MiB logs, owned-PID timeout/cleanup and abort
on guest compilation. Start with focused source/CPU checks. Existing retained
diagnostics remain under 100 MiB, including the fixed 41 MiB tools/inspection
reservation. All producer attempts and resulting bytes belong in this ledger.
No active producer is assumed; inspect before launch. Publishing is still blocked
pending explicit approval; do not push or commit the parent dependency gitlink.

## Verification and retention

Implemented `HostPostOutput`: real native image, descriptor, live layout and
framebuffer; preflight requires single-sample FP16, valid sampling metadata,
matching framebuffer dimensions and scene/depth eye counts. Optical preflight
rejects physical output-image feedback even through a different descriptor or
layout wrapper, and requires valid single-sample mono optical images.

`HostPostRender` and lens-flare submission no longer accept or inspect
`GuestTexture` outputs/optics. Heat and grain use native descriptor indices.
The boundary imports optical sampling views while snapshotting the authored
plan, prepares output views once and carries native completed images between
roots without borrowing a resource header again. The temporary adapter remains
in `PostTarget` for allocation, lifetime and final UI publication. Its outgoing
links and written flag change only after successful native submission under the
same video mutex. Existing queued draws flush before output writes. A preflight
refusal does not detach the adapter's prior aliases.

Source checks: 33 post + 15 scene guards pass. First attempt caught one outdated
destructor spelling in a source guard; corrected to check the actual adapter
release, not removed. These are structural guards, not execution/pixel evidence.

CPU build `host_post_images_test_05`, PID 22152: terminal exit 0, two compile/link
steps in the existing tree. Suite `cpu_04`, PID 9600: terminal exit 0, 30/30 tests
in 3.30 s. Extended `post_images.cpp` exercises invalid output/optical metadata,
framebuffer extents, HDR preservation, physical aliases, live layout propagation
and 1/2/3/64-root handoff for mono/two-eye inputs with exposure/alpha consumed once.
Interface doubles do not establish GPU pixel correctness.

Desktop host build `reblue_08`, PID 23180, session 6773: terminal exit 0. Added
header triggered CMake's glob refresh; codegen reported its module up to date
(zero writes). Actual build finished 19/22 displayed host/link steps, no guest
object compilation and no shader generation. Two existing CRT deprecation
warnings in `draw_framebuffer.cpp`; no new post compile warnings. Free after
build: 64,715,182,080 B, 2,498,560 B below this preflight snapshot.

Next bounded diagnostics reuse the existing 75 s run wrapper: normal flat with
MSAA disabled, and xrsim with explicitly synthetic flare/heat/animated-grain
coverage. The latter adds three temporary preview settings via `-OpticalPreview`,
permitted only with `-NativeImages`, zero capture count and post enabled. Original
profile restoration and cumulative storage/owned-PID guards remain unchanged.
The vrsim skill was read in full; its existing absolute runtime manifest was
verified. No device run and no new raw captures are authorized by this probe.

Preserve the protected flat/VR raw baselines,
unresolved-failure evidence and the existing window PNG. No new pixel result is
claimed from source/CPU tests or a capture-disabled game run.

### Final independent test checkpoint

Separated output checks into `tools/native_texture_test/post_output.cpp` and its
own target in the same test tree, so the native API contract can be committed
without recording the unpublished scene-resolve dependency. This test depends
only on the existing sampled-input/sequence contract and ordinary Plume framebuffer
interface, not the new attachment-resolve API or opaque-alpha integration. The
recorded parent dependency is `eb7b03c`; its framebuffer getters and the parent
`HostPostInputs` contract were inspected for this dependency boundary.

`host_post_output_test_01` PID 22604 and `host_post_images_test_06` PID 24644 both
terminated with exit 0. `cpu_05` PID 21232 then passed 31/31 tests in 3.31 s, exit 0.
The separate output test exercises root exposure ordering; opaque-alpha handling
remains in the image/integration tests, not this independent checkpoint. Splitting
the CPU test did not change runtime source or require another host/game build.
This adds one 27,648 B executable and a small object in the existing test tree;
all attempts share the original budget. Final CPU binaries:

- `host_post_output_test.exe`, 00:07:30, 27,648 B,
  SHA256 `908b84852c34b6b05b087a749bd79c4f948afced5315f5052c8a1c663e53bde4`.
- `host_post_images_test.exe`, 00:07:38, 73,216 B,
  SHA256 `564aa86e6236a3f8ac33ff124b4ce2770ed400489785c7f4a5d41e33a98e7768`.

### Desktop execution evidence

Both runs used `reblue_vk.exe`, linked 2026-09-05 23:58:41, 47,582,208 B,
SHA256 `a653bc9b06014382df615ad9a545bfb8b595d41e50016286e6b17c561806804b`:
root base `06739de` plus local renderer changes, actual Plume `81bdca8`. Both
mounted all 1673 shipped archives / 119346 names. No raw frames were produced,
the wrapper detected no configured error patterns, and its finally blocks stopped
the owned producers and restored the five-key owner profile byte-for-byte.

- XR `reblue_825.log`, PID 21576, session 30640, 00:00:19--00:01:35, terminal.
  All 19 settings audited; existing xrsim, 1440x1584 per eye, multiview, mirror off,
  native post/DoF on, zero capture delay. Synthetic flare/heat/grade-preview 3.
  Last census: 8,401 native post roots, scene inputs and final publications;
  zero imports/original roots/container scopes/sequence refusals. Flare visible
  8,401, sprites 126,015, heat and grain each 8,401. Four native optical images
  prepared. Native scene resolves/deferred colours 8,400, recovered zero.
  This exercises optical sampling and output submission, not authored activation,
  optical appearance, stereo depth or Vulkan validation-layer correctness.
  Log 491,356 B; `perf-20260906-000022.csv` 1,437,696 B; metadata 112 B.
- Flat `reblue_826.log`, PID 2220, session 19785, 00:03:08--00:04:24, terminal.
  All six settings audited; `bd_msaa=0`, native post normal, previews off.
  Last census: 3,601 native post roots/inputs/final publications, zero imports,
  original scopes or refusals. Scene results completed/consumed 3,600;
  materialized colour 1 (startup), depth 0; native resolves/deferred colour zero.
  Log 233,921 B; `perf-20260906-000310.csv` 602,112 B; metadata 112 B.

Runtime-source SHA256 (normalizing nothing; actual workspace bytes):

- `post_chain.cpp`: `e9cd0b5002731b061dfa75f426b5bf9a97200e089ee8e229374502a10125e5a5`.
- `post_chain.h`: `b8e013193c587e7eb4bf5599c119b0c7d10eb8deffefec0debc830e157f27398`.
- `native_post_bridge.cpp`: `b93d5fbf82c6e5098c0ad270c867a18a39e6687ee8b653ee934c1d3261c59427`.

The runtime exercised one root per sequence. Multi-root cases are CPU contract
checks only. Neither run is a new normal flat/VR pixel sequence or comfort/FPS
qualification. Full fields/battles/cutscenes/menus/transitions/reloads, remaining
guest rendering, native output allocation, depth/getter/UI ownership and Quest
gates remain open. No extra publication copy or shader-format change was added.

## Completed cleanup and closing ledger

Read-only process inspection confirmed no live renderer/build/test producer before
cleanup. All targets were exact files under workspace `out/`, with no directory
or reparse targets and checked non-reparse ancestors. No recursive deletion.

Removed 13 superseded files: stdout/stderr pairs for `attachment_resolve_reblue_07`,
`attachment_resolve_cpu_03`, `attachment_resolve_cpu_04`,
`attachment_resolve_host_post_images_test_04` and
`attachment_resolve_host_post_images_test_05`, plus `reblue_824.log` and
`perf-20260905-233307.csv/.meta.txt`. They are replaced by successful host build 08,
CPU suite 05, image test build 06, independent output test build 01 and normal
non-MSAA diagnostic 826. Logical bytes removed: 884,291. Immediate volume free
64,712,921,088 -> 64,713,809,920 B: **888,832 B actually reclaimed**. The tests/runs
can be repeated; these exact historical logs/perf data are gone, with their
findings and binary/settings evidence retained in this and previous worklogs.

Retain the current build/CPU logs, native resolve GPU failure/passing evidence,
runtime 825 optical coverage and 826 normal non-MSAA coverage. Retain 823 as the
latest normal capture-disabled XR case, and 821/822 for distinct recovery/default
MSAA evidence. The protected raw flat/VR baselines and existing 3,196,857 B window
PNG are untouched; no new raw allowance or archive exception was created. Retire
these small diagnostic logs when a matching qualified replacement exists; retain
protected pixels until their documented correctness question has a replacement.

Cumulative retained diagnostics since the original 20:47 start: runtime logs
2,170,497 B, perf 5,985,040 B, build/test logs 128,945 B, existing PNG 3,196,857 B,
zero modified runtime cache files. Adding the fixed 41 MiB tool/inspection
reservation still fits the 100 MiB diagnostic ceiling (conservatively counting
the PNG again). New runtime diagnostics in this continuation total 2,765,309 B.
No new downloads or build trees. The original 2 GiB peak cap was enforced by all
wrappers; they do not log a measured minimum-free/peak trace, so no precise peak
allocation is claimed from ending volume free space.

Closing measured free: 64,713,678,848 B (~60.27 GiB), 4,001,792 B less than this
continuation's preflight, and 749,109,248 B below the original checkpoint start.
This net volume change includes external filesystem activity; only the immediate
888,832 B cleanup measurement is credited as this continuation's reclaimed space.
All owned sessions are terminal, profiles restored, no active producer to resume.

The independent API header, standalone CPU test/build registration and current
documentation can be committed locally. The renderer wiring, earlier native scene
integration and parent Plume gitlink remain unstaged/uncommitted. No push was
attempted: prior publication denial still needs explicit owner approval.
