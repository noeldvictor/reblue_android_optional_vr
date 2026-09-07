# Owned selected-light publication

2026-09-07, parent `ada121d`. This removes the three-light publisher body
`sub_8218B0F0` from normal supported rendering execution and supplies owned semantic
lights to the existing object packet. It does **not** complete the rigid-object
path, replace selection scoring/animation, or connect native rigid game shaders.

## Source and ownership

The repository guest-source skill directed the audit to complete translated
bodies before changing the host hook. Relevant local sources:

- `generated/reblue_recomp.4.cpp:4029`, `sub_8218B0F0`: three signed selected IDs,
  cached-ID comparison, descriptor writes and inactive partial-write semantics.
- `generated/reblue_recomp.93.cpp:2656`, `sub_82142C58`: visual-wide selection at
  visual+3132 or non-null per-node entry in visual+3376 when visual+3380 is set.
- `generated/reblue_recomp.55.cpp:4018`, `sub_8218B310`: later per-node callback.
- `generated/reblue_recomp.54.cpp:4204`, `bdLightListUpdateSnapshot`: authored
  300-record snapshot, changed IDs and publisher-cache invalidation. The existing
  `frame_interp.cpp` hook preserves its interpolated-frame behavior; unchanged.
- `generated/reblue_recomp.6.cpp:32452`, `sub_826BF690`: original cosine helper.
- Existing `native_lighting_bridge`, normal-lit HLSL, native light/fog core,
  rigid pass packing, object texture scope and host draw capture.

`NativeLightDefinition`/`NativeSelectedLights` have named position, direction,
colour, range and cone values. There are no source addresses or register-layout
keys in the consumer data. A bounded source adapter reads authored76-byte records
and prepares at most42 writes, tracking at most128 control dependencies. It
refuses missing/misaligned/overflowing data, invalid IDs/counts, relevant
nonfinite/singular values and source/control/destination aliases before mutation.

Only changed selected IDs write the compatibility payload. An unchanged ID keeps
the previously owned value; a fresh snapshot is not permission to invent a new
publication. Unknown unchanged payload remains unowned. Disabled slots preserve
the original's partial legacy writes but have canonical zero native values.
Unused directional inverse-range/cone data are canonical zero rather than stale
payload or infinities. Spot cosine comparison allows absolute error2e-6; all
other original output words compare exactly. Actual draw comparisons ignore
unused payload lanes, not active semantic differences.

Ordinary object defaults are selected only for the exact publication and frame.
Later per-node publication binds at the authored producer boundary, validating
the current object/context, owned node and selected source entry. The native
consumer selects by owned pose/node and copies values into the retained packet;
it never reads shader registers or selection tables. Different nodes cannot
borrow that override. Null/inherited per-node selections remain unowned.
Unsupported/unbound publication invalidates the current scope's light values.
Nested object scopes retain the existing restoration behavior and4 MiB bound;
only one current node override is retained, not an accumulating light cache.

With `bd_native_lighting=true`, normal supported publication does not call the
original body. `bd_native_materials_verify=true` executes the original once to
compare, then applies the computed write plan. The original also remains the
explicit unsupported/disabled fallback. Compatibility staging is still consumed
by unconverted draws; its removal waits for their migration. Selection scoring,
authored light update/snapshot functions and object/pass setup still execute.

## Verification and corrections

The devloop skill kept checks in the existing material/desktop trees, host-only
and readiness-driven.250 Python guards/scenario tests pass, including source
boundaries, stale/startup/wrong-scene/reset rejection and visible mismatch failure.
Material22/PID30824 builds; CPU20/PID23656 passes in0.11 s (0.13 s CTest). C++
fixtures cover changed/unchanged/inactive slots, descriptor/lane writes, alias
refusal, bounds, singular ranges, source destruction, node isolation and lifetime.
Existing light/fog numerical and material/ownership fixtures also pass.

Host79 failed on the diagnostic shader field name; corrected to
`shaderCacheEntry->hash`. Host80 passed. Run923/PID14416 ended at04:34:21 with
matching publishers but zero normal-lit draw checks and unowned selected-object
lights. Its317,845 B log hash was
`B5EA5CAEA76285F09B9542ECB158F7045EBF778146E03796378FAA2B29E2EF97`.
Object-wide snapshots missed later per-node overrides, so the scenario correctly
failed. Corrected the producer-to-node association and moved the bounded packet
diagnostic after the callback; no weakening of the consumer readiness gate.

