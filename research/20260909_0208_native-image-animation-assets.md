# Ready-owned image animation assets

2026-09-09. Extends source2741450's connected image transaction, not a second
renderer. Source/C++/host checkpoint; live consumer qualification is pending.
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

## Verification so far

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
Banner2741450dirty, not restamped by this source commit. Live key-to-packet use,
refusal classification, pixels, reload and stereo are **pending**, not inferred
from the CPU fixture or build. Preserve material-images193 and unresolved failures.

Storage uses the same cumulative ledger/floor and exceptions. First free63547826176B,
after host63542943744B; no cleanup yet. Existing fixture/build trees reused, no
new capture/cache/dump/recook/device output. Retained growth and replacement
cleanup will be reconciled after the scoped integration observation.
