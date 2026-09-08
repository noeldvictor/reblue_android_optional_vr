# Native ordinary visual effects: connected producer and consumer

2026-09-08. Source starts at41711b8. This removes callback dispatch from visual
transitions opened by ordinary native deferred packets, not the full host frame.

## Ownership and contract

`ConsumeDeferredList` now opens `NativeDeferredVisualScope` directly for a native
entry's visual switch. It reuses the effect lifecycle, primary receiver producer,
native blend/alpha owners and native scene consumer, without invoking the visual
registry ABI, shader/receiver/no-op virtual callbacks or `sub_82425C28` setter
dispatch. Legacy-opened scopes retain their compatibility path. Same-visual
native/legacy alternation does not synthesize extra transitions: the scope that
opened a visual owns its close, including at list drain.

`NativeDeferredEffects` retains native image/camera/colour, blend, alpha-to-coverage
and frame values. Finalization validates every sibling before changing any; it
reads no visual/descriptor/device cache/material staging. Lighting still resolves
through the existing ordered Bind/Keep recipe and outgoing mirror at submission.
The explicit compatibility port preserves diffuse-class zero and paired restore
for unmigrated draws. Those writes do not become native material inputs. Restore
reads saved colour at end, not begin; scalar NaN quieting, signed zero, subnormals,
dirty flags and wrapping revisions are preserved. Participant active bytes remain
outgoing compatibility state. Receiver parameter flush still precedes the late
authored colour read (including alias writers). Direct blend production reuses
the tracked owner and its outgoing shadows without bootstrap, readback or original
setters; original-setter comparison mode explicitly refuses this producer.

Provenance, reusing the completed callback census rather than repeating it:

- `config/hooks/render_list.toml`: replay hook and visual-switch boundary.
- `generated/reblue_recomp.88.cpp`, `sub_82174648`,3726; deferred ordinary branch
  `loc_821749E4`4278 onward: diffuse-class staging writes, return2. Direct material
  and special-technique work remain excluded. Class shift uses low six bits;
  bit5 produces zero. Constant82055230 reads00000000 in the verified loaded image.
- `generated/reblue_recomp.24.cpp`, `sub_82174C60`,3931..4022: restore only when
  shader+12 equals1; late shader+28 colour, two staging colours, revision+2.
- Live callback slots, ordering and six blend recipes were recovered in
  `20260908_0206_native-deferred-connection.md`. Unknown/mutated contracts still
  refuse, without replaying accepted native work through the guest list.

## Verification

Output51/PID37604, deferred3/PID29712 and CPU34/PID28444 all exit0. CPU34 passes32
tests in7.63 s: two fixtures rebuilt, unchanged fixture binaries reused. New cases
cover six blend modes, class-shift boundaries, late restore, non-one active values,
untouched staging fields, stale frames/owners, nonfinite values, multi-sibling
atomic rejection and retained effect values. Existing receiver alias-order test
also passes.360 Python checks pass in0.168 s. `--deferred-effects` extends the
existing mixed-consumer scenario gate with matched frames, balanced visual scopes,
reads equal to consumed packets and fresh ready-field deltas. It checks both
epochs independently with `--rigid-reload`; missing/duplicate/stale/bad receipts
cannot qualify.

Host131/PID29472/session42367 exits0,41711b837 dirty. Codegen0 written/one module
current; no guest object rebuilt. Production shaders unchanged; prior55 two-eye
Vulkan fixture cases are reused, not new mixed-game pixel evidence.
EXE48,800,768 B SHA256
`D47CF3265E092E0125BAE4DA53E6AD7B6009074786DF50DE2D8D3268B5982FE3`;
PDB110,325,760 B SHA256
`A5405116ECB753B8F0C4B3A52464570422E59255752FD164318140F592ACB2A2`.

Final review made the no-original-call rule structural: direct blend updates
explicitly disable reference execution even if the comparison setting changes
after preflight. Host132/PID37724/session81881 links this final source, exit0,
same41711b837 dirty/codegen0/no guest objects.360 Python checks pass again.
Current EXE48,801,280 B SHA256
`817C81E561DC3C71D9AA7EC0B506C822E14013B860BA6EC6878B5AC6053F0C5A`;
PDB110,325,760 B SHA256
`732017E9A527E3A5745F92E5FE8BB48BBF0A6124D576A7343768C3EAF7E13743`.

Fresh runtime/pixels are pending at this source checkpoint. Run966 qualifies
host130, not this binary. The route remains opt-in; no timing/full-frame/both-eye
claim. Source visual identities still occupy the bounded late-input sidecar;
receiver descriptors, legacy-opened scopes and outgoing state remain adapters.

## Storage

