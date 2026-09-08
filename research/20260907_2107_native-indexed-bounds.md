# Native indexed bounds and per-primitive occlusion (2026-09-07)

## Connected ownership change

`BuildNativeMeshBounds` derives bounds from checked v2 little-endian indexed
Position.xyz values, including signed base vertex, before GPU allocation/map.
Unused stream vertices and Position.w cannot enlarge the box. Both import and
source-free asset load share `native_mesh.cpp::Upload`; the resulting native
geometry retains values after source/CPU payload retirement. Existing files,
checksums, content identities and disk budgets are unchanged: no recook, new
format, sidecar or duplicate asset payload. V1 has no guessed position schema.

The existing native rigid VS uses `float4(vertex.position.xyz,1)` transformed
by `object_data.world` (`native_rigid_vertex.h`). Bounds use that same packed
matrix, not the walk's radius scale. Affine interval arithmetic supports shear,
reflection and nonuniform scale, with outward FP32 rounding/error padding;
invalid/non-affine/overflow inputs stay visible. Whole-node material/light
effects and sibling preflight still finish before culling. Then each pending
primitive requests/consumes its own query, keyed by instance/model generation/
node/primitive. Missing bounds or one sibling's visibility cannot hide another.

The broad node sphere and `world_bounds` handoff are removed from this native
consumer. The host walk's remaining source discovery/frustum/distance adapters
are not removed or relabelled native. Only recognized native rigid scene draws
use this path; skin/wind/unsupported material consumers remain tracked elsewhere.

The native query push ABI is now96 B: exact camera rows, world box center and
independent half extents. Boxes inflate conservatively per axis, including flat
geometry; the complete proxy must remain beyond homogeneous near/eye planes.
Clip-plane testing adds an FP32 error envelope. Exact camera/depth identity,
bounds, originating-frame age, two-zero warm-up and fence requirements remain.
This does not solve moving-camera or moving-occluder temporal visibility.

## Verification before the live gate

327 Python source/scenario checks pass (0.150 s). Mesh13/CPU11 pass0.12/0.14 s;
the first mesh12 compile exposed only a Windows max macro in the new test,
corrected before retry. Output40/CPU23 pass0.36/0.38 s. Tests cover unused large
vertices, signed base indices, source destruction/file round trip, unchanged
identity/bytes, malformed/nonfinite data, flat geometry, rotated/scaled/sheared/
reflected corners and large translations/overflow. Occlusion cases retain prior
camera/near/lifetime/capacity safeguards and add anisotropic boxes and sibling
identity separation.

GPU42 recompiles the shader and links the actual mesh-bounds producer into the
existing fixture. Occlusion5 passes all8 mono MSAA1/2/4/8 × translated/rotated
cases in1.09/1.11 s, validation0 errors/0 warnings. Indexed native boxes transform
through two real submissions/fences; only the hidden sibling receives Occluded.
All first-submission HDR/depth pixels remain exact and the descriptor-resume
check passes. This fixture is not an oracle for skipped game-object pixels.

Host120/PID33752/session92497 exits0:38 host steps, codegen0 written, no guest
objects. Actual binary stamp edbcd32 dirty; later commits do not restamp it.
Exe48,708,096 B SHA256
`80E06B21649EA0BACE29CBEE31848F413ED2A29E16B64114EC795BDA9109DDA3`;
PDB109,707,264 B SHA256
`33A6B8BA495325680411369542E2C39D4CB7BB9DFDBE6751250EC59C35C729E4`.
GPU fixture SHA256
`7D74CB68C34ACA3ACB964BE5B00EE74D27BEDF11BAC5C2D2021B89EC2C052B74`.

Run960/PID33516/session76067 started21:07:49 with full strict cold/reload flags,
Count0, one110 KiB owned-window JPEG,300 s/800 KiB text/192 MiB free-drop/75 MiB
diagnostic overlap/original floor, and guaranteed profile restoration. Its final
acceptance is not recorded at this source checkpoint. Early152 native primitive
skips prove the live branch is reached, not visual correctness or speedup.
Keep945 accepted pixels,956 tree-gap and957/948/941 failure evidence. The earlier
940 generic mono reload JPEG was retired after checking its supersession by945;
its hash/findings remain in1140. See the [same cumulative storage ledger](20260906_0333_native-scene-state-bridge.md).

## Final live result: culling reached, reload gate failed

