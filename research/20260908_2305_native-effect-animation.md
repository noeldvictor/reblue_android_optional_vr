# Native controller -> effect UV -> material consumer

2026-09-08. Implementation196d85d; host186 built from b9f54bb plus exactly that
implementation. This is a connected opt-in path, not full-frame completion.

## Delivered boundary

`Controller` in `native_animation_bridge.cpp` now prepares the entire effect
transaction from `ExecuteController`'s native channels before original execution
or source writes. It does not consume the skeleton's one-shot handoff or create
a source-address cache. Pending cue loads/malformed inputs refuse the complete
controller before mutation. Collision remains once in its original order.
Verification executes the full original once, compares every timeline word and
all planned UV results, then publishes native outputs. Normal execution publishes
after collision and before the ordinary object-UV tail, without bdEffectUpdate.

`native_effect_animation.h` owns clock decisions and all three UV motion modes.
`native_effect_animation_source.h` is a checked, transient input/output adapter,
not a persistent timeline/asset owner. UVs reach the existing
`ReadMaterialTextureInputs` / `ComposeMaterialTextures` owner and renderer path.
Source timeline/catalog reads, per-object152B inputs, outgoing UV/channel storage,
ordinary UV-tail inputs and other original effect callers still need retirement.
No default enable, new GPU shader, asset recook or Quest work.

## Reusable source contracts

Complete relevant bodies were read; generated files/hook configuration unchanged.

- `bdEffectUpdate`0x82144260, generated27:2652..3066: controller calls it only
  when visual+3560 is nonnull. Separate AnimeData cue state occupies+2212..2244,
  not the six skeletal56B slots at+1872..2208. Delta is float0x82DDA880.
  Nonzero first cue advances time+2224 by float-rounded double FMA with rate+2228.
  Without a queued first ID, clamp or subtract maximum duration ONCE according
  to loop+2220; below zero clamps. A queued positive-rate crossing is strict `>`;
  negative-rate crossing is `<0`; zero rate never transitions. Resolve two
  queued IDs in order. Equal entry pairs retain old IDs, queue and advanced time.
  Changed pairs copy queued IDs/entries, reset time0/rate1, preserve loop bits,
  and clear both queued IDs. No active cue means dormant clock is untouched.
- Same body's UV tail: visual+3560 records, signed+3564 count, stride152.
  Record+20 enables UV; +28/+32 offsets advance from +36/+40 rates and delta.
  Byte+120, nonnull visual+2628 and signed nonnegative record+12 select a dense
  joint. Record+16 nonzero selects rotation, otherwise translation. Drivers
  REPLACE scrolling and read dormant payload regardless of channel activation.
  Translation x/y divide by+44/+48. Rotation forward yaw/pitch divide by
  float(+52/+56 * float0x8208EA64). Native admission bounds joints and shares
  the existing material owner's256-record bound; aliased visual/channel tables
  refuse. Nonpositive signed counts are no-op. Only planned output words write.
- `bdVisualObjectGetMaxDrawDistance`0x82141410, generated56:2653: despite the
  historical name, this caller uses durations. Entries at+2232/+2236 point via
  +16 to AnimeData whose+312 float is truncated through signed32 and rounded
  back to float. NaN/out-of-range negative conversion yields INT_MIN; positive
  overflow clamps INT_MAX before float conversion. Null entry contributes0.
  `config/hooks/render_tweaks.toml`/`bdDrawDistanceScaleHook`0x8214148C scales
  BOTH double candidates with bd_effect_distance before max. Behavior retained;
  this audit does not validate older spatial-culling descriptions of that cvar.
- `AnimeData_method_BC48`0x8218BC48, generated13:4328: separate catalog+16 head,
  entry+8 ID/+4 next/+16 AnimeData. First match wins; poll result signed>=6
  returns entry, otherwise null. `AnimeData_PollLoadState`0x82150BB0,
  generated44:3071..3710: only states1..4 execute asynchronous/dependency/setup
  side effects. Other states return unchanged. Native ready lookup never polls;
  pending1..4 refuses, stable signed>=6 is ready, other stable states are absent.
  No later duplicate may replace a pending/absent first match; traversal capped4096.
- `AnimeData_method_1338`0x82141338, generated72:2662: request writer for the
  separate timeline. Negative mode resolves first ID once and uses it twice;
  nonnegative mode resolves both. Unless forced, equal entry pair is no-op.
  Otherwise publishes IDs/entries/loop, resets time0/rate1 and clears queue.
  `bdEventSceneSelectAnimation`0x821E6B30, generated65:6415, is a gameplay caller;
  scene type/actor state chooses cue IDs. Neither gameplay nor loaders rewritten.
