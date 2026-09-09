# Authored gaze to native instance-owned eye materials

2026-09-09. Source checkpoint58d591a; live binary host188, banner5c80becdirty.
Final review corrects floating-point-mode ordering separately; its fixture/build
evidence is recorded below and in the cumulative storage ledger. No restamped run.

## Connected outcome

Authored controls -> native eye UV calculation -> fixed NativeInstanceRegistry
entry -> existing object material inputs/composer -> native scene packets.
No additional source-address cache, dynamic eye allocation or renderer framework.
For eyes, the source-bound index holds only table/count association. Native values use
native instance identity/model generation and copied material selectors/channels.

The admitted non-verifying path no longer executes sub_822BA028's translated
floating-point body. The native material importer consumes the owned offsets,
not an imported copy of their exported source floats. Verification still runs
the original exactly once and compares all four output words bit-for-bit before
publishing. Refusal clears the old publication before retaining the original.

The four-float caller scratch export and subsequent table stores remain outgoing
adapters. At object setup the table/count, bindings, enables and output words are
checked against the native snapshot. A mismatch invalidates the owner until the
producer republishes; restoring old bytes cannot resurrect it. Untracked later
writers retain the explicit current-source material route. Other material/image
inputs are not claimed native-owned merely because eye UVs are connected.

## Reused and completed source contracts

The parser/binder contracts and bounded installed-content scan are retained in
[effect input coverage](20260908_2331_effect-input-coverage.md). No new asset copy,
decompression, game-data write or codegen/source rewrite was needed.

- sub_822BA028, generated77:9934..10052: first record base76/80, limits60/64/68/72;
  two controls at r4; four float outputs at r5. Horizontal signs differ between
  eyes, vertical is shared. Rounded subtraction then double FMA/float store;
  comparison constant0x82055230 is zero. No clamping or eye-index/mode/count test.
- sub_822B8DF0, generated63:10960..12928, fully audited: actor+8 visual,
  actor+6168 controls. Controller/root handling precedes the eye call; caller
  copies up to two152-byte records to+28/+32, using r7. Late layers, bones,
  actor/visual callbacks and attachments follow. Do not freeze parser values.
- sub_823CAF90, generated60:17166..17949, fully audited: visual=object+392;
  +3108 branch can override UV/rates; +3128 enables gaze from+3132/+3136;
  eye caller count is r6. Later visual vtable+40/+44 callbacks remain original.
- sub_823CE3F0, generated92:16854..18629, fully audited: viewer input route,
  button14/type1 guard, analog controls and optional persistent control writes;
  local r30 holds count. Later animation selection/time edits remain original.
- All three callers copy min(signed table count,2); negative/empty unsupported
  cases retain the original. Preserve r6/r7; no caller-specific register count.
  No existing eye/caller whole hook was found in src/config. Relevant unchanged
  frame_interp/pso_predictor TOMLs were read; no instruction hook/codegen edit.

## CPU and source verification

422 artifact-free Python source/scenario checks pass. They prove wiring only.
The existing native material C++ fixture tests asymmetric limits, mirrored gaze,
extrapolation, signed zero, nonfinite/overflow refusal, one/two/extra records,
missing/bounded source input, incomplete caller export, late UV writes, selector/
channel/enable changes, table replacement/count changes, generation/refusal/
reload retirement and immutable snapshots surviving source destruction.

The connected test forbids exported-eye UV reads inside the actual material
importer, then runs ComposeMaterialTextures. It checks ordered channel selection,
provenance preservation, later non-eye replacement and reset. The fixed eye data
shares existing entry accounting; retirement returns that residency to zero.
Material74/CPU71 pass; host188 builds without guest objects or shader compilation.
Final review moves ctx.fpscr.disableFlushMode() before ReadEyeControl/native math
and adds a denormal UV test plus a source ordering guard. This preserves the
original mode even when inherited host floating-point state differs; host188's
successful runtime did not qualify that edge case.

