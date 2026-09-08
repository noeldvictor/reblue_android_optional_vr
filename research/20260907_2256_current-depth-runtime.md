# Current-depth visibility connected to native rigid draws

2026-09-07. Desktop Vulkan only; no full-host-frame, stereo, Quest or speedup
qualification. This continues the [GPU contract](20260907_2209_native-depth-visibility.md).
The cumulative storage ledger remains
[the existing checkpoint](20260906_0333_native-scene-state-bridge.md).

## Ownership and consumer change

Runtime source4c890ad connects the tested maximum-depth pyramid and indexed
command generator to the real native rigid queue. Each retained batch carries
its native depth owner, indexed world bounds and exact view projection. Batched
bounds are the union of every sibling; any uncertain sibling keeps the whole
command visible. Camera/depth identity changes split batches.

One shared64 MiB/16-owner budget covers both in-flight frame slots, including
indirect and readback buffers. Within one recording, the same depth work is
reused. Current depth is refreshed at queue-flush entry and after intervening
non-native draws, which may write depth non-monotonically. The native segment
uses depth-tested nearer writes; an older maximum pyramid is conservative
within it. Compute interruptions invalidate all cached emitter bindings before
the native shader draws. No CPU visibility wait is inserted between generation
and indexed-indirect consumption.

All siblings and authored light side effects still preflight before submission.
The production temporal query pools/history/filter are removed, including their
begin/collect/end-pass calls. Historical query math and the proxy fixture remain
as regression evidence, not a production fallback. Sun occlusion is independent
and unchanged. Native rigid scene rendering is still an opt-in integration path;
other renderer families retain their tracked guest/compatibility boundaries.

## Output is distinct from command generation

Per-item receipts require exactly one actual draw recording, then exactly one
visible/culled resolution and one retirement. The three submission paths seal
work before ending the command list. Existing frame-slot fence completion
precedes GPU receipt collection; packet/descriptors/images stay retained until
classification and retirement complete.

Only nonzero GPU instance counts advance scene, cutout and selected-regression
emission counters. Culled records still retire their source resources. Logs
separate visible retirement, culled retirement and all resource retirement, with
exact submitted = visible + culled + pending conservation. The900 actual visible
emissions per cold/reloaded epoch and250 ms continuous readiness remain unchanged.
Generation alone, a zeroed command or retirement alone cannot satisfy output.
Current-depth observation additionally needs fresh generated/draw-recorded/
fence-collected/visible/culled deltas in the ready field. It cannot qualify reload
or image correctness by itself.

## Build and fixture evidence

- Output42/PID38292 builds; CPU25/PID34036 passes the expanded C++ lifecycle,
  camera/depth batch separation and uncertain-sibling bounds assertions in
  0.37 s (0.40 s CTest).
- 339 artifact-free Python guards/scenario tests pass.4c890ad's one stale guard
  is corrected to check the current selected-regression condition and ensure
  the culled return precedes actual emission; no threshold is weakened.
- Host122/PID21964 fails during configure because the backend-only list still
  named deleted `occlusion_cull.cpp`. Remove it, list Vulkan-aware
  `native_depth_visibility.cpp` in the backend sources and guard both facts.
- Host123/PID23020/session42964 then links successfully. Its actual build stamp
  is4c890ad dirty, not a later commit. No guest objects rebuilt; the codegen
  dependency check ran. The shaders remain those verified by72 GPU cases,
  46 rigid-shader cases and8 query regressions in the previous checkpoint.

| Host123 artifact | Bytes | SHA256 |
| --- | ---: | --- |
| `out/build/win-amd64-release/reblue_vk.exe` | 48,731,648 | `292D88305D92DB97FE46ED333B21220F4AD2298BCE305876847F6CAA033C3395` |
| `out/build/win-amd64-release/reblue_vk.pdb` | 109,654,016 | `F70D257023CCAAC93851795229B4CD002F819140867EC827516AB63B6F5C9DC0` |

## Live integration gate

Run962/PID27128/session52256 began23:05:05 with host123. The inspected supervisor
uses the complete receiver/lighting/caster/cutout cold/reload chain,300 s and
800 KiB text, no raw/perf/dump/cook and at most one110 KiB mono JPEG. The observation
trigger now reads `[native-depth-vis]`. It is not the historical
`run_scene_handoff.ps1` helper. The supervisor terminates0 at23:07:28 and all21
profile settings took effect; the owner profile is restored exactly.

