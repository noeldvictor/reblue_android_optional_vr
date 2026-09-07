---
name: guest-source
description: Recover Blue Dragon rendering and loader contracts from translated C++, hooks and the existing call graph to replace connected producer-to-consumer paths. Use before binary analysis or guest-render boundary changes.
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
sets the ownership outcome. Read the relevant `config/hooks/*.toml` before the
callback and inspect the exact generated body/callers for a new contract. Existing
hooks may replace that body: identify which path actually executes.

## Investigate a dependency, not a helper queue

Name the native consumer, its missing owned input and the producer/lifetime
boundary that can supply it. Trace the connected path far enough to implement
that contract as a bundle. Do not turn every original callee into a separate
porting milestone or make the source's dirty caches/register packing the native
design. For example, scene lighting needs owned light/object inputs, selection
and native consumption, not another permanent preview of legacy shader writes.

Reuse recorded findings when the source and hook paths are unchanged. Keep a
compact provenance record at the existing subsystem evidence: symbol/location,
active hook, data contract, lifetime/order and unresolved question. Reopen the
relevant body when those facts are incomplete, changed or contradicted; do not
repeat the same whole-function audit at every continuation. The indirect-call
and hook caveats below still apply.

Before a runtime probe, state which unresolved observation changes the design.
An inconclusive probe requires a different hypothesis or observation method,
not repeated boots until a pass. Preserve failures without letting an unrelated
legacy adapter's investigation replace work on the connected native owner.

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

For model loader/lifetime changes, the
[ownership frontier](../../../research/20260906_1531_static-model-ownership-frontier.md)
locates the original hooks; read it when that boundary is in scope. The active
transition queue maps newer owners and current gaps. Lighting, pass or shader
work should follow its own relevant sources, not load the historical model map
by default. Do not restart the old player-anchor search:
`src/xr/xr_player_anchor.cpp` and `src/xr/xr_game_camera.cpp` already publish
character anchors. Inspect their current behavior only when that subsystem is
actually in scope.
