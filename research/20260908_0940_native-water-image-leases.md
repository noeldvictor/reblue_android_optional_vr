# Native water image-owner connection — 2026-09-08

Source base f0fa499, dirty source during verification. This is a prerequisite
connection, not a claim of another live converted rendering family.

## Changed boundary

The previous water packet demanded `NativeTargetImageHandle` for every dynamic
sample. The live snapshot producer actually uses `NativePostImagePool`, and
MSAA results use `NativeSceneResolves`. Duplicating those images or borrowing a
view from `GuestTexture` would not establish native ownership.

`NativeImageLease` now has typed `From` handoffs for all three existing owners.
The retained array view comes from the same owner as its image/layout/descriptor;
there is no cast back from an erased owner. Image-only leases remain useful for
copy/post interfaces but cannot supply a sampling view. Equality distinguishes
the allocation control block, physical image, layout record, extent/format,
descriptor/sample count and view; two resolve roles sharing one owner differ.

Installed in the real publishers: `native_scene_pass_bridge.cpp`,
`native_shadow_pass_bridge.cpp`, `native_scene_snapshot_bridge.cpp`,
`hooks/native_deferred_visuals.cpp` and `Video::PublishNativePostOutput`.
`NativeWaterImages`/the shared binder consume these leases directly. No new
allocation, copy, residency store, descriptor import or resource lookup was
added to the water consumer. Producer ordering and existing fenced stores remain.

## Verification

- 371 artifact-free Python source/scenario checks pass. Old source-string
  assertions were updated to the typed handoff; existing ordering/no-copy guards
  remain. These checks are not C++ or game qualification.
- CPU output build57/PID35536 and test33/PID31072 pass (0.48s).
  The existing pool test now retains the real sampling lease, blocking overwrite
  and eviction. New mono/stereo resolve cases retain both sampled roles and the
  source attachments through framebuffer destruction; invalid roles, missing
  views, image-only handoffs, MSAA sampling, owner/role equality and shared layout
  behavior are checked. Existing scene/batch/order/receipt tests still pass.
- GPU build61/PID7280 and water9/PID38340 pass:16 two-eye 8x8 cases,1.14s,
  Vulkan validation0 errors/0 warnings. One pre-existing missing GOG overlay
  manifest loader diagnostic. The actual post-image pool owns the snapshot;
  source handle retirement before the draw cannot permit a new writer, and
  image/view/framebuffer destruction still waits for the recorded fence.
  Production packer/binder, indirect draws and existing strict pixel oracles
  are unchanged. Raw/image files produced:0. No threshold was relaxed.
- Host139/PID34368/session14299 terminal0. Codegen reports0 writes/deletions,
  one module up to date; no guest objects compiled. Shared header changes rebuilt
  host users.17 warnings in unchanged settings/resource/mirror/framebuffer code.
  No new game run or profile edits. Last live evidence remains host134/run969;
  unchanged55 rigid GPU cases remain the prior build58/rigid20 evidence.

| Artifact | Bytes | SHA256 |
| --- | ---: | --- |
| host139 reblue_vk.exe | 48,880,128 | 4FE4E315F717E29BDBBECA26B920038B62400B284CF7231A9CEBB5F4A124B190 |
| host139 reblue_vk.pdb | 110,653,440 | B42B4BBEF05B6F778D9F662D7F2553210F62B4C0B3F8BB116FB2292939F2FE8A |
| GPU61 native_scene_snapshot_test.exe | 1,119,232 | 0D2578B14F36E3420B3951E4FEBFA208698B8B9EE520FCF50C80F25130ECD4C2 |
| CPU57 host_post_output_test.exe | 1,090,048 | F0BF068809AC29FD8C0E53DD9CA5FD42AC5627CC303B519F30FBEDFC894DBB4A |

## Still required

No game producer calls `SubmitNativeWaterScenePackets` yet. Connect completed,
writer-ordered material values and these image roles to native instance/model
identities, then admit water into the existing mixed sorted consumer without
per-entry legacy material/resource execution. Preserve snapshots, authored
updates, blend/depth and upstream wave-displaced walk bounds. Actual game
stereo inputs, water shadow views, all authored controls and game art/sequence/
reload/both-eye qualification remain open. No full host frame or speedup claim.

Storage/producers/cleanup are appended to the existing cumulative ledger in
`20260906_0333_native-scene-state-bridge.md`; prior raw/image gates remain intact.
