# Native sorted producer -> owned packets -> mixed consumer

2026-09-08. Source base f3131a5; host130 built that revision dirty with this
connection. The preceding push-only interaction verified Git state, not renderer
progress. This continuation changes the production connection; the full goal
remains active and unqualified.

## Delivered boundary

`SubmitNativeRigidScene` admits ordinary phase0/pass-mode0 rigid deferred siblings
only with `bd_native_rigid_deferred` (default false). Object preparation owns
geometry, material values, textures/samplers, world/camera, fog, shadow and light
recipes. `StageNativeRigidSceneDeferredForObject` resolves the temporary visual
identity and `NativeDeferredQueue` takes copied, address-free packets. Immediate
siblings retain their original order; delayed siblings have independent final
light tickets. No compatibility list entry is allocated or copied for this path.

Queue limits:5140 combined native/legacy entries,8 MiB retained packet metadata,
4096 siblings per source submission; the existing GPU retention cap remains4096
records per fence slot. Geometry/image payloads reuse their existing owner budgets.
The queue preallocates its bounded entry vector and publishes only after all new
siblings are prepared. Bad depths/recipes, stale frames, decreasing insertion
anchors, overflow and staging while draining are rejected. A packet can be taken
once; an incomplete drain cannot be reset. Source visual identities and their
authored blend modes are confined to the consumer's temporary bounded sidecar.

`ConsumeDeferredList` merges both kinds before stable back-to-front sorting;
equal-depth ties preserve original insertion positions. Sort-disabled and
nonfinite legacy depth behavior remain explicit. It closes a preceding legacy
capture before native work, preserves exact visual begin/end grouping, then sends
the owned native packet to `SubmitNativeRigidScenePackets`. The native branch
does not call material begin/end, bind source textures/geometry, copy material
constants, import world/bones, or submit a D3D draw. Existing queue/indirect draw,
current-depth visibility and fence owners are reused. Lost list imports or an
off consumer cannot fall back and silently drop already accepted native work.

After visual begin, the branch consumes the native blend/alpha publication and
native receiver colour. It verifies the retained shadow image/matrix still match.
Light Bind/Keep resolution and outgoing light compatibility publication occur
at actual sorted submission. No walk-time inherited light value is frozen.
An unexpected pending stencil surface, changed callback/visual mode, missing
late owner or non-ordinary visual result fails explicitly.

## Callback decision / source provenance

The earlier audit reused the current decrypted executable in RAM and verified
the cached loaded image against `assets/default.xex`; image SHA256
`1c3f70de1eda48b8b3e7cc4f670a3def06e9f75fa71fd377cbf92c4c910b1048`.
Its loaded mapping is VA minus0x82000000, **not** PE raw section offsets.
No executable/image/census output was created for this connection. Existing
translated source and `config/hooks/render_list.toml` were reused/read.

- Model callbacks: light object82E246F4, vtable8206C78C, begin8218B310,
  no-op end820DFA50; shader object82783A58, vtable8206A72C, begin82174270,
  no-op end. Priority0 precedes shader priority-10000. Masks1/7 admit both to
  the model group. Light constructor `sub_821839A8` writes that vtable.
- Shader ordinary view3/tech0 material branch (`generated/reblue_recomp.64.cpp`,
  `loc_82174364`) chooses shaders; it does not rewrite the entry material blob.
  Light callback (`.55.cpp`, `sub_8218B310`) handles visual/node selection cache
  and descriptor flush; the owned action plus outgoing mirror replaces it.
- Primary shadow visual begin82176708 (object82DD6100), indexed begin82177650
  (82DD62A0 + index*420, eight), auxiliary begin820D1998 (82DD6FC0) precede
  shader visual begin82174648/end82174C60. Shadow participants have mask6, not
  model participation; other reflection/pass-only tables have mask4.
  Indexed setup is tech14-only; ordinary tech0 returns1 without writes.
- Shader deferred visual begin (`.88.cpp`, `loc_821749E4`) skips direct visual
  material/suppression work. Ordinary tech0 skips special PS67..72 branches and
  returns2. Class-based zero/restore affects shared material staging, not the
  already copied entry. Paired end (`.24.cpp`, `sub_82174C60`) restores it.
  These visual callbacks remain for mixed legacy interoperation.
- `bdSceneNodeDrawSingle` (`.40.cpp`, stack304 copied to entry240,28 bytes):
  entry244 is the visual, so `PrepareEffectModel::InputResource` invokes that
  visual's virtual32/36. Both must be820DFA50 before their omission is admitted.
  `CheckNativeDeferredContract` validates actual live methods and ordered object
  identities, rejecting duplicates/unknowns; a static vtable census is not the
  runtime proof. Models must be lights then shader; optional known visuals may
  be absent, but the terminating shader is required.
- `.40.cpp`,82280C3C..82280C6C: shadow participation (stack120) gates the
  value at High(-32035)-26168 passed in r6 to the depth helper. That same
  sort-disabled word and fixed depth High(-32251)+20912 are imported once into
  the producer scope. Existing pure bounds/depth math supplies each native key.
- `sub_82425C28` (`.49.cpp`,18182) is **blend-factor selection**, not an arbitrary
  shader-constant resource callback. Modes0..5 set states72/76. Native packets
  consume the resulting typed publication after this visual transition. Changed
  saved modes are not silently accepted, nor is the walk-time blend reused.

## Verification and limits

All launched jobs terminal0:

- Output build50/PID32180 and deferred build2/PID37152, same existing fixture
  tree sequentially. CPU33/PID36876 passes all32 tests in7.42 s. Changed tests
  cover the actual production queue, direct/deferred partition, transactional
  refusal, packet owners after producer destruction, late blue Bind before Keep,
  frame/anchor/reentrancy/duplicate/incomplete-drain failures and optional/unknown
  callback/resource/visual modes. Existing mixed-order/depth tests also pass.
-353 Python source/scenario checks pass in0.576 s; these are not GPU behavior.
- Host130/PID37044/session14334 terminal0. Build stamp f3131a5d5 dirty. Codegen
  wrote0 files,1 module already current; no guest objects compiled. EXE48,791,040 B,
  SHA256 `AE12CB1BEAB1219D3F5A65A965B0B458B9BC3B97D426B060E3E05CE57A97BC33`;
  PDB110,235,648 B, SHA256
  `3824CB990C5D540030062ACCA40BEB59608CFCAAC7C1DBF1F0E2336928BC9656`.
- Production shader/binding code is unchanged. Prior rigid18's55 two-eye Vulkan
  cases (validation0/0) are reused; they do not exercise the live mixed consumer.
- No game, GPU fixture, raw capture, image export, asset cook or profile edit.
  Owner profile SHA256 remains
  `2F1BC38D763A1B7BDBA31F560684FD4AA7E42A714600B8D344F19DA7F38E23B0`.

Next meaningful observation is a bounded live admission/consumption run of this
changed connection, with fresh staged/consumed receipts and a concrete first
refusal if unsupported. It must be followed by actual mixed-order pixel and
scene/reload qualification. The unanswered image-budget question still pauses
larger image exports, not independent source work. Prior945 accepted image and
all unresolved941/948/956/957/960/962..964 evidence remain protected. No speedup,
whole-scene, stable sequence, both-eye game or full-frame completion is claimed.
Retire the visual transition and outgoing mirrors when their remaining consumers
move to the existing native effect/state owners; they are not the final renderer.

Storage accounting continues in
`20260906_0333_native-scene-state-bridge.md`, without resetting any exception.
