# Current-depth GPU visibility and fence receipts

2026-09-07. Source baseline2637bba, with this checkpoint's changes. Plume2d206ee.
The preceding turn made progress by pushing the source-only prototype. This
checkpoint compiles, connects and verifies its GPU producer/indirect consumer;
it does not connect the game queue or complete the host-renderer goal.

## Delivered GPU contract

`src/gpu/native_depth_visibility.*` consumes an owned mono D32_FLOAT_S8_UINT image,
its native allocation identity and an owned current camera. There are no source
addresses, translated shader constants, console depth layouts or temporal-query
results in this compute path. The native indexed command is20 bytes, with32-byte
independent output slots. Only instance_count can change, to zero or its exact
requested value; every other field remains unchanged.

The initial level takes the maximum of every2x2 pixel tile and every original
MSAA sample, then further levels reduce by maximum. An ordinary MIN depth resolve
is deliberately present in the MSAA fixture but never used as pyramid input.
Odd right/bottom edges participate; nonfinite/invalid depth becomes visible.
Inflated indexed world bounds project through the same camera; uncertain near/eye/
far-plane bounds remain visible. Viewport padding and conservative maximum-depth
coverage prevent a small hole or uncovered MSAA sample from becoming an occluder.

Work pins its original depth image, program, descriptors, pyramid, indirect buffer
and readback. All in-flight callers must share `NativeDepthVisibilityBudget`:
at most64 MiB of requested buffer bytes and16 owners, with tighter limits allowed.
Each pyramid is at most16 MiB; each recording has at most4096 command slots and
4096 snapshots. Driver allocation granularity, shared images/programs and CPU
metadata are distinct from those requested buffer bytes. The budget counts both
indirect and readback buffers; allocation refusal precedes GPU allocation.
The reservation is noncopyable/nonmovable and is destroyed after GPU resources.

`RefreshDepth` rebuilds the same scratch buffer with a fresh camera/depth snapshot
within the same image/frame/command recording. Descriptors stay immutable; prior
commands keep their own output slots. Caller must refresh after an intervening
clear/non-monotonic depth write and explicitly restore graphics bindings. This
is ordered GPU scratch reuse, not reuse of previous-frame visibility.

`DrawCommand` records a real indirect draw exactly once. `Seal` copies final
commands to retained readback; `CollectAfterFence` requires the caller's actual
completed submission fence and adds no wait. Receipt publication validates every
field before exposing results. Merely generated commands have zero emitted
instances unless a draw was recorded; culled draws also have zero. These receipts
are accounting, never next-frame culling inputs. Runtime still needs its fence
caller and lifecycle connection; the method name alone does not prove a wait.

