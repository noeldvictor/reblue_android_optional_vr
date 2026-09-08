# Indexed animation ownership and first-match import verification

2026-09-08, after717b499. Full desktop host-renderer goal remains incomplete.
Normal animation defaults stay off; no Quest work or performance claim.

## Connected result

The existing asset/clip/residency/layer/skeleton/instance path now admits indexed
T/R and T/R/S inputs as well as named linear/cubic clips. Native assets carry
joint-index versus name binding and channel participation, not source descriptors.
Indexed assets need no synthetic name table. The existing8MiB/4096-entry owner
charges actual retained capacity, including retired leased payloads. Source keys
are read at selected-slot preparation, not by the steady-state sampler.

Indexed whole reset writes model headers; weighted application preserves them.
Indexed traversal ignores named exclusions and the keyed forced-subtree flag,
including following siblings as the source does. T/R-only application leaves
scale flags and bytes untouched, even when those bytes must not be interpreted.
Masked native channel blending shares the original tested interpolation math.
Both types reuse existing original comparisons before publishing replacement
records and leave sampler globals unchanged. No new renderer/pose framework,
shader, disk cache, guest rebuild or capture producer was introduced.

The outgoing48-byte compatibility records, original slot clocks/controller,
late writers and source-backed dormant catalog still exist. This connection
does not claim a fully host-owned frame or eliminate the final channel adapter.

## Source contracts

Read guest-source/devloop/disk policy, census/frame-interpolation hook TOMLs,
model ownership frontier and current queue. Reuse previously recovered channel,
quaternion, clip and loader lifetimes rather than repeating those audits.

| Original boundary | Recovered behavior |
| --- | --- |
| `sub_82288C00`, file60:9706 | Complete direct T/R walk:12-byte descriptors indexed by node+0; pointers+0/+4, counts+8/+10. Clears flags, writes node hash, recurses children and following siblings. |
| `sub_82288DC0`, file24:10102 | Complete direct T/R/S walk:20-byte descriptors, pointers+0/+4/+8 and counts+12/+14/+16. Scale uses bounded scan/final hold. Passed parent-scale argument is not consumed at entry. |
| `sub_82289EF0`, file95:10198; `sub_82289FA8`, file88:9743 | Complete weighted walks reuse T/R/S blend helpers and authored rest fields. Do not write headers or consult keyed name/exclusion/forced-subtree controls. Type0 does not call scale blending. |
| `sub_82289888`, file50:9867; `sub_8228A3E8`, file43:9917 | Complete dispatchers: whole reset clamps clock and zeros all records; preserve does not clamp and has epsilon no-op. Dense branches neither set nor clear sampler globals. |
| `sub_82288680`, file66:9955 | Completed initializer supplies samples per30Hz tick/duration. Only types2/3 build descriptor name hashes. |

The inspected HDB relocation functions82198DE0/98F30/98FF8 use36-byte named
records. They do NOT establish that live indexed content has passed a complete
12/20-byte load/fixup path. The checked importer accepts the completed relocated
representation, but real dense loader/content observation remains required.
Inherited nonzero compressed-mode state and nonzero Euler modes remain explicit
runtime refusals. Model import still drops optional ambiguous name tables;
although indexed pure application needs no unique names, that model admission
boundary has not yet been changed. Named binding still refuses duplicate model
hashes; it must not guess the original cursor's separate repeated-model semantics.

## Verified first-match fix: host174/run982

Host174 built committed717b499 code (only ledger dirty). No guest objects or
shaders rebuilt. Capture-free mono, native model/instance/skeleton/animation
comparisons on; PSO precache, mouse menu, perf CSV and captures off. Original
profile restored byte-for-byte and its SHA256 independently checked.

Run982 positively imported122 descriptors as121 unique tracks/93424B, with
prepare-refused0. Address25B5EF7C differs from981; it is not persistent identity.
Last frame1433:7590 sampled/checked,wrong0,whole8,preserved7582,weighted113,
mixed/checked8,cubic6,filtered/subtree0. All1070 registrations admitted,
16 resident assets1290176B; one model unavailable. PID29656 stopped19:06:00,
operator exit1 explicitly means diagnostic-observed stop, not scene acceptance.

This replaces981's duplicate-descriptor refusal provenance with a successful
changed-code observation. It does not prove named filtering, advancing/interior
keys, diverse dense content, full field/reload, skin pixels or both eyes.
Preserve979/980's insufficient weighted/subtree/mix coverage and975/971/962's
reload/pixel failures. No unchanged retries or relaxed thresholds.

- Host174 EXE49235968B, SHA256
  `8F1AFCBE9BEC892BC3154296381BB20A77C79E9DFFE8E3328FAFFC784A13D0DC`;
  PDB113000448B. Binary subsequently replaced by175; do not restamp this run.
- Run982 log149357B, SHA256
  `AAFCE6EB640ECAF3076FC93AC8524B1F39415F8B47646F03B4B5F4004FAB49EE`.

## Indexed verification: host175, no live run

Material58/PID37628 build0. CPU56/PID28036 PASS0.12s/CTest0.13s;410
artifact-free source/scenario tests PASS0.221s. C++ fixtures exercise129 advancing
times per indexed format against equivalent named inputs, reordered native joint
IDs, exact budget/cap-minus-one, null counts, unreadable active keys, address
overflow, weighted header preservation, ignored name/exclusion/subtree controls,
untouched nonfinite scale bytes, missing-channel rest blending, source destruction,
and retired assets feeding immutable native completed poses. Existing exhaustive
angular/cubic/blending, model/material/lifetime tests remain passing.

Host175/PID31752 build0: one private animation bridge object plus link; codegen
0writes, no guest objects/shaders. No further game run:982 had no dense loads,
so replaying its opening sequence cannot establish the missing indexed coverage.

- Current host175 EXE49236480B, SHA256
  `D6DF442958BCDF7BDA70CBEA34EB80563B3533CD21B4A4335B195D4A9FEDC59F`;
  PDB113004544B.
- Material58 EXE1208832B, SHA256
  `E3861776CAA071CAC907912989F0CA0B1C316AEFAAEC381FBB06B0EC4911E4A5`;
  current fixture tree10247849B/43 files.

Next ownership bundle: native controller/layer channels and clock selection
through completed-pose handoff, removing outgoing scratch from active consumers.
Recover real authored layer/subtree/indexed scenarios for the missing live gates;
do not let an unobserved format block independent active-path ownership work.
Persistent cooked identities, special/late bones and full desktop scene/event/
sequence/both-eye acceptance remain mandatory before Quest2 optimization.

## Storage

Same cumulative ledger and limits, no reset: floor62509998080B,75MiB runtime
stop/100MiB diagnostics,10MiB aggregate build logs. Runtime982 reserved409600B
overlap and192MiB free-drop; only149357B text retained. No new raw/perf/cache/
dump/image files. Existing raw/image gates and protected evidence remain.
Retired superseded981/host173 text only after174/982 verified its purpose;
retired57/55 test logs only after58/56 passed. Exact reclamation/net drive figures
are in the shared scene-state ledger, with unrelated/unattributed drive growth
kept distinct from measured file growth. All producers are terminal.
