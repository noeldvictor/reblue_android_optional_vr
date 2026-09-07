# Ordered native scene-light inheritance

2026-09-07, parent4a33339. Initial implementation built as host108; follow-up
host110/run948 exercises wider native scene draws but stops at a reloaded-field
UV mismatch. Host107/run945 remains the last accepted live/pixel checkpoint.

## Recovered contract and implementation

The guest-source skill bounded the investigation by the missing inherited input,
not a helper-port census. Exact translated-source observations:

- `sub_82142C58`, generated93:2656: a non-null per-node light entry replaces the
  object's selection; null skips selection/publication, not falls back to the
  object's default. Its wind-specific tail is separate and remains unconverted.
- `sub_8218B310`, generated55:4018: relevant draw views are0/3/5/6/9. Its cached
  visual/node may skip the callback. Otherwise it invokes the above producer,
  then flushes both light buffers. Inheritance follows actual participating
  draws, not numeric node order or unordered instance-index iteration.
- `sub_8218ADA0`, generated23:4259: publisher setup initializes descriptors and
  lookup IDs. The new whole-function hook invalidates native inheritance before
  original initialization. No instruction-site hook/TOML or generated edit.
- `bdLightListUpdateSnapshot`, generated54:4204, especially0x8218A464: resets
  cached light IDs and visual/node, **not** the retained light values. A valid
  next handoff therefore preserves known copied values until an explicit bind.
- The object preparation block at0x82142E1C (generated8) also selects/publishes
  the default for objects without per-node lighting. Observing the actual
  publisher covers that ordering before the traversal scope exists.

The existing handoff/instance producer now imports explicit Bind or Keep actions.
Invalid/unreadable bindings have no native key and cannot become Keep. Selection
inputs and native instance/model/node keys remain address-free. A bounded,
sorted source-selection identity index is confined to the compatibility bridge;
no consumer imports bounds, dirty slots, selected IDs or shader values at draw.
Maximum65,536 keys,4 MiB native binding capacity and2 MiB source-index capacity;
replacement overlap is at most two of each, plus fixed light data. No history
cache, disk asset, new instance registry or per-turn archive is added.

`NativeSceneLightingPublication` owns one ordered set of copied semantic values.
Preparation returns a ticket with publication/update and order revision. It does
not advance order. All node packets are prepared before a participating node
commits; a wholly suppressed node does not bind. Intervening writes, replacement
publications, missing identities and reused tickets fail closed. Successful
handoffs preserve values, not stale ticket eligibility; invalid producers and
publisher/device initialization break the chain. Startup Keep before any known
bind remains unavailable, never guessed dark or object-default lighting.

Native draws commit through the existing object/scene submission path. The
outgoing44-write adapter mirrors their values for still-unconverted consumers,
invalidates the old ID and visual/node skip caches, and flushes via the existing
host parameter publisher before backend locking. It calls no guest setter or
selection routine. A later backend failure is terminal, never legacy fallback
after a partial node replacement. This outbound descriptor adapter is temporary
and must retire with the last legacy consumer; it is not a native input API.

The compatibility publisher independently selects from handoff-owned inputs at
its actual callback position. It seeds inheritance only if its computed values
match the compatibility result; that result is a guard, never copied into the
native owner. Unsupported/unowned observations break the chain. Parameter
producers validate relevant writes against already-owned semantic values, ignoring
unused lanes; unknown or differing writes invalidate rather than import them.
This covers the existing tracked publisher/invalidation boundaries, not a claim
that all remaining inline guest writers have been removed or live-qualified.

## Verification and limits

Devloop grouped the connected data, lifetime, mirror and draw changes. Existing
material34/PID28764 and CPU32/PID29920 pass (0.56 s assertions/0.62 s CTest).
The fixture covers Bind->Keep, cross-object/view inheritance, suppressed/speculative
preparation, intervening binds, stale/double commits, same-ID changed values,
successful/invalid handoffs, source retirement and actual GPU-input packing.
It also executes the production outgoing mirror and relevant-write validator,
including missing destinations, aliases, address overflow and unused lanes.
Output32/PID29284 and CPU15/PID33180 pass (0.44/0.48 s), checking existing scene
packet/layout/lease behavior.312 artifact-free Python source/scenario checks pass.

