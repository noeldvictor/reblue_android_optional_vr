# Bind-owned image/effect catalog — 2026-09-09

Connected bundle after095918a: replace material-image and queued-effect linked-list
selection with one immutable native catalog in the existing animation residency.
Full host rendering, desktop sequence/both-eye acceptance and Quest remain open.

## Source contract and lifetime

Reused completed image/effect/parser audits from the preceding reports. Newly
read complete translated functions (no generated code edited):

- `sub_8218BB10`, generated/reblue_recomp.101.cpp:4299: r3 is the catalog
  container at visual+2248. Allocate148B entry and452B AnimeData, write authored
  ID/kind at8/12 and owner16, start async load, append via next4 to head16,
  increment count12. Allocation failure can leave the catalog unchanged.
  Its two direct callers are the model text parser and viewer `sub_823CB530`.
- `sub_823CB530`, .22.cpp:16656: image-file branch unlinks the first ID+kind
  match, decrements count, destroys it, then calls the same append function.
  Other branches edit skeletal/other assets; they are not image-catalog writers.
- `AnimeData_method_B888`, .63.cpp:3999: polls image owners through existing
  `AnimeData_PollLoadState`; does not rewrite catalog metadata. Existing ready
  image-asset hooks remain responsible for parsed keys/native texture leases.
- `AnimeData_method_BC48`, .13.cpp:4328: first ID match **ignores kind**. Poll
  that owner only; return entry when state>=6, else0. A pending first match
  must not fall through to a later ready duplicate.
- `sub_8218B4D8`, .2.cpp:4276: clear skeletal list, then unlink/destroy every
  image entry, decrementing count12. Called by visual reset and catalog dtor.
- `sub_8218B408`, .96.cpp:4410 and allocating dtor `sub_8218B3B8`, .99.cpp:4151:
  clear before releasing requests/directory storage and optional allocation.
- Entry dtors `sub_8218BCB8`, .103.cpp:4381 and `sub_8218BD08`, .89.cpp:4386:
  release image data through already-hooked `sub_821502F0` before freeing owner.
- `sub_8218B5A8`, .21.cpp:4282 copies **skeletal** list data, not image catalog;
  `sub_8218B310`, .55.cpp:4018 is unrelated render state. Neither is hooked here.
  Relevant hook TOMLs were unchanged; no existing hooks for the new raw sites.

Material selection differs from cue selection: ID and exact signed image kind
must match. Kind0 uses visual2212; other nonnegative kinds use2216. Both selectors
preserve the first authored match; native image keys still use last-active-window
semantics. Poll states1..4 keep the whole original controller before mutation.

## Implementation

`NativeImageCatalog` owns ID/kind entries and two sorted ordinal indices. It has
no source pointers, loader calls or source reader. `LoadedCatalog` keeps the
temporary ordinal-to-node/owner export association, instance/model generation,
and exact flat late-write guard separately. Import validates the complete
count/head/list,4096-entry bound, aligned extents and budget before publication;
it refuses cycles, partial lists and invalid owners. Empty is known, not absent.

Final material binding publishes through the same8MiB/4096-entry animation
residency. A third typed key separates catalogs from motion/image payloads in
the **one existing map**. All vector capacities and pinned retirement are charged.
Catalogs cannot be evicted into a per-frame reimport path; budget pressure refuses
new publication. Append refreshes established identities, including viewer edits;
clear/rebind/reset/model-generation retirement remove lookup before destruction.
No native lock is held across original append/clear/poll calls. Nested store
access uses instance->animation ordering; catalog lookup never reverses it.

The real material updater now selects native ordinals and consumes existing owned
image animations/leases. The real controller passes the same catalog to queued
effect selection. Neither selector follows source links or reimports ID/kind.
Lookup still compares flattened source words once; it invalidates on changes,
never revives until explicit binding. Active duration/clock/state/scratch and
outgoing node/cache/table exports remain adapters, not claimed native ownership.

## Verification before source checkpoint

-427 Python source/scenario guards PASS0.225s. Updated exact call-string guards
  for the new argument; moved bounded-list guard to the actual bind importer.
-material82/PID32584/session32938 build0: two C++ objects and link.
-CPU79/PID33636 PASS0.11s/CTest0.13s. Tests exercise distinct first-match rules,
  duplicate pending/terminal states, missing cues, cycles/counts/extents, every
  late guard word, identity/generation mismatch, exact budgets, typed aliasing,
  non-eviction and pinned retirement. Actual material composition receives selected
  image leases while fixture reads throw on catalog/keys/descriptors. Queued
  transitions throw on source ID/kind/link reads; active duration still reads16.
