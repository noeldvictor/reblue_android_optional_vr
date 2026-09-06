# Load-owned geometry and primitive associations

2026-09-06, EDT. Previous goal checkpoint made progress: bounded mesh storage,
now published as `cf95217`. This checkpoint advances the active geometry producer
and consumers; it does not finish the native static-object/frame path.

## Implementation and source contracts

Read the physical/predictor hook manifests, complete generated
`bdSceneGraphNodeProcess` (file 46, 9719..10965) and `bdSceneNodeDrawIndexed`
(file 65, 10125 through its return). Node processing builds the mesh's counted
8-byte index table and counted 12-byte vertex records. Vertex records carry
vertex count, declaration-cache slot and buffer. The declaration is slot+12,
also observed by the existing predictor. Registered physical byte length divided
by vertex count recovers the expanded stride. The indexed draw uses triangle
strips, base vertex zero, StartIndex from operand two and count operand one+2.

- The existing post-builder model publication now resolves those tables once.
  A reader-injected, checked decoder rejects missing/null words, out-of-range
  records, unsupported streams, zero counts and address arithmetic overflow.
- Supported physical single-stream base geometry is imported/uploaded during
  loading, before model publication, independently of optional PSO precaching.
  Primitive ordinal associates native geometry with its material. Identical
  geometry under different materials retains separate primitive records.
- Geometry carries its source-content ID, layout key, stream strides and GPU
  views. The source address index stays outside the native program, is bounded
  by the existing model owner and retires with its graph. Aliasing leases retain
  immutable associations across source reuse. The separate 256 MiB GPU arena
  still retains cached allocations; streaming reclamation is not implemented.
- Four material/skin/reflection/shadow-policy consumers no longer read guest
  IB/VB association tables. Live control-table values, visual inputs and other
  explicitly tracked source inputs still remain.
- Converted base-geometry replays select the load-owned handle before the old
  importer. Unsupported/deferred/layout variants and generated LODs retain the
  unconverted importer. Declaration-compatible packed layouts and shader ABI,
  source keys, templates and original graph loading are not final native APIs.
- Diagnostic mode independently recomputes geometry identity from the actual
  draw's buffers: every new binding plus a warm binding each frame. It compares
  the resulting content-keyed handle, not just source addresses. Normal rendering
  does not perform that comparison. Diagnostic imports read existing caches but
  suppress new mesh writes; normal persistence keeps the bounded storage policy.

## Verification

The existing material fixture now covers source-table decoding, bounds/overflow,
destruction of source data before native lookup, distinct materials sharing
geometry, association count validation, retirement/reuse and concurrent leases.
Its core tests have no GPU/runtime requirement. These are association/lifetime
tests, not standalone native-scene pixels.

- Debug build 04 failed on a test variable-name collision, corrected in build 05
  (PID 28088). CPU 04 passed, 0.13 s. Release configure 01 /build 06 (PID 28948)
  /CPU 05 (PID 28588) passed, 0.12 s. Checks remain enabled under `NDEBUG`.
- Host builds 44..47 passed, host objects only; no guest objects or shaders
  rebuilt. Build 47 PID 26316, about 6 s, is the final runtime binary. Later
  changes are comments/source guards/docs only, not another binary qualification.
- Host executable 47,832,064 B, linked 17:37:22; SHA-256
  `a4eb57a12c773e3b94bf173ee18c0a6df3abb3fa46cc74af769d87e3551fabef`.
  Source: `cf95217` plus this checkpoint's reviewed implementation.
- Material fixture executable 254,976 B; SHA-256
  `2d092736e64750123eadece8dc73fafdc3443091f8b9bad54581d087a02b7e46`.
- All **130 source guards pass**, 0.029 s. They do not replace behavioral or GPU
  evidence. New guards enforce load-before-publication, source-table-free
  consumers, native-first selection, warm diagnostic sampling and lease accounting.

Three bounded flat PSO-off attempts shared the existing 75 s /400 KiB-log limits:

1. Run 891, PID 17900, 17:34:40..17:35:56: 2,973 primitives loaded, 325 geometry
   checks matched, but checks stopped once cached. The required fresh field delta
   failed. Fixed diagnostic sampling rather than accepting startup evidence.
2. Run 892, PID 28840, 17:38:04..17:38:55: fresh field counter gate passed;
   image helper failed on assignment to a ValidateSet-constrained parameter.
   No image was written. Fixed its local destination variable, then tested the
   actual parameter/routing prefix in memory for both PNG and JPEG.
