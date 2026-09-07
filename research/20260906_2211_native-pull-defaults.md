# Native vertex pulling preserves missing-input defaults

2026-09-06 EDT. Parent `ed2e884`. Previous checkpoint was source/CPU progress;
the host link and runtime qualification remain pending the original storage gate.
This follow-up corrects a native-input contract mismatch discovered during
read-only review. It is not a claimed pixel regression fix or direct object draw.

## Source evidence and change

Read the load-owned material/geometry producer and registry after the physical/
predictor manifests, then native vertex input, declaration construction, complete
pull staging, and the shader compiler's input prologue and fetch helpers.

- The declaration builder intentionally supplies absent secondary POSITION1..4
  as float4 zeros: their w component is a blend weight and must remain zero.
- `NativeVertexInputLibrary` previously left every synthetic pull-table entry
  at zero. `BD_PullDwords` returned zero raw words, but `BD_PullF` then used its
  default branch, which returns **(0,0,0,1)**. The prologue calls that helper
  directly in the pulled variant; it has no secondary-position special case.
- A missing color's alpha-one default does agree with that branch. Do not
  change every missing input to float4 zeros: the format matters. Existing CPU
  tests checked the zero table entry, not equivalence of all resulting lanes.

Native inputs now retain encoded format/slot metadata for synthetic attributes
as well as real streams. `Streams()` still describes asset streams;
`PullStreams()` includes the explicit synthetic source. Native staging supplies
the existing 64-byte zero buffer with offset/stride zero, independently of the
old draw's slot15 binding. Its buffer has STORAGE as well as VERTEX usage and
initialization fails closed if mapping fails. Synthetic offsets other than zero
are rejected. Float4 defaults therefore use the existing float4 fetch case;
float1/float3 defaults keep their default w=1. No shader code or cache changes.

This fixes the **native** owner/stager contract. The legacy declaration path is
unchanged and still tracked for removal. No affected field/material incidence,
visual artifact attribution, performance or Quest result has been established.
The old shader signature and captured draw templates are still transitional.

## Verification limits

- Configured host syntax checks pass for `vertex_pull.cpp` using its Vulkan
  target/PCH and `native_mesh.cpp`/`native_mesh_cook.cpp` using common target
  options/PCH. Compiler options are read from the current Ninja rules, not guessed.
- Updated `vertex_input.cpp` and `cook.cpp` fixtures pass syntax checks. New
  compile-time assertions execute the actual constexpr encoding function for
  synthetic float1 and float4 formats. Runtime cases now require format-aware
  entries, the synthetic pull mask, owned lifetime and bounded offsets.
- **156 source-boundary checks pass**, including the owned STORAGE source and
  stride-zero staging wiring. These are not GPU or full behavioral evidence.
- **The updated C++ runtime fixtures have not been rebuilt or executed.** The
  preceding fixture exe remains246,272 B, SHA256
  `14b1bf578128fcdcb2fe5b95ec11879ae2128d53d587928c1d714c2a811e2977`.
  Its earlier pass does not qualify this changed header/stager.
- Game exe remains48,192,512 B, SHA256
  `9d07efe8811e44fe074ec30d8032409af732fb4e1ed8cd7e41359d0d7a891383`.
  No host link, game run, image, profile change, cache conversion or Quest run.

Next verification: existing mesh fixture, incremental host link, then fresh
canonical-geometry and native-pulling observations with pixels. Keep the full
static-object producer-to-direct-scene/shadow outcome, not a replay-only endpoint.

## Storage observation

Same cumulative ledger and limits; no budget increase was authorized. Start
free63,426,617,344 B, below operational floor63,583,739,904 B. No space-producing
build or runtime job started. Low-storage source edits and no-output checks only.
The earlier request for a 3 GiB cumulative cap remains pending.

A read-only process-counter sample found an unrelated project's Python data
workflow writing10,170,513 B/s. Script/parent metadata identifies it outside this
workspace; argument values were omitted from the report, no private file contents
or project data were read, and no process or file there was changed. One sample cannot attribute exact net
disk growth, so no bytes were subtracted from this ledger or credited as cleanup.
Pagefile allocation was2,048 MiB, current use43 MiB/peak58 MiB at the sample;
there is no earlier allocation sample proving its contribution to prior growth.

At22:11 free63,516,618,752 B, a90,001,408 B drive-wide increase since preflight,
not task cleanup. No new build/test logs, fixture binaries or captures; no
deletions. Source/docs/Git retained growth is not fully baselined. Existing
evidence and profile stay protected. Final publication free space is reported
separately; the original floor and full renderer goal remain unchanged.
