# Canonical rigid mesh values and source-free asset loading

2026-09-06 EDT. Parent `9ab1bbd`; Plume unchanged. Previous turn was verified
test-loop progress. This checkpoint advances the native static-object data
contract, but does not complete direct object submission or qualify new pixels.

## Implementation and source evidence

Read the current mesh importer, file codec, storage owner, runtime vertex-input
owner, complete vertex-declaration builder and its format mapping. Read the
existing shader fetch/decode helpers, specialization binding, pulling and
host replay's geometry substitution/dispatch. No generated code, shader
translator, hook configuration, game asset or owner profile changed.

BDMESH v2 now carries sorted named attributes and finite little-endian float4
values in a checked interleaved stream. Its layout/content identities have no
source declaration hash, address, location or unpack mask. V1 remains readable.
The loader converts supported rigid layouts once; constrained/dynamic/unsupported
contracts keep their explicitly transitional path. Native input binding derives
from v2 alone and clears all unpack masks; dispatch clears the old packed-normal
specialization as well. Details: `docs/NATIVE_MESH_FORMAT.md`.

The compatibility conversion follows the current IA plus `swapFloats`,
`sintTexcoord` and `tfetchR11G11B10` contract, including full four-lane defaults.
Review caught absent-color alpha (one, not zero) and packed missing tangent w
(zero, not the unpacked float3 default one). The latter becomes actual zero
values in the asset, not a persisted old specialization flag. The signature
adapter's location mapping has an equality check against the existing shared
shader table; it is not the final native shader ABI.

`LoadNativeGeometry(content_id)` reads, checks, binds and uploads a native asset
without declaration/source storage. The game's current producer still imports
through source wrappers and the scene still consumes replay templates; no direct
object caller or runtime source-free load count has been verified. That remains
the next ownership path. Float4 expansion may increase vertex bandwidth/storage;
no overall speedup, compact asset budget or headset qualification is claimed.

Both file versions share the original 256 MiB/16,384-file cache budget/path;
there is no recook of the installed library. GPU arena remains separately bounded
to 256 MiB. Added explicit metadata limits of 16,384 geometry owners and 32,768
import-key aliases; direct native loading does not use those aliases.

## Verification and failures

- Mesh build05 /PID29192 failed on Windows' `max` macro; corrected the qualified
  function invocation. Build06 /PID29976 passed.
- CPU05 /PID29384 rejected the half-float fixture: only its first vertex had
  been changed, leaving half NaNs in the others. Corrected all three vertices;
  the nonfinite guard was not weakened. Build07 /PID31040 and CPU06 /PID17900 pass.
- Final build08 /PID26768 and CPU07 /PID26192 pass. CPU test 0.11 s, ctest 0.12 s.
  Runtime checks remain active under NDEBUG. Includes numeric conversion, sign/
  half subnormals, packed bits that resemble NaNs, four-lane defaults, layout/
  content identity, corruption/truncation with transactional failure, unsupported
  input rejection, source destruction, IA/pulling and owner lifetimes. Existing
  disk-limit, reserve, restart, conflict, repair, writer contention and symlink
  fixtures also pass. A tiny v1/v2 cache proves one shared budget and source-free
  persistent reopening; private fixture output is cleaned by its owner.
- Read-only cache verify04 /PID28436 loads **3,510 files /36,510,144 B** with no
  source storage, writes, repair or conversion. This tests retained v1 decoding,
  not in-game v2 graphics.
- **155 source-boundary tests pass**, including signature agreement and canonical
  dispatch/source-free load wiring. These are supplementary structural checks.
- Final `native_mesh.cpp`, `native_mesh_data.cpp`, `native_mesh_cook.cpp` and
  `host_draw.cpp` pass `clang++ -fsyntax-only` using the actual configured host
  defines/includes/options/PCH read from build.ninja. No object or link output.
- Fixture exe SHA256:
  `14b1bf578128fcdcb2fe5b95ec11879ae2128d53d587928c1d714c2a811e2977`.
- **No full host build or game run.** The existing game executable is unchanged,
  SHA256 `9d07efe8811e44fe074ec30d8032409af732fb4e1ed8cd7e41359d0d7a891383`.
  Run911 remains the latest runtime evidence for the preceding source state.
  Full link, actual canonical draw/pulling observations, GPU pixels, movement/
  reloads and source-free GPU lifetime qualification remain pending. No Quest.

## Cumulative storage gate

Same original ledger `20260906_0333_native-scene-state-bridge.md`, original
2 GiB peak/100 MiB diagnostics/10 MiB logs, and zero new raw allowance.
Preflight free **63,733,166,080 B**. Initial plan <=4 MiB fixture/log growth and
<=128 MiB host-build overlap, retaining only the final fixture and equivalent
logs. Current image aggregate remains 10,106,400 B; no images were produced.

Free space continued falling much faster than scoped outputs: the entire mesh
fixture tree grew from 1,151,798 B to about 1.4 MiB. At 21:48 the volume had
63,642,419,200 B free, only about 56 MiB above the original supervisor floor
63,583,739,904 B. The installed executable/PDB are 48,192,512/106,319,872 B, so
the planned full link cannot safely fit that remaining operational headroom.
The future host-build plan needs <=192 MiB overlap for both files plus objects;
128 MiB was not an adequate worst-case allowance for that pair.
Do not silently lower the floor or reset the checkpoint. Requested owner approval
to raise the cumulative peak cap to 3 GiB; pending at this checkpoint.

Continued bounded CPU/read-only/syntax work under the original floor. Final CPU
and cache checks left 63,637,315,584 B free. No large producer remains live, no
raw/perf/cache files were added, and the profile was never modified. Keep final
build08, CPU07 and cache verify04 logs; equivalent predecessors are superseded.
Cleanup and final measured retention are appended to the cumulative ledger.
Removed16 superseded logs after verification:7,919 logical B, **20,480 B** measured
reclaim. Final mesh tree1,412,368 B /260,570 B growth; aggregate build logs145,872 B,
down144 B. Comparable retained growth260,426 B is for new canonical coverage,
not a full accounting of source/Git/metadata. Post-cleanup free63,626,686,464 B;
drive-wide use106,479,616 B from preflight is mostly not explained by those outputs.
Pre-publication free subsequently fell to63,581,585,408 B, below the operational
stop floor with no producer live. No further build/run starts; source/docs/Git
remain low-storage work pending the budget decision.