3. **Run 893**, PID 24096/session 73810, **17:40:22..17:41:14**, passed counters
   and window inspection. All seven profile settings audited: autoplay on,
   perf/capture-after off, dormant capture thresholds 600/120, native material
   comparisons on, PSO precache off. Each attempt restored the 116-byte owner
   profile exactly; every producer/session is terminal.

Run 893 observes an opening event, then frames **1756 and 2056** in `bg41_01`,
FieldActive/state 0, player present, event/movie/loading/icon absent. Between
those post-event samples: **51,173 load-owned draws**, **2,650 new matching
geometry checks**, **15,253 diffuse /14,541 specular matches** and 61,309 model
lookup hits. Zero wrong checks, model load/missing/input/budget failures.
2,973 primitive geometries loaded across 114 publications; one retirement,
113 live model owners /930,680 B. The final sample has 761 unavailable geometry
lookups: this is not complete replay-path migration. Reflection checks are zero.

The image shows standing Shu, foreground rocks/foliage and terrain. Background
blur remains visible. It is one flat sanity image, not movement, sequence,
shadow-system, both-eye, effect-event or full-game qualification. No Quest or
controlled performance run. Existing normal Toon flat/XR and unresolved failure
evidence remains protected.

Retain `out/build/win-amd64-release/logs/reblue_893.log` (179,253 B), SHA-256
`5473f8c507e98cc8ff7d648d490949c0405d2d3e1a00f5235ac67cbb84fb82b6`, and
`out/verification/native_model_geometry_window.jpg` (1920x1080, quality 95,
373,929 B), SHA-256
`bb05ff0af14c75bec9e13b4dc7882a556a8c190a06a790db94fc6c3b437c2cb3`.
JPEG encoding is lossy; it is not exact pixel-diff evidence. The helper enforces
1 MiB for this image and the unchanged 10 MiB aggregate across PNG/JPEG together.

## Storage and next boundary

Continue the original cumulative scene-state ledger, without resetting its
2 GiB peak /100 MiB diagnostics /10 MiB build-log /20 GiB-reserve limits.
Initial free **64,970,539,008 B**. Planned <=256 MiB temporary build overlap and
<=4 MiB fixture/log growth; actual expanded Debug fixture grew 1,167,864 B before
Release replacement. Runtime reused assets, no raw/perf/shader dumps/downloads.
Mesh cache remains **3,510 files /36,510,144 B**. No cache/dump files changed during
this checkpoint. The log's 1,170 meshes "cooked" means built in memory here, not
written to disk: mesh disk writes are zero. GPU arena remains 32 MiB in this run.

Material fixture exe/library/objects initially 15,101,171 B; final Release set
**2,613,079 B**. Removed the obsolete test PDB, one unreferenced old test object,
superseded build/test logs, old field log 890 and failed/replaced logs 891/892
only after their replacements passed. Original baseline/failure PNGs and raw
evidence, game data and active build trees are unchanged. Deleted diagnostics
are reproducible; their historical findings above/in earlier reports remain.

First exact cleanup: 17 files /4,115,765 logical B; free
64,946,659,328 ->64,950,788,096, **4,128,768 B reclaimed**. Second: eight files /
790,356 logical B; free 64,960,868,352 ->64,961,671,168, **802,816 B reclaimed**.
Total **4,931,584 B actually reclaimed**, counted once. This excludes the
previous mesh-storage checkpoint's already-reported cleanup.

Fixture plus retained runtime/image payload shrank **12,111,253 B** versus this
turn's starting files. Another 3,229 B of new final build/test logs remain;
therefore these diagnostic categories shrink by at least **12,108,024 B**, even
without crediting removed old build logs. New JPEG adds scoped geometry evidence;
replace it with equivalent or stronger geometry qualification, not a per-commit
archive. Post-cleanup free **64,961,671,168 B**, drive-wide use **8,867,840 B** from
initial inventory, not attributed to the shrinking task payload. Final docs/Git
writes remain charged. No larger runtime-tool reservation is needed.

Next: complete native instance/update and texture/pass contracts, independent
layouts and direct scene/shadow consumption of these records. Remove the source
index, per-draw resource checks and retained interpreter templates for that path;
then movement/reload and broader desktop/stereo qualification. The full renderer
and modern-GPU requirements remain active before Quest 2 optimization.
