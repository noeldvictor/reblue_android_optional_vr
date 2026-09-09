# Native material selection and clock — 2026-09-09

Connected bundle afterbf31b73: completed binding/cue writers -> instance-owned
selected IDs, cue ordinals and clock -> native effect/controller updates -> next
native tick and actual image/material selection. No full-frame or VR completion.

## Source and scope

Reused preceding catalog, effect, image-key and controller audits. Read complete
`AnimeData_method_1338`, generated/reblue_recomp.72.cpp:2662, and reread the image
consumer `sub_821444E0`, .80.cpp:2623 for the remaining scratch contract. Relevant
`config/hooks/render_tweaks.toml` read completely and unchanged. No existing raw
hook for1338. The direct-call index finds21callers for1338 (event/gameplay/viewer
writers), and just the image updater calling `sub_82151E10`. This direct-call
census is not proof that no other code reads its scratch storage.

1338: r3visual,r4signed mode,r5firstID,r6loopbits,r7restart,r8secondID. Negative
mode resolves the first cue only and uses that result/ID for both slots;
nonnegative mode resolves both. Each lookup polls the first ID match via the
already-audited BC48 contract. Restart or changed entry pair writes IDs2212/2216,
loopbits2220, entries2232/2236, resets clock2224/rate2228 from authored globals,
and clears queued IDs2240/2244. Equal pair with restart0 preserves the entire
timeline. The new hook runs the complete original **once**, then imports its
completed output; it does not duplicate polling or invent request semantics.

The image updater converts clock2224 to integer ticks, calls key selection,
reads its returned scratch vector, and writes a nonnull selected image to the
material table. Hold behavior and selected procedural work still depend on that
outgoing scratch across fallback calls. This bundle removes timeline **input**
reads from native evaluators; it does not prematurely delete scratch exports.

## Ownership and consumers

`NativeMaterialAnimation` is a fixed-size value in the existing instance entry:
authored ID pair, queue pair, loop bits, time/rate, native cue ordinals and a lease
on the same native catalog. No source entry address or source reader is in it.
No extra index/budget or per-tick allocation. Catalog memory remains charged to
the existing animation residency while any state snapshot pins it.

`material_animation_source` is the explicit import/export adapter. It translates
selected entry pointers to existing catalog ordinals, rejects foreign/stale
entries, and keeps the exact nine-word late-write guard in the existing instance
source binding. Read mismatch invalidates; restoring words cannot revive state.
Instance/model/catalog identity changes cannot reuse stale ordinal selections.

Final material binding, completed cue requests and completed original controller
fallback publish state at writer boundaries. Append/clear/rebind invalidate before
source mutation/retirement. Binding releases the instance lock before refresh;
no lock crosses the original writer or reverses instance->animation ordering.

`PrepareEffectUpdate` advances the owned clock and cue selections, then creates
outgoing words. `Controller` publishes the new native state directly; the next
tick consumes it. Material-image selection receives the same state and reads its
native IDs/time, not visual2212/2216/2224. Actual immutable image leases still
reach the existing material importer/composer/native packets.

Active cue duration/readiness, scratch/hold/procedural behavior, late writers and
outgoing timeline/material exports remain tracked adapters. Unknown writers may
force a whole original controller before a new completed publication. Native
lookup itself never imports. Defaults and strict comparison checks unchanged.

## Verification checkpoint

-428 Python source/scenario guards PASS0.229s; these are not behavior/pixel tests.
-Material83/PID28800/session48482 build0: three fixture objects and link.
-CPU80/PID36412 PASS0.12s/CTest0.13s. Actual production instance state/late guard,
  consecutive effect updates and image-to-material composition exercised with
  timeline reads forbidden inside the evaluators. Every late timeline word,
  no-resurrection, foreign/invalid cue, model/catalog generation, snapshot copy,
  fixed budget and source/instance retirement cases pass. Existing UV/eye/image
  ordering and pending/duplicate cue tests retained.
-Host196/PID32336/session34875 build0,link26/29; shared instance header rebuilt
  its actual host dependents. Review then added the disabled-feature early return
  to the refresh bridge. Host197/PID27036 build0, one bridge object and link3/4.
  Both codegen0writes/no guest objects or shaders; bannerbf31b73dirty.
-Fixture1726976B/tree11973062B/43files,SHA256
  `C45D57327B5AB5896ECA4957243D57A33BA5E092307C98D262E4C43407AA9FC5`.
  Host197EXE49474048B/PDB114651136B,EXE SHA256
  `0130832366A55A4D49C57CC5B2E125E950AC713107F1B9D3A50A9AF8F351D54F`.
  Runtime/pixels/reload/stereo are pending at the source checkpoint; no old
  binary/log is relabelled to qualify this new state owner.

Live acceptance will require native publications/reads advancing with stable
boundary-import counts in fresh idle-field samples, plus exact image updates and
positive material packets, retaining prior program/UV/eye/controller/late checks.
Queued transitions and viewer/reload/pixels require actual corresponding content;
the opening scene previously had zero queued transitions. No FPS claim.

The original cumulative ledger remains
[scene-state bridge](20260906_0333_native-scene-state-bridge.md#2026-09-09-native-material-selection-and-clock-afterbf31b73).
No capture, recook, download or device work requested; preserve catalog195 and
older unresolved failures until a verified replacement serves the same purpose.
