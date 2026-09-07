# Host receiver setup and retained direct-scene input

2026-09-07 EDT. This checkpoint replaces one original rendering callback and
joins its image/camera/late-colour output at the existing native scene consumer.
It does not independently load authored shadow/light data or remove all source
adapters. Quest, both-eye game acceptance and performance claims remain gated.

## Ownership and source contract

Read generated sub_82176708 (file99), bdShaderConstantFlush (file38), the existing
texture-binding hook and host parameter importer. The receiver is indirectly
invoked; there are no direct generated callers. This is not a claim that all
indirect registration or authored projection constructors were converted.

The host callback preserves technique14's early return and phases0/3/5/6,
preflights its temporary source/stack/descriptor boundary, binds compatibility
texture6, flushes through the existing native parameter importer, then reads
the four live colour floats. RGB/dirty/revision/alpha publication keeps the
original write order for unconverted draws. The colour read must remain after
the flush: parameter source/destination aliasing can change it. Native
publication uses that local copy, not the compatibility staging cache.

The new descriptor preflight uses the same complete importer checks without
writes/counters. The explicit flush adapter cannot call a guest original or
enter comparison mode; unsupported/disabled native publication visibly refuses.
bd_native_shadow_receiver defaults true. Explicit false retains the old
callback for diagnostics, but is refused with bd_native_rigid_hard_off. Native
receiver setup requires bd_host_parameters enabled and its original-comparison
switch disabled; there is no silent fallback after binding.

NativeReceiverPublication retains one NativePrimaryReceiver: a shared native
depth image, world-to-shadow matrix and colour. Publication checks that the
temporary texture wrapper names the completed native primary image exactly.
The direct consumer reads this packet with frame/view/object freshness; it no
longer independently resolves current shadow state or source memory. Copies
retain their values/image after replacement/reset. Visual IDs remain temporary
producer lookup identities, not persistent native asset keys.

Authored projection/colour storage, per-node light/shadow participation inputs,
legacy parameter/colour publication for other draws and other receiver families
remain explicit conversion work. No shader/program/layout change here. Mono
inputs are not per-eye camera production or fully source-free scene ownership.

## Verification

299 artifact-free Python guards/scenario checks pass. The optional
--receiver-setup gate requires fresh native setup/publication/consumer reads,
zero original/refused/missing counters, valid count relationships and recent
post-event field windows. With --rigid-reload it runs independently in both
epochs, in addition to every existing native consumer/movement/lifecycle gate.
Fixtures reject publisher-only, stale, wrong-scene, empty and failing epochs.

Output27/PID29384 + CPU11/PID31436 pass,0.38 s CTest: early-exit/refusal order,
late writes, local-copy publication, exact stamps, finite payloads, replacement
and retained image lifetime. CPU10 first exposed an older scene-plan fixture's
uninitialized disabled-fog payload and timed out on its assertion; fog{} fixes
the fixture without relaxing production admission. Output26 compiled before
that correction. The final fixture passes the existing scene-plan checks too.

Host104/PID29348 passes:13 host objects and link; codegen0 written/up to date,
no guest objects. Binary48,558,592 B, SHA-256
51E715A16727ACEC27AF74D00D3A8E193F105EB9F19B9DE7D53D8472690F5E5F;
PDB108,822,528 B; built from aa00b09 plus this source diff, watermark aa00b09
dirty. No restamp build for later docs/tests. Unchanged GPU26/rigid05 remains
the native shader fixture evidence; no new GPU-fixture result is claimed.

Run941/PID23416/session68217 ran12:22:17–12:24:10 with the extended existing
bounded wrapper. It **failed**, and no image was accepted. The20 effective
settings include native receiver and both native rigid routes/hard-off/reload;
raw capture/perf CSV are disabled. Exact owner profile restored, process terminal.
Cold generation93/instance144 added900 ready-field native emissions per consumer
(787->1687). Source retired at1688 submitted/emitted with1686 fence-retired;
all1688 retired before title. Generation207 loaded afterward, but its field
epoch did not complete acceptance.

