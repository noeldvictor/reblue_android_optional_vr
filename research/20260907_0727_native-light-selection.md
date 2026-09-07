# Host object-light selection and scoring

2026-09-07, parent a2004cb. Supported `sub_8218A8C8` execution now computes its
selection on the host, including full/changed-list updates, filtering, scoring,
priority insertion and category publication. This is the named light-selection
dependency for the direct rigid object, **not a direct native game draw**.

## Contract and source

The guest-source skill directed inspection to the complete translated selector,
rebuild, insertion, score, category and ray/angle helpers before implementation:

- `generated/reblue_recomp.12.cpp:4365`, `sub_8218A8C8`.
- `generated/reblue_recomp.85.cpp:4255`, `sub_8218A808`.
- `generated/reblue_recomp.58.cpp:4341`, `sub_82189F40`.
- `generated/reblue_recomp.18.cpp:3727`, `sub_82189358`.
- `generated/reblue_recomp.44.cpp:4230`, `sub_82189E70`.
- `generated/reblue_recomp.34.cpp:4125`, `sub_82184A38`.
- `sub_8213F1D8`/`sub_8213F240`, files71/20; angle wrappers
  `sub_826C29F0`/`sub_826C2830`, files74/18.
- Existing node callback `sub_82142C58`, selected-light publisher, object scope,
  native rigid pass, frame-interpolation metadata and native shadow source bridge.

`NativeLightCandidate`, `NativeLightSelectionInputs` and the three selected slots
are CPU-owned named values, without shader registers or source addresses. The
bounded source adapter prepares five compatibility words transactionally, with
at most300 snapshot/changed candidates. It preserves unrelated dirty-view bits
and neighboring category bytes. It rejects missing/overflowing data, unsupported
object classes, relevant invalid numeric inputs, and source/output/stack aliases
before stores. No persistent light cache or disk producer was added.

Equal scores retain authored order. Priority lights precede ordinary lights;
the designated priority ID can precede other priorities. Directional-only mode
preserves category and its zero-weight priority semantics. Disabled mode clears
IDs without clearing weights/priorities/category. A changed ID already present
in the **evolving** selection triggers a full rebuild, even if a previous changed
record just inserted it. Classification uses the live owner's kind/priority,
not the potentially older scoring snapshot.

Point/spot scoring uses explicit float rounding, ordered NaN attenuation,
near-zero ray behavior, spatial cone exclusion and safe byte quantization.
Native `acos` replaces the original angle helper; exact pathological cone-edge
equivalence is not established. Spatial/spot cases have focused CPU coverage;
this field run is not evidence for every authored light family. Relevant invalid
inputs refuse the native path visibly rather than inventing defaults.

`bd_native_lighting=true` selects the host route. Comparison mode calls the
original once, checks all five output words exactly, then applies the prepared
result. Disabled/unsupported input retains an explicit original fallback. The
field scenario requires fresh updates, actual rebuild/candidate work, complete
comparisons and no fallback growth; startup-only success cannot pass.

The existing shader callback still invokes the producer. The next direct node
route must invoke selection/publication before interpreting/capturing anything,
with whole-node scene/shadow preflight. Authored light animation/snapshot/storage
and compatibility writes remain; this is not the final native light-world owner.

## Verification

The devloop skill kept the inner loop in existing trees: material29/PID27160 and
CPU27/PID26168 pass,0.11 s fixture/0.13 s CTest. Tests include128,000 candidates
against an independent stable-ranking reference, spatial score cases, dirty and
incremental selection, live category, bounds/aliases and the actual boundary:
selector output -> existing descriptor publication -> owned lights -> native
GPU-pass packing after source storage is destroyed. No GPU draw is claimed by
that CPU test.152 focused Python checks and267 all-boundary checks pass.

Host91/PID29640 passes. CMake reconfigured after header glob changes and refreshed
version-dependent host objects; codegen reported0 written / module up to date.
No guest objects or shaders rebuilt. Run933/PID31508,07:26:37-07:27:38, uses that
binary and passes the existing complete field gate plus `--light-selection`:

| Fresh post-event windows2017/2317, after event1417 | Delta |
| --- | ---: |
| Host light-selection updates / exact original comparisons | 14,332 / 14,332 |
| Full reselections / candidates | 36 / 9,936 |
| Selection mismatch / compatibility growth | 0 / 0 |
| Owned light publications / changed slots / normal-lit draw checks | 14,332 / 900 / 1,336 |
| Owned sampler checks / draws | 3,887 / 41,439 |
| Instance comparisons / object-input comparisons | 114,193 / 80,528 |
| Shadow receiver comparisons | 15,501 |
| Observed movement | 51 samples / 66.022831 units / 13.984 s walking |

All15 temporary settings audited; exact116 B owner profile restored. No new
raw/perf/cache/dump/cooked files and no owned process remains. The1920x1080
144,660 B JPEG was actually inspected: running Shu, ground, vegetation, fence
and shadows coherent; known cliff marks and distant blur remain. One flat image
does not qualify sequences, reloads, other scenes or both eyes. Direct native
rigid program use and source-free GPU loads remain0; no speedup claim.

| Artifact | SHA-256 |
| --- | --- |
| Host91 exe48,427,008 B, tested by933 | `40515EC44D36D28693D1E92BA64B9E732E2E473F600EDD17293A26980BA243A0` |
| `out/build/win-amd64-release/logs/reblue_933.log`,249,997 B | `46DEE7C01FEE0D0D1F1B40A540EF2FA9888385127EF88775134AAC0020497BD3` |
| `out/verification/native_light_selection_window.jpg` | `916C26FE1B0B9E2EAA8A21DC344AA1680240068024B6438AB1A531211DC48A0C` |
| Restored profile | `2F1BC38D763A1B7BDBA31F560684FD4AA7E42A714600B8D344F19DA7F38E23B0` |

## Storage

Same original cumulative3 GiB exception/floor/raw0 gate; no reset. The build
supervisor enforces a256 MiB free-drop stop and runtime192 MiB,75 s,400 KiB log,
one160 KiB JPEG/10 MiB aggregate images. Reuse existing trees and selected asset.
After replacement verification, eight exact superseded agent-created files were
removed: host90, material28/CPU26 stdout/stderr,932 text and sampler JPEG.
373,861 logical B / **389,120 B (380 KiB) measured reclaimed**. Reproducible build
logs and the exact retired runtime text/image are gone; hashes/findings remain
in prior research. All distinct protected GPU/baseline/VR/motion/failure/raw
evidence and game data remain intact.

Known retained component growth277,258 B supports new code/fixtures and replacement
evidence, not an extra evidence set. Ending free58.62 GiB; drive-wide free rose
64.48 MiB from the first source-only measure, mostly unrelated/unattributed
activity, not cleanup credit. Other object/source/Git deltas are not fully
attributed. The existing cumulative ledger records measurements and retention.
