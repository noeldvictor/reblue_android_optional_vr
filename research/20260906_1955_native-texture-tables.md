# Load-owned native texture tables

2026-09-06, EDT. Previous goal turn was a read-only planning review, not an
implementation checkpoint. Continued the unpublished table draft on `e85b290`.
This advances load-time associations and lookup execution, not complete native
material/object ownership or direct static-object submission.

## Production boundary

Immutable native tables hold native GPU image leases, including dimensional
companions, known-null slots and explicitly unavailable dynamic/unconverted slots.
Monotonic runtime IDs never derive from source addresses. Source keys remain in
the temporary association/return-ABI adapter, not the native table core.

Synchronous `hcgLoadTextureArray` publishes after its final entry writes;
`hcgTextureListRelease` retires before resource release. The field uses the
asynchronous LoadTexlist path: `sub_8217B3C0` returns 1 only after all 20-byte
requests finish and record+24 is written. Publish asset+184 there, not after
`sub_8217B050` merely allocates the table. `sub_8217B518` separately releases
requests and frees that table directly, so it retires before the original body.
Complete generated bodies and their constructors/request/consumer paths were
read; generated source is unchanged and uncommitted.

Normal `bdLookupCurrentTableTexture` uses the published native association, not
original table entries or the original lookup body. Current-table/offset and
out-of-range fallback globals, the source-address return ABI and dynamic
overrides remain explicit adapters. Reflection has a native-image-handle consumer
before its resource fallback; this field does NOT exercise it (zero direct
native-image reads). Do not equate matching native image leases with removal of
every resource lookup or complete native sampler/material submission.

Mirror replacement/eviction rebuilds affected immutable table generations;
old readers retain their exact leases. Collection AND publication now share
the mirror mutex, with mirror -> table lock order and no Video lock in the
publication callback. This removes the initial global-epoch rejection and the
gap between collecting image handles and publishing their owner. Exceptions
release the lock; failed updates invalidate lookup instead of retaining stale data.

Limits: 4,096 slots/table, 16,384 live table generations, 16 MiB of table/index
accounting, including pinned retired/replaced tables. GPU residency keeps its
separate existing fence-aware budget. The original 2,048-record limit was too
small: constructor `sub_8217AE90` creates separate inline-list objects and
`sub_8217B5B8` only attempts name sharing for the file-list case. Bounded runtime
examples verify distinct assets with vtable **0x8206BF00**, table=asset+0x100,
count=1 and entries=table+0x10 through publications 128/256/512/1024/2048/4096.
These are not 4,096 distinct GPU images. The byte budget was NOT increased.

`bd_native_texture_tables` defaults on. Its independent, default-off
`bd_native_texture_tables_verify` calls the original read-only reference and
compares returned associations and all available non-null native image handles.
The initial draft used `bd_native_materials_verify`; separating the switch allows
fresh field/pose/material checks while proving texture lookup needs no original call.

## Verification and failed attempts

The production C++ fixture covers literal layout/overflow/truncation, async
incomplete/complete publication, null/unavailable distinctions, alias updates,
immutable old generations, source destruction, retirement/non-reused IDs,
pinned backpressure, 8,192 inline lists, aggregate accounting and a concurrent
image updater excluded through publication. A throwing publication releases
its mutex. Checks remain enabled in the existing Debug/-O0 fixture.

Final binding build/CPU 05 pass (CPU **0.04 s**, ctest 0.06 s). CPU 04 timed out:
the new test incorrectly decremented its one-shot latch twice; fixed in the
fixture, not hidden by increasing its timeout. Material build 11 /CPU 10 pass
(0.09 s). **146 source guards and 18 scenario cases** pass. Source guards
complement behavioral/runtime checks; text matching alone does not prove dispatch.

Host 54..59 pass, no guest objects or shaders rebuilt. Host 58 rebuilds host
consumers of the snapshot API; 59 changes only the independent comparison cvar,
one host object plus link, **3.11 s**. Final exe **48,160,768 B**, SHA-256
`3f3e8a1f3fd66f3750359e13d304efe40113e4c79693b668e276c0a331e7d629`.
Comparison run 905 used host 58, SHA-256
`240a91ee87405690abae02136ae0f456ff23747fbe375578e2b7f7ef8e50aa5d`.

All runs use normal flat MSAA, PSO precache off and autoplay; material/geometry/
pose comparisons establish fresh field observations. Raw and perf output are
disabled, with a 75-second/400-KiB supervisor, cumulative disk gates and exact
116-byte profile restoration. No Quest/XR run or controlled performance result.

| Run / PID | Outcome |
| --- | --- |
| 901 /30136 | Only synchronous tables published; field has zero native lookups and 243,359 fallbacks. Failed, no image. |
| 902 /30172 | Async boundary active; 2,048 indexed tables hit the initial count cap, 2,341 refusals. Stopped before field. |
| 903 /28884 | Bounded source examples prove distinct valid inline-list assets. Same cap, 2,312 refusals; stopped. |
| 904 /28540 | Corrected record cap; 4,347 indexed /1,622,544 B, but 16 refusals remain amid concurrent image loading. Global epoch rejection removed by atomic snapshot publication. No image. |
| 905 /28068 | Comparison passes, 19:50:25..19:51:22, about 57 s. Scoped image inspected. |
| 906 /26412 | Final host 59; normal texture lookup passes, 19:57:54..19:58:52, about 58 s. Original texture comparison disabled; scoped image inspected. |

