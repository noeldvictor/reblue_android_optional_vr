# Native animation controller and completed channel handoff

2026-09-08, afterb3d1f7b. Full desktop host-renderer goal remains incomplete.
Normal animation/skeleton defaults remain off. No Quest run or speedup claim.

## Connected ownership

The admitted `bdAnimationUpdate` path now snapshots authored selection/clocks,
builds an address-free native controller plan, leases the existing immutable
clips, computes native TRS working layers and publishes one outgoing boundary.
No shared24576-byte-stride scratch arrays, source memset/memcpy or per-layer
guest sampler dispatch execute in its normal native path. Existing standalone
sampler adapters remain for other callers and refused controllers.

`native_animation_controller.h` owns clocks/ordering without source addresses,
48-byte records or rendering APIs. `native_animation_controller_source.h` keeps
native channel payloads with temporary flag/name sidecars; it encodes only the
completed outgoing result. This is not a second skeleton or renderer framework.
The existing blend, hierarchy, clip, residency, instance and render-pose owners
are reused. Source keys are not decoded during native layer execution.

Completed native values pass directly to `EvaluateNativeSkeleton`. The handoff
is thread-local, one-shot, at most4096 joints, keyed to visual/graph/model
generation/output boundary. Replacement, failed publication and retirement
discard stale ownership. Until late writers migrate, every exported channel
word must still compare unchanged; a mismatch/read failure falls back to the
existing checked channel import. Those reads and the outgoing channel/palette
adapters are explicitly NOT removed. Persistent source slot selection and
cooked clip identities also remain future work.

All layer work preflights before publication. Strict mode runs the complete
original controller once, compares slot clocks/loop counts, sampler globals,
every active channel within the existing tolerance and every inactive/header
word exactly, then publishes native values. Collision/effect side effects run
once, not once per comparison candidate. Mismatch throws without a second
side-effect replay or silent fallback. Normal mode calls those side effects
after native publication. Original-comparison mode is not a performance run.

## Recovered contracts

Main read guest-source/devloop/disk policy, active queue and the existing
animation findings. Census, frame-interpolation and render-tweak hook TOMLs were
inspected. Generated files were read, not modified; no hook-site/codegen inputs
were changed. Whole-function hooks remain in the existing OBJECT owner.

- `bdAnimationUpdate`, file53:2663: six physical56-byte slots; active count+2208.
  Weight/time use scalar float-rounded double FMA and delta0x82DDA880. Weight
  clamps only above target. A looping clock beyond duration increments once
  and subtracts duration ONCE; exact duration does not wrap. Negative clock stays
  in gameplay state while the ordinary slot sampler clamps its sample time.
- Slot state writer `bdVisualObjectSetAnimation`, file86:2636: resets time/loop
  count on changed/restarted selection; unchanged selection may retain them.
  Snapshot this authored state; do not invent a separate autonomous clock.
- `bdVisualObjectAnimSlotUpdate`, file89:2624: replacement mode0x82DEBEEC bypasses
  weights and whole-samples; ordinary mode clamps sample time/weight independently
  of stored clocks. Missing entry skips the slot. Nonpositive weight is a no-op.
- Multi-layer composition samples positive contributions, first copies a layer,
  then uses the current/(adjacent previous+current) contribution, NOT an accumulated
  normalized sum. Third/later mixes preserve the source's in-place flag activation
  semantics in the boundary adapter. Final base-weight mixing uses a separate
  previous-channel copy, not that alias rule. Positive absent layers or no supplied
  multi-layer output would read stale scratch; native plans refuse before writes.
- Overlay vector indexing `sub_82144E18`, file94:2712, is checked pointer-vector
  access. Overlay slots already in the active range advance again. Names choose
  a forced subtree via native owned model names, preserving traversal ordering.
  Exclusion IDs use `sub_8227EF60`, file89:9039, node+0 pose identity, not a hash.
  Unresolved exclusion IDs leave an uninitialized source slot: native admission
  refuses rather than manufacture a name. `bdSceneGraphFindNodeByName`,
  file48:9690, is first-match preorder by inline string, not hash equality.
- `bdEffectUpdate`, file27:2652: consumes channels but writes separate effect
  state. `bdVisualObjectCollisionTestNearby`, file71:2694, and recursive
  `bdVisualObjectCollisionTestNodes`, file79:2614, update separate collision
  records, not controller channels. Retain these gameplay/effect calls once.
- Controller UV tail adds offsets then wraps toward zero, including negative
  offsets and signed zero; it is NOT just division by one. Native math preserves
  its vector-denormal flush. CPU coverage is not live UV/pixel qualification.
- `bdVisualObjectInitBones`, file30:2686: ordinary skinned evaluation consumes
  visual+2628 channels, then existing secondary/late pose work follows. The new
  handoff occurs at that existing evaluator boundary; it does not skip later work.

## Verification