The last receiver report (frame4115) has625,951 host invocations including
294,234 ignored phases/techniques,331,717 compatibility publications/owned
packets and2,588 native reads; original/refused/missing are all zero. These
observations establish live use, not a passing two-epoch or pixel qualification.
At12:24:09.785 the existing light-selection comparison failed:
selection237FA614/view0, address237FA618, actualFFFFFFFF, expectedFFFFFFFE.
Log430,499 B, SHA-256
F177A5A1AF65403CBAE585E18474BBD86997BA74614E376A12832947751B4C36.

Full selector sub_8218A8C8(file12), rebuild sub_8218A808(file85), insertion
sub_82189F40(file58), classification sub_82189E70(file44), priority getter
sub_82184A38(file34), and invalidation writer sub_8218A998(file1) were inspected.
Rebuild clears its view bit before candidate insertion. The invalidation writer
can set all bits in visual+3136 and per-node selections; other dirty writers
also exist. A late invalidation is a hypothesis, **not an established cause**:
941 lacks before/after producer observations, and attribution to the receiver
change is unproven. Do not ignore the bit, disable comparison or repeatedly run
until a pass occurs.

The production exact-write comparator is now reused by a16-view C++ regression:
full dirty mask -> cleared current bit; exact result matches; resetting dirty
afterward must mismatch even with identical IDs; missing output also refuses.
A Python regression rejects941's exact error after otherwise passing samples.
Failure-only instrumentation adds one context header and five before/expected/
observed write rows, bounded within the existing diagnostic limit. It remains
observational, not an atomic snapshot or a fix.

Material31/PID31080 + CPU29/PID28688 pass (0.13 s CTest). Host105/PID17772
builds that comparison instrumentation with one host object/link, no guest
objects. Latest built binary48,564,736 B, SHA-256
E535D6E8E7DE126A5457E2F829AA648A53C1C73F6A3488C907E1D253151FDD80,
PDB108,843,008 B. **Host105 was not run**; host104 is the failed live binary.
Host103/run940 remains the last accepted live/pixel checkpoint. No second game
run or new capture was launched. Next: establish the failed producer/consumer
ordering with this bounded context, fix the actual cause, then rerun the entire
two-epoch/receiver/pixel gate before advancing per-eye inputs/material families.

## Storage and retention

The original cumulative scene-state ledger owns the same3 GiB exception,
62,509,998,080 B floor and zero incoming raw allowance. Fixture/build preflight
63,202,775,040 B free; post-tests63,200,669,696 B. Removed eight replaced
output25/26 and CPU9/10 stdout/stderr logs:2,870 logical B,
63,200,071,680 ->63,200,079,872 B,8,192 B recovered once. Exact failed CPU10
log is gone; assertion/cause/correction are retained here and in the ledger.
These diagnostic logs are reproducible; no game data/profile/raw/build tree
was deleted. Runtime preflight63,047,393,280 B; the intervening volume change
is unattributed, not new renderer evidence or a cleanup credit.

Audit after941 finds zero new raw, cache, HLSL dump or perf-CSV files. Keep941's
small log for the unresolved mismatch; retain940/image as last accepted live
evidence. No existing live image/log may be retired on941's partial evidence.
All historical raw/baseline/VR/movement/failure evidence stays protected.
Final cleanup removed four superseded material30/CPU28 stdout/stderr logs,
2,203 logical B:63,175,114,752 ->63,175,118,848 B,4,096 B recovered. Total this
continuation:12 superseded reproducible logs,12,288 B measured recovery; no
protected evidence/data removed. Ending58.836 GiB free,26.375 MiB drive-wide
use versus output preflight. Known retained net growth661,444 B (0.631 MiB)
is fixture/binary/debug-info growth and941's unresolved failure log, not all
drive activity. The cumulative ledger records component sizes/retention.
No producer remains; owner profile is exact. Source/fixtures/report published
in6de0c2f; no live acceptance or performance claim accompanies that commit.
