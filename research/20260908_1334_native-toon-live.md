# Native Toon scene consumption passes reload; window pixels fail acceptance

2026-09-08. Source checkpoint `3ba183e`, using the already-built host164 EXE
without rebuilding or changing rendering code. This follows the authored Toon
producer connection, not a new renderer implementation or performance test.

## Live result

Run971, PID37324/session17745, 13:29:05..13:31:38, supervisor exit0. All24 profile
settings took effect: native skin scene/shadow, rigid scene/shadow/deferred,
hard-off routing, native material/texture/light/receiver paths and reload enabled.
Normal desktop mono/MSAA; mouse-menu override off, autoplay on, raw captures and
perf CSV off. The complete strict mixed-consumer chain was retained, including
separate `--skin-scene` and `--skin-shadow` requirements in both reload epochs.

| Fresh field window | Skin scene emitted / fence-retired | Skin shadow emitted / fence-retired |
| --- | --- | --- |
| Cold 1428..1728 | +13,200 / +13,200 | +13,200 / +13,200 |
| Reloaded 3964..4264 | +13,200 / +13,200 | +13,200 / +13,200 |

Old model generation93/instance144 fully retires before new207/480. Both epochs
also pass the900-emission selected scene/shadow regression and receiver, owned
lighting, wider caster/cutout, geometry/material and delayed mixed-consumer checks.
The sampled reloaded native scene family emits/retires16,694 primitives, with
272 culled instances. Its16,694 instances use16,694 calls: no merged group or
speedup is established. These are repeated visits/draws, not unique assets.

This closes the prior ongoing-scene reachability gap: host161's ordinary skin
scene counters advanced only during the opening events. The authored Toon
connection now reaches the native node/packet/indirect/fence consumers during
interactive gameplay. Source-address visual setup, animation and unconverted
families still exist; no complete host-owned character or frame is claimed.

## Visual verdict: failed

The captured `out/verification/native_skin_scene_window.jpg` is1920x1080,
101,438 B. Inspection shows the large Blue Dragon title logo over the village
and windmills, not the logged interactive player view. This reproduces run962's
window/context discrepancy. The image is retained failure evidence, not proof
of native character pixels, art parity or culling correctness.

Next observation must use the existing renderer-owned post-gamma/fence request
and receipt path to distinguish stale window pixels from wrong actual output.
Its earlier encoder/export budget failure and larger per-image approval remain
separate constraints; independently fit aggregate overlap before that producer.
No unchanged PrintWindow retry, relaxed context predicate or replacement capture
framework. Broader source ownership can continue independently, but stable
sequences, authored effects, both eyes and the full desktop gate remain required.

## Evidence identities

- Host EXE:49,090,560 B, SHA256
  `81A5E42635335A036A9FBE1942392C37DE173248C3539D015229F846AAD63B36`.
- Run971 log:537,422 B, SHA256
  `03605F5ED86CA97C3665CCACDB0906C118C3A78C0C5038436CEFB2DF47F61428`.
- Run971 image SHA256
  `4A19BA8DDEE6B913F536C599B5686FAB0E80F5B05AD7B742DA995A660DB32881`.
- Exact116-byte owner profile restored after both runs, SHA256
  `2F1BC38D763A1B7BDBA31F560684FD4AA7E42A714600B8D344F19DA7F38E23B0`.
- Host164/CPU50 and388 Python checks remain source/CPU evidence; the unchanged
  GPU28 shaders retain345 cases, validation0/0. No build/GPU rerun this turn.

Run970/PID37808 first passed the same complete strict text chain, then stopped
when its requested JPEG exceeded70 KiB even at the bounded encoder qualities;
no image was written. The follow-up changed only available output capacity,
not renderer code or correctness thresholds. Both renderer processes are terminal.

## Storage and cleanup

Same cumulative ledger/limits; both runs were bounded180 s,800 KiB log,192 MiB
free-space drop,75 MiB diagnostic stop. Run971 preflight diagnostics77,185,187 B
plus819,200 B log and112,640 B image reserve fit below75 MiB. No new raw,
perf, asset cook, cache or shader dump was observed.

Lossless repacking of the older Toon PNG saved75,463 logical B. Every filtered
byte, non-IDAT chunk and decoded pixel matches; measured free-space gain73,728 B.
The same10 MiB image cap then accommodated the normal110 KiB JPEG allowance.

Runs969/970 are now checksum-recoverable in
`logs/retained-native-runtime-969-970-20260908-skin.zip` (222,514 B), archive SHA256
`96EB3EB08C905CCACB513363CCA0AD62BFB49C6128CFF3164CA38252EC730FA9`.
Both entry lengths/SHA256 values and unchanged plaintext were verified before
removing those exact redundant files:885,515 logical B saved,823,296 B measured
free gain across the archive operation. Run971 stays plaintext for current use;
all baseline/failure images, profiles, game assets and active builds remain.

Total cleanup960,978 logical B,897,024 B measured free gain, credited once.
Selected runtime-log/archive/image retention grows215,271 B net for the new
live scene connection and unresolved visual evidence. Image aggregate10,434,657 B
leaves51,103 B under10 MiB. At13:34:23 free78,415,912,960 B (73.03 GiB), down
48,566,272 B drive-wide from13:22:35; unattributed system activity is separate
from selected files. No budget reset. Git push remains restricted pending the
safety reviewer's requested exact owner approval; no bypass was attempted.
