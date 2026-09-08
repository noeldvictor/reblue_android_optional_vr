# Deferred packet order and late light actions

2026-09-08, parent8d6f4e4. Host128 is8d6f4e426 dirty, not a restamp of host127.
The preceding turn made progress by pushing the explicitly unconnected source
checkpoint. This continuation verifies that groundwork and corrects two ownership
contracts; it does not claim live native deferred draws.

## Source decision

The guest-source skill bounded the investigation by the missing sorted consumer
inputs. Reused the prior node/depth audit and read the actual callback boundary:

- `bdSceneNodeDrawSingle`, generated40,822809A4..D0: the object cutoff override
  is direct-only. Deferred entries copy the running cutoff from stack+324 to
  entry+260 before any such setter. CPU coverage now distinguishes emitted
  deferred entries from suppressed sorted policies and their direct siblings.
- `ConsumeDeferredList` establishes GE for the ordinary pass; a preceding direct
  comparison is not an input. Native sorted plans ignore even an invalid unused
  comparison. Owned bounds/world/view produce the far-extent key; the saved
  shadow-participation flag determines depth writes and fixed-key eligibility.
- `bdInitInputSystem`, generated13:9893, initially installs default/no-op list
  callbacks. Initialization at generated29:2928..2967 replaces list+36/+40 with
  `sub_8221D530`/`sub_8221D548` (8221D530/8221D548), respectively.
- `sub_8221D530`, generated98:7523, forwards entry+240 and the deferred flag to
  `sub_8221DB00`. Its active host replacement is in
  `native_effect_activation_bridge.cpp`, using `PrepareEffectModel`; the end
  hook uses `FinishEffectModel`. Registry iteration is native, but participant
  virtual methods and resource begin/end still execute via the dispatcher.
  Therefore `sub_82174270` alone is not the complete material contract.
- `sub_82174270`, generated64:3551: ordinary view3/9,technique0 selects a shader
  from material feature words and live light/receiver classification. That branch
  does not rewrite the material blob. This says nothing about other participants.
- `sub_8218B310`, generated55:4018: actual participating visual/node updates call
  `sub_82142C58`, then flush light buffers. As recovered in the ordered-light
  report, a null per-node entry is Keep, not the object's default. Resolving a
  Keep before sorting can capture the wrong previous draw's values.

The remaining decision is to account for the complete ordinary participant chain's
late material/pass outputs and suppression before replacing it. No diagnostic
game boot is needed to learn the callback addresses above. Do not treat generic
callback execution as a permanent native-material API or enable a partially
frozen deferred plan while that contract is incomplete.

## Implementation and tests

`NativeSceneLightingPublication::Capture` returns owned explicit light values or
a Keep action with publication identity; it retains no source/object lookup key.
`Resolve` reads current inherited values only at final draw position and returns
the normal revision-checked commit ticket. Same-frame republishing, another frame,
unknown intervening writes and reused tickets remain fail-closed. Existing direct
`Prepare` immediately composes these same operations, preserving its behavior.
No second light registry, persisted asset or compatibility cache was introduced.

The deferred metadata merge is bounded and transactional, preserves original
mixed submission ties and contains no guest addresses. Its index arithmetic now
also refuses uint32 overflow when callers supply larger custom limits.

The devloop skill selected existing fixtures; all producer jobs completed0:

- deferred build1/PID35932, CPU1/PID17756: mixed order, invalid anchors/capacity,
  finite depth, source-free depth recipes and existing arena tests pass.
- post-output build48/PID34348, CPU31/PID36396: native deferred plan bounds,
  copied resource lifetime, fixed depth, depth-write policy and cutoff tests pass.
  Existing JPEG failure controls print expected incomplete-scanline diagnostics.
- material build39/PID33772, CPU37/PID38420; extended build40/PID32744,
  CPU38/PID30464: direct/deferred cutoff and reordered Bind/Keep regressions pass,
  alongside the existing identity/lifetime/material/lighting fixtures.
- GPU build50/PID36768, rigid17/PID38268:55 two-eye cases pass in1.33 s on
  RTX3060, validation0 errors/0 warnings. Three added cases run overlapping
  native production-shader indirect commands in sorted order, reversed unsorted
  control order and sorted order without depth writes. Each matches an independent
  scalar source-over/depth oracle. Maximum new colour error2.3745e-05 under the
  unchanged0.003 limit. This models mixed metadata, not actual legacy shader
  interoperation. All GPU readback stays in memory; no raw/image files.
- 351 Python source/scenario checks pass. The first run failed a literal source
  guard expecting the old single/two-instance expression; updated that assertion
  for the new deferred two-instance case after real GPU validation. No behavior,
  threshold or failure gate was weakened.
- Host128/PID38716: incremental target `reblue` links, codegen0 written/1 module
  up to date, no guest objects. EXE48,761,856 B SHA256
  `2E909E34A408D3F95F26E81A18DCBD4F1DFF8DEFAC6A943286FAC981100E22F5`;
  PDB109,977,600 B SHA256
  `2835553A93424514A660B8C9C121DFA084A77C6D26DB1880F2A9C3E68FA0DA2C`.

`PrepareNativeRigidSceneForObject` and `ConsumeDeferredList` still do not exchange
native deferred packets. No additional guest rendering dependency is removed by
this checkpoint. No new game run, accepted frame or stereo/Quest qualification.
Preserve all existing live failures and the pending bounded post-gamma observation.
Storage uses the original cumulative ledger in
`20260906_0333_native-scene-state-bridge.md`, not a new allowance.
