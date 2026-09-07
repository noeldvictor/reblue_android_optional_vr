# Native pipeline programs in the shared backend

2026-09-07, parent `9d3bab0`. Removes the pipeline builder's mandatory
`GuestShader`/main-layout contract for native programs. **No game producer uses
the new branch yet.** This is not a native rigid-object conversion or speedup.

## Implementation and remaining consumer

`src/gpu/pipeline/native_pipeline_program.h` owns an immutable compiled vertex/
optional pixel shader pair, pipeline layout, native vertex input and up to eight
explicit specialization ID/value pairs. Layout, vertex shader and input are
required; duplicate specialization IDs and oversized arrays are refused. Names
and specialization values survive destruction of their source storage. Shader
resource allocation remains the producer's budget; this owner does no imports,
compilation, file IO or source-memory lookup.

`PipelineState::native_program` is a runtime-only pointer to that owner. Mixed
native/translated shaders, declarations, vertex inputs, specialization masks,
instancing flags or occlusion fragment replacements are refused. An absent pixel
shader is explicit depth-only intent, never permission to inherit another draw's
shader or silently omit required colour. Engine-origin draw intent clears the
native field when returning to the compatibility producer.

The existing pipeline cache consumes the program's shader/layout/input/spec
values, reusing its existing raster/depth/blend/target/multiview/foveation pipeline
construction. Native builds bypass `GetOrLinkShader`, `MainPipelineLayout`, the
translated id-0 specialization mask and shader-hash colour-write workaround.
No parallel renderer or copied guest pipeline template was introduced.

Callers hold a program lease until enqueue/build returns. Async work takes its
own lease; successful cache entries retain it and destroy the pipeline before
releasing its program/resources. This also prevents native program addresses
being reused while their pointer-keyed cache entries remain. Native failed work
removes its dedup key, allowing retry without retaining a retired pointer's key.
Enqueue exceptions roll back pending tokens, native quota and dedup insertion.
Native retention is bounded at256 queued/compiling requests and2,048 cached
variants. Capacity refusal does not evict potentially in-flight pipelines.
Legacy cache/queue capacity policy is unchanged.

The packed state grows166 ->174 B. CSV/header records are named fields, not raw
struct images: their schema remains unchanged, existing designated initializers
default the appended field to null, and native rows are excluded from console
PSO recording. In-memory keys are recomputed; generated guest files were not
edited or rebuilt. Plume and its gitlink are unchanged.

The removed dependency is the shared builder's unconditional guest shader/layout
selection. Its engine branch remains because live draws still use it. Next:
provide the real named rigid vertex/pixel shader pair and C++/GPU input layout,
native light/fog owners, source-free model/instance primitive packet and direct
scene/shadow producer. The current decoded material ranges, source index,
translated instance gather and captured templates are not that final contract.
The selected model/family must be recorded, and its interpreter/capture/replay
disabled before cold-load and reload acceptance. Additional empty contracts or
broad compatibility expansion are not substitutes for connecting these producers.

## Verification

The new C++ fixture is part of the existing `native_texture_binding_test` target,
using real Plume interface types with mock shaders, not a GPU. It checks explicit
native descriptor construction, specialization copy/IDs/zero values, null/size/
duplicate refusal, every mixed-state field, depth-only selection and leases
surviving source/library/producer teardown through simulated pending/cache owners.
It does not run actual worker threads, fault-inject the queue, fill cache quotas
or validate native SPIR-V on a GPU; those integration checks remain.

- Binding fixture19/CPU17: pass, 0.05 s test /0.07 s CTest.
- Intent fixture20/CPU18: pass, 0.03 s /0.04 s, including native-field clearing.
- 231 Python source/scenario tests: pass, 0.047 s; eight runner tests:0.007 s.
  Wiring guards cover async/cache leases, retry/quota/rollback integration and
  native CSV exclusion; source guards are not behavior proof for concurrency.
- Host72/PID28040: terminal success,97 scheduled CMake/host edges. No guest
  objects or shaders rebuilt. Existing CRT deprecation warnings remain.

### Existing-path field regression, run918

