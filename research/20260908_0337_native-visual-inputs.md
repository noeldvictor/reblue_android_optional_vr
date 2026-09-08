# Native visual inputs: writer-ordered ownership

2026-09-08. Source checkpoint45dc891 removed the deferred per-entry guest-address
sidecar. Native instance/generation keys now identify bounded blend/class values
and receiver publications. Source-input adapters remain; this is not a full frame.

## First host connection and causal failure

Host133/PID38216/session26287 exits0; source45dc891 dirty only in scenario checks.
Codegen0 written/one module current; no guest objects rebuilt.363 Python checks
pass. Previous output52/deferred4/CPU35 still supply32 C++ tests; unchanged shader
GPU results are reused, not new mixed-game pixel evidence.

EXE48,817,664 B SHA256
`01FB95D4C663419D646F2BBA69BA9EA14788A9C71F41A04D1A6A986ECB954B3E`;
PDB110,440,448 B SHA256
`E90487D3E5253B1412AE99653AA8E381CF366C5712515CA76866B2151C991321`.

Run968/PID23728/session19334 exits before field qualification (03:37:22..03:37:50).
Frame600 still reports zero native staged/consumed work. The last log is frame685;
no native-input consumption, reload or pixels qualified. Capture-free,22 applied
profile overrides,180 s/800 KiB and192 MiB growth limits; exact original profile
restored, SHA2562F1BC38D763A1B7BDBA31F560684FD4AA7E42A714600B8D344F19DA7F38E23B0.
Log `out/build/win-amd64-release/logs/reblue_968.log`,85,004 B, SHA256
`73FF7FD79B6C738201870949D2B37C8E0C6023CA9846D6833116E08710886FB2`.

Windows independently emitted77,048,323 B in
`C:/Users/leanerdesigner/AppData/Local/CrashDumps/reblue_vk.exe.23728.dmp`, SHA256
`C40AE92C295D5CF648203D3C0325AB71AB69ECA919F6DDC2F0B3AFC84FEDD9AA`.
This unplanned diagnostic counts against the cumulative budget, not a fresh
allowance. No further producer starts while that excess remains unaddressed.
The WER Report.wer is82,998 B; no temp dump remains in the named report archive.

Installed Windows cdb, local symbols only (`-y out/build/win-amd64-release -z`
the exact dump, no symbol downloads), establishes:

- Exception0xC0000409/subcode7 is `FAST_FAIL_FATAL_APP_EXIT`, not evidence of a
  buffer overrun. `ucrtbase!abort -> terminate -> __scrt_unhandled_exception_filter`
  follows an uncaught `std::runtime_error`.
- Throw return address00007FF7444B8049 is
  `bd::gpu::scene::ConsumeDeferredList+0x5149`, deferred_consumer.cpp:554.
  Caller is `sub_8227F360+0x21`, then `__imp__bdSceneNodeSetupRenderPass+0x158`,
  `__imp__bdSceneSubmitRenderList+0x1ff`, native view scheduling and Draw Thread.
- `ub 00007FF7444B8049 L18` identifies the runtime_error string at
  00007FF7465B2F8C: **Unknown legacy resource writer inside native deferred batch**.
- `.frame /r 0n11`: r12=8208D52C,r14=000000018208D54C. The failed resource check
  is vtable8208D52C+32. Relevant heap/source pages are absent from the minidump;
  the dump cannot supply the visual address or parameter alias values.
- Read-only in-memory decoding of the legally owned XEX (existing
  `tools/dump_xex_image.py` parser/decryptor, validated MZ/PE image base82000000,
  section/file offset8D52C) identifies live-slot candidates +32=82454720 and
  +36=824548A8. These are water preparation/cleanup, not arbitrary callbacks.

The symbolized cause, exact binary/dump hashes and callback provenance above are
the minimum retained crash evidence. The complete dump is eligible for retirement
after this extraction; that retirement does not mean the rendering failure passed.
Run968 and its failed qualification remain retained.

## Correction in progress

The no-op requirement conflated two contracts: callbacks OMITTED by a native
model must be no-ops; callbacks EXECUTED by an intervening legacy water draw may
write authored parameter storage. The latter must republish native visual inputs,
including indirect aliases, before the next native consumer. Merely adding an
allowlisted method pair to an immutable batch would freeze legitimate late writes.

Reuse `NativeVisualPublication`, the existing instance source index and the water
whole-function hook. A scoped native publication refreshes after the complete
known writer, on both host and original-preflight paths. Native draw consumers
still read only native keys/values. Unknown resource writers and native omission
of non-no-op methods still refuse. Failed refresh cannot resume/fall back.
Known C++ deferred failures now use the existing fatal shutdown with a flushed
bounded message, avoiding another unhandled-exception WER dump for this refusal.

Source: native_refraction_material_bridge.cpp; generated/reblue_recomp.38.cpp,
sub_82454720 at19825 (indirect factor write, descriptor clamps/flushes, state,
textures, optional snapshot); generated/reblue_recomp.62.cpp,sub_824548A8 at20014
(texture unbinds only). Host callbacks and dynamic light/blend/receiver outputs
remain at their actual ordering boundaries. Fresh build/CPU/live acceptance of
this correction is pending; host132/run967 remain the preceding passing run.

