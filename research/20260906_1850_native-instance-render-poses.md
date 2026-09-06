# Native instance identities and completed render poses

2026-09-06, EDT. The previous planning turn was read-only, not implementation
progress. This resumes the unpublished instance draft on `ad5aa25`. It advances
render-pose storage/consumption, not complete native animation or direct submission.

## Implemented boundary

Native instance IDs refer to model publication generations. Immutable poses hold
native matrices, no guest addresses/resources/register state. Limits: 4,096
instances, 4,096 matrices per pose, 16 MiB including pinned retired/replaced poses
and index bookkeeping. Unchanged poses reuse storage; invalid publication clears
stale state. IDs/generations cannot alias a reload. These are runtime identities,
not persistent scene-asset IDs.

`bdVisualObjectInitBones` establishes identity only: later writers change its
output. The actual derived render-copy callback `sub_8213F5E8` publishes after
the original conditional copy, only when its input dirty word equals **3**.
Import the completed render palette once there. Native staging/render slots
share one immutable lease; there is no second native matrix copy. Normal host
traversal/replay world-transform lookup neither discovers instances nor imports
matrices. Verification independently compares current source values.

Full model-unload entry `sub_82140DF8`, also called by the base destructor,
retires identity/source bindings before original resource release. The source
index, original pose calculation/copy, secondary palettes, per-draw skin gathering,
guest topology, templates, shader ABI, independent layouts and direct scene/shadow
submission remain. This is not a fully native instance/animation/frame path.

Source evidence: physical/predictor manifests and complete generated pose,
getter, allocation, unload and copy routines. Getter `sub_820FC3F8` selects
container+8/+20 then vector.data. Allocation/unload both confirm **visual+2632
(0xA48)**, correcting the old header's 0xA40 metadata pointer. Also corrected
the radius-scale comment: complete `bdSceneTreeDraw` selects maximum, not minimum.
Constructor `sub_8213F588` installs **0x8206A9DC**; a bounded read of the existing
`out/native_view_basefile.image` maps its first word to **0x8213F5E8**. This callback
copies vector0.data to vector1.data using vector0.count*64 for state 3, then clears
the word. Adjacent base helper `sub_82144D10` was not the active callback. No new
dump/decompiler/download; generated source remains unedited/uncommitted.

## Verification and corrections

The Release material fixture uses the production reader-injected boundary:
literal layout/overflow, thread selection, late edits, delayed render publication,
wrong generations, source reallocation/partial copies/destruction, retirement,
concurrent immutable leases, finite/count/byte limits and pinned backpressure.
Checks remain enabled in Release. Material builds 07..10 /CPU 06..09 pass; final
build 10 PID 24420, CPU 09 PID 28280 **0.14 s**. Host builds 48..53 pass; final
53 PID 24676 **2.85 s**, one host object plus link. No guest objects/shaders rebuilt.
All **137 source guards** pass (0.031 s), **11 scenario cases** pass (0.001 s).

Final host exe **48,125,440 B**, linked 18:44:03, SHA-256
`b38e59e35e8882551845b4b8a0d5cf36baf833bf23d0b55861df0a9ab84a2f5a`.
Fixture **287,232 B**, SHA-256
`cf8a246a2778f2065cd0ab5aa867443b0d9d885687e76b048c633cc1bee0f483`.

All attempts share normal flat MSAA, PSO precache off, native material comparisons
and autoplay on, raw/perf captures off. Supervisor: 75 s, 400 KiB comparison log,
cumulative byte/reserve limits, exact profile restoration in `finally`.

