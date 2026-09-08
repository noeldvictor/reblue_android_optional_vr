# Selected motion residency and remaining cubic coverage

2026-09-08, after af0020d. Opt-in desktop host170; no default changes or Quest work.

## Failure and connected change

Run977 used host169's owned type3 curves with an explicit repeated-cubic probe:
at least256 bound cubic calls, every sampled call strictly compared against the
original, timeout60s, log400KiB, captures/perf/cooking disabled. It failed before
testing cubic math: the first124 eagerly loaded clips consumed8388240B of the
8MiB motion cap; selected calls were unavailable and sampled/checked remained0.
Do not interpret this as curve parity or retry unchanged load order.

`NativeAnimationResidency` now uses its existing loader/source index as a bounded
catalog. Completed standalone/packed initialization registers backing lifetime;
the selected visual slot prepares immutable named curves before entering the
sampler. Dormant entries cost conservative256B metadata, not a decoded clip each.
Catalog metadata, resident curves and retired-but-pinned assets share the unchanged
8MiB/4096-entry budget. Shared aliases retain one payload. Under pressure, only
unleased payloads unused for more than two frames may evict; registration remains
until loader retirement. Failed imports retry only with more available budget or
new source generation, preventing a steady-state failed decode/allocation loop.

The sampler itself only finds an owned asset. No first-draw discovery, per-sample
key decode, disk cache, duplicate model/pose owner or raised memory allowance was
introduced. Dormant registrations still depend on original backing lifetime:
this is a temporary streaming boundary, not persistent source-free asset cooking.
Slots, source clock/layer logic and outgoing48-byte channels remain explicit.

Native channels continue through the existing skeleton/instance owners. Whole
reset and weight1/full-root preserve calls retain their original eligibility and
strict comparison, including exact inactive bytes/flags and unchanged active-float
tolerance. No check was disabled to reach a native call.

## Source provenance and regression

Reuse the [loaded-motion loader/retirement audit](20260908_1653_loaded-animation-sampling.md)
and the compressed contract in the cumulative scene-state ledger after af0020d.
Read guest-source/devloop, disk policy, active queue and full ignored operators.
Read complete `bdVisualObjectAnimSlotUpdate`, generated file89:2624, and selection
writer `bdVisualObjectSetAnimation`, file86:2636. The writer stores a ready lookup
entry at visual+1920+slot*56; update reads its initialized motion pointer at+12.
The hook captures visual/slot before original register mutation and prepares that
source under the existing loader registration/mutex. No new hook/codegen site.

Material53/CPU51 PASS0.12s (CTest0.13s): fill a dormant catalog before selecting
the working set, exact cap/count boundaries, pressure refusal before allocation,
recent-use and alias pins, dormant eviction, source destruction, zero repeated
imports across106 slot ticks, retirement/reuse, and memoized oversized refusal.
Existing all65,536 compact-float encodings, asymmetric Hermite tangents, several
rates, angular short/long segment rules and channel/pose consumer tests still pass.
406 artifact-free source/scenario checks PASS0.227s. Host170 builds without guest
objects/shader compilation; codegen reports zero writes.

## Runtime scope

Run978, same strict capture-free probe on the changed residency code, completes
its60s bound with **insufficient cubic coverage**, not a scenario PASS. At frame1931:

- sampled12989, checked12989, wrong0; whole27, preserved12962, cubic1;
- loads14, load refusals0, registered1070/catalog1067;
- resident9/581952B including catalog metadata; reported peak942672B;
- unavailable462, prepare refusals2. Those two import failures are not classified.

This supplies sustained native channel-comparison evidence and observed whole/
preserve reachability, not repeated nonconstant cubic motion. A single cubic call
does not prove clock/interior-key precision. Catalog/resident counts fall during
the run, but do not by themselves prove every loader retirement identity/path.
No scoped full field/reload/skin rendering, motion pixels, both eyes, measured
speedup or completed host frame is claimed. Prior975 reload and971/962 pixel
failures remain open. The old PrintWindow recipe is not being retried unchanged.

Next own weighted/subtree/layer application through the current native assets/
channels, classify the two selected-clip refusals, then verify repeated advancing
cubic motion. Keep the256-cubic observation and strict comparisons. Dense types0/1,
slot clocks, late writers/special bones, persistent cooking, outgoing channels/
palettes and the full desktop acceptance matrix remain before Quest work.

## Artifacts and storage

Host170 EXE49209856B SHA256
`1A83BCA475A858650107D94FBC20D4C1B9A589AFBF79527E51C33979E0D62E1B`.
Material53 EXE949760B SHA256
`C0C0F5032A929F9B7C8C172C674C0F2D4BFBA369EA0B7395F9DCBE9535151323`.
Run977 log217191B SHA256
`46F49374373F051E9135C95F5445E927D00475264DD8ED7308EED765A93F26AF`.
Run978 log218900B SHA256
`3C527E64EA163AC9DEC941E23C76E3F9F4DD3EFAA8BE8699628E31DF86389CFF`.

Both game PIDs and all build/test handles are terminal. Owner profile restored
byte-for-byte after each run; no new raw/image/perf/cache output. Keep977 for the
causal starvation failure and978 for current comparison/insufficient cubic
coverage until a relevant qualified replacement exists. Removed15 superseded
agent build/test/runtime text logs,50633B logical,61440B observed reclaim. Run978
replaces976's initial type2 admission, so976 and successful host168 build logs
were retired after hash/receipt review; prior report/hash remains. Current53/51,
host170, host169/run977 and earlier48/46 angular failure remain. No game data or
raw evidence deleted. Build-log aggregate415613B/208; material tree9561657B/43.
Pre-commit free63755919360B (59.38GiB), net drive gain679936B since this turn's
first snapshot. Known retained diagnostic change+390149B; selected EXE/PDB growth
+46080B separately. Those do not enumerate all object/source/Git/system writes;
drive-wide gains are not additional cleanup credit. Exact run guards and
drive/retention reconciliation remain in the cumulative scene-state ledger;
no new budget or exception. Push remains blocked pending specific upload approval.
