# Host immediate UI preparation and geometry

2026-09-06, desktop; root 7ce40af plus this change and the existing pending
renderer integration, local clean Plume 3094b35. Guest-source/devloop guided
exact source tracing and bounded verification; vrsim guided the existing
desktop runtime check. No Quest work or complete-native-frame claim.

## Change and remaining boundaries

Read the complete exact `Visual__DrawVerticesUP` at 0x82425710 (generated file
53), `Visual__DrawSortedQueues` at 0x82424AF8 (file 81), and the current host
Begin/End/upload/overlay implementation. Immediate vertices are a six-word,
24-byte triangle-strip input, not the distinct 20/28-byte glyph/sprite layouts.
The whole immediate body now executes native colour/default initialization,
optional translation, publication and direct host upload/submission. No normal
guest Begin/End allocation, guest memcpy, or original immediate callback.

The bounded CPU owner holds at most 65,536 vertices / 1.5 MiB, accepts all byte
alignments, preserves vertex bits and invalidates its visible count on refusal.
Only submitting threads allocate it, lazily; no disk cache or CPU readback of
mapped GPU upload storage. Upload lifetime stays in the existing fence-managed
host allocator. Scoped overlay classification restores on errors as well as
normal return; shape heuristics are not broadened.

The temporary parameter importer preserves sequential overlapping colour writes,
lazy-default side effects even with explicit colour, dirty-mask high/low halves,
and null-translation inheritance. Computed colour and translation publish into
native parameter storage directly; guest getter mirrors remain. Unsupported
counts/unaligned parameter pointers and stack/self-modifying vertex aliases are
explicit pre-effect fallbacks. Nested entry cannot overwrite an active CPU copy.

`bd_native_immediate_ui` defaults on. The default-off verifier plans without
publishing, executes the original exactly once, and observes the actual original
EndVertices scratch before upload: parameters, topology/count/stride and every
vertex word must match. It fails mismatches rather than overwriting the reference.

The sorted scheduler is deliberately still tracked as original: it handles
models, callback-driven setup, shader/state choices and deferred primitives as
well as these draws. Its surrounding legacy parameter scope still imports full
blocks. Replacing just this immediate body does not remove that scope, authored
vertex production, native UI assets/materials, shader-register/bool/fetch/state/
texture adapters, emulated deferred resolves or complete frame/game gates.

## Verification and exact binaries

- CPU fixture build 03 / PID 22236 passes. Suite 25 / PID 25656 passes 31/31,
  3.56 s; 99 source guards pass. Expanded fixture compares 256 original-order
  publications including aliases/nonfinite bits/default/translation states,
  16 poisoned-source vertex alignments, malformed counts/ranges and stale-data
  refusal. Existing 38,640 parameter copies remain passing. Fixture 112,128 B,
  SHA-256 `40243c8ac9fa0f264a06c83702d2f6f256f28ab0d84777301f4bb5b560a86120`.
- Host build 31 / PID 24864 / session 69277 passes without guest/shader rebuild.
  Binary 49,311,232 B, SHA-256
  `1416bcab4f8f3952f6056bced5d957e520062cda4267f3b057eb95e7beb0fbbc`.
- Flat original UI + independent native-parameter comparison log 876 /
  PID 27056 / session 13646 passes: last samples 16,664 checked UI preparations
  and uploaded vertex payloads, zero wrong/fallback/refusal; 203,105 checked
  native parameter blocks, zero wrong; 301 field water updates. All seven
  temporary settings audited, raw/perf capture disabled.
- Normal flat log 877 / PID 24072 / session 94346 reaches its 75-second stop:
  84,502 native immediate submissions, zero fallback/refusal/upload failure;
  856,741 native blocks, 11,139,284 imported words, 168,936 legacy blocks.
  Native water updates 2,701 without fallback/refusal. Translated and empty UI
  entries were not observed; those branches currently have CPU coverage only.
- Viewed `native_immediate_ui_window.png`, 1920x1080, 3,352,997 B, SHA-256
  `42a9f3943b25a031d1f850aaedc1d234d1f1b1e47da40bc02554d70a558a88ff`.
  Shu, shadow, ground/rocks/foliage and background DoF look sane. This is a
  standing Talta-field image, not menu/title/animated UI, sequence, both-eye or
  full-game qualification. Perf pair 124446 totals 610,416 B, not an FPS claim.
- After the flat check, changed only CPU owner allocation from a large TLS
  value to a lazy per-submitting-thread heap object. Host build 32 / PID 19944
  passes; final binary 47,738,880 B, linked 12:47:05, SHA-256
  `700aaa0d3e42fd3e8c7f7778ba98714873f4fb9c6a76632b9d7c3919ae14c0d2`.
  This removes 1,572,352 logical binary bytes versus build 31; not separately
  credited as physical disk reclamation. The flat image above is from build 31.
- Final binary desktop XR parameter check log 878 / PID 26828 / session 4754
  passes: last reports 35,350 normal native UI submissions, zero fallback/
  refusal/upload failure; 264,067 checked native blocks, zero mismatch; 301
  water updates. All 17 settings audited; 1440x1584 per eye, scale 1, layered
  multiview, camera mode 2. Logged eye differs from game camera, confirming
  active composition. No new eye images/perf/raw frames: data-path/lifetime
  verification, not stereo visual qualification.

All producers are terminal and the owner's exact five-line profile is restored.
No new build/game jobs launched after the owner's status interruption. A normal Plume push
was retried after the renewed push request and rejected again by auto-review:
the destination must be explicitly authorized. No workaround, root push or
unpublished dependency gitlink staging. Local checkpointing continues.

## Storage closeout

Original cumulative ledger remains `20260906_0333_native-scene-state-bridge.md`.
Initial cleanup's ancestor-type check refused before deletion; corrected it
using actual FileInfo/DirectoryInfo types and revalidated every exact ignored
path/reparse-free ancestor with no active producers. Removed 13 superseded
files: build 30/31, fixture 02 and CPU 24 stdout/stderr pairs; logs 873/874,
perf-121336 pair and the older parameter PNG. Logical 4,351,617 B; immediate
free 65,271,209,984 ->65,275,572,224 B, measured reclaimed 4,362,240 B.
After equivalent XR coverage, removed log 875 / 321,803 logical B; immediate
free 65,273,499,648 ->65,273,823,232 B, measured reclaimed 323,584 B.
Total 14 files / 4,673,420 logical B / **4,685,824 B measured reclaimed**.
Regenerable diagnostics only; reports preserve results/hashes. No game data,
assets, source, build trees or distinct protected evidence removed.

Gross produced diagnostics 4,677,313 B, net retained payload growth 3,893 B:
combined original-UI/parameter coverage replaces the earlier parameter log,
with small replacement-size drift. Replace at equivalent future coverage.
Reserved accounting 64,399,348 B: 94 build logs / 138,209 B, 14 runtime logs /
3,937,404 B, 20 perf files / 8,967,264 B, eight fixtures / 8,364,855 B and
unchanged 41 MiB tools/inspection reservation. Two PNGs / 6,687,153 B fit inside
that reservation; cache outputs since the original checkpoint: zero files.
No new raw capture, asset cooking, tool download, guest/shader rebuild or tree.

Closing read-only check before documentation/Git writes: 65,273,823,232 B free
(60.79 GiB), drive-wide use +28,876,800 B since 12:41; +188,964,864 B since the
original baseline. Drive-wide changes include unrelated activity and metadata,
not solely this task. Later documentation/Git changes count too.
