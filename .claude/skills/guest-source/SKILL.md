---
name: guest-source
description: Investigate Blue Dragon guest/render-loader behavior using existing statically translated C++, PowerPC comments, hook metadata and the source call graph before binary analysis.
---

# Read the translated program first

The local `generated/` contains the statically translated executable, including
18,777 function bodies in the recorded census. This is **not the original
high-level source project**. Register operations and interleaved PowerPC comments
are the behavior reference the application executes. Do not install a decompiler
or regenerate an existing tree just to find a function. Use deeper binary analysis
only for a specific question this source and its call sites cannot answer.

[AGENTS.md](../../../AGENTS.md) is authoritative; the
[transition queue](../../../docs/HOST_RENDERER_TRANSITION.md#active-work-queue)
sets the ownership outcome. Read relevant `config/hooks/*.toml` before the
callback, then the exact generated body and callers. Existing hooks may replace
that body: identify which path actually executes.

## Cheap navigation

```powershell
rg -n 'DEFINE_REX_FUNC\(bdSceneNodeDrawSingle\)' generated -g '*.cpp'
rg -n 'bdSceneNodeDrawSingle|8227FEE8' config src -g '*.toml' -g '*.cpp'
python -B tools/callgraph.py sites bdSceneGraphNodeProcess --limit 40
python -B tools/callgraph.py frontier bdSceneTreeDraw --depth 1 --limit 40
```

PowerShell does not expand positional `src/*.cpp` patterns for `rg`; pass the
directory and `-g '*.cpp'`. Use `rg --files` before opening uncertain paths.
Start with bounded slices, then read the complete relevant function/control flow
before concluding. Do not repeatedly load whole subsystems to locate one symbol.

The callgraph reuses a valid index and does not write by default. Optional
`--cache` needs the current storage budget. Its indirect/original-body frontier
is not a host-helper/preprocessor-complete graph or runtime hotness measurement.
A hook declaration alone does not prove activation or guest-free execution.

## Read the contract, not just an offset

- `r3`, `r4`, ... carry arguments; `lwz r11,7224(r31)` reads a field, `lfs` a
  float. Confirm layout at its writer and another relevant reader/caller; record
  ownership, count, stride, null meaning and lifetime too.
- Source data is big-endian. Use checked boundary readers and BE types; validate
  ranges/counts and complete imports before source storage retires. Never use
  upload-ring/write-combined memory as a CPU source.
- Follow indirect callbacks and `__imp__` original-body calls explicitly. Host
  C++ can still depend on source memory, register layouts and resource wrappers.
- Preserve ordering, null/override semantics and late writers. Convert producer
  -> owned data -> consumer, not every console helper one-for-one.
- Never edit/commit generated code or game data. Hook/codegen input changes can
  rebuild the guest; inspect that dependency first.

For the active static-object path, the
[ownership frontier](../../../research/20260906_1531_static-model-ownership-frontier.md)
locates loader/lifetime hooks; the transition queue maps newer native owners and
remaining shader/submission gaps. Do not restart the old player-anchor search:
`src/xr/xr_player_anchor.cpp` and `src/xr/xr_game_camera.cpp` already publish
character anchors. Inspect their current behavior only when that subsystem is
actually in scope.