Run918/PID24168,02:13:14..02:14:13 EDT, normal flat1920x1080/native MSAA/precache
on. The existing bounded wrapper enforces75 s,400 KiB log, one quality60 JPEG
<=160 KiB, raw0 and exact profile restoration. All14 settings were effective:
autoplay on; performance CSV/automatic captures off; inactive capture threshold/
count600/120; material comparisons, precache, native instances, shadow inputs,
material textures, primitive policies, host materials and native tables on;
table comparison off. The original116-byte profile was restored byte-for-byte.

Fresh post-event `bg41_01` contexts2040/2340 qualify existing engine rendering:
+111,862 matching pose reads; +20,912 normal table lookups, zero original
comparison/fallback calls; +155,043 native-input/pulled records; +142,063 canonical
draws; +47,576 load-owned geometry draws; +15,503 shadow/policy checks;
+58,651 material texture checks; +61,434 cull replays; +57,533 normal-lit queued
uses; +200,673 explicit draw commands /134,342 descriptor binds /900 layout binds.
Movement: one episode,30 fresh observations,+37.625316 world units,12.940 s
walking. Source-free GPU loads, live cull changes and compound refreshes remain0.
Reflection checks remain0. These do not qualify those unexercised paths.

Inspected the135,961 B1920x1080 JPEG: Shu running beside the blue structure,
fence, ground, vegetation and cast shadows are coherent. Thin dark cliff marks
and distant blur remain. One image is not a sequence, reload, both-eye or whole-
game qualification. Native programs have **CPU coverage only**, not a GPU-use
counter: source inspection finds their creation only in the fixture.

| Artifact | SHA-256 |
| --- | --- |
| Host72 executable actually run | `27FD0888A8CB98E030BA232F05C3A98AF597128DC8C378E8B18750BAA43922E9` |
| Binding/native-program fixture | `67C7074F8F15D206A6FAD0677D114921546D3DA9BF2D4417FDD8B156E79466DC` |
| `logs/reblue_918.log` | `4A8DB9E9277C2B1E4ED13CCE376B494F88E55E8AF092E2DD9ECFF7A9A6D7ACD9` |
| `out/verification/native_pipeline_regression_window.jpg` | `9F7318310B1EB427A3D019F129CE68C3DE47705774F4AA31B3408F5B2D199138` |
| Restored owner profile | `2F1BC38D763A1B7BDBA31F560684FD4AA7E42A714600B8D344F19DA7F38E23B0` |

## Storage

Same cumulative ledger `20260906_0333_native-scene-state-bridge.md`, original
start65,462,788,096 B, owner-approved3 GiB exception and operational floor
62,509,998,080 B. No reset. Plan<=3 MiB fixture/log growth,<=512 MiB temporary
host build overlap. Existing trees reused; zero new cache/dump/perf/raw files
after runtime start. Protected historical raw252,177,116,500 unique B, baseline,
motion and failure evidence unchanged; original game data/profile untouched.

After replacement validation, removed14 exact superseded files: run917 log/JPEG,
binding05/CPU05, intent18/CPU16, host70/71 stdout/stderr.380,999 logical B;
immediate free63,334,731,776 ->63,335,129,088 B, **397,312 B (388 KiB) recovered
once**. Old reports/hashes remain; old runtime files are gone, build logs are
reproducible. Current material17/CPU15 numerical evidence remains distinct.

Known comparable retained fixture/log/image growth890,087 B: new fixture object
809,228; binding exe+60,928 (now256,000); intent exe unchanged66,560; aggregate
build logs−727 (now176,692); runtime log+13,292 (232,086); JPEG+7,366 (135,961).
This adds native program/lifetime coverage, to be replaced by purpose at the next
equivalent qualification. Other object/metadata/helper/source deltas lack full
baselines, not zero. Host exe/PDB total grows162,816 B. Image aggregate10,242,361 B
leaves243,399 B overlap headroom. No owned producer remains.

Current first free63,012,909,056 B; cleanup ending63,335,129,088 B (58.99 GiB):
drive-wide gain322,220,032 B, not task savings. Only397,312 B is attributed to
cleanup; other volume activity is not fully explained by identified outputs.