Cold generation93/instance144 qualifies scene756->1656 and shadow757->1657.
Title teardown closes both at1658/1658/1658 submitted/emitted/retired. Reloaded
generation207/instance437 qualifies scene786->1686 and shadow787->1687 at23:07:23.
The independent receiver/owned-lighting/caster/cutout checks pass. The current
source receipt counting does not hide culled work as visible output.

The fresh reloaded4634..4934 window has300 new snapshots,3,248 generated and
draw-recorded commands,3,247 collected receipts,3,236 visible instances and11
culled instances. Cumulative culled12 includes one earlier event result; only
the fresh11 count as interactive-field culling. Scene totals conserve pending
work; this sample has singleton native batches (retain959's distinct merging
proof). No timing comparison or frame speedup is claimed.

**Pixel acceptance fails.** The97,976 B PrintWindow JPEG shows terrain, trees,
shadows and a large title logo rather than the expected moving player view.
The log still records FieldActive/event0 and walking through23:07:27. Neither
stale window capture nor incorrect presentation is established as the cause.
Supervisor exit0 means the text chain and image production completed, not that
the image passed visual inspection. Keep945 accepted mono pixels and956's tree
gap; the new JPEG is additional unresolved-discrepancy evidence.

| Run962 evidence | Bytes | SHA256 |
| --- | ---: | --- |
| `logs/reblue_962.log` under the build tree | 521,322 | `00876D1ACDF8A40306DB32143E2498BDDF1386F1F28F84A6276D1FCD6566156C` |
| `out/verification/native_occlusion_window.jpg` | 97,976 | `5F032D349E1704505A4D5CB6ADBE8615EB7571FE1EB7E0F46E0DA2A9847CAB57` |
| Restored `profiles/default/reblue.toml` | 116 | `2F1BC38D763A1B7BDBA31F560684FD4AA7E42A714600B8D344F19DA7F38E23B0` |

Next decision: use the existing renderer-owned `ServiceOnPresent` screenshot
path with an identified frame/fence and bounded encoded output to compare the
actual post-gamma back image with the observed window image. Review its current
present-index retirement assumptions before reusing them. No unchanged boot,
relaxed scene gate or new raw archive is authorized by this failure. This leaves
controlled moving-view/hidden/visible pixels and all broader desktop gates open.

## Storage and retained evidence

The first measured free space was80,008,998,912 B. Existing fixture and log sizes
were unchanged from the prior checkpoint; no producer remained live. No cause
is assigned to the intervening drive-wide decline. The original3 GiB exception,
62,509,998,080 B floor,100 MiB diagnostics and10 MiB attachment-log caps remain.

Runtime preflight initially refused the planned overlap. Retained940/949/951
logs are now losslessly stored in
`out/build/win-amd64-release/logs/retained-native-runtime-940-949-951.zip`.
All three entry lengths and SHA256 were checked against the originals before
removing plain copies:1,148,083 B becomes196,094 B (951,989 logical B saved).
Both supervisor preflight and live totals include that archive. Baseline/failure
evidence is recoverable byte-for-byte, not discarded or excluded from accounting.
Eight superseded output41/CPU24/host121/host122 stdout/stderr logs were removed
after replacement passes,5,479 logical B; the resolved configure failure remains
documented above. No raw/image, game asset, save or profile was removed.

Counted retained artifacts shrink269,175 B overall after compression/cleanup,
despite the new521,322 B runtime log and97,976 B discrepancy JPEG. Texture fixture
growth84,880 B and net attachment text27,788 B are included; GPU fixture unchanged.
Image archive10,411,120 B leaves74,640 B headroom, so another image needs a new
bounded preflight and eligible reclamation if necessary. No incoming raw allowance.
Ending free79,974,555,648 B is34,443,264 B below the initial reading; this is volume
activity, not solely attributed retained growth. All owned producers are terminal.

Delivery:9e7ee1e is pushed.4c890ad and this continuation remain local pending
payload-specific approval after security review rejected the newer push. Do not
retry or bypass that rejection without new authority. This does not block safe
local renderer work or mean the full host-renderer goal is achieved.
