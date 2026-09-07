# Explicit queued graphics bindings

2026-09-07, parent `1bf9811`. This removes the existing queue emitter's global
constant-set lookup and fixed three-offset binding contract. It does **not**
complete the native rigid-object path or establish a performance gain.

## Producer, owner and consumer

- `src/gpu/draw_bindings.h`: a bounded value snapshot of up to four descriptor
  sets/eight dynamic offsets, with an explicit pipeline layout. Offsets are
  copied; the producer must retain borrowed GPU resources through the submission
  fence. This is neither a persistent asset identity nor a root/push-constant ABI.
- `draw_bindings_bridge.h`: the temporary engine producer captures device-owned
  sets/layout, current offsets and the current frame's optional occlusion set.
  Native producers can supply a different layout without this bridge.
- `draw.cpp` captures bindings before queueing. `draw_queue.cpp` emits those
  bindings directly. Layout changes rebind required sets; unchanged sets/offsets
  are deduplicated. Invalid snapshots are refused before binding-core side effects.
- Batch/group keys include semantic layout/set/count/offset identity, ignoring
  only the current translated per-instance offset. Exact binding comparisons
  additionally guard merges. Inactive tails/padding are not binding identity.
- A flush snapshots and restores its immediate/post caller's actual engine
  bindings, including offsets, rather than leaving another layout or guessed
  zero constant windows behind.
- Translated instance-record gathering/fallback remains explicitly opt-in.
  Passing native records into that ABI without opting in is refused. The current
  producer opts in; this checkpoint does not implement native instance records.

The removed compatibility consumer is `EmitBindings`' call to
`Video::ConstantDescriptorSet()` and hardcoded three-offset bind. The main
engine producer, record gather, shader wrappers, source index and retained
templates cannot yet be deleted. Native light/fog ownership and the explicit
C++/GPU input layout remain unfinished; the existing scalar evaluator is not
that raw layout. Next: connect one source-free model/instance primitive packet
and native shader pair to this same queue for direct scene and shadow drawing.
No second renderer or broad adapter expansion is needed.

## Verification

The existing `host_draw_intent_test` target now includes a source-free command
fixture. It checks snapshot lifetime, deduplication, layout switches with equal
set pointers, a native one-uniform layout, multiple dynamic sets, sparse/empty
layouts, exact caller restoration, batch identity and atomic invalid-input
refusal. GPU object lifetime is a producer obligation, not proven by opaque CPU
fixture objects.

Fixture18 and `draw_intent_cpu16` pass: 0.03 s behavior /0.05 s CTest. The final
Python pass runs 228 source/scenario tests in 0.056 s and eight runner tests in
0.007 s. The new scenario gate requires fresh post-event binding counters,
rejects refusals even before later successful samples, and bounds input at
400 KiB. These timings exclude process startup and are not a dev-speed multiplier.

Host70 rebuilt the shared-header-dependent host objects and linked successfully;
no guest object or shader rebuild. Run917 used that binary. After the run, the
unsupported-record-ABI error was capped at four messages. Host71 rebuilt only
`draw_queue.cpp` and relinked in 4.270 s; the codegen up-to-date probe wrote no
generated files. No guest objects rebuilt. Host71 was not rerun on GPU: only the
failure diagnostic changed, not the verified successful draw path.

| Artifact | SHA-256 |
| --- | --- |
| Host70 executable actually run | `DF95FC81C89DBF54750B6FB78674B993B9F2B5D36AD727995C76A99D1F7E0391` |
| Final host71 executable | `6F1FC4A4E8C0701A29EE9D8024BA54A77B93B70D1CC4C88FD53CA997471A9E6D` |
| Binding/intent fixture executable | `47BDB0F7FF81E1A53EE2E646C811D5145C48B0112D334684498E8287ACF44001` |
| `logs/reblue_917.log` | `A31D1CADF803E018C51DC23B8080719919BA39BA340083438F0DDA021B0C8F20` |
| `out/verification/native_draw_bindings_window.jpg` | `520C1EFEA91D620321FCC90087E6D45FA0FC7F46068490C9E3681A3E56DA3E94` |

### Flat field run917