An initial source guard caught compatibility packing in the semantic header;
that packing was moved to the existing source adapter instead of relaxing the
ownership test. Wiring guards were updated for the actual changed signatures and
the added outgoing invalidation; original42-write selector checks remain intact.

Host108/PID29072/session2796 passes in the existing desktop tree. Codegen writes0
modules; no guest object or shader rebuild. Both the preceding layered scene
bundle and this ordered-light connection are now linked, but not game-qualified.
The unchanged twelve-case GPU shader evidence is reused, not rerun/restamped.

- Host108 exe48,659,456 B SHA-256
  `4DBABAED0B02FEB65085B0B6D02F0EC68620CD817CCA8425BD8BE22D34375DD1`.
- PDB109,326,336 B. Exact116 B owner profile unchanged, SHA-256
  `2F1BC38D763A1B7BDBA31F560684FD4AA7E42A714600B8D344F19DA7F38E23B0`.
- No game run, pixel capture, cook, Quest activity or speedup measurement yet.
  Run941 stays strict/unresolved; current945 and940/941 evidence are protected.

Next integration must establish reachable broader scene packets and inherited
consumption, then actual representative emissions/retirement with inspected
pixels and reload regression. Aggregate submitted/layered counters alone cannot
prove a particular class was emitted. Sequences, animated effects, both eyes,
other material families and the complete desktop/Quest gate remain open.

## Storage

The original cumulative ledger and3 GiB allowance are unchanged. Unrelated
free-space recovery allowed <=32 MiB sequential fixture replacements and the
<=192 MiB host link, each supervised at300 s, its own free-drop cap and original
62,509,998,080 B floor. No new tree, raw/image/shader/cache output or disk-budget
increase. The unanswered4 GiB request was not treated as permission.

After replacement acceptance, removed10 exact superseded material33/CPU31,
output31/CPU14 and host107 build/test logs:8,124 logical B;
65,604,374,528 ->65,604,390,912 B free,16,384 B recovered once. They are
reproducible logs, not runtime945 or protected failure/raw/game/profile/build data.
Current fixtures remain necessary for continued checks; review on replacement.

Known retained net+380,179 B: material+68,065 to8,227,410 B; texture+16,911
to70,291,504 B; exe+37,376/PDB+258,048; build logs-221 to191,504 B/136files.
Other host objects/CMake/source/Git and drive-wide activity remain unallocated.
First measured62,592,278,528 ->cleanup-end65,604,390,912 B is a drive-wide
3,012,112,384 B gain, **not** a cleanup claim. Final free check belongs in handoff.

## Follow-up: active material channels and a precise integration boundary

After ccb8dc2 was pushed, capture-disabled run946/PID30068 (15:50:41-15:51:18,
session11109) submitted the selected native node and then refused the next
node's shader resources. Its generic refusal was narrowed in host109/run947:
instance129/generation78/node1/primitive0, geometryF9F0B95507EC870C,
material63B8D67932573E51, material mask1/layers1/image-mask1. All21 temporary
settings applied and exact profiles were restored after both terminal failures.

The producer contract in `native_material_data.cpp:192` and
`native_material_data.h:48` intentionally omits specular when the owned object's
shininess flag is off; the same flag disables specular shading. This object uses
the same immutable material as the passing mask3 object. Requiring both channels
unconditionally was therefore a native consumer error, not a missing asset.
The earlier exact-mask3 check also unnecessarily rejected unused reflection data.

The scene builder now requires diffuse (always used as object colour), and
specular only if its owned feature is enabled. Disabled specular GPU fields are
canonical zero; missing active data cannot turn an authored feature off.
Inactive reflection values, including nonfinite unused data, are not consumed.
The existing scene fixture tests masks0..7, inactive specular/reflection and
missing active channels. Failure-only diagnostics identify the native packet
and failed owner group before submission; no partial node or weaker comparator.

