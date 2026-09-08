# Native skin caster verification and live admission frontier

2026-09-08 EDT. Source319586c plus this checkpoint. Full desktop native rendering
remains the goal; this verifies the new skin data/shader connection, not a complete
native character, frame, game scene, stereo sequence or Quest qualification.

## Changes and contracts

The guest-source/devloop skills reused the recovered joint-local skin contract,
model ownership frontier, hook definitions, existing build trees and bounded
supervisors. No generated guest code, dependency, asset or shader translator edits.

- Native shadow vertices use explicit model-local IDs and1/2/3 independent joint-
  local positions. Completed instance poses supply joint-to-world matrices; the
  shader must not reapply object world. GPU tests poison that unused world matrix.
- Per-joint indexed envelopes feed animated bounds. Added conservative padding
  for the asset validator's1e-5 weight-sum tolerance and weighted FP32 arithmetic;
  otherwise coincident joints far from the origin can escape an affine-only box.
- CPU ownership tests now cover exact16384-matrix capacity, shared-pose reuse at
  capacity, compatible-prefix splitting, oversized poses, invalid bounds and
  retained old pose/model owners after source replacement/retirement.
- The GPU fixture rebuild exposed an older water-publication aggregate initializer
  after a production field addition. Explicit field assignment fixes compilation;
  the water shader and pixel oracle are unchanged.
- Native caster admission now returns an exact refusal reason. A maximum of eight
  distinct skin model/node examples is logged, without changing eligibility or
  adding a fallback. This identifies the consumer dependency to migrate next.

## Verified artifacts

| Artifact | Result |
| --- | --- |
| Mesh17 / CPU15 | Passed; indexed skin data, deformation, animated envelopes and arithmetic margin |
| Output68 / CPU43 | Passed; palette capacity, deduplication, lifetime, batch prefix and strict admission |
| GPU69 / rigid24 |61cases passed:55 existing plus six skin cases; mono shadow depth and two-eye receivers |
| GPU69 / water15 / snapshot23 |20water and8snapshot cases passed |
| Host158 | Incremental build passed; codegen0written, no guest objects rebuilt |
| Python boundaries/scenarios |379passed; these are wiring/scenario checks, not pixels |

GPU: RTX3060, Vulkan validation0errors/0warnings. One unrelated missing GOG overlay
manifest loader message. Readback remained in memory; no raw/image files. Skin
cases exercise distinct local positions/transforms, nonzero structured-buffer
offsets, one/two instances, producer destruction and no second world transform.

SHA256 identities:

- Host158 EXE49037824B: `6F129E066A9FB66AC879699CCF52801BC2824B572650A1F471A54C6DE59EA863`
- Host158 PDB111685632B: `12A58347CCE50D629C6CEBC83150541CD8812D10B619EE9D5B9FE48B8937CDA8`
- Mesh17: `DACE015169FF70F1993AF44B26EBCC4141037376286B8FC4C3A5029EFBEA68D2`
- Output68: `404EDDEBB0B36CCDCEA7D0846B08A5834151187DDEF3A54ECC9888A79D824E88`
- GPU69: `C7CAF71B755E8054C7F892A7C2F84AAE30A37078603CF75B5922F28FD9CFA828`

## Live evidence changes the next bundle

All probes enabled native skin/shadow/instance data explicitly, disabled captures,
performance CSV and cooking, isolated mouse-menu input, and restored the exact
116-byte owner profile (SHA256
`2F1BC38D763A1B7BDBA31F560684FD4AA7E42A714600B8D344F19DA7F38E23B0`).

Host156/run981 reached reload but stopped after104s at the192MiB drive-free-drop
limit. It produced only365353B of game log; no native skin-shadow emission was
observed. This is neither a full strict-reload pass nor a rendering correctness
failure. Keep its log/hash and the prior full host155/run980 result.

Host157/run982 first established native skin geometry was present. Host158/run983
then observed the exact refusal in21s and stopped intentionally via the diagnostic
supervisor, not a successful field/reload verifier:

| Model generation / node | Primitives | Observed contract and refusal |
| --- | --- | --- |
|97 /47 |4 | technique0, phase1, mode0, no texture-effect routing; first primitive3influences,8joints, native geometry present; cutout sibling |
|97 /62 |6 | same ordinary pass; first primitive3influences,10joints, native geometry present; cutout sibling |
|38 /1 |44 | technique1 with texture-dependent participation; first primitive3influences,5joints, native geometry present; unconverted technique |

983log89700B SHA256
`4796FBE511641E26E0D429D9ED31E4051D0794C028C592523D6DE2D30C05F7CC`.
These are admission observations before ready-field qualification, not fresh
field emissions or character identification. They prove the opaque-only scope
does not cover the observed whole nodes. No additional live guest-call removal
or performance gain is claimed.

Next connect skin deformation to `PrepareNativeRigidShadowForObject` and the
existing fixed0.6-alpha cutout shader, owned base-image/UV/wrap-sampler inputs,
shared descriptors/queue/fences. Add the observed mixed-sibling regression before
retesting; do not simply ignore an alpha sibling or relax whole-node admission.
Technique1 additionally needs owned texture-class participation, not assumed
ordinary textures. Current phase1 source contract lives in
`native_material_texture_source.h`, `native_rigid_shadow.h` and the updated
`native_rigid_shadow_cutout_{vs,ps}.hlsl`; the historical1742 report's earlier
alpha formula was superseded by the later integration fixes.

Skinned scene shaders, full animation/secondary poses, remaining replay/register
consumers and every desktop scene/event/reload/animated-effect/both-eye gate stay
open. Quest work remains gated on the completed desktop frame.

## Storage and retention

Same cumulative3GiB exception,62509998080B operational floor,20GiB reserve and
diagnostic/log/raw/image limits; no new budget. The detailed cumulative ledger
is `20260906_0333_native-scene-state-bridge.md`.

Removed35 verified superseded test/probe logs (115608B). Losslessly archived958/
959's full historical text with SHA256-verified entries before retiring plaintext,
reducing1013088B to156174B. Archive
`retained-native-runtime-958-959.zip` SHA256
`EA1F252DE014B378907ABF30A355FBE89CEBD896BD14E553893E56C4140031DB`.
959's distinct multi-instance proof remains recoverable. Total logical cleanup
972522B, counted once; no asset/save/profile/raw/image loss.

Measured selected retained growth4435838B includes fixture trees, EXE/PDB, build
logs and current game evidence after archival reduction. Source/Git/other objects
are separate. Ending free79327395840B, down721645568B drive-wide from the initial
80049041408B; scoped game/NVIDIA-cache checks do not attribute most of that drop
to renderer outputs. No owned producer remains. Retain981/983, full980, current
fixtures and all protected visual/failure evidence; replace only after equivalent
verification. No new raw/image captures, downloads or persistent game asset cooks.
