# Effect inputs: absent joint drivers, authored eye controls

2026-09-08 EDT. Source5006b1e; host187 built from876b633 plus exactly that
source/test change. This checkpoint diagnoses a wrong scenario assumption; it
does not remove another rendering dependency or qualify an unexercised feature.

## Decision changed by evidence

The opening scene's failed positive joint-UV observation is not explained by
native controller admission hiding named drivers. The new census observes every
enabled, non-reference controller attempt BEFORE admission. At frame2342 there
are108890 admitted and18100 boundary-refused observations; both partitions have
unknown0/named0/translation0/rotation0/unresolved0/no-channels0. UV records observed
are2232 and2 respectively. No later rejection stage occurs in this run. These
are repeated observations, not counts of unique objects/assets, and do not cover
effect callers outside this controller. They do not prove that drivers never
exist elsewhere or explain every ordinary controller boundary rejection.

The parser/content audit identifies the next useful effect bundle: authored
eye/gaze controls and their late UV producer, through the existing instance and
material owners. Do not keep probing the opening scene for absent UVCON inputs.
The positive joint-driver and queued-transition gates remain unqualified, not
waived. Their dedicated scenario must be justified by real authored inputs.

## Reusable source contracts

Read the complete bdMdlTextFileParse0x82292170, generated5:10129..17113,
including all labels beyond its early return, plus unchanged pso_predictor.toml
and frame_interp.toml. This completes the partial parser audit recorded in the
previous effect report. No hook/config/generated file changed.

- Temporary records start at stack+4384, count+176, stride152; first exact name
  match at record+88 merges commands, otherwise zero-init a new record. On
  successful exit with positive count, replace visual+3560 storage and copy
  records, then publish+3564. Zero count does not replace the existing table.
  Model requests may occur before this publication; do not infer ready ownership
  from the optional PSO predictor's two technique hooks.
- UVSCR at0x82293A10 writes record+36/+40 rates and+20=1, or the separate
  visual+3440/+3460/+3464 ordinary-UV fields for its special name.
- UVCON at0x82293B98 parses three strings: material name, joint name, mode.
  It copies the joint name to the32-byte field+120, writes+16=1 for ROT,
  initializes+52/+56=90 and+44/+48=1, enables+20=1. **+120 is a string, not a
  boolean field.** The effect updater tests its first byte. The parser does not
  resolve+12; it also does not clear an existing mode for a non-ROT command.
- UVRATE at0x82293D2C overrides+52/+56 and+44/+48. UVEYE at0x82293508 writes
  +0 eye index,+28/+76 base U,+32/+80 base V,+60/+64/+68/+72 limits,+20=2.
- Complete bdVisualObjectInitDebugDraw0x82144148, generated92:2605..2760:
  misleading historical name; it binds these records. Material name+88 chooses
  the first exact match in visual+2624's28B texture-name entries; writes+4 or-1.
  Nonempty joint name+120 uses visual+2620->+16 graph and
  bdSceneGraphFindNodeByName; writes node+0 pose index to+12, or-1 if absent.
  No rewrite or new owner was added at this boundary.
- Complete sub_822BA028, generated77:9934..10052, computes two eye UV pairs
  from input r4's two scalars and the FIRST effect record's base+76/+80 and
  limits+60/+64/+68/+72. Horizontal sign differs between eyes; vertical input
  is shared. Each branch computes a float-rounded distance from base to limit,
  then float-rounded double FMA. Four floats write to r5's16B output. It does
  not read packed animation channels and is not the UVCON rotation path.
- Caller sub_822B8DF0, generated63:11647..11737 (only this part audited here),
  limits the outgoing eye copies to at most two records, calls sub_822BA028
  with actor+6168 gaze input, then writes+28/+32 at stride152. Later it calls
  AnimeData_method_1A60 and sub_822D3CB0. Finish the complete caller/control
  ordering and existing hooks before migrating its ownership; no guessed hook.
- A separate late writer in sub_823CAF90, generated60 around17624..17789,
  writes effect offsets+28/+32 and rates+36/+40. This is navigation evidence,
  not a complete audit of that caller. Parser-only snapshots would miss it.

Literal names/default90 were checked against the existing, read-only
out/gamedata/default.image PE sections, not decrypted/extracted again. Its
SHA256 is1C3F70DE1EDA48B8B3E7CC4F670A3DEF06E9F75FA71FD377CBF92C4C910B1048.
Image base0x82000000; .rdata has equal RVA/raw offset. Strings:
0x82077424 UVEYE,0x8207746C UVSCR,0x8207747C UVCON,0x8207748C ROT,
0x82077490 UVRATE; float0x8205F6E0 is90.0. No image copy retained.