Run960 terminated with exit1 at21:12:50 after its300-second limit. The supervisor
reported that the load-owned model/material comparison had not reached matching
fresh field samples; the retained-log parser identifies the missing prerequisite
as completed same-process selected-asset reload. There is no reload-qualified or
complete marker. This is a failed acceptance run, not a passed reload or a proven
culling regression. No game image was produced: capture awaited that full gate.

The last cumulative sample at frame8400 records49,457 native requests,24,011
queries submitted and fence-collected,12,091 zeros and1,026 native primitive skips.
Decisions: invalid-view0, invalid-bounds25,446, ambiguous0, capacity0, no-history758,
changed-depth10, changed-camera20,361, changed-bounds336, stale0, warming80,
visible1,440 and occluded1,026. History ends empty after the scene changes.
Frame1500 already had152 skips. These are real native consumer skips, not unique
objects, matched-state pixel proof or a measured FPS gain. Prior run959 queried
nodes rather than individual primitives; its totals are not a timing comparison.

Cold generation93/instance144 qualifies at21:08:53.944 with scene/shadow emitted
789->1689, satisfying both900-emission windows. It then retires its source and
closes at title with1690/1690/1690 submitted/emitted/fence-retired for both paths.
Reload generation207 loads at21:09:09.293; instance385 reaches the native route.
FieldActive bg41_01/event0 samples occur, but no reload qualification follows.
At21:10:55.189 its source retires with scene and shadow each2228/2228/2226.
Generation255 then loads and autoplay leaves the test field. Those matching
scene/shadow totals do not establish why the continuous gate failed.

The unchanged `native_rigid_lifecycle_bridge.cpp` requires unpaused walking in
stage `(2<<32)|4101` and an observation younger than250 ms. The output window
resets on lost readiness or generation changes and requires900 additional scene
and shadow emissions continuously. Existing logs omit reset/freshness causes.
Next obtain bounded reset provenance and separately timed culling pixels; do not
raise250 ms, lower900, relabel skipped draws as emissions or retry unchanged.
Moving-view/occluder safety, controlled visibility and both-eye game acceptance
remain open. Preserve the tree-gap and prior legacy UV/light failure evidence.

Retained log: `out/build/win-amd64-release/logs/reblue_960.log`,780,656 B, SHA256
`517D72619B74980777D0E2C1C737834DD0728B7CE5EF44228CEB4575F36C5DCF`.
The116-byte normal profile was restored byte-for-byte, SHA256
`2F1BC38D763A1B7BDBA31F560684FD4AA7E42A714600B8D344F19DA7F38E23B0`.
Source is committed/pushed as6f0838f; host120 retains its actual edbcd32-dirty
build stamp. All producer handles are terminal. No rebuild/boot is needed to
publish this result.

Final storage accounting is in the same cumulative ledger. This continuation
retired the superseded940 JPEG and14 replaced fixture logs:15 files/134,170
logical bytes total. Counted retained net is+1,439,545 B, chiefly expanded fixture
coverage and the distinct timeout log. Three new reusable material-cache records
total204 B; no raw or new image output. Last recorded free space81,040,945,152 B,
213,766,144 B below the starting drive reading; that drive-wide change is not
wholly attributable to the counted files. Prior cleanup is not credited again.

## Readiness/culling observation follow-up (host121, 2026-09-07)

Source checkpoint9c4a6b7 separates observation from acceptance, without changing
rendering or autoplay movement. `NativeRigidReloadReadiness` preserves paused/
walking/stage and exact250 ms freshness checks. The bridge reports at most64
window transitions, with blocker bits (1 paused,2 not walking,4 wrong stage,
8 stale), observation age, input poll gap, autoplay blockers/episode and actual
scene/shadow window endpoints. No new guest reads or per-tick logging.

330 Python checks pass0.151 s. Output41/PID26688 and CPU24/PID37792 terminate0;
the C++ fixture passes0.36/0.38 s, including249,999,999 vs250,000,000 ns and the
loss of899 prior emissions on stale observation. No shader changes or fresh GPU
fixture claimed; prior occlusion5 remains the actual query/binding evidence.
Host121/PID38032/session58654 terminates0,18 scheduled host steps/codegen0 written,
no guest objects. Binary stampfc8fdbf dirty, not the later source commit.
Exe48,711,168 B SHA256
`5BED5D92099DAC39BD3369E541BF1C31AAE45677A2631A8E65F8E14250D4792A`;
PDB109,723,648 B SHA256
`BFBC27D8D742F9DED02D0102463EE8E616194EBBD8C5A7EF6F4C43476BC7B5A9`.