The synchronization follows the primary Vulkan descriptions for
[depth/compute/indirect/host dependencies](https://docs.vulkan.org/guide/latest/synchronization_examples.html)
and the [indexed indirect command ABI](https://docs.vulkan.org/refpages/latest/refpages/source/VkDrawIndexedIndirectCommand.html).
Plume's GRAPHICS barrier stage includes DRAW_INDIRECT; compute transitions end
rendering and preserve the original native depth owner/layout.

## Verification actually run

Existing configured desktop tree, target `native_scene_snapshot_test`, Vulkan
fixture on NVIDIA GeForce RTX3060. No host executable build, game launch, profile
change, asset conversion, raw/image export, FPS benchmark or Quest run.
The existing ignored bounded wrapper enforces32 MiB free-drop,300 s process,
30 s CTest,5 s fixture fence and10 MiB aggregate attachment-log limits.

- GPU43/PID35916: first five shader compilations and fixture link pass.
  Visibility1/PID32484:40 original cases pass in1.43/1.45 s.
- GPU44/PID37592: compile fails on missing `<span>`, fixed before retry.
  GPU45/PID35828 passes; visibility2/PID34056 passes44 cases in1.20/1.21 s.
- GPU46/PID37480 and visibility3/PID5612: final72 cases pass in1.27/1.28 s.
  Sixty cases cover ordinary/rotated/odd/perspective views, solid depth, absent
  occluders, center/edge holes and sample0-only depth across1/2/4/8 MSAA.
  Every pyramid cell and command field is compared with independent expectations.
  Twelve four-step sequences reuse the same depth/scratch/command buffers,
  alternate hidden/exposed/hidden/camera-exposed, retire source references before
  submission and verify lifetime through the actual fence. Eight sequences
  deliberately corrupt geometry or instance-count fields after drawing; receipt
  publication correctly refuses them. Pixel assertions still verify all draws.
- Rigid15/PID30452: all46 existing two-eye production-shader cases pass,
  1.28/1.29 s. Occlusion6/PID28096: all8 old query regressions pass,1.04/1.05 s.
- All GPU tests report validation errors0/warnings0. One loader diagnostic names
  a missing GOG overlay manifest; it is not a Vulkan validation warning.
- GPU47/PID32088: compile/link verifies noncopyable/nonmovable budget ownership.
  No shader or runtime behavior changed after46, so its GPU results are reused;
 47 is compile-only verification, not another pixel run.
- `python -B tools/host_checks.py --all-boundaries`:334 source/scenario tests pass.
  These source guards are separate from GPU behavior evidence.

Final visibility log: `out/verification/attachment_resolve_visibility_pixels_3.stdout.log`,
15,342 B, SHA256 `88CEA9343457806BC6B9F5CBB26E7B811FA4C237BC513F65B1953A12656C4546`.
GPU46 tested fixture exe966,144 B, SHA256
`41F45F2AB72C483B130E758D837E32121B55421C9990F39FF6627526917D71E3`;
its PDB8,585,216 B, SHA256
`7793C69DC75581D5DB9C5CA0E55A001FC0F4E0921C68484789B5BFFB22AB3F55`.
Current GPU47 compile-only fixture exe SHA256
`4828F1CBBE1C6B5485E70BE1D6B5FC50571C76C09910426614895A1E80BE5A55`.
The game executable remains host121, as recorded in the indexed-bounds report.

## Runtime integration decision

No guest rendering dependency is retired by this GPU-only checkpoint. The game
still calls `OcclusionCullRequest` after whole-node preflight and ordered light
publication, then batches/emits surviving native records. Do not relax that
history's exact-camera checks while the replacement remains unconnected.

Reuse `NativeRigidDrawStore` frame-slot retention, `NativeRigidBatchItem`,
`PrepareNativeRigidBatchDraw`, the existing queue and native scene image owners.
Capture depth ownership and bounds with the item; compare exact depth/camera
compatibility for merged instances. A union bound can conservatively cull an
entire batch without breaking instancing; per-instance compaction is later work.
Do not allocate a new pyramid per draw: refresh one retained recording owner
after intervening writes and share its budget across all in-flight slots.

`EmitOne` currently increments emission counters when the CPU records a command.
GPU culling makes that insufficient. Route through actual-draw receipts, seal
before submit, and collect at `DrainNativeRigidDrawsLocked` only after the slot
fence, before native-image retirement. Resource/source retirement still counts
all retained submitted records; visible output counts only GPU-confirmed draws.
Current scene/cutout parsers assume emitted and submitted equality and retired
<=emitted. Explicit pending/culled/visible evidence and causal lifecycle/parser
tests must replace that ambiguity; do not relabel generated work as output or
reduce900 visible emissions/250 ms readiness. Do not exempt the selected asset.

Only then remove the old production query pool/history calls and do the bounded
host link and fresh game-culling image/reload observation. Prior run960 timeout,
961 missing image,956 tree gap and948/941 failures remain unresolved. Host107/
run945 remains accepted game-pixel evidence. Full scenes/events/animation/both-eye
desktop gates and eventual Quest2 optimization remain required. No speedup or
whole-host-frame completion is claimed.

## Storage

The cumulative ledger remains `20260906_0333_native-scene-state-bridge.md`, with
the same3 GiB owner exception and62,509,998,080 B floor. First measured free this
continuation80,956,170,240 B. Existing trees reused, no raw/image/perf/cache/asset
producer added. A scoped NVIDIA DXCache check found no files modified since this
preflight; unexplained drive-wide changes are not attributed to this fixture.
Only superseded agent-created text logs are retired after replacement passes.
Exact final retained sizes, cleanup and ending free space are in that ledger.