Run 905: 5,747 publications, 11 retirements, **5,736 indexed /2,172,192 B**,
12 image-driven replacements, zero refusal/fallback/source/image mismatch.
Consecutive post-event samples add **22,326 matching table lookups** and
**22,006 matching non-null image checks**. Last totals 111,965/110,574.
Fresh pose delta **118,995 matches**, no misses/refusals/drift. Fresh geometry
adds **51,741 draws /2,700 checks**, with 15,420 diffuse /14,680 specular checks;
separate geometry fallback count 761 remains. Reflection/direct native-image
consumer counts are zero, not qualified by these checks.

Inspected 1920x1080 JPEG: standing Shu, terrain, foliage, fencing and shadows;
no obvious new texture/geometry breakage. Distant blur remains. A single lossy
image does not establish movement, sequence stability, reloads, authored effects,
both eyes or the full desktop gate.

Retain comparison log 905, **210,357 B**, SHA-256
`789a473909d91d8cfe0901e7f6696830663ce023fd6a8f9de67255d7d3290c03`,
and initially `native_texture_table_window.jpg`, **389,816 B**, SHA-256
`a3a99961749c29f6efcd6d9e01e571ca8d5184dc6073b93961611c32f353144f`.
Normal-table run 906 uses the final independent comparison switch set false;
all 10 profile settings audited. It adds **21,745 fresh native lookups**, zero
original comparison/image-reference/fallback calls, zero refusals, and the same
5,736 /2,172,192 B live table state. Fresh poses add **118,941 matches**; geometry
adds **51,863 draws /2,700 checks**, diffuse/specular **15,280 /14,568** checks,
all matching. Separate geometry fallback count 756 remains. Other material/pose
verification remains enabled for the field gate; this is not a whole-renderer
diagnostics-off run or a performance comparison.

The final 1920x1080 normal-table image was inspected: standing character, terrain,
foliage and moving windmill shadow appear intact; distant blur remains. Its scope
and limitations are the same as the comparison image. Keep normal log 906,
**205,661 B**, SHA-256
`2be31cb657a8d5c6ebd29a22ad1e5844b60c3bf02d7afe9b084afdf4c76db8b9`,
and `out/verification/native_texture_table_normal_window.jpg`, **395,416 B**,
SHA-256 `cefc0100ccf0e370bc6a135cf2d8eb016a233e870eb2d757f9848372badc4097`.
The normal image supersedes the redundant comparison JPEG; retain the comparison
log as independent input correctness evidence. All owned sessions are terminal
and the 116-byte owner profile was restored after every attempt.

## Storage and next work

Original cumulative ledger remains `20260906_0333_native-scene-state-bridge.md`:
65,462,788,096 B starting free, unchanged 2 GiB peak /100 MiB diagnostics /
10 MiB build-log /20 GiB reserve ceilings. Table draft preflight 64,571,400,192 B;
this continuation pre-build 64,515,645,440 B. No relevant active producer at
preflight; intervening drive-wide changes are not attributed to this task.
Reuse existing trees with <=256 MiB build overlap and <=4 MiB fixture/log growth;
JPEGs share the existing bounded inspection allowance. Zero new raw allowance.
The two retained failures at continuation start totaled 340,454 B.

Binding fixture exe/object now **195,072 +1,991,588 =2,186,660 B**, against the
pre-table **1,232,759 B**: **953,901 B growth** for new layout/lifetime/concurrency
coverage. Keep one current fixture, not an artifact per attempt. No new asset
cooking/cache/dump/raw/perf files were produced: scoped post-draft timestamp checks
find zero in all four output areas. Mesh cache unchanged at 3,510 /36,510,144 B.
The prior material-fixture accounting set now totals 2,768,387 B (+373 B).

After replacement validation, deleted **39 exact superseded agent-created files**:
host 53..58 log pairs, binding build/CPU 01..04 log pairs, material build 10 /CPU
09 log pairs, runtime logs 900..904, old pose JPEG and the intermediate comparison
JPEG. Findings/hashes remain in research; those historical logs/images themselves
are no longer retained. Current logs 905/906 and normal JPEG, final host 59 /
binding build-CPU 05 /material build 11-CPU 10 logs, all normal flat/XR/broad-failure
evidence, game data and active build trees remain protected.

Deleted logical payload **1,478,619 B**. Two immediate measurements:
64,503,181,312 ->64,504,315,904 B (**1,134,592 B reclaimed**) and
64,423,895,040 ->64,424,288,256 B (**393,216 B reclaimed**).
Total **1,527,808 B measured reclaimed**, counted once. The comparable fixture,
scoped build-log, runtime-log and JPEG set grows from **4,606,580 to5,770,147 B**:
**1,163,567 B retained growth** for new ownership/concurrency coverage and separate
comparison/normal-execution evidence. Replace these same purposes after equivalent
or stronger qualification; do not keep per-attempt archives.

Post-cleanup free **64,424,288,256 B**: **147,111,936 B drive-wide use** from the
table draft preflight, distinct from the measured 1,163,567 B diagnostic growth.
Intervening drive-wide changes include unattributed external use; no active
renderer/build producer or new asset/raw/perf/dump output explains that difference.
Source/docs/Git writes follow and remain charged. No budget reset or new raw
allowance; full desktop/both-eye qualification remains required.

Next remains native object/texture/pass contracts and independent layouts feeding
direct scene/shadow submission, replacing source-key selection and retained
templates. Animation/skin/effect/UI producers and the complete desktop gate remain
required before Quest work. This checkpoint is not a full native static-object path.
