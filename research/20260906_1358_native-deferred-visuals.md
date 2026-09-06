# Native deferred visual scheduling and scene-sized snapshots

Date: 2026-09-06, EDT. Previous goal turn: **progress**, publishing the reviewed
scene/post integration as `b2910d4`. This checkpoint replaces another complete
rendering body; the all-rendering desktop/native-Vulkan goal remains open.

## Exact source and replacement

Read the complete generated `sub_824252D0` (file 66), its primitive helper
`sub_82425220` (73), `bdBeginRenderPass` (70) and `bdEndRenderPass` (92).
Checked the `sub_82425540` caller (90), original sorted scheduler mapping in
`20260906_1323_native-visual-schedule.md`, and queue initialization in complete
`sub_824249D0` (52). Inspected hook definitions first; no instruction hook,
generated source, shader or game data changed. Guest-source and devloop skills
guided the exact mapping and focused existing-tree verification.

Queue base remains a temporary authored input at 0x82DC9784: pointer list +0,
count +1052, snapshot getter +1060, conditional depth policy +1064. Entries
are 52 bytes: texture +8, vertices +12, colour +16, count +32, flags +36,
optional translation +40. The producer has a 512-pointer allocation/limit.
The parent originally snapshots once only at an inferred 1280x720 extent,
sets sampler 8 U/V to 1, begins pass 5, dispatches live entries, ends the pass,
then clears the count. Nonmatching extents clear the queue without drawing.

New `native_deferred_visuals.h` owns that scheduling and bound without guest
addresses or a design-canvas gate. Pass startup establishes mode 5; bit 0x40000
chooses mode 5, otherwise mode 6 uses the entry's translation. Mode changes
reload authored shader/declaration recipes at +72/+76/+80 or +84/+88/+92.
Counts reload after startup and every draw. Entries reload after shader
callbacks, then remain fixed through their primitive preparation. End precedes
queue clear; failures after snapshot recording never replay the original parent.

The whole-function hook uses native scene commands' explicit color-read image,
a bounded post-image lease and the existing `CopySceneSnapshot` implementation.
The source owns dimensions, sample count and mono/two-layer shape; the old
getter adopts the source extent and borrows its native descriptor/layout.
Ordinary attachment MSAA resolves complete before the whole-image copy. No
EDRAM inference, console resolve, old fixed-resolution skip or packed sampler
write executes on the supported native path. Primitive preparation uses native
blend/raster/sampler producers and `DrawNativeImmediateUi`, not the old primitive
helper or guest immediate scratch allocation. An empty queue does no GPU work.

Remaining adapters are explicit: authored queue/vertex production, snapshot
getter/constructor lifetime, shader/declaration/default-texture recipes, draw
state/resource bindings and begin/end material callbacks. Unconverted scopes,
disabled mode and pre-effect refusals retain a counted original path. This is
not complete native effect/frame ownership, nor removal of all emulated resolves
from every compatibility scope.

## Verification and limits

- Source guards: **107 pass**, including four new deferred boundary checks.
- CPU fixture 06 / PID 22496 builds; suite 28 / PID 15132: **31/31**, 6.81 s.
  The new actual scheduler template is exercised over all 1,048,576 low-20-bit
  flag words, alternating modes at full capacity, unchanged initial mode 5,
  startup/draw count changes, callback-updated flags, refusals and exceptions.
  Existing blend and immediate colour/translation/vertex tests remain active.
  This is CPU behavior coverage, not an authored effect image comparison.
- Host target `reblue`, attempt 35 / PID 20472 / session 50592: exit 0,
  linked 13:53:27. New hook discovery reconfigured the existing tree and rebuilt
  host version/metadata users; no guest object or shader rebuild. Codegen:
  zero writes, one up-to-date module. Root `b2910d4` plus this source, Plume
  published `3094b35`.
