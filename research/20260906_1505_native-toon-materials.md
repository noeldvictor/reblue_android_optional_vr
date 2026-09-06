# Native Toon texture animation and edge parameters

Date: 2026-09-06, EDT. Continuation of the open checkpoint in
`20260906_0333_native-scene-state-bridge.md`. Previous work made progress through
source changes, completed builds/fixtures and a read-only development-loop audit.
This continuation qualifies the pending code and changes the active queue to
whole ownership paths. The complete desktop goal remains open; no Quest work.

## Source and ownership

Checked hook definitions before reading the complete generated bodies:

- `sub_821837B0`, 0x821837B0, `reblue_recomp.11.cpp`: Toon update. Select images
  from signed frame counters divided by six, bind slots 6/7, then advance the
  three counters with limits 18/18/21. Preserve active-list offsets, null lists,
  out-of-range fallback and live second-source/counter reads.
- `Visual__Shader__Toon__vf04`, 0x82183910, `reblue_recomp.39.cpp`: leaf begin.
  Six authored float words plus two inherited stack words publish VS c50/c51.
  Existing fur vertex shaders consume c51.z, so the inherited values cannot
  simply become zero before a native fur parameter schema replaces that ABI.
- `sub_82183990`, 0x82183990, `reblue_recomp.85.cpp`: leaf end clears the active
  engine mode at 0x82DDB220+40.

`native_toon_material.h` contains address-free animation/edge algorithms.
`native_toon_material_bridge.cpp` installs three whole-function hooks in the
host OBJECT target, binds textures through `Video::SetTexture` and publishes
computed values directly into native parameter storage. The previous original-
leaf dirty wrapper is removed. Native material-pass dispatch recognizes these
callbacks before the function dispatcher and counts native/remaining guest
participants separately. Unknown/disabled/refused inputs retain an explicit
counted fallback; faults after side effects never replay the original parent.

This removes execution and binding/parameter producer dependencies, not all
Toon data ownership. Authored counters/list/edge fields, texture lookup/resource
wrappers, the shader-register ABI and two inherited words remain explicit
temporary imports. Initializer/release/destructor and full asset lifetime are
not replaced by this checkpoint. The update still enters through a host hook;
the native material-pass begin/end route is direct.

## Verification

Reused completed builds after checking their terminal state and identity:

- Host build 37: linked 14:51:20, 47,787,520 B, SHA-256
  `8b1db841a33b5cf619c78c6f8163cb7f9e012baa19c3865bafd2402304105508`.
  Incremental log spans 14:51:09-14:51:21; no guest object or shader rebuild.
- CPU fixture build 08: 585,728 B, SHA-256
  `bf9e5f5decfb2894430279c5ae2c64d35b63ab8697ca492534a804e141cb4b19`.
  Suite 30 passes 31/31 in 6.94 s. Tests cover all 6,804 normal counter states,
  signed overflow/corner cases, 100,000 independent division/float conversions,
  list selection, live callback changes and no counter advance after bind failure.
- All 115 source-boundary guards pass again in 0.027 s. They supplement CPU,
  runtime and pixel checks; token checks alone do not prove behavior.
- The bounded supervisor now supports Toon leaf comparison with the existing
  fresh field/camera parameter gate. Six in-memory fixtures execute its actual
  parsed verification loop: accept fresh evidence; reject startup-only Toon,
  missing/wrong Toon, stale parameters and an inactive camera. No fixture files.

Flat run 886, owned PID 27108/session 83110, terminal 15:02:36-15:03:52:
all five settings audited, captures disabled, no config/runtime error matches.
Last sampled Toon counts: 3,301 updates, 6,640 begins/ends, 6,602 texture binds,
zero null/fallback/refusal/faults. Original comparison is off in this normal run.
Material passes report 549,891 starts, 13,280 native participants and zero guest
participant dispatches. Native parameters: 899,684 blocks, 633,532 imported
words and zero full legacy blocks. The deferred queue is empty, not qualified.

Inspected `out/verification/native_toon_material_window.png`, 1920x1080,
3,333,941 B, SHA-256
`7c5610c521c86f17eb8d1a61979324a1e9eb312feb5d77fac2c6c73af355423c`:
Shu, character shadow, foliage, ground and background/DoF are present. One
standing-field image is a smoke check, not a sequence or both-eye qualification.

Desktop XR run 887, owned PID 28072/session 83298, terminal 15:04:22-15:04:53:
all 18 settings audited, layered multiview, 1440x1584 per eye, render scale 1,
camera mode 2, simulator height 0, mirror/raw capture/perf CSV off. Field marker
(301 water updates) at 15:04:44.405 precedes camera at 15:04:48.055:
game (19.9,149.2,35.5), eye (16.7,149.2,35.5). Both subsequent comparison
samples are at 15:04:53.066: cumulative 1,510 matching original Toon leaf
publications and 2,627,009 matching native parameter blocks. Zero mismatch,
full legacy blocks or Toon fallback/refusal/faults. Sampled 5,701 updates,
11,402 binds, 3,020 native material participants and zero guest participants.
These are cumulative counts sampled after scene/camera readiness, not all
field-only counts. No eye images were captured in this run.

The original comparator executes only the CPU begin leaf before comparing its
eight words/dirty mask; it does not double-execute GPU binds. CPU fixtures cover
the update selector, but this does not qualify every authored texture/fur event,
teardown, battle, cutscene, menu or scene reload. Prior later-scene scenery/text
failures remain unsuperseded. No speedup or Quest performance claim.

## Storage, publication and next direction

Both runtime processes are terminal and the original 116-byte profile restored
byte-for-byte. No new raw frames, shader dumps, cooked assets or caches were
observed. New six build/test logs total 6,523 B; runtime 886/887 total 648,195 B;
flat perf CSV/metadata total 598,128 B; one PNG 3,333,941 B. Gross new diagnostic
payload 4,586,787 B, within the existing checkpoint reservation.

After replacement checks passed, removed 11 exact ignored, reparse-free,
inactive predecessors: host 36, CPU fixture 07 and CPU suite 29 stdout/stderr;
flat 883, XR 885, perf-20260906-142258 CSV/metadata and material-pass window PNG.
Results/hashes remain in the preceding research report; the old log/perf files
and image are no longer retained. Logical removal 4,625,184 B; immediate free
65,201,819,648 ->65,206,452,224 B, **4,632,576 B actually reclaimed**, once.
Net retained log/perf/PNG payload decreases 38,397 B. Host grows 9,216 B and
CPU fixture 12,800 B; tiny helper edits fit the existing tools reservation.
Protected raw, non-MSAA, original-UI and unresolved-failure evidence is retained.
The cumulative ledger remains authoritative; no per-turn budget reset.

README now summarizes current scope/capabilities/limits instead of duplicating
407 lines of chronological status. Dated research and the transition history
remain. The transition's active queue now prioritizes complete static-object
asset/material/instance submission, then characters and specialized producers.
The read-only source audit identifies `NodeTag`, `NativeMeshImport`, material
command discovery and retained interpreter templates as the shared first-path
dependencies. No claim that this next ownership conversion is already done.
