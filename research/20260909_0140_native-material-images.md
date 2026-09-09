# Native animated material image transactions

2026-09-09. Production source **f28789f**, host193/PID37784; final extra CPU
regressions are test-only. This extends the existing native instance/material
owners, not a second renderer or source-address cache. Full host desktop frames
and the subsequent Quest gate remain unfinished.

## Removed dependency and remaining boundary

For admitted calls, the native image transaction replaces `sub_821444E0` and
its image-selection/vector helpers. Binding-owned material selectors, channels,
image enables and animation-kind IDs feed selection. Immutable native texture
leases enter the existing instance registry, then the real material importer,
ordered composer and primitive inputs. That consumer no longer imports table
image pointers or recaptures their guest texture wrappers. Verification mode
still calls the original once to compare exact outgoing writes; this run used
verification, not a guest-free performance configuration.

The update still reads source animation catalogs, key windows and source cache
state. It exports the cache/table writes for remaining original consumers.
Procedural selected-key callbacks and scratch allocation retain the whole
original before mutation. Existing late-write comparison stays enabled. Viewer
overrides, the separate late-image vector, special image callbacks, load-time
binding/name resolution and eventual source-export retirement are not completed.

Image leases share the unchanged **16MiB** instance ownership budget with poses,
UV descriptors and UV values. Pinned replacement/retirement remains charged.
The existing source index's fixed exact image-pointer comparison association is
accounted conservatively by increasing per-entry overhead from512 to2048B; a
compile-time assertion covers its size. There is no new index. Texture/GPU
residency and fence retirement continue through the existing owners.

## Source contract

Read the complete translated bodies of `sub_821444E0`
(`generated/reblue_recomp.80.cpp:2623`), `sub_82151E10` (`.66.cpp:3080`),
`sub_82154938` (`.78.cpp:3080`), `sub_821549F0` (`.104.cpp:3026`) and
`sub_8215A200` (`.81.cpp:3137`). The sole direct caller is `bdCameraRender`
(`.8.cpp:2757..4592`), which selects images before beginning its ordinary pass
and traversing native material consumers; its following code does not consume
the helper's volatile register results. Full caller control flow was read,
with its relevant frame/output/render-tweak TOMLs. No instruction hook or
generated code was changed. Reused the previously audited152B parser/binder
contract; the selected procedural callback's internals are not ported here.

- Table at visual+3560/count3564, stride152. Nonzero+24 enables image selection;
  +0 is a signed animation kind. Negative skips catalog selection. Nonnegative
  kinds select visual+2212 for0, +2216 otherwise, but catalog+12 must still match
  the **exact** kind. First matching catalog+8/+12 wins; owner is+16.
- Owner state+316 must equal6 to select keys. Other states clear its scratch
  vector at+320 without replacing the material's prior image.
- Key-pointer vector+80/+84: start+60, duration+64, end+68, repeat count+124,
  loop flag+128. The original compares integer-truncated visual+2224 converted
  back to float. Inclusive intervals and zero-duration hold are preserved.
  Repeat uses integer elapsed/period semantics. Last active key wins.
- **No active key keeps the preceding scratch selection.** It is not a null
  replacement. Selected+212 invokes procedural side effects and refuses native
  admission. Capacity+332 must allow the existing scratch+324/+328 vector to
  hold one pointer; allocation remains an explicit original route.
- Selected+260 -> +4 -> +24 supplies the image. A null final image leaves the
  previous table+84 intact. Native material ordering still gives UV early-exit
  its original priority and distinguishes Keep from unavailable nonnull images.
- Preparation is read-only, capped at256 slots,4096 catalog/window entries,
  262144 aggregate checked reads and768 outgoing words. Its staged reads see
  preceding slot writes. Malformed/trapping/nonfinite inputs refuse before
  mutation. Source pointers are kept only in temporary exports/comparison
  associations, never in the owned image publication.

## Verification

425 artifact-free guards PASS0.227s. C++ material79/PID25432 and CPU76/PID32528
pass. Final alternate-kind/null-key/table-replacement/rebind tests:
material80/PID36192 build0; CPU77/PID32332 PASS0.11s/CTest0.13s.
Fixture1641472B SHA256
`3CFC2FA8289EACBBBCB84C338352A9DBFCC9296847EE1FFF24E8DF836A0BF864`.
The connected CPU path rejects descriptor imports in the producer and image
pointer/descriptor reads or guest recapture inside the actual material consumer.
It covers repeat/hold/endpoints/order, null keys, late writes/no resurrection,
wrong generation/reload, unknown images, source retirement and pinned budgets.
Opaque CPU test leases are not GPU/pixel evidence.

