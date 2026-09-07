# Native rigid instance storage and indirect submission

2026-09-07 EDT, parent62a095f. The previous turn made verified progress with
the direct scene consumer. This checkpoint connects native instance batching
and indexed indirect submission to that same scene/caster route, not a new
renderer. Full host ownership and lifecycle acceptance remain incomplete.

## Implementation

Each queued native item owns a784-byte object+pass record, geometry, images,
samplers and exact pass context. Per-instance lights/fog stay per-instance;
they are not replaced by the final object's uniforms when draws merge.
`SV_InstanceID` selects the native structured record, and a flat fragment input
carries its identity to the pixel shader. The three native shaders no longer
use separate per-draw object/pass dynamic uniform bindings.

The existing queue's safe reorder runs may bring compatible records together.
At flush, the native branch admits only a consecutive compatible prefix of at
most256 items. Geometry lease, pipeline/layout, framebuffer, viewport/scissor,
images/samplers, render view and frame/slot ownership must match exactly. Hashes
may group candidates, but never establish compatibility. Non-native/ordered
draws stop the merge; hybrid native/translated record requests throw. The
translated gather, masked constant windows and plain-draw fallback are not used.

The shared CPU packing core preflights the complete batch before writing its
output. The existing host upload arena then receives those CPU records, never
reads back mapped upload memory, and also holds one20-byte indexed indirect
command. Structured offsets are elements while Vulkan alignment is bytes:
the placement core uses the stride/alignment least common multiple and bounded
headroom, without padding every record or allocating one GPU buffer per draw.
This handles non-power-of-two784-byte records in the power-of-two upload arena.
Native descriptor sets are built once per batch, not once per staged object.

The shared emitter records `drawIndexedIndirect`; counters separate represented
instances, indirect calls, merged instances and fence retirement. CPU items and
GPU descriptors remain retained in bounded frame slots until their fence, with
the existing8-program/4096-item/4096-batch limits and one256-record scratch area.
The arena's existing64 MiB allocation/256 MiB in-flight limit still applies;
INDIRECT usage is added to its existing vertex/index/storage/uniform capability.

The selected family remains opt-in, mono, and unchanged in scope. Object/pass
source adapters, authored light/cache/fog/camera producers and the original
receiver callback remain. Source-free loading, a hard-off guard covering all
entry paths, teardown/reload and live repeated-object batch coverage are not
established here. Other material families still use compatibility rendering.

## Verification and development loop

The devloop skill selected existing fixtures before the host/game run. No guest
investigation, code regeneration edit, new build tree, dependency download or
asset conversion was needed. All284 Python source/scenario checks pass in0.070 s.
New scenario tests require fresh native scene/shadow instance and indirect-call
deltas, reject stale/refused/impossible counters, and explicitly permit/report
zero merged instances rather than mislabelling singleton draws as merge coverage.

Output20/PID28180 and CPU5/PID28536 pass:0.34 s assertions/0.36 s CTest. The
pure batching fixture tests distinct transforms/colour/lights/fog, changed
resources/contexts, source retirement, new geometry leases, stale frame/slot,
ordered/null boundaries,256-item cap, transactional refusal and byte/element
placement across multiple storage alignments and nonzero arena offsets.

GPU26/PID15436 compiles all three changed native shaders and existing fixture
objects. Rigid05/PID30300 passes five8x8 two-eye cases on RTX3060,128 pixels per
case. The added case renders two differently transformed/lit/fogged instances
in one indirect command for each scene/caster pass. It uses nonzero structured
view and indirect-command offsets, a poisoned prefix, explicit albedo/shadow
2D-array views and D32/S8 sampled shadow depth. Both eyes' colour/depth and
the caster depth are read back and checked. Maximum error0.0000339895 overall,
0.00000584126 for the two-instance case.1.21 s fixture/1.22 s CTest; zero Vulkan
validation errors/warnings and one known missing GOG overlay loader diagnostic.
No raw/image files. The fixture uses production shaders/descriptor schema and
storage placement; CPU tests cover grouping/packing separately. It does not
execute the full production queue with a multi-object group.

