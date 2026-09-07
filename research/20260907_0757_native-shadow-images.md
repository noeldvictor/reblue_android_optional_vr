# Native primary-shadow images and pass commands

2026-09-07 EDT, parent95cd24c. This removes a named direct-object dependency:
the main shadow image now reaches the existing sampling adapter as a retained
native image, without a surface-pool allocation or legacy resolve link. It is
not the first direct native rigid game draw; that remains unfinished.

## Producer to consumer

`native_shadow_pass_bridge.cpp` now requests an explicit mono, single-sample
`D32_FLOAT_S8_UINT` native image. The existing bounded native image and scene
framebuffer stores own its allocation, descriptor, layout and fence retirement.
The shared scene command recipe supports depth-only passes; a fresh scope clears
once, including no-caster passes, while resuming preserves contents. Colour
snapshots refuse a depth-only scope. No second renderer or asset recook.

Output and framebuffer/command creation preflight before engine-visible pass
publication. Unsupported begins retain an explicit counted fallback; after entry
the scope must unwind natively. Pass end flushes queued casters, ends the render
pass, transitions the exact image to sampling, then publishes its lease through
`Video::PublishNativeImage(..., false)`. It checks exact owner, texture,
descriptor and shared layout, and requires no `sourceSurface` link. Existing
material sampling consumes the header adapter backed by that native lease.
No copy, emulated resolve or post-chain/tile publication in this handoff.

The guest-source skill kept investigation in translated source and existing
producers. The remaining receiver callback `sub_82176708` binds owner+12 at slot6,
flushes the projection descriptor and copies owner+52..64 as shadow receiver
colour. `sub_821764F8`'s host replacement already computes projection. These
values still need a correctly scoped native pass publication, alongside fresh
camera matrices and per-node light production before the old shader callback.
The unscoped `GetNativeRenderTransforms` alone cannot prove frame/view freshness.
Do not freeze observed values or recover them from captured shader registers.

Still present: engine camera fitting/caster scheduling/rendering, secondary
shadow lifecycles, receiver callback/staging, header/getter adapters, and the
selected object's interpreter/replay branch. Native image ownership does not
claim completion of any of those or the desktop/both-eye gate.

## Verification

The devloop skill selected the existing fixture and incremental host tree.
`host_post_output_test`15/PID28844 and `post_output_cpu`1/PID26548 pass
(0.34 s fixture,0.36 s CTest). Tests use the production native framebuffer and
command cores: mono/layered depth-only descriptors, invalid shape/sample/format/
clear/density inputs, repeated acquisition, empty/resumed/next-frame clears,
colour-snapshot refusal, shared-layout receipts and source retirement after the
matching fence while a sampled-image reader remains alive. Existing scene,
MSAA, image/descriptor and snapshot tests also pass in that fixture.

271 all-boundary Python tests pass (including three fresh shadow scenario
tests); these are source/scenario guards, not GPU fixtures. Host92/PID28092
passes with host objects/link only. Codegen reports0 written/up to date; no guest
objects or shaders rebuilt. No new GPU-fixture/both-eye qualification is claimed.

Run934/PID20808,07:56:07–07:57:05, passes the prior complete field gate plus
`--shadow-images`. Shadow reports following ready-field contexts1723/2023 add
300 begins,300 ends,300 publications and300 exact ownership checks, with no
fallback/mismatch or new empty clear. The latest2323 context has no subsequent
shadow report, so it is not claimed as a shadow sample. Other latest field
metrics use their own complete windows2023/2323 after event1423:

- 14,466 light-selection updates/comparisons,37 rebuilds,10,212 candidates.
- 14,466 light publications,900 changed slots,1,217 draw comparisons.
- 3,642 sampler checks/41,233 owned-input draws;1,217 fog checks/2,434 layers.
- 119,502 instance comparisons and85,862 object-input comparisons.
- 184 observed movement samples,+207.877897 world units,12.847 s walking.

The actually inspected1920x1080 image shows running Shu and coherent character,
tree, fence and large-prop shadows. Existing cliff marks and distant blur remain.
One flat image is not a sequence, reload, other-scene or both-eye qualification.
Direct native rigid program use and source-free GPU loads remain0. No speedup
claim. All15 settings audited;116 B owner profile restored exactly, no owned
producer remains and no new raw/perf/cache/dump/cooked outputs were produced.

| Retained artifact | SHA-256 |
| --- | --- |
| Host92 exe48,434,176 B, tested by934 | `ED72FFCDB762439F617A8BD72E9F8B82B91E26BF6AE0957D4C5BE6D8B3D24948` |
| `out/build/win-amd64-release/logs/reblue_934.log`,239,424 B | `5CD23E808305D7B74AD07B67FA74BA43BCFBF49EB335D71F2170901A403FE7F5` |
| `out/verification/native_shadow_images_window.jpg`,135,867 B | `264FF33A4127ABD08FA3FAA4B2C1B4B1B5914B9D7C7577F3776C75F7735E917C` |
| Restored profile | `2F1BC38D763A1B7BDBA31F560684FD4AA7E42A714600B8D344F19DA7F38E23B0` |

## Storage

Same original cumulative3 GiB exception,62,509,998,080 B operational floor and
raw0 gate; no budget reset. Existing supervisors bound build free-drop256 MiB,
runtime192 MiB/75 s, log400 KiB, JPEG160 KiB, aggregate logs/images10 MiB each.
Reuse existing trees; linker overlap remains within the prior192 MiB estimate.

After validating replacement evidence, removed six exact obsolete agent files:
host91 and output14 stdout/stderr,933 text and light-selection JPEG.397,349
logical B removed; **405,504 B (396 KiB) measured reclaimed**. Reproducible build
logs can be regenerated; the exact retired runtime text/image are gone, with
hashes/findings preserved in prior research. Distinct GPU/flat/VR/movement/
failure/raw evidence and all game data remain untouched.

Known retained component growth158,905 B supports the changed code/fixture and
replacement evidence, not another retained runtime set. Cleanup-end free
62,889,103,360 B (58.57 GiB),1.41 MiB less than output preflight. Drive-wide free
rose142.34 MiB from the first source-only measurement, unrelated/unattributed
activity rather than cleanup credit. Other object/source/Git deltas are not
fully attributed. The existing cumulative ledger retains the measurements and
same next-checkpoint replacement/cleanup rule.
