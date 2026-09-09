# Ready-owned image animation assets

2026-09-09. Source **2f075dd**, host194/PID28412. Extends source2741450's connected
image transaction, not a second renderer. Source/C++ and targeted live consumer
checks pass; pixel/reload/stereo qualification remains open.
Full host desktop frames, scene/event/reload/both-eye gates and then Quest remain
the objective, not completion of this one material boundary.

## Connected change

Ready-load registration -> the existing8MiB animation residency -> immutable
image windows and native GPU leases -> actual image selection -> existing16MiB
instance image publication -> material composition/primitive inputs. The motion
and image assets use one index with disjoint typed keys, the same4096-entry
ceiling, eviction/backpressure and pinned-retirement accounting. No visual cache
or independent image residency budget was introduced.

The evaluator no longer decodes source key windows, walks their pointer vector
or resolves selected texture chains. It selects native ordinals and copies
native leases. The temporary boundary owns ordinal-to-source export associations
and exact flattened comparison words, charged with the asset. **Those source
word and GPU-identity comparisons remain on lookup.** Thus this is native input
ownership, not elimination of all key reads or resource-wrapper comparisons,
nor a demonstrated speedup. Loader registration prepares only selected payloads.
Unregistered/changed/retired backing cannot be imported from a render lookup.

Catalog selection, timeline/state words, scratch-capacity handling and outgoing
cache/material words remain adapters. Static/prior table images still use the
existing capture path. Procedural selected keys and required scratch allocation
retain the original whole call before mutation. Missing native image residency
refuses import rather than freezing an Unknown image forever; transient upload
readiness beyond the loader's ready state remains to qualify. Exact comparisons
are not weakened. The aggregate262144 source-read cap includes import and guards.
New bounded refusal logs distinguish boundary, missing asset, time, procedural,
scratch and held-key causes; earlier193's three refusals remain unclassified until
a fresh observation actually establishes their counterparts.

## Source contracts and lifetime

Reused the completed image selector/material/camera and parser/binder audits in
[the preceding report](20260909_0140_native-material-images.md). No guest code,
hook TOML, shader, defaults, profile or asset files changed.

Read/finished the complete relevant translated bodies, with PPC comments or
comment-filtered executable control flow:

- `bdD2AnimLoadFile`,82150060,generated57:3075: invalidates old lookup before
  either load. Synchronous parse completes at state6; async returns at state1.
- `AnimeData_PollLoadState`,82150BB0,generated44:3071: parsing, child readiness
  and texture polls progress through1..4, errors5, completion6. Register only
  after an original1..5 ->6 completion, not every poll. Recursive original polls
  execute without the native store mutex. The no-vector route can reach6 directly.
- `AnimeData_vf00`,8214FD20,generated70:2968 calls `sub_8214FD70`,generated31:3015;
  the latter first calls **sub_821502F0**,generated19:2967. That data-release body
  destroys all key/child objects, clears vectors, releases backing and sets state0.
  Hook this actual release, not only the allocating destructor: lookup retires
  before source destruction; pinned native assets/texture leases may outlive it.
- `AnimeData_SetAnimTime`,82153780,generated28:2926 updates clock336/state7->6,
  child clocks and the separate192 vector, not this image-window data directly.
- `AnimeData_ResetAnimChains`,82153CC0,generated96:3079 and
  `AnimeData_ResetChildTimers`,82153FC8,generated38:2997 reset chain/timer state.
  `sub_82151030`,generated50:2978 applies named position/scale/rotation variables.
  These observations do not prove all key writers have been migrated.
- TexOrg constructor8214D188,generated0:2989, copy helpers8214D2D0 (72:2950) and
  8214D470 (97:2845) initialize/copy authored60/64/68/124/128; the latter copy
  also sets212. The misleading ctor_2 name8214D250 (45:2876) is destruction.
- Full `sub_82151310`,generated56:3056 shows the selected-key procedural update
  interpolates/copies image-related channels by kind. Key+212 is linked data,
  not a function address. The earlier refusal condition remains correct; porting
  those channel effects and all late writers is still separate unfinished work.

The importer limits4096 keys and budgets vector capacities before allocating.
It preserves authored order, inclusive/zero-duration/repeat/hold and null-image
semantics. GPU identity changes at unchanged source pointers invalidate lookup.
Flat guards stay until concrete producer ownership replaces every such writer.

## Source and CPU verification

426 artifact-free source/scenario guards PASS0.232s. Material81/PID35644/session26019
build0; CPU78/PID32512 PASS0.12s/CTest0.13s. The connected C++ fixture forbids
source key/vector/texture-chain reads inside evaluation and descriptor/image
recapture inside the actual material importer; covers native composition, exact
exports, hold/null/procedural/allocation cases, every guarded word, GPU replacement,
unregistered/wrong-type lookup, source destruction and pinned-budget retirement.
Fixture1666560B SHA256
`10B4247BCFF8E3D9FE7CA0A805285D7E99B9E9A9FC9B33E29C4D3783A5442070`;
tree11735092B/43files (+121929B versus material80).