## Correction build and retained diagnostics

Deferred5/PID38392, CPU36/PID19872 and host134/PID27788 all terminal0.32 CPU
tests pass in7.65 s;364 Python checks pass. The causal fixture uses the production
water destination decoder to alias diffuse-class storage, then runs the water
sequence with factor/clamp writes and publishes only after its final step. The
old retained native value remains unchanged; the next read sees the completed
writer. The water pair is accepted only as an executing writer, never as an
omittable native callback. Changed production hook/source connections compile;
no guest objects or shaders rebuilt. Fresh runtime/pixels remain pending.

Host134 is45dc891 dirty. EXE48,820,224 B SHA256
`EE87626353893C08F870ECB611A405A90B1E53084C0B931FC73AA539C73266FA`;
PDB110,465,024 B SHA256
`2E41374D56962EF8C2591A1387ED7431FC01C1BABC498D988041B8F9B9CA329A`.
The existing runner adds `-DeferredInputsVerify` (requires the existing effects/
mixed gate); the parser pairs input batches/visuals/refreshes with same-frame
consumption/effect receipts and requires fresh deltas in both epochs. Missing,
stale, duplicate, invalid-capacity or reset receipts cannot qualify.

The diagnosed77,048,323 B dump was hash-checked and removed after the extraction
above. Full minidump memory is no longer retained. Immediate free
80,176,091,136->80,253,140,992 B (77,049,856 B physical gain). The85,004 B failed
run log and82,998 B OS Report.wer remain; the runner now counts the latter in its
unchanged cumulative diagnostic ceiling. Scoped runtime cache/hlsl/perf checks
show no files created/modified since run start. No new raw or image payloads.

## Connected cold/reload verification: run969

Source0784d47 clean; unchanged host134 EXE/PDB hashes above. Existing capture-free
mono strict chain, same recipe as run967 plus `-DeferredInputsVerify`: native
model/material/geometry/instance/table/pulling, movement, canonical/shadow/material/
primitive policy, lighting/fog/features/samplers, direct scene/shadow/batching,
hard-off, receiver, scene lights, caster/cutout families, rigid deferred/effects,
and same-process reload. `--deferred-inputs` requires completed-writer refreshes
as well as initial native batches; no readiness or comparison threshold relaxed.

PID35028/session53430 terminal0,03:58:48..04:00:53 under180 s/800 KiB/192 MiB
free-drop, original cumulative/reserve bounds. All22 settings applied, captures/
CSV/dumps/cooking off. No new runtime cache/hlsl/perf files or OS crash dump;
original profile restored byte-exact to the recorded hash.364 Python checks and
CPU36 remain the checks for this unchanged source. Both log epochs independently
pass `verify_rigid_epoch(..., True, True, True, True, True, True, True)`.

| Fresh deferred window | Cold1800..2100 | Reload4500..4800 |
| --- | ---: | ---: |
| Native packets staged/consumed/effect reads | 10,865 | 3,053 |
| Native visual begin/end pairs | 3,356 | 223 |
| Initial native input batches | 300 | 300 |
| Native visual identities published | 959 | 322 |
| Completed-writer refreshes | 175 | 139 |
| Pending native packets | 0 | 0 |
| Continuing legacy draws | 5,619 | 5,351 |
| Legacy material bridge calls | 11,238 | 10,702 |

Old model generation93/instance144 retires; title is reached and new207/434 owns
the reloaded field. Final frame4800:63,858 native packets consumed,16,390 balanced
native visual scopes,2,915 input batches/6,022 visual publications/988 writer
refreshes. This removes the per-entry source-address sidecar from the exercised
native consumer without freezing intervening water writes. The field does not
prove that an actual game allocation aliases class storage; that case is the
focused CPU regression. The runtime proves the real writer/consumer connection.

Reload aggregate scene GPU window:9,687 submitted,8,882 emitted/fence-retired,
835 culled/retired-culled,18 pending and9,717 resource retirements. These combine
direct/deferred work, not per-deferred-family fence evidence. Scene cutouts emit/
retire7,318 textured primitives; textured shadow cutouts300. Wider shadow casters
submit/emit33,034 from15,354 nodes, including4,771 multi-node visits. Merged-batch
delta0; no speedup claim. Water remains a legacy draw family with a host setup
writer; source imports, receiver descriptors and outgoing compatibility remain.
No pixels, complete native frame, broader scene/effect sequences or both-eye
qualification are inferred. Quest2 optimization remains behind the desktop gate.

Retained `out/build/win-amd64-release/logs/reblue_969.log`,511,707 B, SHA256
`4A2A903E51029E0E4EC016DF70D4A76010BBEFF8F77985892CA2772FF0EEADAE`.
After replacement passed, retired run967's509,208 B plaintext success log after
hash verification against the earlier effects report. Its full text is gone;
small results/hash/procedure remain. Run968 failure, its symbolized cause/OS
metadata, distinct959 merging proof and all raw/pixel/failure evidence remain.
Replacing967 with969 grows runtime success-log retention only2,499 B.
