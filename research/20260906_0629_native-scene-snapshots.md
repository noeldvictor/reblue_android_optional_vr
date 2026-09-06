# Native scene-color snapshots and pending-resolve ordering

Date: 2026-09-06, EDT. Goal turn classification: **progress**. The preceding
goal work produced a snapshot prototype, a host build/CPU checks and flat-run
evidence; the intervening instruction-file request was handled separately.
The full all-rendering host/modern-Vulkan/desktop-before-Quest goal remains open.

## Source and conversion

Read the complete generated `sub_8221D248` (file 40), `sub_8221D2C8` and
`sub_82454720` (file 38), and `sub_82455150` (file 89), plus render-tweak and
output-resolution hook definitions. No generated source or shader changed.

The original constructor allocates fixed 1280x720 scene and 256x256 reflection
snapshots. Phase 3 conditionally copies according to subject/shared/ready state;
phase 5 copies every request. The water parent calls it only when signed field
+4700 is positive; the second material calls it after its constant flush. The
existing composite-size hook targets a different constructor, not this one.

`native_scene_snapshot.h` now provides address-free timing and whole-image copy
commands from `NativeSceneCommands`' explicit source/attachment-resolve output.
It refuses invalid/aliased/mismatched images before side effects, ends the native
pass, copies every layer into an independent FP16 image and transitions that
snapshot for sampling. Resumed scene writes must not overwrite retained snapshots.
Ordinary attachment MSAA resolves remain native Vulkan operations, not EDRAM
emulation or a shader resolve. CPU command fixtures cover mono/two layers and
1/2/4/8 samples, barriers, refusal-before-effects, once-only clears and cache timing.

The pending whole-function bridge for `sub_8221D2C8` uses an exclusive native
post-image write lease and the existing fenced lifetime machinery. Inspection
found the prototype's exact getter-size check would preserve legacy resolves
at larger desktop/XR sizes. The pending publication integration now explicitly
adopts the native source extent/layers for this whole-output replacement. Other
publications remain strict by default. CPU tests cover that policy, invalid
extents/owners/samples and unknown policy values; source guards check adapter
dimensions are published after retiring the old backing. First-call telemetry
also covers short-lived use before the periodic report threshold.

This is **not** complete water/refraction or frame ownership: authored timing,
subject identity, getters, constructors/material parents and unowned reflection
scopes remain adapters. The bridge has pre-GPU fallback for unconverted scopes,
with no original replay after recording starts. Its API/integration files remain
in the existing dirty integration, separate from the locally committed core/tests.

## Real GPU failure and fix

Added `tools/native_scene_snapshot_test`, built in the existing desktop tree
when `PLUME_BUILD_ATTACHMENT_RESOLVE_TESTS` is enabled. It uses the actual native
snapshot/scene command code, existing Plume and installed Khronos synchronization
validation. All images are 8x8; readback stays in memory, no shaders/downloads/
game data or disk image output. It checks HDR values, different eyes, two retained
snapshots and a third live-scene write, at every supported 1/2/4/8 sample count.
It fails without validation; there is no unvalidated success mode. GPU waits are
5 seconds and the outer CTest limit is 30 seconds. Diagnostic callbacks are bounded.

The first real GPU attempts passed single-sample mono/stereo but failed MSAA.
The isolated second failure log (`attachment_resolve_snapshot_pixels_02`) records
color half bits 0 instead of 16384 (2.0), with zero validation errors/warnings.
Source preparation was made explicit per-eye before confirming this failure;
deferred clears across different custom layer views remain a separate coverage
question, not qualified by this fixture.

Cause: Plume deferred a depth-only clear for a framebuffer that also resolves
color. A color-only reader flushed only clears whose *own attachment* resolved
to that color image, so the pending depth clear/pass could remain unexecuted and
the snapshot read an older color resolve. Completing the pass writes all its
configured resolve outputs, regardless of which attachment clears.

Plume now checks whether the pending framebuffer writes the requested resolve
output and flushes the pending pass before its read transition. A self-contained
mono/stereo regression was added to Plume's existing attachment-resolve suite.
Local dependency commit: **3094b35ae2e53207d557532748cf2ac7c96a5035**. Plume is
clean; neither its commit nor its parent gitlink was pushed. The prior remote
upload denials still require explicit approval; no retry was made.

## Verification and provenance

- Snapshot build attempt 01 had a fixture const-pointer compile error, corrected
  before runtime. Attempts 02/03 built the reproduced failure; attempt 04 built
  the backend fix. Final build PID 26272, exit 0.
