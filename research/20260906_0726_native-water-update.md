# Host water animation and parameter publication

2026-09-06, desktop Vulkan. The complete water update and shared sampling-mode
counter callbacks now have native replacements. This follows the setup callbacks
in `20260906_0700_native-refraction-materials.md`; it is not full material/asset,
frame or Quest completion.

## Source and implementation

Read the full guest-source/devloop skills, `config/hooks/frame_interp.toml`,
the complete `sub_82454398` in generated file 74, and `sub_8221D460` in file 24.
The tick gate implementation in `src/engine/frame_interp.cpp` uses
`bd::engine::TickDue()`. The earlier setup callback and constructor provide
additional authored-field/parameter-layout evidence. Generated code and hook
TOML remain untouched; no decompiler, tool download or shader regeneration.

`native_water_update.h` owns phase arithmetic and mode-transition policy without
guest addresses, PPC state or D3D inputs. Phase addition rounds to float, followed
by one strict-greater wrap, not modulo. Interpolated non-tick updates do not
advance time. The original authored tick cadence remains; this does not yet
establish a separate native material clock or persistent native state.

`water_material_import.h` maps twenty authored bindings to **23 parameter words**,
including a sequential four-component vector, scaled property, signed scalar
and settings-dependent property. It also publishes mode changes and the two
shared mode counters, preserving add-before-remove and unsigned wraparound.
Authored fields, descriptor/buffer addressing and shared counters remain explicit
temporary imports, not cooked native material identities.

A fixed 32-entry write plan reads through its own earlier writes. This preserves
overlap between outputs, subsequent inputs and descriptor controls, while refusing
unreadable/unaligned/overflowing destinations before partial publication. Vector
components load sequentially, not as a memcpy. The plan allocates no heap/disk
storage. Original-stack aliases are refused before conversion.

`native_water_update_bridge.cpp` replaces both whole functions, gated by existing
`bd_native_scene_textures`. Normal execution publishes the checked native plan,
without the original callbacks or PPC stack construction. The diagnostic-only,
default-off `bd_native_water_verify` predicts native writes, executes the original
once and compares final aliased destinations and the full return register.
A reference scope ensures the original water callback's nested mode call is also
original, not inadvertently verified against another native implementation.

## Verification

Root `dfba06d` plus this change and the pre-existing pending renderer integration;
local Plume `3094b35ae2e53207d557532748cf2ac7c96a5035`, clean. No dependency
gitlink staged and no remote push attempted.

- Existing `host_post_output_test`, focused build 14/PID 23616: exit 0. New cases
  cover all output bindings, tick/hold/strict wrap, phase NaN/infinity behavior,
  mode pairs/counter overflow, missing words, invalid owners/settings, output
  overflow, forward vector overlap, a later source/descriptor changed by an
  earlier write, final-alias comparison and the 32-entry ceiling.
- CPU 22/PID 27576: **31/31**, 3.59 seconds; the expanded output fixture takes
  0.33 s. CPU executable SHA-256:
  `9225cb9521b644271c0620bccfcacd5041f5fa175b942d9d46dd4a490dbd34c1`.
- **88 source guards pass**: water update 5, material setup 6, snapshot 5,
  post 36, scene 24, effect activation/lifecycle 7 and view scheduling 5.
  These are structural checks, not visual or full runtime qualification.
- Desktop build 25/PID 24392/session 54883: exit 0; both whole hooks linked.
  Codegen zero written/one module up to date; no guest objects or shaders rebuilt.
  Binary linked 07:18:56, 47,708,672 B, SHA-256:
  `94dbd4ca4976acb927eb566feffdc4cadafc3639103ba02088483514d142bb1f`.