Owned PID31508, terminal success, approximately 01:47:03–01:48:01 EDT. Existing
bounded wrapper: 75 s maximum, 400 KiB log, one 1920x1080 quality60 JPEG at most
160 KiB, no raw/perf/dump output, exact profile restoration in cleanup. Native
MSAA and precaching enabled; model/material/geometry/instance/table/pulling/
canonical/shadow/texture/policy/lit/movement checks plus `--draw-bindings` enabled.

All 14 audited overrides were effective: autoplay on; performance CSV and
automatic capture off; capture threshold/count 600/120 (inactive); native
material comparisons on; precache, native instances, shadow inputs, material
textures, primitive policies, host materials and native texture tables on;
texture-table comparison off. The original 116-byte profile was restored with
SHA-256 `2F1BC38D763A1B7BDBA31F560684FD4AA7E42A714600B8D344F19DA7F38E23B0`.

Fresh interactive `bg41_01` samples follow the opening event. The binding
samples immediately preceding ready contexts2049/2349 increase by **196,347
emitted draw commands, 128,465 descriptor binds and 900 layout binds**. Indirect
commands count their emitted spans, not just API calls. Layout counts include
start-of-flush binding; they do not prove alternate layouts ran on the GPU.

Other readiness-gated checks pass: +109,806 matching pose reads, +20,898 normal
table lookups with zero original comparison/fallback calls, +150,814 native
vertex-input/pulled records, +137,855 canonical draws, +45,716 load-owned geometry
draws, +15,489 receiver checks, +58,623 material texture checks, +59,453 native
cull replays and +56,936 normal-lit queued uses. Movement observes one episode,
151 additional observations and 172.750286 world units over 11.998 s. Source-free
GPU loads, live cull changes and compound refreshes remain zero; no coverage claim
for those paths. Instancing, pulling and indirect submission remain live.

Inspected the actual 128,595-byte JPEG: Shu running beside the blue bell
structure, fence/ground/bushes/cliff and cast shadow remain visible and coherent.
Thin dark cliff marks and distant blur remain. One image is not a stability
sequence, numerical GPU parity, reload, both-eye or full-game qualification.
The alternative native layout is CPU-tested only. The actual native rigid
model/content family and disabled-interpreter cold-load/reload harness remain
to be selected/implemented.

## Storage and retention

All attempts share the existing ledger in
`20260906_0333_native-scene-state-bridge.md`; no new allowance. The owner-approved
3 GiB exception, operational floor62,509,998,080 B, 100 MiB diagnostics, 10 MiB
aggregate logs/images and **zero new raw allowance** remain. Protected historical
raw evidence (252,177,116,500 unique B), baselines and motion/failure sets were
not changed. No new asset-cache, shader-dump, performance or raw files appeared
during run917. The original mesh cache still has3,510 files /36,510,144 B.

After validation, removed six exact superseded agent files: run916 log, its
named-lit sanity JPEG, and host69/old draw-intent01 stdout/stderr. Logical size
368,777 B; immediate free space increased63,214,342,144 ->63,214,718,976 B:
**376,832 B (368 KiB) reclaimed once**. Reports/hashes remain; the old runtime
image/log are no longer retained. Build logs can be regenerated. Material17/
CPU15 numerical evidence remains because the new fixture does not replace it.

Known comparable retained fixture/log/image growth is374,799 B: new binding
fixture object322,943, fixture exe+40,960, aggregate build logs+31,190,
runtime log−12,282 and JPEG−8,012. This adds native-layout command coverage;
replace the evidence by purpose at the next equivalent qualification. Other
object/metadata/helper/source deltas lack complete baselines and are not zero.
Final exe/PDB total grows11,264 B. Build logs total177,419 B; image aggregate
10,234,995 B leaves250,765 B overlap headroom.

First free63,354,544,128 B; 01:57 closeout measurement63,181,537,280 B (58.84 GiB),
drive-wide use173,006,848 B (165 MiB). That is not all attributable to this task;
identified retained outputs do not explain other volume activity. No owned
renderer/build process remains. Final commit/free-space reconciliation belongs
in the same cumulative ledger and checkpoint handoff.