- Strict snapshot CTest attempt 03, PID 14916: exit 0, **8/8 configurations** on
  NVIDIA GeForce RTX 3060; 1.04 seconds, validation errors 0 / warnings 0. All
  1/2/4/8-sample modes were supported and exercised, mono and two layers.
- Existing resolve target attempt 16, PID 27460, and strict pixels attempt 12,
  PID 27268: exit 0. The complete existing suite, including both new sibling-clear
  regressions, passes in 1.10 seconds, validation errors 0 / warnings 0.
- Both GPU suites report one unrelated loader diagnostic: a missing installed
  GOG overlay manifest. It is not hidden or counted as Vulkan API validation.
- Focused CPU output target attempt 12, PID 26784: exit 0. CPU attempt 20,
  PID 19508: **31/31**, 3.37 seconds. Source guards: snapshot 5, post 36, scene
  24, effect 7 and view 5: **77 passing**, covering the dirty integrated tree.
- Host target `reblue`, attempt 23, PID 24048/session 79333: exit 0, linked
  06:26:19. Source root 4623a39 plus the dirty integration; Plume 3094b35.
  Codegen reports 0 written and 1 module up to date; no guest object or shader
  regeneration. Header changes rebuilt host TUs only. Existing designated-field
  ordering and CRT-deprecation warnings remain. All producers are terminal.

Binary hashes (SHA256):

| Artifact | SHA256 |
| --- | --- |
| Host exe, 47,689,216 B | `f49941987e2e6c0a0caca96b0cb7e1f54e063293704cc9a058d7820a8948db68` |
| Snapshot GPU test, 705,024 B | `cacc2034873bade725dd908f9eb6fea0a7e45e364a2e2958fc5d484b3bfb6e74` |
| Plume library | `3efea110ac4a225a9620b947aa79022bca9a5ee725e5a5ae046c9896e79b285c` |
| Attachment-resolve GPU test | `314220121d73990c3e58875a99af477913d6b889a368a531a56ef80fa0bb2a0e` |

The previous flat run (PID 9876, log 866, 06:03:18-06:04:34) completed and
restored the owner's profile byte-for-byte. Its binary was the **earlier**
prototype `0642e5582a99b1b621450d848c8f64af975cab12fd3ac3da9daf2f78c4e77089`,
not the final build above. Its 1920x1080 PNG is 3,362,161 B, SHA256
`4fc737802af9acf24e257dcbd7ebd03aa4e7e5875e2ffafe0bade4c96b467dea`.
Inspected Shu, terrain, foliage, shadows and depth-of-field without obvious
full-frame corruption. Whole-log error/config/VK_ERROR scans were empty, but
there was no snapshot telemetry. **Authored water/refraction execution remains
unproven.** This one image is not sequence/stereo/full-game qualification.
No additional game/XR/non-MSAA run or capture was launched on this resumption.

## Storage and next work

The single cumulative ledger remains `20260906_0333_native-scene-state-bridge.md`.
24 individually validated superseded logs/perf files and the old lifecycle PNG
were removed: 4,167,824 logical B, **4,177,920 measured B reclaimed**. Their
reports/hashes remain; equivalent diagnostics can be regenerated. No protected
raw/failure evidence, current XR/non-MSAA checks, game data, profiles, saves,
sources, dependency checkout or build tree was removed.

New GPU fixture outputs total 8,364,855 B, one reusable build representation for
previously missing GPU coverage. Retained diagnostics total 63,867,177 B including
the existing 41 MiB tools/inspection reservation and this fixture. It stays until
superseded by equivalent native snapshot qualification. No raw bytes were added.
After cleanup: 64,499,191,808 B free (60.07 GiB); net growth from the snapshot
06:01 preflight is 34,263,040 B, and 963,596,288 B from the original checkpoint.
Volume/Git/driver-cache/metadata changes remain charged, not mislabeled as logs.
Subsequent documentation/Git metadata still counts; no budget was reset.

Next: exercise an authored refraction request, qualify native extent publication
in the actual material binding, and convert its remaining material/resource/
reflection parents. Do not repeat the unchanged field boot as proof it ran.
Full desktop fields/battles/cutscenes/menus/transitions/reloads/both-eye coverage,
native scene/assets/materials/animation/UI ownership and mandated modern GPU
techniques still precede any Quest work. This fixture does not prove full-frame
ownership, headset performance or that every Vulkan clear/view case is correct.
