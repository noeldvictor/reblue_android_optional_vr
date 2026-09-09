# Load-owned joint selection

2026-09-08, after `4df3e0d`. A connected load/selection/sampler checkpoint,
not full desktop rendering or VR qualification.

## Dependency removed

Completed model loading now publishes dense pose-to-node outgoing bindings in
the existing bounded model registry. Native joint values remain source-free;
the pointer bindings live separately in its temporary source adapter. Both
share the same generation, retirement and retained-model budget.

Admitted `8227EF60` calls select that binding instead of walking the original
graph. They publish the same one-shot, generation-checked selection consumed by
the native single-joint sampler. Native controller exclusions likewise select
the owned joint/name, removing the `SourceNode` traversal and model-name rereads.
Overlay names already read for the native plan are reused. Inherited global
exclusions still import at their explicit source boundary.

This reuses the existing model, clip, controller, skeleton and instance owners;
it adds no process-wide reverse-node cache or second renderer. Pointer results,
the outgoing exclusion table, channel records and original caller placement
remain adapters. They must retire when their last consumers migrate.

## Source contracts and safety

Reused the existing loader frontier and sampler/caller findings; checked current
loader publication and the complete translated `8227EF60` (file89:9039) and its
recursive `8227EEE0` helper (file5:9528). Both search node IDs in child-before-
sibling order and return a node pointer or zero. The admitted ordinary skeleton
already requires unique dense pose IDs, so its dense binding preserves lookup
semantics despite reordered tree ordinals. Sparse/unsupported skeletons retain
the original lookup; ambiguous animation-name admission was not relaxed.

The named tree getter (file48:9690) was also inspected. It accepts a subtree
pointer, not a model identity; that separate interface was not replaced by an
invented reverse map. The existing whole-root hook remains independently pending.

Bindings publish only after complete skeleton validation. Partial/duplicate/null/
misaligned/overflowing aliases refuse. `FindJointSource` distinguishes a known
absent pose from an unavailable model. Retirement removes source lookup even
when native poses retain a model lease; graph-address reuse cannot expose the
old binding. A pointer in an outgoing selection is not a lease on guest storage.

Lookup verification executes the original once, compares the exact result and
throws before publishing a one-shot selection on drift. Native sampling and
controller comparisons remain strict and unchanged. No tolerance or acceptance
threshold was lowered, and no timing improvement is inferred from matching calls.

## Verification

Material66/PID33608 built successfully. CPU63/PID31196 passed in 0.12 s
(CTest 0.14 s). The existing fixture now covers reordered child/root-sibling
bindings, complete source destruction, owned name selection, absent vs unavailable
models, failed import transactionality, malformed bindings, retirement/reuse,
pinned generations and exact/one-byte-short aggregate budgets. Existing native
sampling, layer and skeleton/instance consumption fixtures also pass.

417 Python source/scenario checks passed in 0.225 s. These do not substitute for
C++ behavior or game pixels. Host183/PID28668/session99321 built successfully:
shared model consumers and version users rebuilt; codegen wrote zero files and
no guest objects or shaders rebuilt.

- Host EXE: 49,315,840 B; PDB: 113,446,912 B. EXE SHA256
  `28B26DA985D193602BC5BF9D79F216185AE98B4B71B30FE6C8AB8809B18D71D5`.
- Fixture EXE: 1,353,728 B; SHA256
  `251CF22468AB63F6E07B5A77A161DFDBE45D18E788A5B4FB9AB4BFBE4FC94425`.

### Run987: lookup-to-sampler observation passes

Existing capture-free desktop operator: NativeImages, ModelMaterialsVerify,
ModelGeometryVerify, InstanceVerify, AnimationProbe, ControllerAnimationProbe,
LateAnimationProbe, SingleJointProbe and new JointSelectionProbe. The new check
adds positive equal lookup/check counts and positive found results to unchanged
controller/late/single-joint requirements. Exclusion use is reported separately.
Limits: 60 s, 400 KiB text, 192 MiB maximum free-space drop; exact profile restore.

PID29524/session40387 ran 21:37:16..21:37:48. Operator exit1 is the explicit
observation-complete stop, not a crash. Frame1092 records:

- 732 completed/checked/found joint lookups, zero lookup refusals.
- 291 completed/checked/sampled single joints, zero fractional-weight samples
  and zero sample refusals.
- 30,556 matching controller transactions, 4,008 samples, 8 mixes, 3,851 interior
  CLIP times, 4,024 advancing clocks and 4,003 handoffs, zero changed-channel
  refusals. The 5,044 model/controller admission refusals remain unclassified.
- Six matching late updates, samples, reused-channel transactions and handoffs.
- **Zero owned controller exclusions:** that route's live acceptance is pending.

Lookup count grows from441 at frame792 to732 at1092 with291 new samples.
Context1069 is FieldActive, bg41_01, event1: opening-event scope, not independent
post-event, reload, motion-pixel or both-eye qualification. Skeleton1054 has
3,563 matching evaluations/publications with zero wrong/unavailable results.
Canonical import again maps122 descriptors to121 owned tracks/93,432 B with
zero preparation refusals.

Log: 119,516 B, SHA256
`BD44625AA229AE3BF952BC149E70970311A03454C069B33CE8C9D1E6EF433317`.
Profile independently restored to SHA256
`2F1BC38D763A1B7BDBA31F560684FD4AA7E42A714600B8D344F19DA7F38E23B0`.
All producers terminal; timestamp inventory finds no new capture/perf/cache/dump
artifacts. No further producer is queued.

## Storage and next boundary

The devloop/guest-source skills guided existing-fixture reuse, contract recovery
and one bounded changed-code observation. No new build tree or bulk asset work.

Run987 supersedes986's sampler/controller/late/import text. Run985 remains the
distinct missing whole-root/post-event evidence, preserved losslessly as
`out/build/win-amd64-release/logs/retained-root985.zip` (44,650 B). Its sole
`reblue_985.log` member round-trips all267,079 B to the original SHA256
`EE9C04E7E05A1529B8A946417356946D2026715724D46E15B3B160DFF30E7B44`.
Archive SHA256: `D1493DE16E6AC0EA0665905EA10A150EE021605DCD8FB475201B0AB9CAF822DA`.

Eight exact superseded/redundant text files were removed after validation.
Observed deletion reclaim401,408 B minus45,056 B archive allocation gives
356,352 B net cleanup (348 KiB). Partial attributed retained storage shrinks
100,495 B after fixture/binary/log growth and lossless archival; source/Git/build
metadata and unrelated drive activity are outside that subtotal. Cumulative
diagnostics are78,121,118 B, with only112,482 B headroom after another full400 KiB
runtime reservation. Remeasure first; no budget reset. Full measurements remain
in the [shared ledger](20260906_0333_native-scene-state-bridge.md#2026-09-08-load-owned-joint-selection-after4df3e0d).

Next: migrate placement/source selection and the remaining channel consumers,
then remove their pointer/table/channel exports. Authored exclusions, subtrees,
indexed/fractional content, separate root requests, reloads, motion pixels and
the complete desktop/both-eye gate remain required. No unchanged boot for absent
content, no full host frame or FPS claim, and no Quest optimization yet.