Host81/PID18740 passed (host objects/link, codegen up-to-date). Run924/PID29684,
04:39:05–04:40:04, passes the complete existing field gate plus selected lights.
All15 temporary TOML settings took effect. Native lighting/materials, native
images/instances/tables/pulling/shadow/material-texture/policy paths enabled;
material comparison on, texture-table comparison off, precache on. Raw capture,
perf CSV, persistent shader/cache writes and selected asset cooking off. Limit:
75 s,400 KiB log, one160 KiB JPEG. Owner116 B profile restored byte-for-byte:
`2F1BC38D763A1B7BDBA31F560684FD4AA7E42A714600B8D344F19DA7F38E23B0`.

Fresh post-event `bg41_01` contexts2051/2351 follow opening event1451; field
state0/player1/event0/movie0/loader0/icon0:

| Observation | Fresh delta |
| --- | ---: |
| Host light publications and matching original comparisons | 13,870 each |
| Changed light slots | 900 |
| Owned object/node light snapshots | 1,986 |
| Matching normal-lit draw-input checks | 1,322 |
| Light fallback / unavailable / mismatch | 0 / 0 / 0 |
| Owned object-colour reads and comparisons | 74,409 each |
| Owned node bounds and matching comparisons | 1,761,600 each |
| Matching instance reads | 108,395 |
| Canonical draws | 134,024 |
| Shadow receiver checks | 15,383 |
| Observed player motion | 61 samples,64.187271 units |

Four bounded packet observations own `(directional, disabled, disabled)` light
kinds, including selected geometry`258694267A8DBAEE`, material`63B8D67932573E51`,
node64/tech0/view3. Point/spot behavior has CPU coverage, not claimed live field
coverage. No native rigid shader game draw or source-free GPU load occurred.

Inspected actual `native_selected_lights_window.jpg`,1920x1080/JPEG quality60,
136,226 B: Shu running near the bell/fence, terrain, vegetation and shadows remain
coherent; known black cliff marks and distant blur remain. This one flat image
does not qualify temporal stability, other scenes, reloads or both eyes.

| Artifact | SHA-256 |
| --- | --- |
| Host81 executable actually used by run924 | `05F3274C6A6D26B4B38E8349A7D5FB13047AABCD06017D31737031C5C7BB5F54` |
| `out/build/win-amd64-release/logs/reblue_924.log` (239,652 B) | `E5F068701FA775EAB276C24D120C3BD58320E3830E406A21395C8F9C807CD5F5` |
| `out/verification/native_selected_lights_window.jpg` | `CC141CCF9E3F81693DC96238C34BDDB438B349B0594C39DF9F1C8F1C941D516F` |

Final host82/PID27524 passes after adding explicit scope invalidation for unknown,
unsupported or unbound publications;250 guards pass again. It does not change
computed light values or compatibility writes. This defensive ownership guard
was not rerun in the game; runtime/pixel evidence remains tied to host81 above.
Latest executable48,375,296 B SHA-256
`8DB97C27DE9DAF17724D1CECFFC13A752B8D076C4F53889CB0AE9423FFC7E6D8`;
PDB107,732,992 B. No binaries, game data, caches or runtime outputs are committed.

## Next and storage

Reuse the selected17,572 B rigid asset and now-owned light/object packet. Connect
the two fog layers and remaining pass inputs from their real producers, establish
the exact selected UV/shader family, then wire whole-node native scene/shadow
submission into the existing backend. Cold-load/reload acceptance must disable
that family's interpreter/capture/replay before its first draw; current walking
checks are not that acceptance. Quest2 remains gated. No FPS/speedup claim.

All attempts share the original cumulative3 GiB exception/free-floor/raw0 gate,
with per-build256 MiB and per-run192 MiB free-drop stops. No new asset/cache/raw/
perf/dump output. Storage accounting and measured cleanup are appended to the
existing [cumulative ledger](20260906_0333_native-scene-state-bridge.md), not reset
for retries. Keep run924/image, material22/CPU20 and tested/current host81/82 logs;
retire equivalent predecessors only after this replacement's checks/pixels pass.
Historical baseline/GPU/motion/failure/raw evidence and selected asset stay intact.

Cleanup completed:17 exact superseded build/CPU/field/image files removed after
replacement validation,694,095 logical B / **712,704 B (696 KiB) measured reclaimed**.
The two resolved failures remain documented above; their full retired logs and
the previous flat image are no longer retained. Known comparable retained growth
211,701 B adds fixture/binary/consumer coverage; other build/source/Git deltas
unknown. Cleanup-end free58.89 GiB; measured drive-wide use+26.93 MiB from the
turn's first preflight, not wholly attributable to task outputs. No active producer.
