# Load-owned primitive shadow policy

2026-09-06; parent ae04d1e, Plume unchanged at3094b35. Full native static-object
scene/shadow submission remains the active outcome, not achieved by this change.

## Source boundary and implementation

Used guest-source and devloop: read exact generated C++/PPC and existing hook
configuration, then reuse focused fixtures and the configured Vulkan-only tree.
No generated/game data or shader payload was edited.

- `bdSceneGraphBuild` (`generated/reblue_recomp.79.cpp`) finishes node/buffer
  relocation before native publication. For version0x00300200 and newer it copies
  the graph+8 asset control payload into the scene allocation; older graphs use
  the existing pointer. The graph destructor retires the native generation.
- `bdSceneNodeDrawSingle`, loc_822813CC, selects a16-byte E000 control record.
  `sub_8228AB40` dispatches control decoding; `sub_8228AAB0` interprets present
  bit0 and flags bit3 as the shadow-receiving disable. This change moves the
  existing, previously compared policy interpretation to load time.
- `NativeModelMaterialProgram` now owns one Receive/Disabled/Unknown value per
  primitive, charged to its existing8 MiB owner budget including pinned retired
  generations. An unreadable input is unknown, not an enabled default. Explicit
  null tables and omitted commands retain the existing no-op/default policy.
- The pure import adapter bounds the record index and complete word addresses.
  `FindModelShadowPolicy` rejects conflicting or unknown policies on reused
  geometry. `ImportMaterialDisablesShadow` no longer reads control-table words;
  current pass availability and instance visibility still feed composition.

This is load-owned runtime data, not a new persisted BDMAT schema. Original
interpreter execution still reads its controls during warm-up/comparisons. Source
identity lookup, templates, phase1/technique11/deferred gaps remain. Field checks
do not establish all control mutation/lifecycle families or runtime reloads.

## What the next direct-object work must preserve

Source investigation also traced the remaining contracts rather than guessing
that all mesh ranges are opaque:

- Draw tokens1000/2000/3000 select opposite/pass-relative/two-sided culling via
  `sub_82287738`; direct and deferred output can both participate. Current decoded
  material ranges do not yet encode this complete routing contract.
- Ordinary texture tokens at loc_822816B0 apply ordered152-byte visual override
  records (UV and image), special scene-image/color cases, and later84-byte image
  override records. Native table handles alone do not replace these live rules.
- Full `bdSceneNodeDrawIndexed` uses triangle strip, base vertex0, operand2 as
  first index and operand1+2 as count. The canonical import already matches it;
  the otherwise passed range value is not a missing base-vertex correction.

Next implement native texture/override and draw-participation records with their
update producers, then named shader inputs and the direct consumer. Do not freeze
animated overrides or drop alpha/deferred participants to obtain a rigid demo.

## Verification

Material build12 stalled before compilation in the restricted process context.
Only the owned idle process tree was terminated; empty logs were not reported
as success. Retry13 /PID31504 passed (2.94 s wrapper); CPU11 /PID31416 passed
(fixture0.10 s, ctest0.13 s). Tests include destroyed source storage, unknown/null
controls, overflow, feature bits, conflicting shared geometry, owner budgets,
source reuse and retired-generation leases. All157 source guards and39 in-memory
scenario cases pass. The new scenario gate requires policy use and receiver
checks in the same fresh field windows, not startup-only observations.

Host65 /PID26664 passes. In-place CMake/codegen reported an up-to-date module;
no guest object or shader rebuild. Tested exe48,216,064 B, PDB106,508,288 B:
SHA256 `a47b35806a8606dc04f4b64c30e1f0fcce186d4dd41ff6fb2cb6644a52ea81b4`.
Material fixture exe SHA256
`dc745fbc5026e986467b6ab2e59ed493138e098830b453d1893e945ba38678de`.

Run913 /PID26452,23:41:21..23:42:19 EDT, normal flat1920x1080/MSAA/precache-on.
All11 profile settings took effect: autoplay, native instances/shadow inputs/
texture tables and material verification on; normal table comparison off;
perf CSV and automatic capture off. No new raw/perf/cache/dump files. Exact
116-byte owner profile restored; SHA256
`2f1bc38d763a1b7bdba31f560684fd4aa7e42a714600b8d344f19da7f38e23b0`.

Fresh post-event windows frames2050/2350, bg41_01 state0, player present,
event/movie/loader/icon inactive:

| Observation | Result |
| --- | --- |
| Load-owned shadow policies |2,973;209 disabled;0 unknown |
| Native policy lookups / matching receiver checks |+15,019 each;0 mismatches |
| Receiving comparisons / composed replays |+14,407 /+43,037 |
| Unsupported/unmatched shadow adapter lookups |71,945 cumulative; still transitional |
| Model geometry |+29,535 draws /+317 matching checks;946 unavailable lookups |
| Diffuse / specular comparisons |+15,019 /+14,411 matching; reflection0 |
| Pose reads/checks |+100,547 each,0 unavailable/refused/drift |
| Canonical geometry / native pulling |2,206 owners,+122,863 draws /+135,877 records |
| Normal texture tables |+29,887 lookups,0 original comparison/fallback |
| Source-free GPU loads / direct native reflection image reads |0 /0; not qualified |
| Movement |same episode,+31 observations,+38.354962 world units;12.703 s walking |

Inspected `out/verification/native_shadow_policy_window.jpg`,1920x1080,
JPEG quality60,134,082 B: character running near fence, ground/foliage/building
and cast shadows visible. Thin dark background artifacts and distant blur remain
unqualified; this is neither same-camera pixel parity nor a stability sequence.
The former canonical sanity image was also inspected before retirement.
New image SHA256 `60a9bfc7e67df716df90475a5723eb7de8790d95c7cea20226d09230c3242ab9`.
Retained run913 log225,894 B, SHA256
`7b5c50e3a93c3d22e0e9e59ebd93819c044c316c29ec18b601d76f19bc69f47f`.
No FPS/bandwidth/whole-game conversion percentage is claimed.

## Storage and retention

Original cumulative ledger: `20260906_0333_native-scene-state-bridge.md`; approved
3 GiB cap, operational floor62,509,998,080 B, other caps unchanged, raw allowance0.
No reset or repeated cleanup credit. After equivalent canonical/movement/normal
mode checks and image inspection, retired run912 log/canonical sanity JPEG,
material11/CPU10/host64 logs and empty stalled material12 logs. Ten exact files,
417,565 logical B; immediate free63,419,695,104 ->63,420,121,088 B, measured
reclamation **425,984 B**. Historical reports/hashes remain, but those exact old
captures/logs no longer do. Tests/builds are reproducible. Run911's three motion
images, standing/failure evidence and protected raw archive remain untouched.

Reusable material fixture grew45,385 B. New run/image replace old equivalents;
aggregate build logs shrink507 B, to147,849 B. The comparable fixture/log/image
set shrinks **7,771 B** overall; exe/PDB grow35,840 B. Object/build metadata,
source/docs/helpers/Git lack complete before-byte baselines and are not zero.
Images now10,240,482 B,245,278 B under the unchanged10 MiB cap. Retain only the
latest component proof until equivalent replacement; keep broader baseline and
failure coverage. Ending free at cleanup63,420,121,088 B (~59.06 GiB), drive-wide
gain290,381,824 B from initial read-only preflight, not all attributable to task
cleanup. All owned producers terminal; no new device run or full desktop gate.
