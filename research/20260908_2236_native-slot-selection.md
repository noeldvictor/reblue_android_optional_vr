# Ready animation selection to native consumers, 2026-09-08

Source checkpoint8e6fcd6; this follow-up adds boundary/consumer tests and fresh
CPU, host-build and live evidence. This is not a fully native animation system
or host frame. Native animation remains opt-in; original defaults are unchanged.

## Connected boundary and remaining work

Ready `bdVisualObjectSetAnimation` requests now produce an immutable native slot
update, reuse the existing motion asset/residency owner and feed the existing
controller/layer/skeleton/instance chain. Admitted production does not execute
the original selection or lookup/poll helper chain. Catalog reads and six slot
writes remain checked compatibility boundaries; controller input still reads
those slots. There is no new permanent guest-address catalog, registry, disk
cache or second animation framework. This is not persistent asset cooking.

The first matching authored entry determines the result. Pending asynchronous
state refuses BEFORE polling, leaving the complete original path to run once.
Missing payload means known absence, not permission to search a later duplicate.
An actual ready selection prepares only its asset through existing bounded
residency/lifetime rules. Missing owned data refuses before source slot writes.

Strict verification runs the original once and compares the return value plus
all14 slot words exactly before native publication. Mismatch throws; no fallback
can conceal drift. Nested attachment reference calls retain their original-only
scope. The existing source-key-free sampler and channel handoff remain consumers.

Next remove source slot state at its remaining writers and consumers together,
using existing instance/asset lifetime owners; then retire slot/catalog/channel
exports when their final consumers migrate. Pending polling, special/late effects,
other placement callers, persistent native formats and the complete desktop
scene/motion/reload/both-eye gate remain. The separate missing whole-root985 and
attachment988 observations are not resolved by this selection test.

## Source contract and focused regressions

Reuse the established clip contracts in20260908_1612_native-animation-clips.md
and the completed selection audit from the implementation turn:

- bdVisualObjectSetAnimation0x82141298,generated/reblue_recomp.86.cpp:2636:
  r3visual,r4slot,r5ID,f1doubleweight,r7rawloop,r8force; six56B slots at+1872.
  Entry identity, not shared clip identity, controls restart. Unchanged entry
  ignores new arguments unless forced. Negative raw double preserves old weight,
  before float conversion; negative zero is not the preserve sentinel.
  Restart writes only ID,loop,wrap count,weight,time,entry (words0,1,2,3,6,12),
  resetting time/wrap. Return is selected entry or0. Other slot words survive.
- bdAnimClipLookup0x8218BA98,file22:4256: catalog+8head; entry+8ID,+4next,+12payload.
  First match wins, including pending/null entries. No later-duplicate fallback.
- sub_8218FC10,file21:4425: state+36==1 performs side-effecting request polling.
  Completed polling writes state2 and payload from request+184. The new path
  accepts only nonpending entries, not a partial replay of that transition.

C++ regressions cover aliased assets/different entry identities, unchanged and
forced selections, exact preserved dormant words, raw loop bits, negative/tiny
negative weights, signed zero, NaNs, null/absent/first-pending duplicates,
bounded cyclic lists, truncated/misaligned/overflow inputs and slot extent.
Selection snapshots survive source destruction and feed the production controller,
weighted layers, hierarchy and completed instance owner through retirement.
CPU66 exposed a fixture lacking authored rest availability for its fractional
blend. The corrected fixture first requires that production rejection, then
supplies the owned rest data; no production check or comparison was weakened.

## Verification and limitations

Material70/PID37640 builds; CPU67/PID32096 passes0.13s/CTest0.14s.419 artifact-free
source/scenario checks pass; these are not C++ behavior or pixel qualification.
Host185/PID33608/session55045 builds with codegen0writes and no guest objects or
shaders. EXE49346048B/PDB113561600B; EXE SHA256
24699648FADFA4BFFCBA39A46D06428EB081DED5BF72C34671CAFEA85185A078.

Selection run/PID27880/session86472,22:33:32..22:34:17. The planned sequential
label989 was NOT the emitted filename: log rotation reused reblue_981.log after
older plaintext retirement. Identify this run by time, PID, host185 and hashes,
not the historical981 label. Capture-free60s/400KiB/192MiB free-drop supervisor
ends with explicit observation-complete exit1, not a crash or full game pass.

At frame1133:208 completed/checked selections,23 restarts,56 ready and152 absent,
1 admission refusal (cause unclassified, do not assume pending). Controllers:
31036 matching,5124 unclassified refusals,4068 samples,8mixes,3908 interior,
4084 advancing clocks,4063 native handoffs,changed0. Late6 matching updates,
samples,reuses,handoffs. Joint737 matching lookups and296 matching samples;
weighted0,owned exclusions0. Skeleton1102=3719 matching evaluations/publications,
wrong0/unavailable0. Context1103=FieldActive bg41_01,event1. This is an opening
event observation, not post-event movement/reload, every caller or authored
subtree/indexed coverage. No new pixels, speedup or Quest qualification.

All11 temporary settings took effect. Original116B profile restored exactly,
SHA2562F1BC38D763A1B7BDBA31F560684FD4AA7E42A714600B8D344F19DA7F38E23B0.
All matching producers terminal; timestamp inventory finds no new raw/image/
perf/cache/dump outputs. Log122054B retained losslessly as
out/build/win-amd64-release/logs/retained-slot-selection185.zip22757B, sole member
reblue_981.log, full decompressed SHA256
98B9EC34565EB3BDA7049B781CD527DCA0A1B555764BA5CE72C50B5276A46F0A.
ZIP SHA256136BD368C9AF80681A586FCA2501DD7205057CB613052D1FDB9D405A8DBC8156.
988/985 ZIPs and all other unresolved-failure evidence stay protected. Retain this
small selection log until a changed selection/consumer run replaces its purpose.
The existing20260906_0333 cumulative ledger records exact cleanup and storage;
no new allowance or raw capture is authorized by this checkpoint.