- Existing strict 8x8 GPU snapshot fixture exercises the unchanged native copy,
  HDR values, both layers, 1/2/4/8 samples and resumed writes: eight configurations,
  zero validation errors/warnings. It was **not rerun** for this scheduling change;
  evidence is in `20260906_0629_native-scene-snapshots.md`.
- Normal flat run log **882**, PID 25340 / session 49485, 13:55:00-13:56:17:
  all five profile settings audited, automatic capture disabled. Periodic sample
  **5,363 empty native deferred calls**, zero compatibility/refusal/fault;
  **zero nonempty deferred calls, snapshots or translated deferred primitives**.
  Sorted scheduler: 2,610 calls / 80,390 primitives with no fallback/faults.
  Native parameter storage: 1,186,885 blocks, zero full legacy blocks.
  Runtime/config/error scans empty; all producers terminal, profile restored
  byte-for-byte. No XR run in this checkpoint; prior log 881 remains distinct
  XR/parameter evidence, not qualification of this new nonempty effect path.
- Inspected one 1920x1080 standing-field image: Shu, foreground rocks/foliage,
  shadows and background depth-of-field look consistent. This is regression
  sanity coverage, **not authored deferred effects, a stability sequence or
  both-eye/full-game qualification**. Those gates remain required before Quest.

| Retained artifact | Bytes | SHA256 |
| --- | ---: | --- |
| Host executable | 47,770,112 | `2cd6c69f33530e92c63f9cf601807588e8fa1fbc2d8ba971562143cb074258b5` |
| CPU parameter fixture | 550,400 | `2df56c679e89c81fba3233dc68a55efc9ce2816d5be7d88278fafdcba8d5f734` |
| `native_deferred_visuals_window.png` | 3,334,419 | `f7eac68514e8575c03ed6fab530b3082e095cfa1f0ff6ace8a472995228f7434` |

The retained normal perf pair is `perf-20260906-135502`, 610,416 B. No new
overall speedup/FPS claim is derived from an empty deferred queue. Earlier
sampled speed counters and their limitations remain in the sorted-scheduler
report; desktop timings still do not establish Quest performance.

## Storage and publication

Continue the original `20260906_0333_native-scene-state-bridge.md` ledger and
limits; preflight 65,237,831,680 B free. Existing tree/tool/inspection reservations
were reused. Gross new build/runtime/perf/PNG diagnostics: 4,237,027 B.
After validating replacements and exact ignored/reparse-free paths with no
active producers, removed ten superseded files: host build 34, CPU fixture 05
and suite 27 stdout/stderr pairs; runtime log 880, perf-131943 pair and the old
sorted-visual field PNG. Historical hashes/results remain in the prior report.
These are regenerable diagnostics, not game data, profiles, assets or trees.
Distinct original-UI, XR, non-MSAA and unresolved-failure evidence is preserved.

Logical removed 4,249,671 B; immediate free 65,228,255,232 ->65,232,515,072 B,
**4,259,840 B measured reclaimed**, counted once. Retained log/perf/PNG payload
shrinks **12,644 B**. CPU fixture grows 19,968 B for the new exhaustive dispatch
coverage within the existing tools reservation; host binary grows 10,752 B.
Reserved diagnostics 64,419,367 B: 94 build logs/139,942 B; 14 runtime logs/
3,955,690 B; 20 perf files/8,967,264 B; eight GPU-fixture files/8,364,855 B;
fixed 41 MiB tools/inspection. Two PNGs/6,668,575 B fit within that reservation.
No new cache, raw/cooked output, download or tree. Replacement retention remains
by verification purpose; retire this normal flat/build/CPU evidence on equivalent
future qualification, not merely because another commit exists.

Post-cleanup free 65,232,515,072 B: drive-wide use +5,316,608 B since preflight,
including metadata/unrelated volume activity. Later docs/Git writes count and
the final handoff reports the remote hash and ending measurement. Normal commit
and push to the named fork are covered by the owner's standing approval.