Original-comparison run: PID 22500/session 31230, 07:20:57-07:21:25, wrapper
exit 0. Stopped at the first sample over 256 checked updates, within its 45-second
limit. **301 water updates matched**, with 288 tick advances, 6,923 parameter
words, two mode words and zero compatibility/refused/wrong. This covers both
tick and non-tick cases. It is original execution with native prediction, not
proof that the normal path executed natively. Log 868: 111,333 B; all six settings
passed audit. Performance CSVs, PNGs and raw captures were disabled.

Normal native run: PID 26432/session 50037, 07:22:58-07:24:15, wrapper exit 0
after its timed stop. Last update sample: **2,701 native water updates**, 1,719
tick advances, 62,123 parameter words, two mode words; zero compatibility,
refusal or reference checks. Last setup sample: 1,962 water preparations, zero
fallback/refusal/faults/debug images, and no snapshot requests. Post last reports
3,601 native frames/sequences with no original or refused post path. Counts are
sampled totals, not exact shutdown counts or counts of fully host-owned frames.

Normal log 869: 273,140 B; perf-072301 CSV/metadata: 606,320 B. All five settings
passed audit. Both complete logs have zero runtime/config error matches, both
profiles restored byte-for-byte and all producers terminal. Zero new raws or
cache/dump files.

Inspected `out/verification/native_water_update_window.png`: 1920x1080,
3,355,950 B, SHA-256
`1a142e206dc31d1b3d38157f52005f4e7f9fc4ff51d56d9571c52908b09ede69`.
Shu, shadow, terrain/foliage and distant DoF look sane. This single flat frame
does not isolate water animation or qualify a temporal/both-eye sequence.

Limitations: standalone mode-hook dispatch was not observed; its transition
algorithm has CPU coverage and is exercised inline by water updates. Runtime
tests do not cover every mode/alias/exceptional float. Refraction/snapshot events,
full material assets/storage, native scene associations and broad desktop
field/battle/cutscene/menu/transition/reload/both-eye qualification remain open.
No new XR/non-MSAA matrix, raw sequence or Quest run occurred.

## Storage

Same cumulative ledger: `20260906_0333_native-scene-state-bridge.md`; original
65,462,788,096 B baseline, 2 GiB peak/100 MiB diagnostics/10 MiB build logs,
zero incoming raw allowance. Reused existing trees/tools; step plan <=256 MiB
build/link overlap and <=6 MiB diagnostics across both runs and all attempts.

Gross new diagnostics **4,353,076 B**: build/test logs 6,333; comparison log
111,333; normal log 273,140; perf 606,320; PNG 3,355,950. No new test executable
target/tree, tools, raw or cache/dump files. Comparison stops once sufficient
checks appear and disables unused performance output.

Validated exact paths/lengths, ignored status, reparse-free ancestors, replacements
and absent producers, then removed ten superseded files: reblue_24, cpu_21 and
host_post_output_test_13 stdout/stderr pairs; normal log 867, perf-065659 pair and
the refraction-material PNG. Logical 4,236,095 B. Immediate free space:
64,460,967,936 ->64,465,207,296 B; **4,239,360 B measured reclaimed**, once.
Reports/hashes remain; equivalent diagnostics can be regenerated. No protected
data, distinct XR/non-MSAA/failure/raw evidence or build trees were removed.

Retained diagnostic payload growth: **116,981 B**. Of that, 111,333 B is the
first exact water-publication comparison; retain one such log until replaced
by equivalent evidence. The rest is replacement size drift, not extra sets.
Reserved diagnostic accounting: 63,961,991 B (86 build logs/138,207 B;
12 runtime logs/3,504,145 B; 20 perf files/8,963,168 B; eight GPU-fixture files/
8,364,855 B; unchanged 41 MiB tool/inspection reservation). Two PNGs total
6,690,106 B within that reservation.

At cleanup completion: free 64,465,207,296 B (60.04 GiB), net volume growth
2,260,992 B from the 07:11 preflight; cumulative growth 997,580,800 B. Build,
Git/metadata and unrelated volume activity remain charged; later docs/Git writes
still count. The full goal remains active, and remote publication needs approval.
