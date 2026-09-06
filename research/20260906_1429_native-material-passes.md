# Native material-pass lifecycle and direct shader binding

Date: 2026-09-06, EDT. Previous turn: **progress**, establishing the completed
build and publishing status as `eccddc0`. This checkpoint qualifies and publishes
the pending five replacements. The complete desktop host-renderer goal remains
open; no Quest run or optimization was performed.

## Source mapping and ownership

Read hook definitions first (no overlapping instruction hooks), then all five
complete generated functions with their PPC comments:

| Original entry | Address / generated file | Native replacement |
| --- | --- | --- |
| `bdBeginRenderPass` | 0x821324C0 / 70 | Save live shader recipes, publish mode, select recipe and invoke first matching participant |
| `bdEndRenderPass` | 0x82132570 / 92 | Invoke participant, reread/restore saved recipes and bind shaders; no active-mode stack pop |
| `bdRenderPassSetTextureState` | 0x821831D8 / 2 | Nonzero-mode declaration selection and sequential shader recipe publication |
| `bdInitDefaultTextures` | 0x82286A30 / 1 | Live VS/PS binding, cache and force/null rules; despite its name this body binds shaders, not textures |
| `bdSetVertexDeclarationCached` | 0x82287380 / 89 | Nonnull, changed declaration binding and cache publication |

`native_material_pass.h` owns the address-free scheduling/binding algorithms.
Participant traversal is bounded to 256 nodes, selects the first matching live
mode and reads the method only when invoking it. Shader cache publication
rereads the selected recipe after the bind; the next stage is read later too.
Begin saves and end restores are sequential, preserving callback-sensitive
alias behavior. Forced shader binding does not rebind an already-null stage
with a null recipe; null declarations leave the previous declaration intact.

`native_material_pass_bridge.cpp` installs five whole-function raw hooks in the
existing host OBJECT target. It imports the remaining authored recipe/registry
words with checked big-endian reads, preserves/restores callback stack space,
and calls `Video::SetVertexShader`, `SetPixelShader` and `SetVertexDeclaration`
directly. The supported path no longer dispatches the original five bodies or
their D3D setter endpoints. The normal-decoding specialization update now lives
under the shared declaration binder's mutex, so native and compatibility callers
cannot diverge. `bd_native_material_passes` defaults to true.

Compatibility remains explicit for disabled/preflight-refused calls; execution
does not replay the original parent after native side effects. Registry nodes,
active mode/cache mirrors, shader recipes, resource handles/layouts, authored
participant callbacks and shader-register ABI **are still adapters**, not native
scene/material storage or complete removal of guest rendering. The native path
counts participant callbacks separately; these remaining bodies must be replaced.

## Verification

Guest-source guided the exact mapping, devloop the reused host/CPU build evidence,
and vrsim the desktop OpenXR check. No generated source, shader, asset, dependency,
profile or build configuration was changed.

- Host build **36**, PID 19580 / session 24991: exit 0, linked 14:12:42. Source
  is `4dc37cf` plus these edits (later `eccddc0` changes only docs); Plume is
  published `3094b35`. Codegen reported one up-to-date module, no writes, no
  guest object/shader rebuild. No new build was needed for this continuation.
- CPU fixture **07**, PID 25344: exit 0. Suite **29**, PID 27592: **31/31**, 7.10 s.
  New fixture cases cover 162 shader/force/cache combinations, null declaration
  behavior, mutation during binding, sequential save/restore aliases, first-match
  callbacks, bounded/cyclic chains, absent participants and exceptions without
  replay. The actual shared templates execute; GPU pixels are separate evidence.
- **111 source guards pass**, rerun with `python -B`; diff whitespace check passes.
- Normal flat **log 883**, PID 27228 / session 16292, 14:22:56-14:24:12:
  all five settings audited, capture disabled; sampled native begin/end
  **616,830 /616,829**, 7,310 recipe calls, 416,895 shader calls, 5,093 declaration
  calls. Direct binds: 141,775 VS /134,859 PS /13,511 declarations; remaining
  participant callbacks **15,056**. Compatibility/refusal/fault counters all zero.
  1,068,577 native parameter blocks /841,816 imported words /zero full legacy
  blocks. Sorted scheduler: 79,513 primitives; deferred scheduler: 5,362 empty
  calls, **no nonempty authored effects**. Counters are periodic, not exact
  end-of-run totals; the begin/end difference is sampled inside an active pair.
- Inspected the one **1920x1080** normal field window: Shu, his shadow, foliage,
  rocks and background DoF look consistent. This is **one regression image**, not
  a stability sequence, authored effect comparison, both-eye or full-game gate.