Host194/PID36456/session21925 builds0 through link14. Codegen0writes, no guest
objects or shaders rebuilt. EXE49443328B/PDB114343936B (+238592B combined).
EXE SHA256`E81E3BB47E76A904B192B8C2E80B8457B4EE8498BAA6136EA5BCE40C1F58FA3C`.
Banner2741450dirty, not restamped by the source commit. The commit explicitly
left live use pending; the subsequent observation is recorded below. Pixels,
reload and stereo remain pending, not inferred from the CPU fixture or build.

Storage uses the same cumulative ledger/floor and exceptions. First free63547826176B,
after host63542943744B; no cleanup yet. Existing fixture/build trees reused, no
new capture/cache/dump/recook/device output. Retained growth and replacement
cleanup will be reconciled after the scoped integration observation.

## Targeted live observation

PID28412/session79341,02:12:30..02:13:25, existing capture-free supervisor with
`-NativeImages -ModelMaterialsVerify -ModelGeometryVerify -InstanceVerify
-AnimationProbe -ControllerAnimationProbe -LateAnimationProbe -EyeMaterialProbe
-AnimatedUVProbe -MaterialProgramProbe`. All14settings took effect. Mono;
60s/400KiB/192MiB caps, no new raw/window/perf/cache/dump output. Original116B
profile restored byte-for-byte, SHA256
`2F1BC38D763A1B7BDBA31F560684FD4AA7E42A714600B8D344F19DA7F38E23B0`.
The wrapper reached its unchanged prior regression observations and intentionally
stopped with exit1; this is not a crash or a full qualification gate. The new
image observations were separately inspected in the fresh scene contexts.

Idle bg41_01/event0 contexts **1706 ->2006**:

- Image updates/checks/publications2034->2934; selected2514->4314,held42->42;
  **owned-key-inputs2556->4356**. Scopes603->903,composed/primitivepackets
  26532->39732 (**+13200**). These are repeated material inputs, not unique
  assets/draws/pixels or fully host-owned frames.
- Refused3->3. All three bounded log entries say **scratch**, for visual2381EB18
  slot2, owners253F0E20/2540DCE0/253F01A0. This isolates the scratch preflight
  (capacity/shape/readability), but does not distinguish its individual predicates
  or prove a particular allocator ran. No missing-asset/time/procedural category
  was observed.193's old unclassified calls are not retroactively reclassified.
- Fresh2034:883ready registrations,6image imports,4577successful asset lookups;
  refused0/changed0. Between fresh1734/2034, reads2777->4577 while imports remain6.
  Combined image/motion residency1272792B/21resident payloads at2034; the common
  catalog count1948 includes both typed registrations, not just motion assets.
- UV/eye scopes632->932,packets27808->41008 (overlap, not additive throughput).
  Fresh2034:962exact eye updates,230off-center,refused0;1928UVpublications,
  refused0,1924owned scroll inputs. Program5bindings/4832reads,changed0/refused0;
  effect-slot1630->1930 and eye812->962 evaluations.
- Controller94299exact,11970handoffs/changed0,23mixes,11482interior;
  late6exact/reused/handoffs. Skeleton2005:12646exact,unavailable0/wrong0.
  Material2006:19330checks/wrong0; material publication refused0. Effects966exact,
  1930UV,translation/rotation/transitions0. Skin emitted/fence-retired33513->46713
  remains a separate GPU lifetime observation, not image-specific pixel proof.

This establishes connected native key consumption and selects catalog/selection
state plus scratch-consumer retirement next. It does not qualify missing authored
joint-driven UVs, motion pixels, reload, stereo, full frames, speedup or Quest.
No unchanged opening-scene probe is needed for this same question.

## Retention and completed cleanup

Full257919Blog is retained losslessly as sole `reblue_981.log` member of
`out/build/win-amd64-release/logs/retained-image-assets194.zip`,52631B.
ZIP SHA256`B532E74A16B794CCDD1E95D976F77E3371E53AC5141B59E64222469817D05867`;
member SHA256`3E0084A44B57FC219EAB459A7B7C289D30E724EDA0679D734AC5E442A43C95A1`.
Full member/name/length/hash verified before deleting plaintext and superseded
material-images193ZIP.194 replaces its passing image/program/UV/eye purposes
and retains current scratch refusals.193's historical report remains; its full
log is no longer retained. Keep effect187/placement988/root985, earlier unresolved
pixel/reload failures and current81/78/194 receipts. All producers terminal.

Six superseded80/77/193 receipts8988Blogical removed:free63541731328->63541743616,
12288Breclaimed. ZIP allocation63540776960->63540723712,53248B; plaintext plus
old193ZIP310021Blogical removed63540723712->63541035008,311296Breclaimed.
Total8files319009Blogical removed; **270336B net264KiB cleanup** after new ZIP
allocation. No old cleanup credited. Partial retained growth357066B (~349KiB):
fixture+121929, EXE/PDB+238592, ZIPreplacement+529, buildlogs-3984. Excludes host
objects/metadata/source/Git and unrelated drive activity. Post-cleanupfree
63541035008B (~59.2GiB),6791168B below first63547826176B (drive-wide, not wholly
attributable to this task). Buildlogs420690B/212files. No new raw allowance or
budget reset. Final diagnostics78147968B; another400KiB log leaves85632B below
75MiB, requiring fresh preflight.13windowimages10434657B unchanged; scoped
inventory confirms no new raw/perf/cache/dump files. Free63540957184B before the
final documentation push; cumulative accounting remains in the same ledger.
