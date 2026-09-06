# Host sorted visual scheduling

2026-09-06, desktop. Root 23cb4ee plus this change and the existing pending
renderer integration; local clean Plume 3094b35. Guest-source guided exact
translated-source tracing, devloop the bounded incremental verification, and
vrsim the existing desktop XR check. No Quest work or complete-frame claim.

## Ownership change and remaining work

The entire `Visual__DrawSortedQueues` body at 0x82424AF8 now has a host
replacement: model ordering/preparation, primitive setup/order/dispatch,
shader-mode switching, deferred output and final colour publication. Its normal
path no longer executes the original scheduler, guest bucket memset/linked-list
sort, blend-selector helper, thread-buffer helper or matrix-copy helper. The
enclosing full legacy parameter scope is gone; the tail publishes native PS c3.

Read the exact complete scheduler in generated file 81, queue initializer
`sub_824249D0` in file 52, both producers (`sub_82425848` / file 22 and
`sub_82425A18` / file 88), blend selector / file 95, matrix copy / file 78,
thread buffer / file 12, and pass begin/end / files 70/92. The initializer
allocates 2,048 x 104-byte models, 4,096 x 52-byte primitives and 512 deferred
pointers. These are not exclusively HUD entries: primitives include authored
3D positions and models have visual callbacks. No generated source edits.

The host owns one lazy per-submitting-thread order (32 KiB of keys), reused
between model/primitive phases. There is no normal read/write of the guest
bucket heads or entry next pointers. Preserve authored quantized layers,
descending order and reverse insertion ties; model NaNs reach layer 1 whereas
primitive NaNs clamp to the low depth endpoint. Scale/zero/one remain exact
authored constant imports, not guessed data. Unsupported initial counts/ranges
retain a pre-effect original fallback; invalid live inputs after callbacks fail
instead of replaying a partially submitted frame. Reentry is an explicit fallback.

The CPU-tested model preparation port preserves sequential scalar aliases,
special/ordinary scaling, raw matrix copying, bone initialization, live colours,
depth policy, blend mode and vtable rereads after callbacks. Native scalar copies
quiet signaling NaNs; matrix/vertex transport preserves raw bits. Primitives
sort after model callbacks/pass startup; flags remain live, shader selection
inherits between matching entries, and deferred count reloads after each draw.
The original 512-entry overflow behavior is preserved, not silently enlarged.

`bd_native_visual_schedule` defaults on. Remaining explicit boundaries:
authored queue/vertex/model producers and storage, visual/bone/material callback
implementations, pass begin/end dispatch, bool/texture/shader/state/getter
adapters, and the deferred `sub_824252D0` consumer including its emulated resolve.
This is not complete native scene, animation, assets, effects/UI or frame
ownership. The general immediate overlay classification was not changed.

## Verification

- 103 source guards pass. Final CPU fixture 05 / PID 14720 and suite 27 /
  PID 26928 pass 31/31 in 3.98 s. Independent scalar/linked-list oracle checks
  over 400,000 sorted entries, full capacities, reverse ties, raw/nonfinite
  inputs, malformed reads/counts and all 65,536 low-word blend flags. Additional
  tests exercise callback-produced primitives, mode changes, deferred overflow/
  reload, empty/model-only/deferred-only queues and the actual model preparation
  template with callback/vtable/colour/flag mutations and sequential aliases.
  Fixture 530,432 B, SHA-256
  `206574c9cc56acdaf4d9325b88c23072ebcffebd3dad48bef1c77ce07b8988ac`.
- Initial fixture 04 / PID 22612 and suite 26 / PID 27496 also passed before
  adding model preparation tests. Host builds 33 / PID 19548 / session 39460
  and 34 / PID 784 pass with no guest/shader object rebuild; codegen reports
  zero writes and one up-to-date module. Build 33 binary 47,759,872 B, SHA-256
  `4fbc2633e13ffaf4aba1c1c2ace0b6f34ccee726248f18d9c34b5ff15e750252`.
- Flat original-immediate-UI plus independent parameter comparison, log 879 /
  PID 23784 / session 64438, passes on build 33. Samples: 12,806 matching UI
  preparations/uploads, 239,393 matching native parameter blocks; scheduler
  285 native calls / 9,682 primitives, zero fallback/refusal/faults; 301 field
  water updates. All seven settings audited, raw/perf capture disabled.
  This compares original immediate output, not an original whole-scheduler run.
- Final build 34 linked 13:19:12, 47,759,360 B, SHA-256
  `08d36713f496081309fe3d537ef85a821f98f110c939b229eed59c34bbcb47a7`.
  The model preparation code was extracted into its tested template and the
  immediate diagnostic description corrected between builds 33 and 34.
