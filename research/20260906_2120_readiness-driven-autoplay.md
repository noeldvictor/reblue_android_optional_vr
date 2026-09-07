# Readiness-driven desktop walking and displacement verification

2026-09-06 EDT. Parent `b24e545`; Plume unchanged. This implements a verification
prerequisite identified by the strategy review, not additional native rendering
ownership. The full desktop/modern-GPU/Quest ordering is unchanged.

## Production path and source evidence

`xr/autoplay.h` owns a dependency-free, allocation-free pad policy. The driver
supplies native field observations through existing gameplay readers; gameplay
remains recompiled. After the existing START/confirm bring-up, walking requires
0.5 seconds of stable active/idle field, a player/stage, no event/movie/visible
debug overlay, and available shared directional-input gates. A missing source,
stage change, >0.25-second polling gap or invalid position stops movement and
invalidates the episode. There is no elapsed-time fallback to the old 150-second
walk delay. Disable resets the policy without changing real controller input;
enabled automation owns/neutralizes the pad so old real stick values cannot leak.

`Field::DirectionalInputAvailable` reads the shared gates from complete generated
`bdPlayerFieldCanMove` (file70, 0x82207250..0x822072DC): controller inhibit/grace
at +1672/+1676, script at +1824 and its mode+304, active-player entity +112/+116/
+628. Missing words fail closed and arithmetic is checked. The inhibit/grace
precedence is also read at file23 0x8239B90C..0x8239B924; the script update's
0x8219EB80 mode gate motivates conservative mode 0 for automation. This is not
the complete per-character CanMove predicate and does not claim input caused
motion simply because the gate opened. Read all of `bdPlayerFieldUpdateMain` and
the existing loader/cutscene hooks; no generated source or hook TOML changed.

The policy separately accumulates **observed horizontal player displacement**
from `Field::Position()`, and counts nonstationary samples. A >10-world-unit
single-sample discontinuity invalidates the episode; it is an evidence guard,
not a gameplay speed limit. These are game world units, not assumed meters.
One report/second, at most 120 reports per enable session, bounds text output.
Blocker masks distinguish field/idle/player/event/movie/overlay/input/stage/
position rejection; shared input masks distinguish inhibit/script/hold/lock/
state/missing data. The policy itself writes no files.

## Failed attempts and correction

- Host61 /PID29552 (19.756 s) and loader build03 /PID30236 passed. CPU02
  /PID24096 passed, 0.03 s. Run909 /PID31592 reached the idle field but never
  walked; its 75-second supervisor failed and restored the profile. No images.
- Host62 /PID972 added bounded rejection diagnostics. Run910 /PID29340,
  21:14:52..21:16:07, proved the only post-event blocker was mask32 (debug panel);
  shared input blockers were zero. No images. No readiness test was weakened.
- `Game::MindowsPanelActive()` reports the **selected** panel, not visibility.
  Complete `bdMindowsCreatePanel` (file12) stores the selected pointer at
  0x827A7D68 on creation (0x820D0CC0), irrespective of the overlay's hidden flag.
  Existing keyboard input/bridge and `ApplyDebugConfig` use the independent
  0x827A7D6C hidden flag. Autoplay now uses that same visibility contract; the
  existing selected-panel facade is unchanged. A regression fixture covers
  visible, hidden and unavailable observations.

The two failed logs are superseded after the successful replacement below.
Run909: 311,295 B, SHA256
`6b9e7c0bb94e32c302d3662ea288f2995d27e6db4732f906964004709a0f8407`.
Run910: 306,985 B, SHA256
`c1db3b4a5d3945415645a910adfcfba9adbcf2ae10314b9a565cf5083fe9aa3a`.

## Final verification

- Loader build04 /PID26140 and CPU03 /PID22744 pass (test 0.03 s, ctest 0.04 s).
  Reuses `out/native_texture_test`: production policies cover every shared gate,
  missing/overflowed reads, grace precedence, cold starts, settling, stationary
  polls, actual displacement, interruptions, stage changes, stale polls, invalid
  floats, discontinuities and disable/re-enable. Assertions remain active.
- Host63 /PID31040 passes: one input object plus link, Ninja **2.323 s**. Codegen
  reports zero writes and the module up to date; no guest objects/shader payloads
  rebuild. Exe 48,192,512 B, SHA256
  `9d07efe8811e44fe074ec30d8032409af732fb4e1ed8cd7e41359d0d7a891383`.
- 152 source-boundary guards and 28 scenario cases pass. The existing
  `native_instance_scenario.py --movement` requires fresh post-event contexts,
  one uninterrupted episode, increasing observation/walk time, nonstationary
  samples and displacement. Stick activity, stationary samples, later pauses,
  different episodes/stages, stale windows and invalid values do not qualify.
