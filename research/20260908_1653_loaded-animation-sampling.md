# Loaded motion ownership and runtime keyed sampling

2026-09-08, after566af49. Host168, opt-in `bd_native_animation`; native skeleton
and instance producers must also be enabled. Normal defaults are unchanged.
The prior implementation turn made progress; external push remains blocked.

## Connected result

Completed standalone/packed motion loads now own type2 keys before model/slot
selection. `NativeAnimationAsset` reuses `NativeAnimationClip` and retains sorted
authored name associations, not source pointers. The existing model owner gains
dense joint-name bindings under its existing8MiB capacity budget. Ambiguous
animation names do not invalidate independent geometry/skeleton evaluation.

The missing motion residency owner has an8MiB/4096-entry in-memory cap, including
256B conservative per-entry bookkeeping. Keys and metadata use actual vector
capacities. Packed aliases share an asset; retired-but-leased assets remain
charged. Source keys live only in the temporary boundary index. No disk cache,
duplicate model/pose owner, per-tick key import or first-draw discovery was added.
Persistent native motion identity/cooking and broader streaming remain pending.

`sub_82289888` whole-model reset and weight1/no-forced-subtree`sub_8228A3E8`
now sample those owned keys. The latter is scoped to the actual visual slot
caller and its full root. Binding uses owned model names, not source-node walks.
Normal admitted calls bypass the original recursive keyed walk and packed key
reads. Verify mode calls the original once and compares before replacement.

Outgoing48-byte channel records remain for original layers/gameplay/late writers
and the existing native skeleton bridge. Whole mode clears all bytes; preserve
mode keeps unmatched flags/values, writes every node name, clears only absent
matched-channel activation and sets dirty128 for multi-key tracks. Existing
skeleton/instance and completed/render pose owners are reused, not replaced.

## Additional source provenance

Reuse [the parser/key/slot source map](20260908_1612_native-animation-clips.md).
Read guest-source/devloop, census/frame-interpolation hook TOMLs, model ownership
frontier and complete newly relevant generated functions/control flow.

- Standalone constructor`sub_8217BBE8`,file1:3941 initializes+184 and starts its
  backing request at+164. Destructor`sub_8217BD00`,file94:3864 releases that
  request before optional object deletion. Retire native entries before original.
- Pack constructor`sub_8217C410`,file6:3792 initializes192/196/200. Deleting
  wrapper`sub_8217C538`,file14:3862 calls`sub_8217C580`,file46:3718, which resets
  the index and releases+164. Hook the inner destructor for both call paths.
- `bdAsyncRequestRelease`,file82:2071 decrements the referenced loader count,
  marks zero-reference resources for collection and clears the request. One
  slot/request release is not authority to retire a still-shared motion.
- Loader hooks scope`sub_82288680` initialization under the actual owner; import
  follows completed initialization. Packed aliases reuse initialized pointers.
  New loader/import invalidates old lookup even if replacement is unsupported.
- Sampler state`0x82DC99E0`: compression+0,exclusion count+4,name+8,depth+24.
  Nonempty exclusions/name/depth are unsupported. Admitted calls clear compression;
  preserve dispatcher also clears exclusion count, matching original side effects.

CPU/source lifetime evidence does not prove live destructor/reload coverage.

## Verification and limits

Material50/CPU48 PASS0.14s/0.15s: source destruction, dense/reordered model names,
exact asset/model budgets, aliases, retired leases/backpressure, source reuse,
unsupported invalidation, missing/inactive flags/bytes, transactional rejection,
real skeleton evaluation and completed instance publication. Existing384 angular
half-turn cases still pass. Output82/CPU54 PASS0.53s/0.55s for shared model
consumers.405 artifact-free Python checks PASS0.262s. No tolerance/check relaxed.

Host168 builds without guest objects/shader compilation. CMake regenerated for
the added bridge; codegen wrote nothing and reports its module up-to-date.
EXE49198080B SHA256`33745F1F9E348F8AEB2F4A1B2A914F44A0BCF683E85561469F5C032E7D7C7E19`.

Run976 is **initial admission only**, deliberate diagnostic terminal exit1,
not a field/scenario PASS. All11 settings took effect. Frame385:69 loaded,
992 refused,69 resident/6201072B; sampled1,whole0,preserved1,unavailable0,
checked1,wrong0. First four refusal messages identify type3; not all992 refusals
are classified. One real full-model preserve call matches. This does not prove
nonconstant animation, whole-reset reachability, native skin drawing, sustained
comparison, live retirement, post-event field/reload behavior or motion pixels.
No speedup, conversion percentage, stereo or completed host frame is claimed.

Next own compressed type3 tracks and weighted/subtree/layer application through
these same assets/consumers, extending source-clock/interior-key and live checks.
Types0/1, slot clocks, collision/effects, late writers/special bones, outgoing
channels/palettes, persistent cooking and full desktop events/sequences/both eyes
remain. Preserve975's reload failure and971/962's window/title discrepancy; no
unchanged reload/PrintWindow retry. Quest remains strictly after desktop.

## Storage

Existing trees/operator reused; no game assets, caches, captures or downloads.
Run976 log39663B SHA256`9BFDE6156EAE47CE67BEA8B67E60B61FB25AFD6A6803C545D3C4E54B29C8EE99`.
Its process is terminal and the116B owner profile is restored byte-for-byte.
Exact handles, guards, current/failure retention, cleanup and drive measurements
are recorded in the cumulative scene-state ledger; no budget reset.
