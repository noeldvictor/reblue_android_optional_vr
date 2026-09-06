# Native CPU parameter ownership at draw upload

2026-09-06, desktop. Root 3bbe615 plus this change and the existing pending
renderer integration; local clean Plume 3094b35. Guest-source and devloop skills
guided exact writer tracing and bounded existing-tree verification. This is
shared float-parameter ownership, not completion of native materials or frames.

## Ownership change

The address-free `NativeParameterBuffer` owns a bounded array of raw words and
known bits. Publications copy producer data; missing-only import is transactional,
invalidation cannot expose uninitialized data, and no heap/disk cache is added.
Two 4 KiB owners in the temporary shader ABI bridge serve the active device.
Actual zeroed device creation resets both, including address reuse; presentation
resize does not. A device identity switch invalidates both. The renderer retains
its existing uniform upload ring, overrides, native material overlays and lifetime.

Float setters/descriptor flushes capture the actual sequentially written words
and publish them directly, preserving overlapping four-vector/tail copy semantics.
Native transform and deferred producers publish computed host values directly.
Normal transforms no longer read the old 64-word device block; only their
independent original-execution diagnostic needs untouched rows. Deferred byte
imports handle every alignment without typed aliasing or floating-point conversion.

Ordinary VS/PS upload and replay base-block composition consume the native owner.
`CopyGuestVertexBlock`/`CopyGuestPixelBlock` remain independent capture/reference
functions. Existing NaN flushing (not infinity/subnormal/signed-zero flushing),
shadow fitting and shader-specific screen-UV pin ordering remain unchanged.
The default-off `bd_native_parameter_storage_verify` compares full uploaded native
blocks with independently read guest mirrors before shader-specific changes,
forces replay refresh and throws on the first mismatch. It does not execute the
original setters merely to obtain a passing reference.

Compatibility is explicit: guest mirrors/getters, descriptor/source imports,
shader register ABI, bool/fetch constants and inline writers remain. Camera,
viewport, visual setup, foliage and Toon/fur notifications invalidate known rows;
only those missing rows are imported on the next native read. Original UI loops
use a nested exception-safe compatibility scope: draw-time reference imports
inside, owner invalidation/generation advance at exit. Replay cannot reuse a
pre-scope base. Native publishers release the parameter mutex before taking
renderer dirty locks. Counts distinguish native blocks, imported words and legacy
blocks; a native-block count is **not** a fully host-owned frame count.

## Writer findings and failures that changed the implementation

Read the complete render-tweaks and render-list hook definitions before their
callbacks, exact generated viewport/visual/Toon/boolean helper bodies, and the
relevant original node/deferred foliage and fur sequences. Integer constants
live beyond float storage; they are not float-owner writes. Diagnostic node
snapshots are read-only, not guest restore operations.

Three bounded comparison attempts stopped on independently detected stale data:

- Log 870 / PID 17760 / build 27: frame 854, VS c53.x native `00000000` vs
  reference `3f800000`. `bdVisualObjectSetShaderConstants` (0x82143F70,
  generated file 70) writes visual+3544 to VS/PS c53 inline after its c54..57
  setter. It contains no draw; both rows now invalidate on return.
- Log 871 / PID 22052 / build 28: frame 885, VS c57.w `00000000` vs
  `3ef2b022`. Both original foliage paths (node 0x82280488/file 40 and deferred
  0x8227F940/file 84) finish inline c57 with VS bool31 publication before drawing.
  That existing boundary now invalidates c57; ordinary native draws do not
  import it unconditionally.
- Log 872 / PID 26768 / build 29: frame 865, VS c50.x `00000000` vs
  `3ecccccd`. Toon vf04 writes c50/c51 inline, including inherited stack words
  in c51.zw. The original deferred fur loop also writes **both** c50 and c51,
  not just c51 as the old dirty-hook comment claimed. Both paths now invalidate
  that two-row range. No guessed replacement values or disabled verifier.

All three jobs are terminal and restored the owner profile. Their failure logs
can retire after the passing field replacement; this report preserves causes,
exact first differences and source locations. Tested binary hashes for builds
27/28/29: `e81f834e721032403027a556651bbd9b79c0045a92c3dc5c64cdd349bc6108d6`,
`0a55b1275b809c54cab0c249508a97ce1cad198d59bc3131813807d16235af4`,
`50368f5a090f954d0a1bc1771626647edad963820a699fccac2981de8ada769a`.

## Verification

- Final focused fixture build 02/PID 21232 passes; CPU suite 24/PID 26468
  passes **31/31**, 3.57 s. The expanded parameter fixture covers poisoned
  producer data, bit preservation, all 16 source alignments, selective import,
  transactional refusal, invalidation, malformed ranges, stages and lifetime.
  Existing 38,640 overlap/unaligned setter copies and dirty-mask cases remain.
- All **94** source boundary guards pass, including six new ownership/wiring
  guards. These are wiring checks, not substitutes for executed comparisons.