The existing scenario tool adds the standalone read-only command:

```powershell
python -B tools/native_instance_scenario.py <owned-current-log> --occlusion-observation
```

It bounds input to800 KiB and requires consecutive recent post-event field
contexts with advancing native requests, queries, collection and skips; counters
before the latest context, shutdown, teardown, mismatches or stale/zero deltas
cannot trigger capture. It rejects combination with acceptance flags and prints
OBSERVED, never a reload/pixel PASS. The ignored existing supervisor invokes it
once per new query sample, then takes one bounded owned-window JPEG. It continues
the unchanged full strict reload chain afterward; the JPEG cannot satisfy that
chain or change its exit status. PowerShell parsing and artifact-free scenario
tests pass. Completed960's last loading context correctly returns Pending.

Run961/PID33484/session3602 starts21:33:41 with the same21 effective settings,
300 s/800 KiB/192 MiB/75 MiB overlap limits, Count0 and110 KiB JPEG reservation.
Source and profile checks precede launch; existing finally cleanup owns shutdown
and byte-exact profile restoration. Final live result follows below.

### Run961 final: strict text gates pass, requested image missing

The owned process terminates21:35:45; the supervisor exits1 for incomplete
requested window inspection. Full strict cold/reload/material/receiver/lighting/
cutout checks pass before shutdown. This is a text-gate pass, not an overall
acceptance pass. There are no new game pixels. The original profile hash remains
`2F1BC38D763A1B7BDBA31F560684FD4AA7E42A714600B8D344F19DA7F38E23B0`.
Retained log506,703 B SHA256
`7AA01668CDEA82D187C1204B3109D2C5FF7D057FCA74A6DF15A1F7BF4408675C`.

Cold generation93/instance144 qualifies scene/shadow759->1660, then closes at
title1661/1661/1661. Reload generation207/instance385 qualifies762->1662 at
21:35:43.625. Exactly two readiness starts are logged; no reset is observed.
Run960's failure remains unexplained: non-reproduction is not a fix.

Last query frame4800:27,193 requests,7,702 queries,7,697 collected,3,396 zeros,
304 native skips, history5. Refusals:19,491 invalid bounds,72 no history,
10 changed depth,6,860 changed camera,70 warming,386 visible,304 occluded;
other reasons0. All304 skips are already present at frame3900, before the first
reloaded interactive context4084. Subsequent4200/4500/4800 samples have no new
skips. The fresh-culling trigger correctly does not accept event-era totals as
interactive visibility. It does not prove that no individual interactive frame
could ever cull; the evidence is this bounded run's sampled windows.

Fresh reloaded4384..4684:2,450 native scene emissions/2,449 retirements; wider
casters33,226/33,232, textured scene900/900 and textured shadow300/300. Scene
indirect calls2,450 and merged instances0: this window is singleton-only and does
not supersede959's distinct batching proof. Movement episode1 advances143.893886
units; layered/inherited/both-eye coverage remains separately unqualified.

The supervisor also exposed a sequencing mistake: it stopped on the completed
text gate even though a separately requested image remained pending. Its local
condition now requires both before success shutdown, under the same300-second
limit; parser syntax checked, not live-retested. No fresh boot merely to wait for
different framing or a readiness reset. Next change must address native
moving-view/depth visibility with current-frame native depth/queue consumers and
causal GPU coverage. Keep exact-camera history safety until its consumer is
replaced; do not lower900, raise250 ms or relax freshness/image triggers.

Retired only six replaced output40/CPU23/host120 stdout/stderr logs,7,639 logical
B, after output41/CPU24/host121 and current strict reload proof. Findings/build
hashes remain here; commands/source reproduce these superseded logs. No image,
raw, profile, game data or failure log removed. Immediate free-space gain12,288 B
is not isolated from volume activity. Counted retained net+539,321 B, chiefly the
new current-runtime log and expanded CPU fixture; other host objects/CMake/source/
Git are not allocated. Ending measured free80,978,022,400 B is16,195,584 B below
this continuation's starting reading. No new cache/perf/dump/raw/image outputs.
All producer handles are terminal; the same cumulative allowance/floor applies.
