# Bounded persistent native material storage

2026-09-06, desktop. A prerequisite for extending native material ownership:
the existing library capped RAM residency but not persistent file growth.
This change applies to the active native material library and standalone cooker,
not just an unused helper. The all-rendering/desktop/Quest goal remains open.

## Source and implementation

Read the full guest-source/devloop skills, current acceptance checklist, material
format/library/data/import sources and the persistent-material checkpoint from
2026-09-04. Water investigation also read the complete original constructor
`sub_82453AE8` (generated file 100), initializer
`mcl__water_object__vf04_body` (file 13), its predictor-hook TOML, and the current
water update/setup and host parameter bridges.

The initializer establishes two three-vector blocks at parameter rows 50-52.
The native water update/setup still publishes through guest parameter storage;
the host parameter bridge itself still writes the emulated device constant file.
Replacing the initializer alone would not establish native parameter ownership.
Water's parameter owner, consumers, native asset identity and shader ABI remain
the next data-flow work, not an achieved conversion in this checkpoint.

`NativeMaterialDiskBudget` adds independent defaults of 1 MiB logical payload,
4096 files and 20 GiB free space, plus 64 KiB allocation/metadata headroom per
write. Existing files, including invalid/unknown regular files, count across
restarts. The scan is flat and bounded: excess bytes/count or an unknown subtree
or link refuses further writes. Invalid hard-linked targets are not repaired.
Logical byte/file ceilings do not purport to measure actual allocated bytes.

A non-waiting `.bdmat-writer` directory lease serializes cooperating runtime/
cooker writers around inventory and write. New filenames use exclusive creation.
A competing or stale lease is never stolen. A crash can leave a lease requiring
owner review; arbitrary external writers are not controlled by this protocol.
Valid files are reused, invalid single-link derived files can be recooked within
budget, and no disk file eviction is performed to make room. Failed new partial
writes are removed by their owner; the lease is released through RAII.

Disk refusal does not discard the native resident material or invoke guest
rendering. Existing valid content IDs remain loadable with writes disabled.
RAM pinning/eviction rules are unchanged. Separate disk-refusal counters and
last-observed file/byte totals are exposed in `[native-material-disk]`; a partial
scan is explicitly labeled incomplete. Write failures include persistence
refusals. The cooker already exits unsuccessfully if requested persistence fails.
No file format/version/content ID change was needed.

Limits apply to this material cache only: texture/mesh disk retention and complete
scene/material ownership still need work. There is no whole-installation budget
or new visual/performance/fully-host-frame claim.

## Verification

Root `0bc90d0` plus this change and existing pending renderer integration;
clean local Plume `3094b35ae2e53207d557532748cf2ac7c96a5035`. No generated source,
hook TOML, game assets, profiles or dependency gitlinks changed.

- Reused `out/native_material_test`. First focused build 01/PID 1492 and CPU
  01/PID 26724 passed. Added real two-library contention coverage, then final
  focused build 02/PID 26792 and CPU 02/PID 22152 passed: 1/1 CTest, 0.11 seconds.
  This executable also retains the existing decoder, composition, format,
  corruption, identity, skin/reflection, lifetime and pinned-residency cases.
- New cases cover independent byte/file limits, RAM eviction versus disk
  retention, restart accounting, usable memory after failed persistence,
  source-free loads with writes disabled, absent uncooked IDs, zero budgets,
  impossible reserve, unknown/partial files, bounded incomplete inventory,
  repair within an exact budget, held/stale leases, unknown directories, invalid
  hard-link targets and symlinks. The symlink fixture executed, not skipped.
- Eight two-thread/two-library trials share a one-file budget. Both resident
  assets remain usable; exactly one complete disk file survives, one write
  refuses, and no lease remains. These are in-process competing library
  instances, not a crash-injection or hostile-external-writer test.
- Final test executable 346,624 B, SHA-256:
  `23612629c3d1ae12ac189d94fbecb46a34ff6c039177db702f60bb0ccd994ed5`.
- Cooker build 01/PID 25140 passed. Read-only `--verify` loaded all **30 actual
  material assets** by content ID without the game/runtime/GPU; composable
  diffuse/specular/reflection counts **30/30/7**. The actual cache stayed at
  30 files / 2,040 logical B. Cooker 593,920 B, SHA-256:
  `95b8678442321b6622dec2ebfb8e87fb4110a54309c3316d1721cfeb888decd0`.
- Desktop build 26/PID 27524/session 74032 terminated with exit 0. Codegen:
  zero written, one module up to date; no guest objects or shaders rebuilt.
  `reblue_vk.exe` linked 07:55:05, 47,713,792 B, SHA-256:
  `1fd4c98edd6d995b37ab9dc4da893583ba0860f8e9f73c89cb1144898f2f121e`.
- Wrapper parse and focused diff checks pass. All producers are terminal; no
  private material-test directories from this step remain. The owner profile
  stayed unchanged. No game/XR run, screenshots, raw frames or Quest work.

CPU fixtures use explicit zero free-space reserve in their private scratch
directories so low-space CI can test tiny logical limits. The application default
remains 20 GiB; an impossible-reserve case verifies refusal. No disk-filling test.

## Storage and retention

Continue the original ledger in `20260906_0333_native-scene-state-bridge.md`.
Preflight 07:45:04: 64,439,558,144 B free; original baseline 65,462,788,096 B.
Step plan: <=256 MiB build/link overlap, <=1 MiB private temporary fixtures,
<=1 MiB new diagnostics, including retries. Reused both configured build trees
and existing tool installations; no new asset outputs or downloaded tools.

Gross new diagnostic logs **5,934 B**. Validated exact ignored paths, sizes,
reparse-free ancestors, successful replacements/hashes and absent producers,
then removed six superseded logs: reblue_25, native_material_test_01 and
material_cpu_01 stdout/stderr pairs. **4,184 logical B removed**; immediate free
space 64,434,925,568 -> 64,434,933,760 B, **8,192 B measured increase**. These
logs can be regenerated; their recorded results remain. No protected data,
runtime/CPU/GPU capture evidence or build tree was deleted.

Final retained build logs: 92 files / 139,957 B. Net retained diagnostic growth
is **1,750 B**; reserved checkpoint accounting becomes **63,963,741 B** with
all other diagnostic/tool reservations unchanged. The additional material
test/cooker logs establish the new storage coverage and are replaceable after
equivalent verification, not permanent per-commit copies.

At cleanup completion, free **64,434,933,760 B (60.01 GiB)**; drive-wide use is
up **4,624,384 B** from the step preflight and **1,027,854,336 B** from the
original baseline. These volume deltas include build/Git/metadata and unrelated
activity, not solely retained artifact growth. Later documentation/Git writes
still count. No new raw allowance was used. Publication remains subject to the
existing GitHub upload approval; no alternate route or push was attempted.
