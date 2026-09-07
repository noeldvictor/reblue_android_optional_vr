# Ordered native scene-light inheritance

2026-09-07, parent4a33339. Connected implementation and host108 build; game
acceptance is pending. Host107/run945 remains the last accepted live checkpoint.

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