-host195/PID35132/session42600 build0. Link14/17; codegen0writes, no guest objects
  or shaders. Banner095918a dirty; source commit does not restamp this binary.
-Fixture1701888B, SHA256
  `9F70FEB69BEF5B9DB1349D326CD0724BAEE0DC7B5148762F2C6DB68F39CDD27C`.
  Fixturetree11892269B/43files. Host EXE49464320B/PDB114561024B; EXE SHA256
  `7E4FDE836A36D2F4D35F108CD53B6C5E3C82D349533D359B0F5AF21AA7A90F1B`.

Live acceptance is pending at this source checkpoint. A purposeful mono run will
test positive catalog binding/read counts feeding exact image updates and actual
material packets after fresh idle-field contexts, retaining existing strict
program/UV/eye/controller/late checks. A positive lookup alone is insufficient.
Queued transitions, viewer edits/reloads and pixels need their own live coverage;
the opening scenario previously had zero queued transitions. No FPS claim.

Storage remains in the original cumulative ledger in
[scene-state bridge](20260906_0333_native-scene-state-bridge.md#2026-09-09-bind-owned-imagecue-catalog-after095918a).
No raw/window/perf/cache/cook/device output requested. Keep image-assets194 and
all unresolved failures until their actual verification purpose is replaced.

## Live connection after source0defa30 was pushed

Host195/PID34900/session20800 ran02:44:39..02:45:33 with the existing capture-free
MaterialProgram/AnimatedUV/Eye/Controller/Late probe and native model/instance
comparisons. All14settings applied;60s/400KiB/192MiB caps. The wrapper reached its
unchanged regression observations and intentionally stopped(exit1); this is not
a crash or full desktop gate. New catalog/image observations below were inspected
separately; the wrapper does not have a dedicated catalog gate. Original116B
profile restored exactly,SHA256
`2F1BC38D763A1B7BDBA31F560684FD4AA7E42A714600B8D344F19DA7F38E23B0`.

Fresh idle `bg41_01`, event0 contexts1655->1955:

| Evidence | Earlier | Later |
| --- | ---: | ---: |
| Image updates / exact comparisons / publications | 2034 | 2934 |
| Owned image-key inputs | 2278 | 4078 |
| Selected / held inputs | 2254 / 24 | 4030 / 48 |
| Image material scopes | 604 | 904 |
| Composed / primitive material packets | 26576 | 39776 |
| Whole-call scratch refusals | 3 | 3 |

Fresh producer reports1681->1981 follow the corresponding contexts. Catalog
bindings5unchanged,reads2932->3982,refused0/changed0. Image registrations883,
imports6unchanged,reads2493->4293,refused0/changed0. Native indices therefore reach
the real exact image updates and owned-image packet path, not only a lookup count.
Shared motion/image/catalog residency1293872B/26resident payloads/1953registrations.
These are shared totals, not image-only or unique converted-game-asset counts.

At1981:controller94682exact,12017handoffs/changed0,mixes23,interior11526;late6exact/
reused/handoffs,refused0. Eye965exact/refused0/off-center227;UV1934publications/
refused0/ownedscroll1930;program1940bound5/4784reads,changed0/refused0. UV/eye
packets27808->41008overlap image packets and must not be added as unique draws.
Effects969exact/1936UV,translated0/rotated0/**transitions0**. Skeleton1940:
12621exact/unavailable0/wrong0. No comparison exception/error appeared.

Three scratch refusals: visual2381E598,slot2,owners24F6BFE0/24F6A960/24F6B460.
Exact capacity/shape/readability predicate remains unknown; no claim of fixing
the adapter. This run does not qualify queued transitions, viewer replacement,
reload/motion pixels, stereo, full-frame ownership, FPS or Quest. Next is native
selected-state/clock ownership and remaining scratch consumers, not another
unchanged opening probe. Remove validation/exports only when their last readers
and writers migrate.

Full259314Blog retained losslessly in `retained-image-catalog195.zip`52747B,
ZIP SHA256`7730DE9147F565AF5AAB85A10E1A5E20673B8372ABD665B9DE1521744BAC8EBE`,
sole member`reblue_981.log`SHA256
`205743ECD5E1A86FB5F5E556FD9B30963A8436A3A0B077030F5F2608DE00E78F`.
Name/length/decompressed hash verified before deleting plaintext and superseded
image-assets194ZIP.194's report remains; its log is no longer retained. Keep195
for current passing image/catalog/program/UV/eye purposes and scratch evidence;
older unresolved pixel/reload/effect187/placement988/root985 evidence untouched.