Original cumulative3 GiB exception/floor62,509,998,080 B unchanged. First free
80,493,723,648 B; producer preflight80,497,094,656 B. Planned replacement growth
below2 MiB plus128 KiB logs;32 MiB fixture/192 MiB host free-drop,300 s supervisors,
30 s CTest,10 MiB aggregate logs. All build jobs terminal; no runtime producer yet.
Removed eight superseded success logs after validation: output50/deferred2/CPU33/
host130 stdout/stderr.7,268 logical B;8,192 B immediate physical gain. New logs
7,324 B; ending274,255 B/164 files, net56 B. Texture fixture76,845,900 B/132 files,
up268,133 B. Selected fixture/log growth268,189 B; material/GPU fixtures unchanged.
Host/source/Git and unrelated drive activity are separate. Cleanup free
80,501,149,696 B, up7,426,048 B from first read; not all cleanup savings. No new
raw/image/cache/cook/perf output. Protected evidence and unanswered image budget
unchanged. Owner profile unchanged SHA256
`2F1BC38D763A1B7BDBA31F560684FD4AA7E42A714600B8D344F19DA7F38E23B0`.

Host132 adds2,524 B logs. Retired host131's3,313 B success stdout/stderr after
replacement,4,096 B immediate physical gain. Total removed10,581 B/10 logs;
new9,848 B, final273,466 B/164 logs (net733 B smaller). Selected fixture/log
growth267,400 B. Free80,499,597,312 B after this cleanup. This does not re-credit
the earlier eight-file removal. Runtime preflight77,620,288 B diagnostics before
this last cleanup fits the819,200 B reload reservation below the75 MiB stop.

## Connected runtime: run967

Source bad7cc7 clean; unchanged host132 binary/hash above. The existing ignored
`run_post_image_flow.ps1` gained `-DeferredEffectsVerify`, requiring its existing
`-RigidDeferredProbe` and adding `--deferred-effects` to the same scenario chain.
No readiness/comparison threshold was loosened. Full recipe: run966's strict
cold/title/reload chain from `20260908_0225_native-deferred-live.md`, plus this
new switch. Producer PID37656/session64033,02:58:51..03:00:55, terminal0 under
180 s/800 KiB,192 MiB free-drop and the cumulative75 MiB diagnostic stop. All22
profile settings took effect, no captures/CSV/dumps/cooking; exact profile bytes
restored in guaranteed cleanup. No producer remains live.

| Fresh ordinary deferred window | Cold1800..2100 | Reload4500..4800 |
| --- | ---: | ---: |
| Native staged/consumed/effect reads | 10,957 | 2,758 |
| Native visual begin/end pairs | 3,383 | 234 |
| Native pending | 0 | 0 |
| Continuing legacy draws | 6,124 | 7,667 |
| Legacy material bridge calls | 12,248 | 15,334 |

Terminal read-only verification splits the log and calls the full
`verify_rigid_epoch(..., True, True, True, True, True, True)` independently for
each epoch; both PASS. Old generation93/instance144 retires, title is reached,
and207/388 owns the new field. The existing receiver/light/caster/cutout, movement,
source/GPU retirement and material/geometry checks pass. Final frame4800 records
64,410 native packets consumed,16,687 native visual begin/end pairs and zero
pending, host-consumer fallback/refused both0. Aggregate native scene GPU window:
10,858 submitted,9,966 emitted/fence-retired,934 culled/retired-culled,10 pending
and10,900 resource retirements. These GPU counts include direct and deferred work;
they are not per-deferred-family fence receipts. Merged-batch delta0: no measured
speedup or new merging evidence. No images were produced or inspected; full
frame/authored-effect/scene/sequence/stereo acceptance remains incomplete.

Retained `out/build/win-amd64-release/logs/reblue_967.log`,509,208 B, SHA256
`4CBA6786131B4C2DF97B9A50DC6373F8325E38B3B801799002DF4AB1ACC8F7D9`.
The owner profile still hashes to2F1BC38D...E23B0 above. No cache/hlsl/perf file
created or modified after run start; raw/image archive unchanged.

After replacement passed, retired the exact superseded run966 plaintext log,
505,061 B (SHA2567DE6645E...A7E25E recorded in its report). Its full raw text is
no longer retained; small results/hash/procedure remain. Run959's distinct merged
batch proof, every unresolved failure and all pixel/raw evidence are preserved.
Immediate free80,497,496,064->80,498,003,968 B,507,904 B physical gain. Runtime-log
retention grows only4,147 B; combined changed fixture/build-log/runtime-log scopes
grow271,547 B. Total removed this continuation515,642 logical B (ten superseded
build logs plus966), counted once. Final measured free is4,280,320 B above the
first read, not all attributed to cleanup; host/source/Git and other drive activity
remain separate. No new budget exception or unanswered-image-budget override.