- Normal flat log 880 / PID 26048 / session 50316 reaches the 75-second bound.
  Samples: 2,121 native scheduler calls / 78,861 primitives, zero fallback/
  refusal/faults; 77,409 native immediate submissions, zero upload failures;
  1,013,913 native parameter blocks, 767,416 imported words and **zero full
  legacy blocks**; 2,701 native water updates. All five settings audited.
- Viewed `native_visual_schedule_window.png`, 1920x1080 / 3,359,345 B, SHA-256
  `c22cabdc45e49f4661cd221992a1f3b03f3962d5cff2cbcc6cc2dd8d38d10ecd`.
  Standing Talta-field Shu, shadow, rocks, foliage and background DoF look sane.
  One image is not sequence, menu, animated effects or both-eye qualification.
- Final desktop XR parameter check, log 881 / PID 26508 / session 52179,
  passes: 952,257 matching native blocks, zero full legacy blocks; 425 native
  scheduler calls / 36,784 primitives; 35,666 native immediate submissions;
  zero scheduler fallback/refusal/fault or upload failure; 301 water updates.
  All 17 settings audited, 1440x1584 per eye, scale 1, layered multiview,
  camera mode 2. Field game camera (-226.4,124.2,-6.4), eye (-229.9,125.1,-6.5)
  confirms active XR composition. No new eye images, raw frames or perf CSV.

Counters are periodic samples, not synchronized final totals. Queued models,
deferred primitives and overflow were not observed in these field runs; they
have CPU/source coverage, not authored runtime qualification. All jobs terminal;
the owner's 116-byte five-line profile is restored. No next build/run was
started after the owner's status/README request.

## What the speed evidence does and does not say

Current flat perf pair `perf-20260906-131943` totals 602,224 B. Last 600 samples,
frames 2994..3593 / 62.667..72.650 s: median dt 16.667 ms (about 60 FPS),
`other_ms` 6.610 ms, GPU total 5.677 ms, GPU draws 5.479 ms, GPU resolves
0.154 ms. P95 dt 16.773 / other 7.122 / GPU 6.341 ms. These are current desktop
standing-field timings, not an overall before/after speedup or Quest estimate.
CPU/GPU overlap; do not add those columns into an inferred frame time.

The previous immediate-UI report records 11,139,284 imported float words /
856,741 native parameter blocks and 168,936 full legacy blocks in its normal
flat check. Current samples are 767,416 / 1,013,913 and zero legacy blocks.
That is about 13.00 ->0.757 imported words per native block (94.2% lower),
plus removal of the enclosing full-block imports in this field. These sampled
work counters demonstrate less compatibility work, **not 94% more FPS**, a
controlled identical-frame benchmark, or zero guest rendering. No defensible
whole-project percentage completion or overall speedup is established.

## Storage closeout

Same original ledger: `20260906_0333_native-scene-state-bridge.md`, no budget
reset. Preflight free 65,279,000,576 B. After validated equivalent replacement
evidence, checked every exact ignored file and reparse-free ancestor, confirmed
no build/game producers and the restored profile, then removed 18 files:
build 32/33, fixture 03/04, CPU 25/26 stdout/stderr pairs; runtime 876/877/878;
perf-124446 pair and the old immediate-UI PNG. Logical 4,680,814 B; immediate
free 65,261,813,760 ->65,266,507,776 B, **4,694,016 B measured reclaimed**.
Only regenerable superseded diagnostics; results/hashes remain in reports.
No game data, assets, trees, distinct non-MSAA or unresolved-failure evidence
removed. The replacement original UI comparison retains that separate purpose.

Reserved diagnostics 64,407,085 B: 94 build logs / 138,226 B, 14 runtime logs /
3,953,316 B, 20 perf files / 8,959,072 B, eight GPU fixtures / 8,364,855 B and
unchanged 41 MiB tools/inspection reservation. Two PNGs / 6,693,501 B fit in
that reservation. Retained replacement log/perf/PNG payload grows 14,085 B
with run-size drift; replace on equivalent coverage. The reused CPU fixture
also grows 418,304 logical B for independent order/model coverage (within the
tools reservation); the host binary grows 20,480 B versus the last checkpoint.
No new raw/cooked/cache outputs, tool download, build tree or guest rebuild.
Post-cleanup drive-wide use +12,492,800 B since preflight; this includes unrelated
activity and metadata, not solely task outputs. Later docs/Git writes count.

Root/dependency publication still requires repository-specific upload approval.
No push retry, workaround or unpublished dependency gitlink staging this step.
