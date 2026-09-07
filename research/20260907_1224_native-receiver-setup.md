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

## Diagnostic follow-up: host105, runs942/943

The build-only statement above describes the original checkpoint. Host105 was
subsequently reused without rebuilding. Run942/PID31128 exited through title-menu
Exit before any field observation. The source path permits mouse hover to change
the title cursor independently of autoplay's pad; the log does not establish
which pointer/input events caused this exit. The temporary automated profile now
adds bd_mouse_menu=false, with normal manual defaults and owner bytes preserved.
A regression rejects both an early guest-exit and one appended to otherwise
passing reload samples as terminal failures. All300 Python checks pass in0.123 s.

Run943/PID30948 ran12:45:40-12:47:42, all21 settings applied, no raw/perf capture
or image, exact owner profile restored. Both epochs independently pass the full
existing consumer/movement/hard-off/lifecycle gates plus receiver setup. Old
generation93/instance144 adds900 scene/shadow emissions after readiness
(755->1655); source retirement occurs at1656 submitted/emitted and1654 retired,
with all1656 fence-retired before title. Generation207/instance385 adds900
new emissions (767->1667). The newer receiver window adds74,613 native calls,
40,585 packets and300 reads; original/refused/missing remain zero.
Log486,641 B, SHA-256
E4807B454BDB1BCAAF3D811F131F35DC533EE4C411AF541F46A64477A37D3A08.

**943 is passing text evidence, not an explanation or fix for941.** No mismatch
occurred, so the new failure-context rows were not exercised. No new pixel or
sequence qualification is claimed. Keep941's unresolved failure and940's last
accepted image; do not replace either purpose with this successful text probe.

Additional complete-source reads: sub_82399B50(file20) and sub_823AB4C0(file92)
can invalidate visual+3136 and each232-byte per-node selection+4 while toggling
associated light participation. bdLightListUpdateSnapshot(file54) updates the
manager's snapshot/changed list and compatibility publication, but does not
itself write object dirty masks. None of these reads identifies941's actual
writer or proves overlapping execution. Offset3136 in sub_82736750 belongs to
a CRC string table, not this selection layout; it is not a relevant writer.

Next diagnostic must identify the writer of the current live selection's dirty
word and its ordering against selection, not assume941's address persists across
loads. The existing installed Windows debugger is available; no debugger was
launched, downloaded or queued here. A bounded dynamic write watch is a possible
next probe, not completed evidence. Preserve strict comparison and add a causal
boundary regression once the writer/ordering is established, then qualify the
complete receiver reload/pixel gate.

## Storage and retention

The original checkpoint's storage accounting follows. The diagnostic follow-up
shares the same cumulative ledger and limits. Its superseded startup-only942 log
was removed after regression/startup replacement:21,237 logical B,24,576 measured
B recovered. Keep943's486,641 B log for new two-epoch receiver text coverage,
replacing it on equivalent full acceptance. No protected evidence was removed;
the exact942 diagnostic is gone, with its findings/hash retained in the ledger.

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