Output33/PID24532 and CPU16/PID572 pass (0.44/0.47 s); the final causal fixture
passes output34/PID27960 and CPU17/PID26656 (0.44/0.46 s). Host109/PID33228/
session97407 and host110/PID33656 pass, no guest/shader compilation.312 Python
checks pass. Host110 exe48,662,016 B SHA
`1E7C4CD87DFEC71D548C9A7080F70898A5F80E0CBEEF59B36F597C4682AE119F`,
PDB109,334,528 B. The source's subsequent light-order log-label correction adds
the previously omitted unowned-observation placeholder; it does not restamp this
binary. Its unowned-observation total was not emitted and cannot be inferred.

Run948/PID19644/session83944 (16:01:09-16:03:21) exercises the fixed consumer.
The fresh, interactive cold-field window1899->2199 adds1,265 wider-family native
scene submissions, emissions and fence retirements, plus300 multi-primitive node
visits. Selected scene/caster generation91/instance144 progresses763->1663 and
closes at title with1664/1664/1664 submitted/emitted/retired in each view.
Reload creates generation207/instance438 and continues native scene emission.
Light reads remain fresh/missing0 in sampled windows. All batches are singletons;
layered submissions and inherited preparations are0. These are repeated visits,
not unique assets, multi-instance reduction or broad authored-family acceptance.

At16:03:17.835 the strict comparison reports
`[native-material-texture-mismatch] visual 23820098 channel 16`.
The supervisor stops/restores the profile. Channel16 is the exact comparison of
owned UV values with VS vector2 in `host_draw.cpp:1593`, not an image slot.
This new failure is **unresolved**; no successful reload/pixel gate or fix is
claimed. Source follow-up must trace the object UV recipe and ordered native/
legacy writers, then add a causal regression before a changed-code probe.
Do not assume it is an export issue, weaken the comparison, or conflate it with
the separately preserved941 light-selection failure. No game images, raw frames,
performance files or Quest work were produced. An attempted image-helper review
did not launch a capture; the failure had already stopped the game.

| Evidence | Retention / SHA-256 |
| --- | --- |
| 946 generic refusal,76,605 B | Superseded by947's exact packet; removed after948 clears that earlier refusal. Hash `E52260819C74EA4F318D6D9CB35055FE188BD96479195806D278AAE090FFC76B` retained. |
| 947 exact material refusal,74,436 B | Retained causal regression context, review after equivalent field/reload qualification. `5A68EB5ED2837607EBE527DE11E9517703F196CB96848DDACB65F49E44F022F1` |
| 948 new UV failure,397,691 B | Retained until its cause and replacement pass. `BB9A0485A1939C7EDBC405D6E927583377FFFEB4F4E76C90780F566194841D9A` |

Same storage limits throughout. After final fixture/build acceptance, removed
12 superseded output32/CPU15/output33/CPU16/host108/host109 logs plus946's
superseded generic refusal:86,236 logical B. Concurrent volume free rose
64,831,266,816 ->64,831,401,984 B (135,168 B observed); unrelated drive activity
means that increase is not isolated cleanup attribution. Total removed this turn
23files/94,360 logical B; the earlier16,384 B recovery is not credited again.
Current build logs188,192 B/136files. Known retained net+887,586 B relative to
turn start: material+68,065,texture+44,751,exe+39,936,PDB+266,240,logs-3,533,
retained947/948 text+472,127. GPU fixture/image/raw bytes unchanged. Source/object/
CMake/Git changes and volume activity remain unallocated. Firstfree62,592,278,528
->cleanup-end64,831,401,984 B is a2,239,123,456 B drive-wide gain, not cleanup.
No owned producer remains; reused PID27960 was verified to belong to a later
unrelated Android build, which was not touched. Protected945/940/941, original
game data, profiles and build trees remain. Final free measurement follows push.