- Run911 /PID3732, **21:17:51..21:18:52 (~61 s)**, normal flat MSAA, PSO precache
  on, native geometry/material/pose verification, normal native texture tables,
  native vertex pulling. All ten profile overrides audited. Raw/perf off, mesh
  persistence suppressed by the existing material diagnostic. Every producer
  is terminal and the exact 116-byte owner profile is restored.

First observed walking: 21:18:35.453, **41.823 s** after initial pad polling,
0.300 s into episode1. Inferred episode start 41.523 s replaces the former
150-second code delay; this is **not** a controlled overall dev-speed or FPS
measurement. The final movement comparison adds 129 nonstationary observations
and 138.139791 world units in the same ready-field episode, reaching 16.6 seconds
of walking. A partial obstruction produces almost stationary samples earlier;
the new checker does not mistake continued stick input for displacement.

Fresh renderer checks during motion: 99,930 matching pose reads, 63,298 normal
table lookups (zero original comparison/fallback), 131,558 native pipeline/decode/
pulled records. Contexts2053..2353 add 29,050 load-owned geometry draws /316
matching source checks and 15,135 diffuse /14,525 specular matches. No reported
pose/table/material/geometry mismatch or load/budget refusal. Unconverted replay
lookups reach **928**, versus 761 in the prior standing sample; motion exposes
more remaining compatibility activity. Reflection's direct native image consumer
still has no observations. This does not establish guest-free frames.

Three spaced 1920x1080 JPEGs were inspected: changed gait, character position and
camera framing, with terrain, fences, vegetation, water and shadows visible.
Dark thin cliff-edge artifacts and distant blur are visible and unqualified;
retain the old standing reference for investigation rather than claiming a new
regression or a clean full-game result. Three sparse images are not continuous
sequence stability, reload, battle, cutscene, UI, authored-effect or both-eye
qualification. No Quest or XR run.

Retained evidence (SHA256):

- `reblue_911.log`, 229,708 B:
  `811094724dc1856b1795d9c0b482e7cbda8cc6abab5bfb556f1775a8a7496a7f`.
- `native_autoplay_01_window.jpg`, 427,997 B:
  `9ebcd74f8695cad272bfde588cda7fcfb46e4556cdc2e48d7d3303b5b922843c`.
- `native_autoplay_02_window.jpg`, 453,728 B:
  `0425a9ea6844270df4dccd1f3658bbbd174ba22d93f080e55013024ae12316b1`.
- `native_autoplay_03_window.jpg`, 373,488 B:
  `7789b329fbb35ada7d46e3107767caadf8a3e201694bfd0367c30fe36e0e0db8`.
- Loader/autoplay fixture exe, 68,608 B:
  `18550e2189820e6c3e2d85eae6c77a6bc6ec6b11e9a669a85c75a2d7dfca9b7f`.

## Storage and next work

Same cumulative ledger/ceilings in `20260906_0333_native-scene-state-bridge.md`;
no new raw allowance. Preflight 64,093,036,544 B free; plan <=128 MiB build/link
overlap, <=2 MiB fixture/log growth, <=1.5 MiB three-JPEG overlap. Existing
supervisors retain their original 63,583,739,904-byte stop floor and aggregate
log/image limits. No new build tree, download or cooked assets.

The drive fell to 63,789,682,688 B before the host build, much more than the
fixture explains. Scoped output/build/cache inventories found 18 recent files
/1,841,248 B, not a matching large producer. The unexplained drive-wide use is
not attributed to this task; the budget was not reset and each producer still
fit/enforced the original floor. After the motion run free was 63,744,540,672 B.

The reusable fixture exe/PDB/two objects now total 1,483,285 B, **765,043 B** more
than the recorded previous loader fixture. The three new images total
**1,255,213 B** for actual movement coverage; aggregate window images are
10,106,400 B, near the unchanged 10 MiB ceiling. Replace this trio when equivalent
movement evidence passes; keep the standing image until cliff-artifact comparison
is resolved. Keep final host63/loader04/CPU03 logs and run911, not failed attempts.
After replacement validation, removed16 exact superseded logs: host60..62,
loader builds02/03 and CPU01/02 stdout/stderr, plus runs909/910. Logical removal
687,078 B; free63,739,285,504 ->63,739,985,920 B recovers **700,416 B**, once.
Final aggregate build logs146,016 B. Comparable fixture/log/image retained growth
is **2,220,622 B** for new movement coverage; helper/build-metadata/source/Git
changes are not completely baselined. Post-cleanup drive-wide use353,050,624 B
from preflight is mostly unexplained by those outputs. No protected data removed.

The active queue now combines the former dependency/preparation and static-draw
milestones into a single complete native static-object outcome. Next source work
remains canonical self-describing cooked geometry/native shader contracts feeding
direct object scene/shadow submission, not more generic test infrastructure.