## Bounded installed-content census

Reuse tools/extract_ipk.py's entries() to parse IPK1 header tables, seeking only
selected .mdl/.mcl payloads in sorted out/game/**/*.ipk. Bound headers to20000
entries, each selected compressed/decompressed payload to256KiB, cumulative
expanded text to32MiB, and each scan to30s. Use bounded zlib decompression,
require EOF/exact decoded length, inspect UTF16 BOMs before tokenization. No
files written, no bulk extraction, downloads, new cache or asset uploads.

All1673 IPKs scanned;1404 .mdl records, no .mcl entries;2677866B expanded in
memory, no skipped/oversized entries. All1404 are narrow text, no BOM or NUL.
Raw case-insensitive UVCON search finds0. Tokenized check finds415 UVEYE,0 UVCON,
0 UVSCR,0 UVRATE. Actual UVEYE inputs include model/chara/ene/model_bs01.mdl's
left/right eye materials. This check validates the expected parser vocabulary
and encoding; it is not a scan of arbitrary binary/script payloads or proof
that unscanned code cannot create a driver. Initial scan8.937s; warm typed scan
0.625s (not renderer performance). No new source/artifact index.

Typed scan SHA256 over each sorted archive path (Windows separators), NUL,
record name (Latin1), NUL and complete decoded record bytes:
a5ffffecb023bb8ed07d1a169b2f6e95e0ec7c69a43330081b3a4836a6d2f7ac.
Existing database.ipk3116160B SHA256
D3F3AF162DEA55FF98A1C968685994CABD5E3E009DFD33283A6D9D5846B8E227.

## Implementation, tests and runtime

ObserveEffectDrivers is read-only, bounded256 records; unreadable data returns
unknown, never an incomplete zero count. Only bd_native_materials_verify enables
it. Fixed aggregate counters partition admitted/boundary/slots/exclusions/plan/
layers/effects/output; no address cache, file output, native execution, polling,
eligibility changes or changed comparison. Remove the census when its remaining
scenario-selection purpose retires. C++ fixture checks actual source boundary,
inactive/dormant/invalid/missing inputs, no packed-channel reads or source writes.
Also strengthens the pending-effect fixture with a real later ready duplicate.

421 Python source/scenario guards pass0.233s. Material73/PID37800 build0;
CPU70/PID27768 passes0.12s (CTest0.14s); host187/PID32800/session26758 build0.
Codegen0writes, no guest objects or shaders; exact hashes in cumulative ledger.
No new GPU or pixel test because this changes diagnostic observation only.

Runtime23:24:46..23:25:48,PID31124/session79453, host187. Reused filename981.log
identifies this timestamp/PID, NOT older981 runs. Existing60s/400KiB/192MiB
supervisor,11 settings applied, no raw/image/perf/cache/dumps; exact116B original
profile restored. Exit1: requested positive joint-driver gate not reached,
not crash or comparison failure. The new census question IS answered above.

Post-event FieldActive bg41_01 contexts1718/2018/2318 precede fresh effect
samples1742/2042/2342:817/967/1117 matching transactions,1632/1932/2232 UV writes.
Translation/rotation/transitions stay0. Matching ordinary-branch writes do not
prove scrolling motion; UVEYE records can have zero scroll rates and later writers.
Controller2342:108890matching,18100refused,13803samples,23mixes,13228interior,
13793handoffs,changed0,14585advancing. Late6matching/reused/handoffs. Slot selection
1475matching,27restarts,1233ready,242absent,2refusals. Skeleton2303:14409matching,
wrong0/unavailable0. Material2318:1232override publications,246188checks/wrong0,
184188UVblocks. These aggregates are not individual UV-to-draw provenance.
No new motion/reload/pixel/both-eye/Quest/speedup qualification.

Complete278014B log SHA256
909AC0C686F112D090A27FA85FD34BBCF08FF7EE31D5B864012A352A53801DD2
retained losslessly as retained-effect187.zip50311B, ZIP SHA256
9861ADCE4B92F7956A40C58A393D4E30657ACCFC500B7114225955E2506FE713.
Sole member/name/size/full decompressed hash verified before plaintext removal.
It supersedes186's now-explained missing-driver/ordinary-UV/controller/selection
purpose, so186's ZIP is retired. Keep187 until a purposeful authored-effect
replacement preserves the unresolved coverage evidence. Placement988/root985
and all other protected failure/pixel evidence remain. Full desktop/modern-GPU
requirements unchanged; Quest remains gated.
