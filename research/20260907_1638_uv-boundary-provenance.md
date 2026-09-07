# UV boundary provenance and bounded verification

2026-09-07, parent28abb64. Guest-source/devloop skills used. This checkpoint
adds failure provenance and improves the local verification loop; it does **not**
fix run948's UV mismatch or remove another rendering dependency. The complete
host-frame/desktop/both-eye/Quest gates remain open.

## Source decision

Reused the layered-scene contract, then inspected the remaining relevant control
flow of `bdSceneNodeDrawSingle` (generated40, 0x8227FEE8), its active wrapper in
`src/gpu/hooks/scene_node.cpp`, and the census hook TOML. Ordinary initialization
at0x822801A0..0x822802DC copies four object/default offsets to both UV vectors
and the working scratch. The0x6000 material path at0x822816B0..0x8228189C applies
the first matching UV override, stops that early scan, and resets a previously
overridden channel0/1 on a later nonmatching assignment. Modes6/7/8 skip that
ordinary UV path; the special route remains excluded by the importer.

`sub_821981E0` (generated67:4531, complete body inspected) flushes dirty staging
vectors0..4 through the existing setter. The original node initializes/marks UV
staging before that call. `sub_82198138` (generated62:4711, complete) changes
texture-enable/mode state, not UV values. The effect participant adapter retains
callbacks, which remain relevant possible intervening writers. These findings
do not identify948's first divergence or justify an outgoing UV mirror.

The original948 log has no values or node/technique identity at failure. The new
failure-only diagnostic records that identity and exact owned, compared, initial,
reset, live-object, staging and working-scratch words, plus up to eight imported
overrides. It runs only after the unchanged byte comparison fails and is bounded
to the first four mismatch reports. Checked reads are diagnostic only: no native
packet is seeded, mutated, admitted or repaired from legacy storage.

The next useful observation must distinguish a recipe/publication disagreement
from a later staging/flush writer. Do not invent a causal fixture without that
cause, add an export-only workaround, or treat a non-reproduction as a fix.
The current diagnostic's source reads remain outside the native consumer section.

## Verification and limits

Host111/PID34264/session52412 built successfully, with codegen0 written and no
guest/shader compilation. Exe48,669,184 B SHA-256
`295E4FB6B8265DCA01701D411BCCBB9EC4088B0D4DC7BEB143AF2C8F3EFDA655`,
PDB109,363,200 B. An unchanged diagnostic function was subsequently relocated
above the consumer section to satisfy the existing ownership guard, not weaken
it.313 artifact-free Python source/scenario checks pass; C++ behavior and GPU
shader evidence are unchanged/reused, not rerun or restamped.

| Run | Actual result | Retained evidence |
| --- | --- | --- |
| 949, PID31872/session41826,16:21:21-16:24:21 | 180 s expired during the reloaded opening event. Cold scene/shadow teardown1998/1998/1998 complete; no UV observation. | 410,254 B, SHA `D8372C7399ACF76F1C10C9C96C163A29F96FC5CE6FA84F63D41A98E8DDF40FF5` |
| 950, PID33028/session69692,16:25:55-16:27:00 | Revised300 s limit, but cumulative diagnostics threshold stopped the cold-field run. No UV observation. | 253,784 B log removed after its budget failure was recorded and reconciled. SHA `8DFA22E4FC4F3D22C39B0BCFECE37BB7CDC1532A5053E028F8C224DD2E39CA7B` |
| 951, PID35436/session74854,16:31:07-16:33:52 | After compression restored diagnostics headroom, the enforced free-space/drop threshold stopped the cold-field run. No UV observation. | 254,383 B, SHA `3A953CFE06CBDA7961BE0D42C37A81DB175EF196839ECC34302DECE9556DFA8B` |

951's fresh cold-field1549->1849 window adds3,308 wider-family scene emissions
and3,336 fence retirements (earlier queued work also retires). Layered draws and
inherited preparations remain0. These are repeated primitives, not unique assets
or full-scene qualification. No new images, raw frames, performance measurements,
cooked caches or shader dumps. All three runs applied the21 temporary settings;
profile restored exactly each time,116 B SHA
`2F1BC38D763A1B7BDBA31F560684FD4AA7E42A714600B8D344F19DA7F38E23B0`.

The existing ignored local supervisor now exposes a bounded180..300 s reload
timeout (default180), retaining the800 KiB log/192 MiB drop/original floor.
It also reserves the full requested diagnostic text/image overlap before profile
mutation or boot, against the same75 MiB stop threshold. Parser checked; the
new pre-launch guard has **not** been exercised in another renderer run.
No fourth probe was launched. Further booting requires sufficient stable headroom
and a useful opportunity to reach the missing UV observation, not another restamp.

## Retention and storage

The unchanged cumulative ledger is
`research/20260906_0333_native-scene-state-bridge.md`; no allowance reset.
The older accepted945/940 pixels/logs and unresolved941/947/948 evidence remain.
949 retains the timing-window evidence;951 retains the latest limited field and
storage-stop evidence, to be reviewed after a useful replacement run.

Ten older performance CSV/metadata pairs (20 files,8,954,976 B) are now preserved
losslessly in `out/build/win-amd64-release/logs/perf/retained-20260905-215905_to_20260906-150239.zip`.
All20 decompressed entries' lengths and SHA-256 hashes were verified before the
redundant originals were removed. Archive1,519,695 B, SHA
`76100860DFBEB64D03986246F766A1903BE057AFB5D6B394CEA4194D753084AA`.
Original filenames/content are recoverable from that archive; their historical
verification purposes remain. Logical net reduction7,435,281 B; observed volume
recovery5,300,224 B was not isolated from concurrent disk activity.

Also removed950's superseded budget-stop log and the two host110 build logs after
host111 passed:3 files/254,818 logical B,258,048 B observed volume recovery.
These logs can be regenerated by equivalent runs, but exact old text is gone;
their hashes/results are preserved here and in the ledger.

Known retained net change versus this continuation's start is **-6,732,917 B**:
compression-7,435,281, exe+7,168, PDB+28,672, build logs+1,887,
retained949/951+664,637. Other object/CMake/source/Git changes are unallocated.
First free62,920,818,688 B; post-cleanup63,058,419,712 B is a137,601,024 B
drive-wide gain, **not** attributed cleanup savings. Final free check follows push.