- Final host build **30/PID 26444** exits 0; no guest objects or shaders rebuilt.
  `reblue_vk.exe`, linked 12:11:07, 47,724,032 B, SHA-256
  `9b6a5e8a1a2fd2133716466749aa20e1ba411632dde7092fbff2e4566d1ac830`.
  Parameter fixture 88,064 B, SHA-256
  `13199753909a5d8a6a3079a418e78cfbc137f43647d1346ceff682dbd294cc65`.
- Flat comparison **log 873/PID 27016/session 10611** exits successfully at the
  bounded stop: last reported **34,591 checked native blocks, zero mismatch**,
  245,024 imported words; reached 301 native water updates. These are sampled
  cumulative counts, not final shutdown totals. All six temporary settings
  audited; raw captures and perf CSV disabled.
- Normal flat **log 874/PID 25784/session 55678**, 75-second stop with verifier
  off: last sample **855,492 native blocks, 11,144,896 imported words,
  157,468 legacy blocks**, 1,757,852 publications / 66,980,752 words, one
  initialization reset, zero checks/mismatches. Water 2,701 updates and 1,964
  preparations, zero fallback/refusal/faults. No new raw frames.
- Viewed the bounded **1920x1080** `native_parameter_storage_window.png`:
  Shu, shadow, ground, rocks, foliage and background DoF look sane. This single
  standing Talta-field image is not an isolated water, animated sequence,
  cutscene, battle, transition/reload or both-eye qualification. PNG 3,362,832 B,
  SHA-256 `d48b035b62fa14183db9851d282e81f2f143831e03353c94d4de34edfcc0b3d0`.
  Perf pair `perf-20260906-121336`: 602,112 + 112 B, not a performance claim.

- Desktop XR comparison **log 875/PID 22748/session 14864** exits successfully
  at the same bounded stop: last sample **501,224 checked native blocks, zero
  mismatch**, 1,687,580 imported words and 30,880 legacy blocks; 301 native
  water updates. Reused the existing simulator after reading the full vrsim
  skill. All 17 settings audited; 1440x1584 per eye at scale 1.0, multiview,
  layered images and camera mode 2. Game camera (-245.7,180.4,-16.1) versus eye
  (-249.4,181.7,-16.3) confirms active view composition. No perf CSV or eye
  captures: this is a data-path check, not stereo pixel/comfort qualification.

All producers are terminal and owner profile bytes restored. Remaining work
includes original UI/inline writer replacement, native material parameter schemas
and source assets, removal of mirrors/getters and bool/fetch imports, complete
scene/frame ownership and the full desktop game/both-eye gate before Quest.
Root/dependency publication remains subject to the existing upload approval;
no push or unpublished dependency gitlink staging was attempted.

## Storage closeout

Continue the original ledger in `20260906_0333_native-scene-state-bridge.md`.
No new raw/cooked assets, tools, guest/shader rebuild or configured tree. Gross
new diagnostics across every attempt: **4,865,296 B** (build/test logs 14,394;
runtime logs 885,846; normal perf pair 602,224; PNG 3,362,832). Log-only growth
stayed below 1 MiB; the automatic normal-run perf pair is counted separately,
and all outputs stay within the original cumulative diagnostic reservation.

First cleanup removed reblue_26/cpu_22 stdout/stderr pairs: 6,340 logical B,
8,192 B measured increase. After final build/test/hash/profile/process checks
and normal flat pixel inspection, removed 17 more exact ignored, reparse-free
files: reblue_27/28/29, parameter_test_01 and cpu_23 stdout/stderr pairs; normal
log 869, perf-072301 pair, water-update PNG; resolved failure logs 870/871/872.
Logical bytes 4,430,195; immediate free 64,430,690,304 ->64,435,142,656 B:
**4,452,352 B measured increase**. Combined **21 files / 4,436,535 logical B /
4,460,544 B measured reclaimed**, counted once. Regenerable diagnostics and their
results remain documented; no protected data, old distinct raw/eye/failure
evidence, source, assets or build trees were removed.

Net retained diagnostic payload growth **428,761 B**, chiefly the first flat/XR
independent storage comparisons (100,496 + 321,803 B); replace these on the next
equivalent correctness coverage. Other growth is replacement-size drift. The
same-purpose old flat PNG/perf/log and old CPU/build outputs did not accumulate.
Final reserved accounting **64,385,620 B**: 94 build logs/138,514 B, 14 runtime
logs/3,931,563 B, 20 perf files/8,959,072 B, eight GPU-fixture files/8,364,855 B
and unchanged 41 MiB tool/inspection reservation. Two PNGs/6,696,988 B fit inside
that reservation; the non-MSAA image retains its distinct purpose.

12:21:09 closing check: **64,434,106,368 B free (60.01 GiB)**, drive-wide use
up **1,388,544 B** from 11:59 preflight and **1,028,681,728 B** from the original
baseline. Volume changes include source/build/Git/metadata and other activity,
not just this task's artifact payload; later documentation/Git writes count too.
