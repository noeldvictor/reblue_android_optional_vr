# Owned blended cutouts in the native rigid scene path

2026-09-07, EDT; based on b910ae9. Source connection and CPU/GPU fixtures pass;
host compilation/link and game acceptance remain pending. No complete native
frame, textured cutout caster, full-game stereo or Quest qualification is claimed.

## Connected change and source contract

Load-owned material ranges now retain ordered cutoff-reset recipes. The object
scope copies pass defaults and conditional object overrides before traversal.
Non-bootstrapping getters supply the already host-owned comparison, coverage
request and enabled blend factors. Whole-node preparation composes these values;
the existing scene shader, native PSO, indirect queue and fence owners consume
them. No per-primitive cutoff setter, translated specialization mask, shader
register read, capture or replay supplies the native draw. The pass setter and
bootstrap infrastructure remain temporary producer dependencies, not globally
removed machinery. No second renderer framework, asset cache or recook.

- `bdSceneNodeDrawSingle`, generated40, loc82280564 initializes the cutoff to
  zero. Loc822806E8..82280740 resolves zero at every primitive, before suppression,
  using the direct/sorted defaults at the material participant owner +60/+64.
  A nonzero default persists until a control resets it. Sorted participation
  therefore survives suppression separately from the deferred-draw count.
- E000 at loc822813CC..82281484 calls `sub_8228AB40` (generated94). Its second
  handler `sub_8228AB18` (generated67:9913) writes the second payload word when
  present-bit1 is set, otherwise zero. `sub_8228AAB0` (generated79) independently
  owns lighting/shadow switches. Missing cutoff data does not erase those known
  switches. A null table is a no-op; selecting the same non-null record again
  still resets cutoff. The decoder folds that action into primitive ranges.
- `sub_8227FDC8`, generated25:9645, forces blending and alpha testing on for
  direct alpha participants. These are blended cutouts, not opaque draws plus
  an assumed 0.5 discard. The special global byte at the original -26711 offset
  instead preserves external state; the ordinary adapter refuses that mode.
  Loc82280988..822809D0 applies the resolved reference, then visual+3124 when
  visual+3120 is set and scene+212 is clear. The override does not overwrite the
  node's running default; explicit zero here is not a request for a pass default.
- Reference conversion uses recovered float bits 0x3b808081 and an unsigned
  reference without a guessed 0..255 clamp. The blend getter derives enabled
  factors from retained requests, not the disabled effective COPY state left by
  an opaque predecessor. Shared/separate alpha folding is retained; unsupported
  factors refuse rather than borrowing the compatibility ZERO/ADD fallback.

Admission now permits direct cutouts in supported technique 0/phase 0 rigid nodes
with 0..3 layers. Deferred or unsupported siblings keep the whole node on its
explicit legacy route; admitted missing owners refuse before submission.
Depth-only casting still refuses alpha families. GPU layout remains 208-byte
objects / 816-byte instances: reserved words carry native compare and cutoff bits.
Eight comparisons operate on base texture * object * vertex alpha; detail alpha
affects RGB only. Blended draws retain depth writes and cannot enter opaque
reorder runs. Coverage is requested only for multisampled targets; actual MSAA
coverage remains untested.

## Verification and pending gates

- 314 artifact-free Python source/scenario checks pass, including new producer,
  shader and queue wiring guards. They are not game evidence.
- Material35/PID24032/session20560 builds; CPU33/PID34692 passes 0.12 s (CTest 0.15):
  ordered/repeated controls, direct/sorted defaults, suppressed siblings, zero
  override, missing inputs, bounds and destroyed sources.
- Output35/PID21864 builds; CPU18/PID36272 passes 0.41 s (CTest 0.44): scene plans,
  copied lifetimes, comparisons/NaNs, requested/shared/separate blend factors,
  missing/invalid cutout inputs and unchanged depth-only refusal.
- GPU29/PID36328/session27134 compiles four production shaders and the existing
  fixture. Rigid08/PID36048 passes 23 modes, 8x8 pixels per eye, 2.87 s (CTest 2.89),
  RTX 3060. All preceding 12 cases plus 11 cutout cases: eight comparisons with
  exact boundaries, blending over a nonzero destination, discarded pixels
  preserving colour/depth, per-instance references, zero layers and negative-U
  sentinel. Maximum colour error 3.39895e-05, unchanged tolerance 0.003; depth
  tolerance 1e-5 unchanged. Zero Vulkan validation errors/warnings. One loader
  diagnostic names an absent GOG overlay JSON. Readback remains in RAM, raw 0.
  Fixture exe 814,080 B, SHA256
  `CD629F558BC6D00AABE13DE064A8DB8EFB5BE9938F74F65CDD20D006E7A97548`.

All handles terminal. Host112 needed 192 MiB above the unchanged storage floor
and was not launched. A later four-source syntax plan also failed preflight
before any compiler/log started. The local supervisor only added alpha/blend
source names to its existing syntax mode. Host remains host111, SHA256
`295E4FB6B8265DCA01701D411BCCBB9EC4088B0D4DC7BEB143AF2C8F3EFDA655`.
Profile untouched, 116 B, SHA256
`2F1BC38D763A1B7BDBA31F560684FD4AA7E42A714600B8D344F19DA7F38E23B0`.

Next: compile/link the host within the reserve, then verify fresh cutout consumer
reachability, pass/participant ordering and retained-legacy interoperation with
inspected game pixels. Connect phase1's textured shadow producer/program using
its own image/colour rules; phase 0 imports are not shadow inputs. Preserve run948's
UV and run941's light dirty-bit failures and strict comparisons. Run945 remains the last
accepted game/pixel checkpoint; normal acceptance switches remain off.

## Storage and evidence retirement

The existing cumulative ledger preserves the 3 GiB exception and 62,509,998,080 B
operational floor. First measured free 63,759,519,744 B fell to 62,583,951,360 B
before builds. Sequential fixtures fitted 32 MiB peaks then. Later drive-wide
activity took free space below the floor, preventing host/syntax/game producers.
Scoped game logs/cache/HLSL dumps contain 0 files modified since 16:40; no new
game/capture/perf/cook output. Root shader generation is separate from those dumps.

Removed 12 superseded stdout/stderr files: material34/CPU32, output34/CPU17 and
GPU28/rigid07, after replacement CPU/GPU coverage passed. 8,214 logical bytes;
free 62,240,915,456 -> 62,240,931,840 B, 16,384 B observed recovery, not isolated
from other volume activity. Exact old text is gone; tests are reproducible and
prior reports preserve results. No game data, profile, source, active build tree
or unresolved-failure/game-pixel evidence removed.

Known retained output growth 197,469 B: material +63,416, texture +119,466,
GPU fixture +8,731, build logs +5,856 (now 195,935 B/136 files). Root generated shader
headers, CMake, source and Git are unallocated in that subtotal. Inventory ending
free 62,243,811,328 B: drive-wide change -1,515,708,416 B, not attributed to the
roughly 197 KB of counted fixture growth. Retire these fixture logs when equivalent
replacement coverage passes. Original image/raw protections and budgets unchanged.
