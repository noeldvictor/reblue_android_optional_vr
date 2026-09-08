# Native render-pose ownership and connected consumers

2026-09-08. Source checkpoint11c193a plus the water connection and fixture
changes in this report. Host165 is the first built binary for the timed pose
work; host164/run971 is older live evidence, not a run of this implementation.

## Delivered boundary

The existing `NativeInstanceRegistry` owns immutable completed poses, timed
history and render-phase results under its unchanged16MiB aggregate budget.
Pinned history and queued/GPU readers remain charged until their actual last
release. Completed handoff advances history; repeated writes within a logic tick
replace only the current endpoint. Skipped ticks, rewinds, disabled interpolation,
nonaffine matrices and discontinuities cannot blend stale history. Invalid phase,
identity or memory-pressure requests refuse rather than return a stale result.

The bridge freezes one host frame-clock phase across native consumers. Raw
`FindNativeInstancePose` still verifies completed authored/source values;
`ResolveNativeRenderPose` does not import source matrices or allocate guest
shader-upload scratch. It resolves the exact current native publication.

Connected paths reuse existing owners and GPU programs:

- Native rigid/skin pre-culling and scene/shadow preparation use the same
  interpolated world/joint transforms. Rigid-only and mixed-node bounds account
  for scale and attached geometry, without an irrelevant skin-vector requirement.
- Scene packets bind world and pose together before planning. Shadow and scene
  skin palettes retain the render lease through the existing batch/fence path.
- Admitted water deferral retains both the completed and render leases. Sorting,
  native GPU values and wave-expanded bounds use render time; remaining outgoing
  world-state exports use the raw completed pose. The sorted-entry import path
  compares raw source values before selecting native render time.

No shader, asset format, cook, GPU owner, source-address index or second animation
framework was added. The blend preserves the existing scalar matrix/1.5-delta
discontinuity contract; this is not a new native animation-curve evaluator.

## Verification

- Material build44 / CPU42: `native_material_data` passes0.15s (CTest0.18s).
  Production registry tests cover same-tick late writers, shared frame results,
  discontinuities/mode changes, invalid identity/phase, immutable raw/queued
  leases, retirement, exact shared-budget overlap refusal and recovery.
- Post-output build80 / CPU52: `host_post_output` passes0.63s (CTest0.65s).
  The new test publishes real native model/instance owners and builds rigid/skin
  scene and shadow plans. It checks numerical bounds, mixed siblings, actual
  packed world matrices, shared palette planning and delayed lifetimes. Water
  queue tests reject wrong/missing render identities and retain distinct raw
  and interpolated transforms after source/model retirement.
- 390 artifact-free Python source/scenario tests pass; these are wiring guards,
  not animation or pixel qualification.
- Host165 links the real consumers. Codegen reports0written; no guest object
  recompilation. All producer handles are terminal.

Output78 was stopped by its64MiB disk guard before writing an object, during
rapid unrelated/unattributed volume allocation. After stable-space checks,
output79 built. CPU51 then exposed the new fixture's omitted per-range shadow
policy: production model publication correctly refused it. The fixture now
supplies explicit Receive policy; production validation was not weakened.
CPU52 verifies the corrected setup and the added water connection.

| Retained binary | Bytes | SHA256 |
| --- | ---: | --- |
| material fixture44 | 664576 | `505C011AFFA6D5F8A2E09FA121B6706052E9A1F3FA52207D76C344F7C73B1DD3` |
| post-output fixture80 | 1819648 | `AC3CB82610B0DDA3A5AF9260A4709E0F680CDD26708692A402EA5617AB99C792` |
| host165 EXE | 49102848 | `09ADF4E2269BB08D2A09F739BBD77585FFE23415865ADDE5A05727FAAF19C791` |
| host165 PDB | 111812608 | `4D3CADFBAF468767DB1F344906D0070B57CAB38152B04AB77A5341B433EF3C8D` |

## Remaining acceptance and interfaces

No new game run, image or shader/GPU run was performed. Fresh unlocked-FPS
ready-field/reload motion and actual renderer-owned pixels remain pending.
Existing345-case rigid/skin/Toon and20-case water GPU evidence covers unchanged
shaders, not new runtime timing. Run971's title-logo/window discrepancy remains
an acceptance failure; use the existing fence-owned observation after its named
image budget and aggregate overlap fit, not another unchanged PrintWindow retry.

Original animation evaluation and its collision/effect side effects, conditional
source-palette copying, secondary palettes, source identity/import adapters and
legacy interpolation for unconverted families remain. Skin/local animation
assets, specialized materials/effects and full pass/frame ownership are not
complete. No speedup, art parity, stable sequence, game stereo, fully host-owned
frame or Quest readiness is claimed.

## Storage and delivery

Same cumulative ledger and3GiB owner exception/floor62509998080B; no reset.
14:20 free72185679872B;14:39 free68190777344B. The3.72GiB drive-wide decline is
not the size of this change: selected retained growth after cleanup is346718B.
Fourteen superseded build/test logs and one0B interrupted object were removed
after replacements passed:8308 logical bytes,20480B measured free-space gain.
Current fixture/host outputs and unresolved game/pixel/storage evidence remain;
no game data, saves, profiles, assets, active build tree or capture was removed.
See the cumulative ledger for exact producer IDs, guards and cleanup paths.
The owner's116B profile remains unchanged, SHA256
`2F1BC38D763A1B7BDBA31F560684FD4AA7E42A714600B8D344F19DA7F38E23B0`.
Push remains restricted by the security reviewer pending exact owner approval;
this continuation did not retry or bypass that restriction.