Final material75/PID19296 and CPU72/PID38160 pass (0.12s/CTest0.14s),422guards
pass0.226s. Host189/PID37628/session30178 builds successfully through link step12,
codegen0writes, no guest objects or shader compilation. Final EXE
49372672B SHA2562CB159E945C45BF3A93D9FDCAEBCEC9F4151A0CD0D7D540DDAD92803EE0F3BA1;
PDB113737728B. Fixture1491968B SHA256
F05904CD831F572AFBC27218FE40E43FF6DD74ED8C4A859B54D9B0F796230861.
Host189 has no new live run; do not relabel the host188 observation below.

## Host188 live observation

PID38012/session61987,2026-09-08 23:57:53..23:58:48. Capture-free mono,
native animation/skeleton/instances/material verification and native scene/skin/
deferred enabled. All14 temporary settings took effect. The unchanged116B owner
profile restored exactly, SHA256
2F1BC38D763A1B7BDBA31F560684FD4AA7E42A714600B8D344F19DA7F38E23B0.

The existing supervisor's new EyeMaterialProbe observes checked off-center
controls and increasing native eye-material packets after two idle bg41_01
contexts. The previous positive UVCON gate is not requested, changed or waived.
The requested observation succeeded; the wrapper intentionally exits1 through
its diagnostic-stop exception/finally, NOT a test-suite success exit or crash.

- Eye2077:976completed/976exact checks,refused0,off-center216. Earlier1476 had
  off-center0;1776 had65. This exercises nonzero input, not visual eye-motion proof.
- Owner1747->2047:published811->961,reads631->931,changed0.
- Idle field contexts1748->2048 precede material samples:scopes632->932,
  composed/packets27808->41008 (+13200 each),replay reads0. These packets use
  owned eye UV provenance; they are repeated primitive inputs, not unique eyes.
- Skin scene1750->2050:emitted/fence-retired33513->46713. This is separate
  whole-skin GPU evidence, not eye-specific emitted-draw/pixel identification.
- Controller2077:95738matching,15908boundary refusals,12159samples,23mixes,
  11650interior,12149handoffs,changed0,12635advancing clocks. Late6matching,
  reused6,handoffs6. Skeleton2047:12813matching,unavailable0,wrong0.
- Material2048:19051checks,wrong0;effect2077:980matching,1958UVs,zero
  translation/rotation/queued-transition coverage. Prior failures remain open.

No new raw/window/perf/cache/dump outputs. Full263502B log retained as
retained-eye188.zip54079B (sole member reblue_981.log; rotating filename reused).
Full member SHA2561D64BA1D1BA65EFE8B4A3391197E817EBA21593E8C2BB3CEF1481DE4A81F3F75;
ZIP SHA256F10E5244C5BE0663952AC3E85F54359E271952A9CA687637DFCD4F93ED4A26D2.
Full decompressed hash/name/length verified before removing plaintext. Keep this
native-eye consumer evidence until a purposeful replacement; effect187's missing
driver evidence, placement988/root985 and earlier pixel/reload failures remain.

## Remaining work

Move other animated UV/image writers and authored binding publication into the
same instance/material owner, then retire the final source/scratch checks and
exports. Source gameplay gaze inputs, parser/late-writer adapters, unknown routes,
source selection, special bones and full scene/frame migration remain. Do not
repeat unchanged eye/UVCON boots to collect more counters. Next visual work must
address the existing renderer-fence/window mismatch and storage gate.
Motion sequences, reloads, both eyes and full desktop scenes remain unqualified;
no full host frame, FPS gain or Quest2 completion is claimed. Defaults unchanged.

Storage:13 superseded receipt/plaintext files removed,276178Blogical; after the
verified ZIP allocation,233472B net228KiB reclaimed. The complete current log is
recoverable from its ZIP; old build receipts are reproducible, not in Git.
Partial retained growth207901B (fixture,EXE/PDB,ZIP,net build logs); it excludes
host objects/metadata/source/Git. Pre-documentation close63450087424B free
(~59.1GiB),138944512B less drive-wide than the first check. Scoped runtime
inventory accounts only for the54079BZIP; no live producers or new capture/cache
files explain the unrelated drive-wide drop. This is not claimed task cleanup.
See the [existing cumulative ledger](20260906_0333_native-scene-state-bridge.md#2026-09-08-authored-eye-ownermaterial-connection-after5c80bec).