Host96/PID28072 passes:98 scheduled host steps after up-to-date codegen,
0 generated bodies written and no guest objects rebuilt. The shared QueuedDraw
header changed, so its consumers rebuilt; this is not the cost of an ordinary
one-file edit. Known CRT deprecation warnings remain. No failed build/run retry.

Run937/PID19332,10:11:14-10:12:15, uses both direct switches from startup.
First native scene submission at frame788; the caster has already issued one
native indirect instance. Fresh post-event field windows1988/2288 add300 scene
instances/300 indirect calls and300 shadow instances/300 indirect calls, plus
300 scene submissions/fence retirements. **Merged-instance delta is0**: this
selected field target is a singleton, not evidence of a game draw-call reduction.
All prior field, movement, native image/material/light/fog/sampler gates pass.
No timing comparison or performance improvement is claimed.

Inspected1920x1080 JPEG shows running Shu by the fence, coherent terrain/props
and shadows, with the known cliff marks/distant blur. One image is sanity
evidence, not a stability sequence, isolated-object oracle, reload or game
both-eye qualification.17 settings took effect; the owner's116-byte profile
was restored exactly. No owned producer remains; new raw/perf/cache/cook/dump0.

| Artifact | SHA-256 |
| --- | --- |
| Host96 exe48,519,680 B | `FD82781CDEDA41E07F4ABD59DB7E7623B499ECD20CCE5B4518C25F8EB18F2DF0` |
| GPU fixture executable | `A8BA98B092A7E22A1660C9DF56912F7D74410BD226691DF7254AE5F6E7D92AFA` |
| `out/build/win-amd64-release/logs/reblue_937.log`,243,734 B | `B585F2E2A779BED19380F58354095C77D8CFA1666E98CA1D46D5AD86FEFF14B4` |
| `out/verification/native_rigid_batches_window.jpg`,143,601 B | `12E542F0F15684928FFEAD3B29DBD5644DEE51BF0EB7B7412F825C34344487D8` |
| Restored profile | `2F1BC38D763A1B7BDBA31F560684FD4AA7E42A714600B8D344F19DA7F38E23B0` |

## Storage and next gate

Same cumulative3 GiB exception/62,509,998,080 B floor/raw0 gate. Existing
build256 MiB free-drop/300 s/10 MiB aggregate-log limits and runtime192 MiB/
75 s/400 KiB log/160 KiB JPEG/10 MiB aggregate images remain enforced. Planned
host/fixture overlap under192 MiB; true peak not separately recorded. Existing
CPU/GPU/host trees, cooked asset and owner profile were reused.

After replacements passed, removed12 exact superseded agent files: output19/
CPU4, GPU25/rigid04, host95 stdout/stderr,936 text and its direct-scene JPEG.
393,067 logical B; free63,110,656,000 ->63,111,065,600 B: **409,600 B (400 KiB)
actually reclaimed**, credited once. Exact prior runtime text/image are gone;
hashes/findings remain, fixture logs can be regenerated. Keep937/image, host96,
output20/CPU5, GPU26/rigid05, unchanged distinct fixtures and all protected
baseline/flat/VR/movement/failure/raw evidence. No game data or build tree removed.

Known component growth520,492 B, for expanded fixtures, native batch code/PDB
and replacement evidence. Cleanup-end63,111,065,600 B/58.777 GiB free:4,456,448 B
drive-wide use from output preflight63,115,522,048 B. Other object/source/Git and
volume changes remain unattributed. The same scene-state ledger owns cumulative
accounting and the zero incoming raw allowance. Replace equivalent evidence
after validation rather than retaining every run.

Next remains the same complete object: enforce legacy-off ownership before its
first draw across entry paths, qualify cold load, movement, teardown/reload with
fresh generations, and remove remaining source producers at their last consumer.
Qualify repeated real-object batches as this path gains representative coverage.
Only then expand material families; the full character/effects/UI/desktop/both-eye
gate still precedes Quest 2 optimization.
