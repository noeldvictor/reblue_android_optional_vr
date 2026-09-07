# Load-owned hard-off routing for the selected rigid object

2026-09-07 EDT; parent096d732. This closes the missing-pose fallback hole for
the selected geometry258694267A8DBAEE in opt-in acceptance mode. It is not the
complete renderer transition or a completed in-game reload test.

## Boundary and implementation

Previously host walking attempted direct submission only when a native pose
was available. Missing poses could reach the old node interpreter/capture/replay
before the direct draw's own fail-closed admission could recognize the asset.

`bd_native_rigid_hard_off=false` is a separate acceptance setting. When enabled:

- Both native rigid scene/shadow paths, host walking and native instances must
  be enabled. The normal profile is unchanged.
- The existing load-owned model registry classifies the node before palette or
  bounds fallback and before culling. Missing/ambiguous model or node identity,
  missing geometry identity, missing pose/transform/bounds, stale generation or
  unsupported selected view refuses visibly. Unknown identity is not treated
  as proof of an unconverted family. Known other families remain compatible.
- The selected pose must retain the exact current model lease and generation.
  Source-address reuse cannot authorize an old pose against a replacement load.
- The first operation of `bdSceneNodeDrawSingle` rejects selected-family legacy
  entry, before diagnostics, replay, capture or any original-body call. The
  mesh association is resolved at this explicit source boundary, not inside
  the native GPU backend. No additional selection registry/cache is created.
- Existing native scene/caster whole-node admission, batch packing, shared queue,
  indirect emission and fence retirement are unchanged. Fixed-size admission
  counters are separate from actual GPU-emission counters.

Source audit: direct original-node calls occur only inside the guarded hook;
the two generated direct callers (files38 and51) call the hooked symbol.
This selected opaque family cannot generate its old deferred entries without
first crossing that guard. This is not a claim about other material families,
independent original receiver callbacks or all remaining renderer entry points.
The load registry is still produced through the original graph builder and
native source import; hard-off rendering does not mean source-free loading.

## Verification

Output21/PID27848 and CPU6/PID14616 passed, then the missing-geometry-identity
case was added before runtime. Output22/PID24552 and CPU7/PID24236 pass:
0.37 s assertions/0.39 s CTest. The existing fixture now links the real model
registry and covers selection before first pose, both supported views,
unsupported views, missing ownership, invalid instance/generation, short pose,
missing bounds, ambiguous nodes, teardown, address reuse and failed replacement.
Already retained old model leases remain valid for queued work but cannot
authorize traversal of the replacement. No disk or GPU is used by these cases.

All288 Python source/scenario checks pass in0.076 s. Three new scenario cases
require pre-first-submission admission and consecutive fresh field checks, and
reject late/missing admission, mixed generations, refusal and oversized logs.
One initially incorrect mixed-generation test changed every sample to the same
generation; corrected to change only the later sample before runtime. No game
retry was used to resolve this test issue. Source-string guards remain wiring
checks, not substitutes for the behavior fixture or runtime evidence.

Host97/PID27176 passed the initial implementation; host98/PID29904 passed the
identity refinement with one host source rebuilt. Codegen reports0 written,
module up to date; no guest objects rebuilt. No shader change or GPU rerun:
GPU26/rigid05 remains the evidence for the unchanged native programs/schema.
Final executable48,527,360 B, SHA-256:
`D075FACAD584689E98849CDAA3A4087EC39A903F597D149B9E8DB7F889705C4C`.

Run938/PID31104,10:42:22-10:43:25 EDT, enabled the prior full field verification
settings plus `bd_native_rigid_hard_off=true` through the existing supervisor.
All18 profile settings audited; perf/capture/cooking off. First pre-cull native
admission is frame764 before the first native caster and scene submissions.
Ready-field scene/emission windows1964/2264 follow opening event1364 and add:

- 300 scene submissions, real emissions and fence retirements;
- 300 shadow submissions/retirements and indexed indirect instances/calls;
- 300 fresh scene and shadow hard-off admission checks, zero refusal;
- Positive observed player movement and all prior field/material/light/fog/
  sampler/geometry/image gates. Merged native instances remain0.

Admission logs precede their same-frame material-context report. The existing
complete-window matcher correctly associates them with the preceding observed
ready context; admission windows1964/2264 follow ready contexts1664/1964.
No freshness gate was weakened. One aggregate model retirement is observed,
but the selected node remains generation93: this is NOT selected-asset reload.

Inspected `out/verification/native_rigid_hard_off_window.jpg`,1920x1080,
119,222 B: Shu running beside a fence, coherent terrain/props/tree and character
shadows. Known black cliff marks and distant blur persist. One mono image is
sanity evidence, not an isolated-object pixel oracle, sequence or both-eye proof.
SHA-256 `C0BAC4C9D1FBC3162922752432B2818BCD1D3857F1CEC5EC479A47D3E33281DE`.
Log `out/build/win-amd64-release/logs/reblue_938.log`,259,886 B,
SHA-256 `56567FC23F7B86F60E5EAC1053203BD61F6AF4C6A11CFD4B6754253BC65777B5`.
Exact116-byte owner profile restored, SHA-256
`2F1BC38D763A1B7BDBA31F560684FD4AA7E42A714600B8D344F19DA7F38E23B0`.
No renderer/build producer survives. No new raw/perf/cache/cook/dump output.

## Remaining gate and storage

Next is a real selected-asset teardown/reload through game flow with fresh
model/instance generations and native scene/shadow output. No synthetic registry
operation or process restart should be labelled that game test. Remaining source
object/pass/light/fog/receiver producers, multi-object runtime batches, both-eye
game coverage and the full desktop gate remain. No speedup is claimed.

The cumulative ledger remains
`20260906_0333_native-scene-state-bridge.md`; original3 GiB exception/raw0 gate
unchanged. After938 passed and its image was inspected, removed14 exact
superseded files: output20/21, CPU5/6, host96/97 stdout/stderr plus937 log/batch
JPEG.422,147 logical B; measured438,272 B reclaimed once. Test logs are
regenerable; the old runtime text/image are no longer retained, while their
hashes/findings remain in the prior report. Protected baseline/flat/VR/movement/
failure/raw evidence, game data and build trees are untouched.

CPU fixture tree68,896,433 B (+2,777,301) includes the linked real model registry
and stronger lifetime fixture. GPU tree unchanged10,487,791 B. Build logs184,150 B
(-28,516), images10,225,622 B (-24,379), runtime replacement+16,152 B,
exe+7,680 B/PDB+32,768 B: known component growth2,781,006 B. Retain current
implementation/fixture and replacement evidence until superseded; no duplicate
passing run archive. Other host objects/source/Git and drive-wide activity are
not fully attributed. Cleanup-end62,645,186,560 B free;439,316,480 B (~419 MiB) drive-wide use
from output preflight is not a cleanup or renderer storage claim. See the
cumulative ledger for exact measurements; no further build/run is queued.
