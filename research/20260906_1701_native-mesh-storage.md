# Bounded persistent mesh storage before eager geometry imports

2026-09-06, EDT. Previous goal turn made progress through `69de326` and its
corrected post-event field check. This closes the next storage prerequisite;
native model geometry/instances/direct scene+shadow submission remain required.

## Active implementation

The existing importer had a 256 MiB GPU arena limit but unbounded disk writes.
`NativeMeshDiskCache` now replaces its local file reader/writer. It preserves
BDMESH v1 payloads, source-derived content keys and filenames. No generated
code, gameplay, shader, GPU layout or native geometry upload behavior changes.

Defaults are **256 MiB logical payload, 16,384 files and 20 GiB free space**.
The incoming file's entire size plus 64 KiB allocation/metadata headroom must
fit above that reserve. Byte/file caps are independent of the GPU arena and
count invalid/foreign regular files. The flat scan stops at the first excess;
partial inventories are explicitly reported, not presented as complete totals.

The non-waiting `.bdmesh-writer` directory lease follows the material library's
existing policy. Inventory and writing share the lease across cooperating
instances/processes; restart cannot reset the budget. Stale/held leases are
never stolen. Ancestor reparse paths, unknown subtrees/links and hard-linked
repair targets refuse writes. Different valid payloads under one key are not
overwritten; identical files are reused without changing their timestamps.
Only invalid single-link derived files may be repaired. Unreadable does not
mean invalid. New names use exclusive creation; failed new partials and owned
empty leases are cleaned up. A process crash can leave a lease requiring owner
review; this protocol does not control hostile/non-cooperating external writers.

The runtime uploads native geometry before attempting persistence and retains
that result even when writing refuses. This is source/wiring evidence, not a
new live GPU disk-full experiment. Separate `[native-mesh-disk]` counters expose
write/budget failures, conflicts and the last observed inventory. Disk-full
does not evict cache files or deliberately route the draw through guest rendering.

This does **not** move geometry import out of replay yet. Guest buffer/declaration
wrappers, per-draw templates, native instance producers and canonical independent
layout associations still need conversion. Existing key validation remains the
importer's responsibility; the storage class validates file structure/checksum,
not an independently reconstructed content identity from a guest-free asset ID.

## Verification

Expanded the existing `tools/native_mesh_test`, reusing `out/native_mesh_check`:

- Original topology/format/bounds/corruption checks plus tiny disk fixtures for
  byte/file/zero/reserve limits, whole-payload/overflow headroom, restart, unchanged
  reuse, source-free read with writes disabled, conflicting valid payloads,
  truncation/corruption repair, foreign files, partial inventories, held leases,
  unknown subtrees, hard links and linked ancestors. Symlink case executed.
- Eight simultaneous two-instance writer trials under a one-file budget: exactly
  one complete file survives and no lease remains. Not a crash-injection or
  separate-process contention test. Private fixtures unwind before failure is
  reported; no disk-filling test or persistent fixture asset set.
- Debug build/CPU/cache checks 01 passed. Reconfigured only this standalone tree
  as Release to reduce diagnostic retention; all checks use explicit failure
  conditions, not disabled `assert`. Release 02 passed; 03 additionally catches
  exceptions to unwind fixtures rather than produce unhandled crash dumps.
- Final fixture build 03 PID 30212, CPU 03 PID 27704: **0.12 s** suite, pass.
  Read-only cache check 03 PID 29512 loaded all **3,510 files /36,510,144 B**
  without source memory, game/GPU/runtime, leases, repairs or cooking. Cache
  count/bytes remain unchanged. Both earlier cache checks also passed.
- Fixture executable **188,928 B**, SHA-256
  `055775a65b952f6fabc89bc00a2bb8398b5c21c56571070e1381423168d83b6c`.
- **124 source guards** pass (0.028 s final). Four new guards verify active
  reader/writer wiring, native-result retention after failed persistence,
  source/GPU-independent storage and separate disk refusal reporting.
- Host build **43**, PID 27644/session 14558, passed; log 16:54:22..16:54:35.
  Source `69de326` plus reviewed integration. Codegen reports zero files written;
  host objects only, no guest/shader rebuild. Binary **47,821,824 B**, SHA-256
  `5128c724068a2dd84521b7eb4c8e90de8aaf336149d527d5a3f45925b3ae9fd8`.

All producers are terminal. No game/XR/Quest run, captures, perf CSV, shader dump,
download or new cooked game asset. Owner profile remains 116 B and unchanged.
Existing field/material, normal Toon flat/XR and unresolved rendering evidence
remain the live/visual reference; this I/O-only change does not requalify them.

## Storage

Initial inventory free **65,047,642,112 B**; build preflight **65,019,232,256 B**,
then permission-enabled launch **65,005,232,128 B**. No renderer/build producer
or recent large verification/runtime output was found during the intervening
source work; that volume decrease is unattributed, not new mesh data. Reserved
<=256 MiB compile/link overlap and <=4 MiB test-tool/log growth under the original
cumulative 2 GiB/100 MiB/10 MiB limits. Tiny temporary fixtures share that budget.

Original mesh fixture exe/PDB/objects: **3,402,863 B**. Expanded Debug briefly
reached 7,038,207 B; final Release exe/objects total **781,738 B**, or **2,621,125 B
less than the original fixture**. Replaced objects in place; no second build tree.
The unused Debug PDB is not referenced by the Release link flags and was removed
only after Release CPU/cache verification passed.

All new build/test/verify logs total **7,971 B**, including every attempt. Removed
that obsolete PDB and sixteen superseded logs (mesh build/CPU/cache 01/02,
configure 01, and host 42 stdout/stderr). Exact current replacements are retained.
Logical deletion **3,158,859 B**; immediate free **64,970,772,480 ->64,973,938,688 B**:
**3,166,208 B measured reclaimed**, once. The old diagnostics/debug symbols can
be regenerated; no game assets, active build tree or required image/raw evidence
was deleted. Net retained fixture+log payload shrinks **2,634,477 B** relative
to this turn's starting artifacts. No increased runtime-tool reservation needed.

Post-cleanup drive-wide use **73,703,424 B** from initial inventory, not equated
with task payload (which shrank). Final source/docs/Git writes still count in
the existing cumulative scene-state ledger. Next work is actual load-owned
geometry/material bindings and native instance/direct submission, not another
first-draw cache or an unbounded bulk cook.
