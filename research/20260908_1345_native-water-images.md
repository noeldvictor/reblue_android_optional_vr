# Completed frame images feed native water

2026-09-08. Parent42fd400 plus this bundle; Plume191c31c unchanged.
This removes consumer-side frame-image selection through resource getters, not
all water or frame adapters. Full desktop scene/event/pixel/both-eye acceptance
still precedes Quest work.

## Connected boundary

The existing native reflection pass publishes its exact completed HDR image into
`NativeReflectionPublication`. Water selects the named water plane, never the
last reflection drawn or an image matched by size. Publication requires the
producing frame, consumed clear, exact image/layout/descriptor/extent and shader-read
completion. A new write, stale-frame read or compatibility producer invalidates
future reads. Copied leases already in native water packets remain valid through
replacement and GPU retirement. No new image allocation, copy, asset cache or
second draw queue is introduced by this connection.

`NativeWaterMaterialScope` gets planar input from that completed owner. The outgoing
slot7/getter bridge still runs for mixed legacy consumers, and its image identity
must match the native publication: unknown late getter aliases refuse rather than
silently change the sampled image. The mirror is a check, not the native selector.

The existing snapshot producer now accepts a subject and returns its exact completed
or reused lease directly. Native water calls that producer without a guest callback
or a later read of the scene/reflection getter. Refusal does not replay the original
after water material side effects. The old hook still provides preflight fallback
for unmigrated consumers. Authored refresh/shared timing and outgoing getters/slot13
remain inside the snapshot producer; this is not a fully host-owned snapshot scheduler.

## Source contract

- `sub_82454720`, generated/reblue_recomp.38.cpp:19825: after factor and ordered
  descriptors/state writes, planar sampling always reads plane
  `(uint32_t(-32035)<<16)+29040`, member12. Slot12 follows the current scene table.
  Only a late signed material4700 value greater than zero calls `sub_8221D2C8`;
  r4 is the material subject. The whole water body was checked, including its
  final snapshot call and return. Existing setup/animation hook contracts remain.
- `sub_8221D2C8`, generated/reblue_recomp.38.cpp:7125: the whole body uses r4 for
  same-subject/shared/ready scheduling; r3's subobject is not read. Scene and
  reflection phases select distinct outputs. The active native implementation
  already replaces the copy/resolve path; returning its lease does not alter
  ordering, extent, layered copying or cache publication.
- `sub_821875F8`/`sub_821877C8` remain the active native reflection begin/end hooks.
  The existing checked source+40 plane identity selects which completed producer
  publishes the water role. Other reflection planes cannot overwrite that role.
  The publication follows `FinishNativeReflection` and validated outgoing export.

## Verification and limits

CPU65/PID18836 and output40/PID35532 pass (0.47s test). The existing production
framebuffer/image-pool fixture covers one and two layers, incomplete clear,
wrong frame/image/layout/descriptor/extent/view, duplicate completion, same-frame
replacement, stale-read non-resurrection, compatibility reset and queued image
retention after source/publication/framebuffer retirement. The pool cannot reuse
that image until its final queued reader releases it.

378 artifact-free Python guards/scenario tests pass. New guards connect the real
reflection completion and snapshot return to the water consumer, and require the
outgoing alias check. A namespace spelling expectation was updated after the
compiler correction, not removed or broadened. Host152/153 exposed two missing
outer namespace qualifiers in global hooks; corrected host154 and final155 link,
with codegen0 and no guest object compilation.

Run979 (before the final alias guard) passes strict cold/reload/mixed checks.
Its last primary reflection sample has3900 publications/3150 native reads; water
3166 submitted/3078 emitted/3164 fence-retired. This proves live producer-to-consumer
reachability, not unique assets or speedup. Final guard verification is recorded
in the cumulative ledger and the completion note below.

No shader/backend changes: GPU67/water14's20 two-eye water cases, rigid23's55 and
snapshot22's8 remain applicable program evidence, not new game acceptance. No
new raw/image capture. The tested field has not exercised authored shore/bottom
or refraction snapshots; their real scheduling scenario and game-pixel/HDR/stereo
coverage remain unqualified. Bump-image table selection, late material descriptors,
outgoing state/world/samplers, reflection camera/extent/scheduling and other families
remain adapters. No complete host-only frame or Quest readiness is claimed.

## Artifacts and storage

Final host155 EXE SHA256 `CB7C66A2A0BAA1648182FD0645BE985B495F1D75017E37E8310BFD795B18316F`;
PDB `11BF3C812C5B3F08ACA57B0CEE7CA1B0CF592B036329F6D9F6F3A8DE752C2E62`.
CPU65 EXE `40768B79E4E14A7DFAFA180266A0132BB6441EE5F8954EBA3718EB06739C2CA9`.
Run979536971B SHA256 `6067493B7543798D51A3087C548C937B0E71F4EBAB52DC2C974E45B28B781348`.
The exact owner profile and cumulative3GiB/diagnostic/log/raw/image limits are
unchanged. Cleanup and final accounting remain in
[`20260906_0333_native-scene-state-bridge.md`](20260906_0333_native-scene-state-bridge.md#native-water-frame-image-connection-2026-09-08-source42fd400-plus-edits).
GitHub push is blocked by security review pending explicit destination approval;
there is no workaround or automatic retry.

## Final guard-qualified run

980/PID35532/session59700,09:45:22..09:47:28 Eastern (126s), exit0. All22 settings
took effect,0 raws, exact116B owner profile restored. Strict cold/reload/movement/
receiver/light/caster/cutout/mixed-deferred checks pass. Fresh post-event reflection
windows1873..2173 and4273..4573 each add300 publications and300 native reads.
Last reflection3900/3171. Cold water1581..1881 adds300 submissions/retirements and
290 emissions; reload4259..4559 adds300 submissions/retirements and279 emissions.
Old generation94 has1657 packets retired before new generation208 submits1658.
Last water3158 submissions,3069 emissions,87 culls,3156 fence-retired, unavailable0.
The separate mixed rigid window4200..4500 consumes9540 native records, with9540
effect reads,6478 legacy draws and12956 material bridges; strict comparisons remain.
No authored snapshot/bottom scenario or fresh pixels was observed or qualified.
Run980538768B SHA256 `E997247279093E05F317DB1BB9B80746DEBD2FF03F59BE24C82306EDDE51D6A7`.

All producers terminal. Removed25 superseded agent log files across the bundle,
1627399B logical total, including old full976/978/979 text; results/hashes remain,
but deleted text is not retained in an archive. Current155/CPU65/output40/980 and
protected causal/unresolved evidence remain. Measured selected retained artifacts
shrank499465B (CPU+32810, EXE/PDB+6656, logs-7673, run/archive set-531258).
Ending free80110993408B (~74.61GiB),1417216B less than the first reading drive-wide;
that difference includes other filesystem activity and is not all task-attributed.
No budget expanded; no new raw/image payload or asset copy.