Material59/CPU57 and60/58 pass; final61/59 passes0.12s/CTest0.13s.
412 artifact-free Python guards/scenario tests pass. A guard initially selected
the entire remainder of the bridge instead of just `Sample`; its extraction now
ends at `Mix`, preserving all sampler assertions and adding controller/lifetime
wiring checks. These guards are not behavior or pixel evidence.

C++ fixtures cover:65 advancing three-layer transactions against the prior
live-checked ABI implementation; independent literal clock/order expectations;
adjacent denominator, final-base non-aliasing, overlays advanced twice, negative
and exact-duration clocks, unsigned loop overflow, missing layers/assets and
whole-transaction refusals. They retain129 advancing samples per indexed format,
ignored/nonfinite scale bytes, source destruction, native hierarchy/instance
consumption and retired pose lifetime. Handoff tests cover generation mismatch
before reads, exactly-once consumption, late writes, visual retirement, failed
replacement/address overflow. Signed UV fraction/denormal tests pass.

Host176 and final177 build0;176 rebuilt version consumers plus the two bridges,
177 only animation bridge/link; codegen0writes, no guest objects/shaders.

- Host177 EXE49275904B, SHA256
  `4C11FC7D3BA6B601EDEA8A60522B7ABADD7073AB554DDBFE5AFA0DC1FD41D3EF`;
  PDB113266688B.
- Material60 EXE1263104B, SHA256
  `6208293D4C0349278199BB8683AC87A91A064C8C45656D6C1830541A6A689A6B`;
  tree10407859B/43files.

Capture-free mono run983/PID25056/session81327 stopped19:50:13; operatorexit1
is its explicit diagnostic-observed stop. New criterion:>=1000 matching complete
controllers, positive mixes,>=256 interior-CLIP samples,>=256 advancing clocks
and>=256 handoffs. This does not replace979/980's unchanged weighted/subtree/mix
thresholds or assert interior-key sampling. No unchanged retry was performed.

Last controller report frame1162:completed/checked30460,refused5028,sampled3996,
mixed8,subtree0,interior3839,handoffs3985,changed6,advancing4012. Completed count
includes empty plans; it is NOT30460 animated models. Admission failures are
currently model/controller-boundary refusals, not yet classified. Six changed/
unreadable handoffs demonstrate the guard's necessity; exact late writers still
need identification. Standalone sampler report:6 sampled/checked,wrong0.
Skeleton frame1122:evaluated/published/checked3524,wrong0,unavailable0.

Frame1137 context:FieldActive,bg41_01,event1. Not interactive-field, full reload,
motion sequences or both-eye acceptance. No new pixels. Preserve975's reload,
979/980's missing coverage and971/962's capture/presentation failures. Native UV
tail and normal-mode side-effect outputs still need scoped live/pixel coverage.

Run983 re-verifies first-match canonicalization122->121tracks,93432B current
asset charge,prepare-refused0. Older174/982 charged93424B before indexed metadata;
the different source address25C5D13C is not persistent identity.982/174 text is
superseded and retired; its source findings/hash remain in the indexed report.

- Run983 log133297B, SHA256
  `64868E4469A9D62DF9E8642CC72405F9F227EDED4765FEEBE4B14F48D50C7D22`.
- Original profile restored exactly; independently verified SHA256
  `2F1BC38D763A1B7BDBA31F560684FD4AA7E42A714600B8D344F19DA7F38E23B0`.

Final source-boundary review added a focused extent guard: multi-layer imported
source counts above512 overlap its original scratch arrays and cannot be treated
as independent layers. Native/sequential plans still support4096 joints. CPU
tests cover512/513/4096/4097;412 guards pass0.231s, material61/PID32820 build0,
CPU59/PID25736 PASS, host178/PID32484 build0 (private bridge/link only). No new
runtime after this admission-only change;177/983 remains the live evidence.

- Current host178 EXE49275904B, SHA256
  `D176E76A5CB3F0C9592D9A3561FFBB129EEF784DC80D759DC25323D66669A837`;
  PDB113266688B.
- Current material61 EXE1263104B, SHA256
  `0BB1D60B47DB17BFFA535932BD8E2B4298EFDE470140A8C07E98E4541CD29769`;
  tree10409344B/43files. Superseded60/58 text retired after this passes.

## Storage and next work

Same cumulative ledger/floor62509998080B; no budget reset. Runtime reserved
409600B text,192MiB free-drop,60s; preflight78105068B diagnostics under75MiB
stop/100MiB ceiling. No new raw/perf/cache/dump/image output. Existing13 window
images still10434657B; image growth gate remains in force. Final accounting and
measured reclamation are in the shared scene-state ledger.

Next remove remaining source selection/channel writers and their validation
read/export, with source-specific regressions for six observed handoff refusals.
Broaden real authored subtree/indexed/motion coverage without letting absent
content stall other native owners. Dense modes, duplicate model names, special
bones, cooked identities and complete desktop scene/event/sequence/both-eye
qualification remain required before Quest2 optimization.