Host193/PID37240/session10198 builds0 through link39. Codegen0writes; no guest
objects or shaders rebuilt. EXE49421824B/PDB114126848B, EXE SHA256
`93616DCD126AF3D38634C0591918A348F880FECAB1DCD01C6EEE695170C0A4A6`.
Binary banner4625fb9dirty; production contents correspond to f28789f. No rebuild
or runtime restamp for the subsequent test-only additions.

Run PID37784/session60080,01:36:13..01:37:08, existing capture-free supervisor:
`-NativeImages -ModelMaterialsVerify -ModelGeometryVerify -InstanceVerify
-AnimationProbe -ControllerAnimationProbe -LateAnimationProbe -EyeMaterialProbe
-AnimatedUVProbe -MaterialProgramProbe`. Purpose: observe whether the new image
producer executes and supplies actual replacements while preserving prior
regressions. All14 settings applied. No new raw/window/perf/cache/dump output.
Original116B profile restored exactly, SHA256
`2F1BC38D763A1B7BDBA31F560684FD4AA7E42A714600B8D344F19DA7F38E23B0`.
Wrapper's intentional diagnostic-stop exit1 means requested old regression
observations reached, not a crash or full qualification.

Fresh idle bg41_01/event0 contexts **1693 ->1993**:

- Image updates/exact checks/publications2034->2934, selected keys2280->4080,
  held40->40. Image scopes601->901, composed/primitive packets26444->39644:
  **+13200 actual owned-image material inputs**. Not unique draws or pixels.
- Preparation refusals3->3, not publication loss (updates equal publications).
  The run does not classify those three earlier refusals as allocation,
  procedural or another preflight cause. Preserve that limitation; do not
  infer all image routes are converted or silently lower admission thresholds.
- UV/eye scopes632->932, packets27808->41008 (overlap with image inputs, not
  additive throughput). Fresh2019:964exact eye checks,223off-center;1932UV
  publications/refused0,1928owned scroll inputs;1934effect-slot/964eye descriptor
  evaluations. Binding5publications/4769reads,changed0/refused0.
- Controller94586matching/12005handoffs/changed0,23mixes,11514interior;
  late6matching/reused/handoffs. Skeleton12585matching,unavailable0/wrong0.
  Material18954checks/wrong0. Skin emitted/fence-retired33513->46713 is a
  separate whole-skin counter, not specific image proof.

No motion-pixel, reload, stereo, whole-frame, timing/FPS or Quest claim. Live
joint-driven UV/cue/non-eye-preservation gaps from preceding reports remain.
The observation supports migrating ready image key/catalog ownership next,
not another unchanged opening-scene probe; retain procedural/viewer/late-image
work and every full desktop scene/both-eye requirement in the queue.

## Evidence retention and storage

Current full255709Blog is losslessly retained as sole `reblue_981.log` member of
`out/build/win-amd64-release/logs/retained-material-images193.zip`,52102B.
ZIP SHA256`E897963F05666F0B04A4C0C9EF3E8876FD1DCDEB73526D05803D383392E3F206`;
member SHA256`F600CA3AA5A850301E1417DC74FFD74EBCA14E2867B9506FBE9FB60960DB831D`.
Name/length/full decompressed hash verified before removing plaintext and the
superseded material-program192ZIP. Old192log is no longer retained; its report
remains. New193 covers its verified program/UV/eye purpose plus image admission
and consumption. Keep effect187/placement988/root985 and other unresolved
pixel/reload evidence. Keep material80/CPU77/host193 receipts; all jobs terminal.

Same cumulative ledger/floor and exceptions, no reset.12 superseded files,
315353Blogical removed;327680B measured deletion gains minus53248B ZIP
allocation = **274432B net268KiB cleanup**. Partial retained growth435605B:
fixture tree+214096, EXE/PDB+216576, ZIP replacement+233, buildlogs+4700.
Excludes host objects/metadata, source/Git and unrelated drive activity.
Post-cleanup free63549190144B (~59.2GiB),4358144B below first63553548288B;
drive-wide change is not all attributable to this task. Diagnostics78151423B,
buildlogs424674B/212files. Next400KiB log overlap leaves82177B below75MiB;
fresh preflight required.13window images10434657B unchanged; no new raw allowance.
