# Native water/refraction material setup

2026-09-06, desktop Vulkan. Two complete rendering callbacks now execute on the
host. Native material assets, full frame ownership and Quest qualification are
not complete. Work resumed after the storage-instruction update.

## Source and change

Read `config/hooks/render_tweaks.toml` before the complete translated callbacks:
`sub_82454720` in `generated/reblue_recomp.38.cpp` and `sub_82455150` in file 89.
The complete `sub_82454398` in file 74 confirms the water parameter-block
arithmetic and authored-field use; the constructor `sub_82453AE8` in file 100
also writes fields 4700 and 4708. Generated files and hook TOML were unchanged;
no decompiler or tool download was needed.

`native_refraction_material.h` owns address-free preparation order. Water
publishes its signed scene factor, flushes two parameter blocks, enables source
alpha blending and depth testing, binds planar/scene images, then reads the live
snapshot flag. Refraction flushes a parameter block and requests a snapshot
unconditionally. Depth writes, other blend operations and separate-alpha terms
remain inherited. Descriptors and flags are read at use, after earlier effects.

`refraction_material_import.h` isolates checked legacy selection/address rules:
water's image is table +80, distinct from ordinary current/next selection; only
the active table uses its offset. Missing/unreadable entries and address overflow
refuse conversion. Null table/image remains a binding no-op. Scalar indexing
preserves low-word arithmetic, rejecting final address overflow before effects.
These are temporary imports, not native asset identities.

`native_refraction_material_bridge.cpp` replaces both functions under existing
`bd_native_scene_textures`. Mapped-range/stack preflight precedes effects;
descriptors are checked again at use. The scoped stack restores on exit, and
there is no whole-function fallback/replay after side effects. Water's existing
PS c51.w highlight ceiling remains before applicable flushes. Inspection caught
a draft regression: unresolved images would throw, whereas the current texture
setter produces a debug marker. The bridge now preserves and counts that marker
without executing the D3D texture setter. Lookup stays outside the video mutex.

The parameter producer, four state setters and snapshot scope remain counted
child adapters. Snapshot uses the pending native bridge when it owns the scope;
otherwise that bridge tracks compatibility. The material update/animation
callback, parameter storage, getters, scene/reflection ownership and native
cooked-material associations are not converted by this change.

## Verification

Source: root `6f002e0` plus this change and existing pending renderer integration;
local Plume `3094b35ae2e53207d557532748cf2ac7c96a5035`, clean. The unpublished
dependency gitlink was not staged; no push was retried.

- Focused `host_post_output_test` build 13: PID 2968, exit 0. New CPU cases reuse
  the existing executable: ordering/live mutation, exception propagation, signed/
  default factors, NaN/infinity/signed-zero clamping, active/inactive/out-of-range/
  null image selection, every missing selector word, unaligned/overflow reads
  and scalar index wrapping.
- CPU suite 21: PID 18740, 31/31 in 3.32 seconds. CPU executable SHA-256:
  `ff819bca7700e788d0dfca1d33ce41a9a1628282ddfab9449553003f4072d422`.
- 83 source guards pass: refraction 6, snapshot 5, post 36, scene 24,
  effect activation/lifecycle 7, view scheduling 5. They enforce structure, not
  runtime correctness. Both edited supervising scripts parse cleanly.
- Desktop `reblue` build 24: PID 25608/session 79779, exit 0; both hooks compile
  and link. Codegen: zero written/one module up to date, no guest objects or
  shaders rebuilt. Binary linked 06:55:03, 47,697,920 B, SHA-256:
  `8fd71fdae27fe49139b351edca03d8eb5ae4188f06e10f468964b46f99de7c00`.
- Guarded normal-flat run: PID 23888/session 94172, 06:56:56-06:58:12, wrapper
  exit 0 after timed stop. Log 867, 270,022 B; perf-065659 CSV/metadata 606,320 B.
  All five effective settings passed audit; automatic captures disabled, zero
  raw outputs and runtime/config error matches. Owner profile restored byte-for-
  byte. All producers terminal.

Last material sample at 06:58:09: **1,964 water preparations**, zero refraction,
compatibility, refusals or faults; 3,928 parameter adapters, 7,856 state adapters,
3,928 image bindings, 1,964 highlight clamps; zero null bindings, snapshots or
debug images. These are sampled cumulative counters, not shutdown totals or
fully native frame counts. Global parameter/blend telemetry has zero compatibility;
raster still has two compatibility calls and one initial import, outside the
material counter. Native post last reports 3,601 frames/sequences, no original
or refused path.

Inspected `out/verification/native_refraction_material_window.png`: 1920x1080,
3,353,413 B, SHA-256
`c5c7b740428ec45a0eb201b57c6af04a6f8cb08bece958caacc6d585fcf822a1`.
Shu, terrain, foliage, shadow and distant DoF look sane. This is a single field
sanity image, not an isolated water comparison or sequence qualification.

**Unverified:** the field exercises water setup but requests no snapshot; the
separate refraction callback is not observed. No snapshot telemetry is present.
Prior tiny GPU snapshot/resolve fixtures remain copy evidence; unchanged core/
backend code did not justify rerunning them. No new XR/non-MSAA matrix, both-eye
sequence, battle/cutscene/transition/reload or Quest run occurred. Do not force
an authored flag and describe synthetic coverage as an actual event.

## Storage and cleanup

The cumulative ledger remains `20260906_0333_native-scene-state-bridge.md`, with
original baseline 65,462,788,096 B and unchanged 2 GiB peak/100 MiB diagnostics/
10 MiB aggregate build-log caps. This step reused existing trees/tools, planning
<=256 MiB build/link overlap and <=5 MiB diagnostics including the bounded image.

New diagnostic payloads: 4,236,095 B (build/test logs 6,340; runtime 270,022;
perf 606,320; PNG 3,353,413). No new raw/cache/dump files, fixture executable,
tools or build tree. The run wrapper now counts the existing GPU fixture and
build logs within its same small-output threshold; no ceiling was widened.

After replacement/path/length/ignored/reparse/process checks, removed ten files:
reblue_23, cpu_20 and host_post_output_test_12 stdout/stderr pairs; flat log 866,
perf-060320 CSV/metadata and snapshot-window PNG. Logical 4,264,473 B; immediate
free space 64,463,409,152 ->64,467,681,280 B: **4,272,128 B measured reclaimed**,
counted once. Reports/hashes remain; equivalent diagnostics can be regenerated.
No game/save/profile/source/dependency/build-tree or distinct XR/non-MSAA/failure/
raw evidence was removed.

Retained diagnostic payloads decreased 28,378 B. Reserved accounting totals
63,847,547 B: 86 build logs/138,214 B, 11 runtime logs/3,389,694 B, 20 perf files/
8,963,168 B, eight existing GPU-fixture files/8,364,855 B and unchanged 41 MiB
tool/inspection reservation. Two PNGs/6,687,569 B fit within that reservation.
Keep one current normal-flat set, retiring it after an equivalent replacement.

At cleanup completion, 60.04 GiB free. Net volume growth from 06:50: 32,391,168 B;
cumulative original-checkpoint growth: 995,106,816 B. Changed build/Git/metadata
and unattributed volume activity remain charged; a scoped cache/dump inventory
found no new files. Later docs/Git writes still count.

## Next renderer work

Replace water update/animation/parameter production and native asset associations;
obtain actual refraction/snapshot event coverage before qualifying those paths.
Continue complete scene/reflection/frame ownership and all desktop acceptance
requirements. Dependency/parent publication still requires upload approval.