| Run /PID | Outcome |
| --- | --- |
| 894 /31044 | Wrong 0xA40 offset caused invalid imports; corrected from producer/unload evidence. |
| 895 /26844 | Valid imports, 0 reads /1,021,252 unavailable lookups: missing render association. |
| 896 /30240 | Bounded examples prove matching visual/model but different render palette. Base helper has no tracked handoffs; inspected installed vtable. |
| 897 /25284 | Derived callback executes; positive reads but pose mismatches. Failed, not accepted. |
| 898 /28728 | First-drift probe stops in ~19 s. Before-handoff matrix 3/element 0 differs: approximately -1.19e-7 versus 0.020496776. Proves late writers after InitBones. |
| 899 /26264 | Final binary: 1,019,906 matching reads, 0 misses/refusals/drift. Initial checker times out on thread-report ordering. Corrected offline checker accepts fresh windows and still rejects 895/897; no image. |
| 900 /16044 | Same final binary; corrected gate/image pass, **18:47:31..18:48:28**, ~57 s. |

The checker pairs consecutive recent field observations with samples following
each observation. Another thread's extra context cannot discard completed windows.
Startup/stale/nonconsecutive samples, scene changes, refused updates, mismatches
and zero native execution remain failed/pending, covered by negative fixtures.

Run 900: **240 created, one retired, 239 live /368,896 B**. Samples following ready
contexts 1753/2053 add **118,851 matching pose reads**, **33,600 handoffs/imports**,
zero pose misses/refusals/mismatches. Final totals: **544,252 matching reads** and
166,826 handoffs. Geometry/material contexts 2053/2353 add **51,624 load-owned
draws**, **2,700 matching geometry checks**, **15,425 diffuse /14,685 specular
matches**. 2,973 primitive geometries, 114 model publications, 0 model failures.
Separate geometry fallback count 756; reflection comparisons zero. All eight
profile settings audited; every run/session terminal, 116-byte owner profile
restored exactly. No Quest/XR run, no controlled performance measurement.

Inspected 1920x1080 image: standing Shu, terrain/rocks/foliage, fences and shadows,
without obvious new deformation; distant blur remains. **One lossy image does
not qualify movement, sequences, stereo, effects, reloads or broader scenes.**

Retain log 900 **204,108 B**, SHA-256
`eef1bded1bca215584920921f204554bc8b0e98a8f7620a846fc790e1613a1c6`,
and `out/verification/native_instance_pose_window.jpg`, quality 95, **399,365 B**,
SHA-256 `da543c14aa7d77669a9d9516bc33e32c9e7c874d90c125f19ef1060ad6d464f5`.
Replace equivalent scoped evidence; preserve normal flat/XR and broader failures.

## Storage and next boundary

Draft starting free **64,957,763,584 B**; pre-cleanup **64,887,549,952 B**. Same
original cumulative ledger/budgets, no reset: <=256 MiB build overlap, <=4 MiB
fixture/log growth; 2 GiB peak /100 MiB diagnostics /10 MiB build-log /20 GiB
reserve ceilings. Final fixture exe/library/objects **2,768,014 B**, versus
2,613,079 B (**154,935 B growth**) for new behavioral coverage. Retain one fixture.
No new raw/perf/cache/shader dumps since the draft began. Mesh cache unchanged:
**3,510 files /36,510,144 B**. Raw archive, protected evidence and game data unchanged.
After replacement validation, removed **36 exact superseded agent-created files**:
host build 47..52, material build 06..09 and CPU 05..08 log pairs; run logs
893..899 and the prior geometry JPEG. Their findings/hashes remain in research;
historical logs/image themselves are deleted. Current evidence, protected raw/
normal flat-XR/failure images, game data and active build trees remain intact.

Deleted logical payload **1,739,573 B**. Immediate free space
**64,882,774,016 ->64,884,563,968 B**: **1,789,952 B measured reclaimed**, counted
once. Versus the pre-instance fixture/log/image/build-log set, retained payload
grows **204,331 B** for new pose/handoff behavioral and runtime coverage, not
per-attempt archives. Keep only final material build 10 /CPU 09 /host 53 logs.
Replace this evidence after equivalent/stronger qualification. Post-cleanup
drive-wide use from draft start is **73,199,616 B**, distinct from attributable
payload growth; final source/docs/Git writes still count. No allowance increased.

Next: native object/texture/pass records and independent layouts feeding direct
scene/shadow submission; remove templates/source lookup for that path. Original
animation/skin/effect/UI production and the full desktop gate remain before Quest.