- Initial XR **log 884**, PID 26416 / session 99036, stopped at 301 water updates
  with 303,353 matching parameter blocks, but that parameter sample predated
  the qualified field marker; the last camera sample was startup. It did not
  replace field-XR evidence. Strengthened the existing verifier to require a
  nonzero field-camera sample followed by fresh matching parameters, with a
  45 s deadline and 400 KiB log cap. The actual verifier rejects the stale log.
- Qualified XR **log 885**, PID 18080 / session 13685, 14:29:17-14:29:46:
  all **17 settings** audited; simulator 1440x1584 per eye, XR scale 1.0,
  layered multiview, camera mode 2, mirror/capture/perf CSV off. At 14:29:40,
  game camera (-223.8,116.5,-5.0), eye (-227.2,117.1,-5.1); fresh 14:29:45
  sample **2,083,519 matching blocks, zero wrong, zero full legacy imports**,
  713,540 imported words. Material begin/end 159,375 /159,374; recipe/shader/
  declaration calls 12,112 /226,095 /12,960; direct VS/PS/declaration binds
  99,768 /82,916 /16,643; remaining participant callbacks **2,400**. Zero
  material compatibility/refusal/fault counters. 601 water updates and 73,589
  sorted primitives sampled. No XR eye pixels were captured; this qualifies
  field execution and parameter agreement, **not both-eye appearance**.

Both retained runtime logs have zero config/runtime error matches. All owned
producers are terminal and the original 116-byte profile is restored byte-for-byte.
Required fields/battles/cutscenes/menus/transitions/reloads/animated-effects and
both-eye qualification remain open before Quest. No FPS speedup is inferred
from these cumulative counters or monitor-paced timings. The flat perf pair
`perf-20260906-142258` is 606,320 B; earlier speed evidence remains historical.

| Current artifact | Bytes | SHA256 |
| --- | ---: | --- |
| Host executable | 47,778,304 | `3e85baa70f1f1cc03ae692e0259f945c5f150629234dd0e195a362deb1a21fb0` |
| CPU parameter fixture | 572,928 | `4f3c6c4e8454319b763f78c7dd1d4ee3841a81cbf4688f4336968c6bf5b784c6` |
| `native_material_pass_window.png` | 3,364,482 | `63e30379b247585ad474ca8ec6f617ff913a842d8b088107ac124f403b5e3ff0` |

## Storage and publication

Continue `20260906_0333_native-scene-state-bridge.md`; the original cumulative
budgets are not reset. Material preflight free 65,236,197,376 B; continuation
preflight 65,233,252,352 B. Reused builds/tools, zero new raw frames, cache/HLSL
files, cooked assets, downloads or trees. Runtime logs across all three attempts
total 966,032 B, within the <=1 MiB estimate. Existing supervisors retain reserve
and aggregate limits and restore the profile in guaranteed cleanup paths.

After validating equivalent replacements, removed **12 exact ignored,
reparse-free agent-created files**: build 35, fixture 06 and suite 28 log pairs;
flat log 882, perf-135502 pair, deferred-visual PNG; previous XR 881 and stale
XR 884. No game data, profiles, trees or distinct original-UI/non-MSAA/unresolved
failure evidence removed. These diagnostics can be regenerated; historical
results/hashes remain in their reports, not in duplicate raw outputs.

Logical removed **4,878,620 B**. Immediate measured savings: 4,243,456 B for
the first batch, 643,072 B for the second, **4,886,528 B total**, counted once.
Gross new log/perf/PNG payload 4,943,337 B; net retained growth **64,717 B** for
new binding counters, stronger field-XR sampling and the replacement PNG size.
Keep one current flat/XR evidence set; retire it after equivalent future
qualification. Small verifier changes and the +22,528 B CPU fixture fit within
the existing tools reservation; the host executable grew 8,192 B.

Final reservation **64,454,021 B**: 94 build logs /140,120 B, 14 runtime logs /
3,994,262 B, 20 perf files /8,963,168 B, eight GPU fixture files /8,364,855 B,
fixed 41 MiB tools/inspection. Two PNGs /6,698,638 B fit that inspection reserve.
Post-cleanup free **65,211,969,536 B**: drive-wide use +24,227,840 B since
material preflight, including an unexplained 19,922,944 B drop not accounted for
by the scoped project-output inventories. Do not claim it as renderer output or
cleanup. Later docs/Git writes count; final handoff reports ending free space.
Normal commit/push to the named fork is covered by standing owner approval.
