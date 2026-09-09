# Bind-owned animated material descriptors

2026-09-09. Production source f6e254e; host192 banner2691882dirty. Final extra
generation/reload fixture coverage changes no production behavior after that
binary's live observation. Full host-frame and desktop acceptance remain open.

## Dependency moved into native ownership

The actual material/joint binder, bdVisualObjectInitDebugDraw, now publishes
NativeMaterialUVProgram after its original name resolution. It carries native
selectors/channels, dense joint identities, rates/divisors, angular units and
authored eye origin/limits. Controller and eye evaluation consume this immutable
program, publish shared native UV values, and feed subsequent ticks and the real
material importer/composer/scene packets. There is no additional source-address
index, disk cache or renderer framework.

The existing NativeInstanceRegistry owns programs alongside UV/pose data, under
the same16MiB limit. EnsureInstance shares identity creation between bone and
binding producers; a loaded native model supplies the generation. Valid joint
drivers must resolve within its owned skeleton. Rebind clears old descriptors
and UV visibility before original binding; publication failures never replay it.
Pinned descriptor allocations remain charged through replacement and retirement.
Identical bitwise programs reuse their lease, including signed-zero identity.

This does not remove every source read. Original name binding is still a
load-time adapter. Before native descriptor reuse, MatchesProgram re-decodes
source fields solely for late-write comparison; any mismatch invalidates the
program and UV output until an explicit binding publication. Normal evaluation
does not use those source values as input. Unconverted writers retain the prior
explicit route. Removing validation before those writers migrate would freeze
authored updates, not complete ownership. No speedup is inferred from this change.

Contracts reused from [parser/binder evidence](20260908_2331_effect-input-coverage.md)
and [complete eye callers](20260909_0000_native-eye-materials.md). Reread the complete
binder (generated92:2605..2760) and relevant pso_predictor/frame_interp TOMLs.
It resolves material name+88 against first-match texture records, and joint
name+120 against the scene graph, producing selector+4/joint+12. No generated or
hook TOML source was edited. The original body executes once; its output is the
import boundary, not proof that original load-time work has been removed.

## CPU and build evidence

424 artifact-free Python source/scenario checks pass (0.227s); wiring only.
Material77/CPU74 pass. Host191 stopped on a missing REX_EXTERN declaration for
the new original call; fixed with a source guard. Host192/PID36348/session80639
builds successfully through link18, codegen0writes, no guest objects or shaders.
EXE49397760B/PDB113934336B; EXE SHA256
FBE640F3A6275CC0A5AD985B7818F6C6DE7826D8D398F4821C9101CF27356229.

The production clip/controller -> descriptor owner -> UV owner -> eye patch ->
next tick -> actual material composition fixture forbids descriptor/unit reads
inside evaluation and UV reads as material inputs. It tests all three UV modes,
preserved non-eye slots, source destruction and existing material provenance.
Dedicated descriptor tests cover late rates/divisors/limits/enable/selector/
channel/joint/unit writes, no resurrection, table/count replacement, missing
input, overflow, dormant nonfinite payloads, signed zero, wrong model generation,
reload and combined program/UV budget pressure with pinned allocations.

Final material78/PID4232 build0 and CPU75/PID31084 pass0.12s (CTest0.14s).
Fixture1580032B SHA256
039F960E4808088AEE7F40046A2EB9C1798BD943746B434E7875D572FD6565E9.
No new host build/run is needed for the final test-only additions.

## Live descriptor consumption

Host192/PID30080/session13730,01:03:47..01:04:41. Existing supervisor requests
MaterialProgramProbe plus AnimatedUVProbe/EyeMaterialProbe/ControllerAnimationProbe/
LateAnimationProbe and the existing native image/model/geometry/instance setup.
Mono, captures/perf/cook off; native scene/skin/deferred on. All14settings applied.
Requested observation reached; intentional diagnostic-stop exit1, not a crash
or CTest pass. The original116B profile restored byte-for-byte, SHA256
2F1BC38D763A1B7BDBA31F560684FD4AA7E42A714600B8D344F19DA7F38E23B0.

- Descriptor owner1922:5binding publications,1878reads,changed0/refused0.
- Evaluations1663->1963:1620->1920effect slots,807->957eyes using native programs.
- UV1963:1918publications,refused0,1914owned scroll inputs;957exact eye checks,
  231off-center. Preserved non-eye slots, translation/rotation and transitions0.
- Idle contexts1638->1938: UV/eye scopes632->932,composed/packets27808->41008.
  These are repeated native primitive inputs, not unique materials or pixels.
- Controller93914matching,15604boundary refusals,11931samples,23mixes,11433interior,
  11921handoffs,changed0,12435advancing. Late6matching/reused/handoffs.
  Skeleton12525matching,unavailable0/wrong0;material19277checks/wrong0.
- Skin1640->1940 emitted/fence-retired33513->46713, separate whole-skin evidence.

No new raw/window/perf/cache/dump files. No motion pixels, live joint-driver/cue
coverage, reload/stereo, complete desktop scene/frame or Quest qualification.
Next migrate remaining descriptor/UV/image writers and then delete outgoing
source/scratch validation and exports when their final consumers are gone.

## Retention and cumulative storage

Full254910B log retained as material-program192ZIP51869B; sole reblue_981.log
member name/length/full SHA256 verified before plaintext removal. Member hash
20E47B916951C82DBB0BCD0BC4E01B7EF52675CDA02138C29034F9EC9657C3F7;
ZIP hash88080D12C4BA4DF855E2BA77B18BB1272E66D345B608464E5D0053A69DC2C3FF.
It replaces animated-uv190's verified purpose; that old archive was removed.
Historical reports remain. Keep effect187/placement988/root985 and unresolved
pixel/reload evidence, plus final78/75/192 receipts and this replacement log.

14superseded receipt/plaintext/archive files removed,322244Blogical.
Measured deletions339968B minus newZIP allocation53248B =286720B net280KiB
cleanup. Current full log is recoverable from its ZIP; old190log is no longer
retained, and build receipts are reproducible. Partial retained growth275790B:
fixture+153694,EXE/PDB+125440,ZIPreplacement+353,buildlogs-3697. This excludes
host objects/metadata/source/Git and unrelated drive activity.
Post-cleanup63556747264B free (~59.2GiB),5251072B lower drive-wide than the first
check. Only measured deletions/allocation are claimed cleanup. Diagnostics78146490B,
buildlogs419974B/212files; next400KiB overlap has87110B headroom under75MiB.
No new raw allowance or budget reset. See the
[cumulative ledger](20260906_0333_native-scene-state-bridge.md#2026-09-09-bind-owned-material-descriptors-after2691882).
