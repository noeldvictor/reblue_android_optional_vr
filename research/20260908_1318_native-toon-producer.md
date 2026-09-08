# Authored Toon producer connected; live validation pending

2026-09-08. Source/fixture checkpoint after f1808f1. The owner requested a
commit/push before the next game run. No game, capture, asset conversion or
profile change was performed for this handoff.

## Connected ownership

`native_toon_source.h` reads the authored visual/scene lighting contract into
the existing address-free `NativeToonSurface`. The current object scope owns
that value; the existing texture recipe composes ordered per-layer tints.
Packets feed the shared rigid/skin scene plans, indirect queue and retained
frame-fence path. Animated pre-culling and scene submission now use the same
exact object/pose/pass admission and owned texture classification.

The importer is still a source-address adapter, not native animation or complete
visual setup ownership. Supported nodes are wired to bypass the old node draw
route, but actual ongoing character consumption is not yet demonstrated.
Fur, outline, lattice, volume and deferred/sorted Toon routes remain excluded.
Missing specular colour or enabled-but-unknown power stays unknown; no preceding
node's state is replaced with an invented black/default material.

## Source contract recovered

This supersedes the earlier Toon report's unresolved producer search, not its
GPU results. No decompiler or new source-generation pass was needed.

- `generated/reblue_recomp.88.cpp`, `sub_82174648`: visual begin combines scene
  additions at +68/+84 with visual +3752/+3768, and multiplies scene +100/+116
  by visual +3784/+3800. Scene +132 disables this with explicit zero additions
  and unit multipliers. The original publication is at 0x82174BC8. Native
  consumers receive named values, not that shader-register layout.
- `generated/reblue_recomp.64.cpp`, `sub_82174270`: technique1 selects fur and
  outline variants through visual +3660/+3668; visual +3652 enables lattice
  deformation. These are not the ordinary Toon surface implemented here.
- `generated/reblue_recomp.40.cpp`, `bdSceneNodeDrawSingle`: 0x822802DC resets
  three texture tints to white. The late texture path at 0x82281A74/0x82281A98
  uses visual +3712/+3716 and one last-tinted channel. Early image overrides
  skip this tint/reset operation. Volume-only ignore-alpha is excluded by
  owned ordinary texture classification.

## Verification and next acceptance

- Final output77 build and CPU50 pass: `host_post_output`, 0.54 s, CTest 0.56 s.
  Tests cover importer identity/adjustments/refusals, retained values, ordered
  tints/early overrides, whole-node classification and real packet production.
  The fixture now links the actual material asset/data implementation.
- Host164 links the final source. Codegen writes zero files; no guest objects
  compile. EXE 49,090,560 B, SHA256
  `81A5E42635335A036A9FBE1942392C37DE173248C3539D015229F846AAD63B36`.
- CPU EXE 1,790,976 B, SHA256
  `A648EDBD8624CA657A0216DBCCA4061977049B3B4933F915BB0F188F7DD6A0F6`.
- `python -B tools/host_checks.py --all-boundaries`: 388 pass at handoff.
- No shader changes: retain prior GPU28's 345 cases and validation0/0 as shader
  evidence only, not proof of this new live producer/admission connection.

Host161's 12:15 run969 remains the prior character regression: skin shadows
advance in both fields, ordinary scene skin advances only in opening events.
The next bounded desktop run must require `--skin-scene` independently in both
fresh post-event reload epochs, plus current renderer-owned pixels. Do not
substitute shadow counts or startup scene totals. Art parity, stable sequences,
both eyes and the complete desktop gate remain open; no speedup is claimed.

## Storage and retained failures

The existing cumulative ledger and limits remain in force. CPU output74's
missing link dependency was corrected with production sources. Output76 stopped
at the 32 MiB free-drop guard before link; retain its logs. Output77 passed under
a 64 MiB link-overlap guard, CPU50 under 8 MiB, host164 under 192 MiB. The local
supervisor now reports actual launch free space/floor and checks final free space.

After replacement passes, 14 superseded attachment logs (output73/74/75,
CPU48/49, host162/163 stdout/stderr) were removed: 13,234 logical bytes, measured
free increase 28,672 B. No game data, profiles, images, active trees or unresolved
failure evidence was removed. This is not credit for previous cleanups.

At 13:18, free space is 78,473,715,712 B (73.08 GiB), down 257,216,512 B
drive-wide from the 13:04 preflight. Selected retained fixture/EXE/PDB/attachment
logs grew 2,160,212 B net, principally the real material-linked C++ fixture;
other host objects and drive-wide activity are separate, not attributed to a
specific system process. Texture fixture 85,137,417 B; attachment logs 406,877 B.
No new raw capture or image growth. Full measurements and the transient storage
failure remain in the cumulative scene-state ledger.