- Rotation: complete `sub_824927D0` generated43:21990 produces quaternion matrix;
  existing native `JointRotation` uses that row-vector, nonnormalizing convention.
  `bdVec3TransformByMatrix` generated63:9230 transforms forward(0,0,1).
  `bdVec3RotateByMatrix`0x8214CD60, generated85:2859, calls `sub_822A3D70`
  generated33:10228: pitch=-atan2(y,sqrt(float(z*z+float(x*x)))), yaw=atan2(x,z),
  with explicit all-zero pair results0. `sub_826C1B10`, generated80:34061,
  is the quadrant-correct atan2 approximation; native uses std::atan2 with the
  existing1e-4 relative/absolute UV comparison. Rotation parity remains a LIVE
  missing gate, not proven by scalar test angles alone.
- Existing material consumer `native_material_texture_source.h` copies
  selector+4/channel+8/UV+28,+32 into owned overrides at object setup, not per draw.
  `ComposeMaterialTextures` preserves first-match/channel-pair/image ordering.
- Navigation for next authored-input decision: `bdMdlTextFileParse`, generated5
  starts10129; around11378 it allocates visual+3560 as count*152 and copies parsed
  records from stack+4384. That parser has NOT been fully audited here. Trace its
  driver-byte/joint/divisor inputs before selecting a new authored scenario.
  `bdUvAnimInit`0x8216EF60 (generated10:3578) is a separate UI character-UV cache
  initializer, not evidence for these per-visual driver flags.

## Verification and failed coverage gate

420 Python source/scenario guards pass0.244s. Material72/CPU69 pass0.12s
(CTest0.13s). Fixture covers actual imported clip -> native controller -> effect
UVs -> existing material importer/composer, then source/clip destruction; all
three modes, dormant fields, yaw quadrants/pitch, exact untouched output words,
pending/terminal/cyclic catalogs, clocks/queues/negative counts/bounds. CPU68
correctly rejected a new fixture's missing second active cue duration; explicit
negative assertion added before supplying that input. No production relaxation.
Host186 passes; codegen0writes, no guest objects/shaders. Exact hashes/storage
are in the cumulative ledger's controller-to-effect section.

Fresh runtime:2026-09-08 23:03:45..23:04:46,PID35436/session9932. Logger reused
`reblue_981.log`; identify by this time/PID/host186, NOT historical run981.
60s/400KiB/192MiB supervisor, all11 profile settings applied, no captures/perf/
cache/dumps. Original116B profile restored exactly. Exit1 is the requested
channel-consumer observation NOT reached, not a crash or passing integration.

Atframe2335:1111 matching effect transactions,2220 UV updates, translated0,
rotated0,transitions0. Fresh FieldActive/event0 contexts1709/2009/2309 precede
effect samples1735/2035/2335:811/961/1111 transactions,1620/1920/2220 UVs.
This proves post-event admitted scrolling output, NOT native joint-driver or
active cue-transition exercise. The strict probe required positive native drivers
as well as256 matching transactions and unchanged controller/late/selection
regressions; it FAILS coverage. No unchanged retry or threshold reduction.

Regression sample2335:108314 matching controllers,18004 unclassified admission
refusals,13731 samples,23mixes,13156interior,13721handoffs,changed0,14533advancing.
Late6 matching/reused/handoffs; selection1399 matching,27restarts,1245ready,
154absent,2unclassified refusals. Skeleton2308:14409 matching,wrong0/unavailable0.
Material2309:1232 override publications,246323 checks/wrong0,184513 UV blocks.
Those material aggregates are not one-to-one provenance for each effect output.
No movement/reload/pixel/both-eye/Quest or speedup qualification is inferred.

Complete273696B log SHA256
B3CBB2E21667EB4C0F086F95BBB9FBB1072DC6AC885F4477F910A0A66534B12E retained
losslessly as `retained-effect186.zip`49644B, ZIP SHA256
BECDA36519081446C5060D201C0C4C926D07B7F4511AA30C50C6E12262DBF7A1.
Single member/name/size/full decompressed hash verified before plaintext removal.
This preserves the new missing-driver gate and supersedes selection185's
selection/controller/late purpose; its older ZIP is retired. Placement988 and
root985 ZIPs and all other unresolved failure/pixel evidence remain protected.

Next: distinguish absent authored drivers from controller admission refusal via
the parser/driver-enable boundary before any new probe. Keep the native material
and controller owners; do not invent a second UV cache or force fake live inputs.
Other rendering producer/consumer migration can proceed without calling this
unexercised behavior qualified. Full desktop scene/event/reload/both-eye gates
and all modern Vulkan requirements remain unchanged; Quest stays gated.
