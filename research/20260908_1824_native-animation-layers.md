# Native weighted animation and layer composition

2026-09-08. Source checkpoint9c41128 plus this connected bundle; opt-in
`bd_native_animation`, `bd_native_skeleton`, `bd_native_instances`. Normal
defaults unchanged. Full desktop host-frame/scene/pixel/VR gate remains open.

## Ownership delivered

Existing immutable keyed/cubic clips now feed weighted channels, owned hierarchy
subtree masks and whole-layer composition. Their outgoing records feed the
existing native skeleton evaluator, instance publication and render-pose owners.
No second animation/renderer framework or budget increase. In non-comparison
mode, admitted calls bypass the original weighted sampler and buffer mixer.
Comparison mode deliberately executes originals once before native publication.

`bdAnimationUpdate` now establishes the model scope and prepares its six physical
slot selections before whole-body or subtree sampler calls. The existing slot
scope remains for direct callers. Selected-slot bounds are explicitly six, not
unchecked pointer arithmetic. Residency reuses completed loader registration,
8MiB aggregate accounting, pinned leases and retirement. No source-key decode
in the sampler or buffer mixer. Dormant source-backed catalog entries remain a
temporary streaming interface, not completed native clip cooking.

Authored rest T/R/S is captured independently of base-transform activation.
Missing optional rest data only refuses blends needing that channel; it does
not discard otherwise valid geometry. Quaternions and rest/hierarchy values are
owned, source-independent inputs. Subtree masks use parent/preorder ordinals,
not dense pose IDs; a forced subtree excludes following siblings.

## Recovered contracts

The guest-source skill directed reuse of completed source findings and a full
read only for the newly connected buffer-layer contract. No generated edits,
hook-site TOML changes, decompiler import, new binary dump or game-data copy.

- `sub_8228A3E8` (file43:9917): abs(weight)<2^-23 is a no-op before any
  channel/global writes. Unit/full traversal has the direct keyed route;
  other weights and forced subtrees use `sub_8228A090` (file23:10240).
- `sub_82289990`, `sub_82289B28`, `sub_82289D58`: matched missing channels
  clear activation at weight1. At non-unit weights, one missing side blends
  against the node's raw authored rest value. Both absent is untouched.
  Multi-key tracks set dirty128; hashes and unrelated flags are preserved.
- `sub_824931B8` (file70:21937): short-arc quaternion interpolation, linear
  near-parallel threshold1-2^-16, no normalization. Native standard-library
  trig replaces the source SIMD polynomial; strict live tolerance is unchanged,
  not a claim of bit-identical transcendental arithmetic.
- `sub_822774D0` (file28:9176): global byte0x827A7EE6 selects Euler ordering.
  This connected sampler explicitly refuses nonzero modes rather than silently
  using qZ*qY*qX. Named/exclusion traversal state remains unconverted.
- `bdAnimationUpdate` (file53:2663): six56-byte physical slots at1872..2207;
  ready entry at1920+56*slot, initialized clip at entry+12. Whole-body layer
  sampling and forced subtree calls can bypass `bdVisualObjectAnimSlotUpdate`;
  the broader model scope makes them reachable without changing clocks/effects.
- Full `sub_82284BE0` (file39:9943) and both calls(file53:3307/3371): T/R
  lone inputs copy, scale lone inputs fade to/from unit; both absent writes
  canonical0/identity/1. Flags union, first input hash. Output==left occurs for
  third/subsequent layers. The original writes union flags before rereading
  input flags, so aliasing can activate previously inactive payload values.
  Preserve this temporary ABI ordering in `MixChannelRecords`, not native
  channel math. Exact aliases supported; partial overlaps refuse pre-execution.

## Verification and limitations

Devloop used existing bounded CPU trees before host integration. Material55 /
CPU53 pass0.11s (CTest0.13s). Cases cover rest fallback, disabled base flags,
negative/tiny/unit weights, quaternion endpoints/hemisphere/non-normalization,
reordered pose IDs, subtree/sibling masks, exact untouched bytes, transactional
failure and source destruction through real native hierarchy/instance consumers.
Whole-layer tests enumerate448 activation/weight combinations, poisoned inactive
bytes and left/right/both aliases. Shared skeleton consumers output83 / CPU55
pass0.51s (CTest0.53s). All408 artifact-free source/scenario guards pass0.226s;
these are not C++ or pixel substitutes.

Host171 and172 pass; codegen0writes, no guest objects or shaders. Host172 only
rebuilds animation bridge/link. Current executable49228288B SHA256
`F0DF6D55E71AAD0679D8F7498890F4921E37D569B1731E6AB49E986A6C903B96`,
PDB112939008B. Material EXE1017344B SHA256
`40E3554BC1C5E380E56FF254CF6A7E0E4C496E6F22307D4D86C52AAE13E98FAF`.
Output fixture EXE1848832B SHA256
`26965166F836ECC8FFD85160B4D43EDAB8989037BB2DF65EEB8E3FDEC2C18BB5`.

Both60s live probes used exact original comparisons: all flags/hash/inactive
words exact, active floats finite and within1e-4*max(1,abs(original)). Failures
throw before any native replacement. No comparison thresholds were changed.

| Run | Final reported sample | What was actually observed | Requested gate |
| --- | --- | --- | --- |
|979 / host171|frame2352:13822 sampled/checked,wrong0|113 weighted,0 subtree,380 cubic|FAIL: wanted>=256 weighted and positive subtree|
|980 / host172|frame2355:13858 sampled/checked,wrong0;23 mixed/checked|113 weighted,0 subtree,382 cubic|FAIL: wanted>=256 mixes plus>=256 cubic|

Run980 has whole405/preserved13453; one unavailable model, two unclassified
selected-clip preparation failures. Resident11/682432B at the last sample;
reported peak970672B. These are scoped matching executions, not full field,
reload, advancing/interior-key motion, source-retirement or pixel qualification.
The earlier>=256 repeated-cubic **call-count** criterion is now observed;
it does not prove diverse/advancing compressed motion. Neither probe satisfies
its new complete observation request. Do not retry unchanged boot/input or
lower counts to call these passes.

979 log260831B SHA256
`A8228C1CAB59B3605461FF36F9619837683CD6FB46B42E74B154727A11E86DB0`;
980 log263220B SHA256
`6EEA64ACCDC792ABC90522E306B71A9FEE882424CF41219EE0205D82F72EE2A8`.
Both producers terminal; owner profile restored exactly to SHA256
`2F1BC38D763A1B7BDBA31F560684FD4AA7E42A714600B8D344F19DA7F38E23B0`.
No new images/raw/perf/cache/dumps. Unchanged GPU fixtures are not fresh motion
pixels;975 reload and971/962 pixel discrepancies remain unresolved.

## Next connected work

Classify the two selected-clip import refusals and own remaining named/excluded
and dense-type contracts. Extend the current native channel owner through slot
selection/clocks and completed handoff, retiring outgoing48-byte scratch rather
than retaining it as the final API. Preserve gameplay collision/effect ordering,
late/secondary/view-dependent bones and both loader retirement contracts.
For new live verification, choose an actual authored layer/subtree transition
and track advancing clip clocks/interior keys; this opening sequence does not
exercise the pending cases. Do not substitute another idle boot or diagnostic
counter for that consumer coverage. Persistent cooking, full scene/sequences,
both eyes and the complete desktop frame remain before any Quest optimization.

Storage/cleanup is recorded in the existing cumulative ledger
`20260906_0333_native-scene-state-bridge.md`; no budget reset or new raw allowance.
External push remains denied pending payload/destination-specific approval.
