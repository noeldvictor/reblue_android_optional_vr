# Native attachment placement, 2026-09-08

Base49d785a. Implemented connected producer-to-skeleton path; CPU/build pass.
**The new authored-placement live gate does not pass.** Run988 ends at its60s
bound without positive attached-placement observations. No identical retry.

## Delivered dependency boundary

`AnimeData_method_4638` (0x822D4638, generated/reblue_recomp.100.cpp:11750)
now has a whole host replacement. On admitted attached inputs it selects the
root from immutable model names/pose identities and samples the existing owned
clip/channel representation. No source node pointer,48B scratch export, name/ID
tree walk or original matrix helper is required by the production placement.
Native source-free matrix composition feeds the existing skeleton and immutable
instance owners. The scoped root is valid for one visual/model generation and
one evaluation, not a new process-wide address cache or persistent model lease.

Verification uses the original placement prefix exactly once. The existing
bdAnimationUpdate hook compares its world matrix (unchanged1e-4 relative rule),
exact clock/appearance/translation-clears and sampler state BEFORE publishing
the native root or running native controller/late/skeleton consumers. The
animation-selection hook compares its arguments and calls the original once.
Failure throws before native publication, never falls back to hide a mismatch.
The original prefix's nested sampler/selection hooks are reference-only; the
reference flag is restored at the pre-consumer boundary and on scope exit.

The remaining world-buffer export supports source late effects/InitBones. Bone
evaluation compares the by-value boundary root exactly (including one-ulp late
writes), then consumes the scoped native matrix on matching identity/generation.
Changed or retired roots are not reused. Final source channel/palette exports,
animation catalog lookup, late effects and other placement callers remain.
Normal bd_native_animation default remains false; no whole host frame claim.

## Recovered source contract

- Child is the ordinary visual view. Parent at child+15188 is a distinct view:
  corresponding fields have offsets8 greater than the ordinary visual layout.
  Do not normalize it by adding8. Its graph is+2628 and world container+2392;
  child's graph is+2620 and world container+2384.
- child+15240=0 returns; +15228=0 keeps only the common update tail. Active copy
  mode is parent+5640=0: copy its current-thread world, select animation from
  parent+1880 (IDs1..10 remap through parent+4*(ID+1387)), then clear only the
  selected root's outgoing translation. No flags/rotation/scale clear.
- Sampled mode uses parent entry+1928, ticks+1904, weight+1892. Missing named root
  selects pose0; missing entry leaves zero sampled fields. Dense clips use their
  first descriptor; named clips use the selected owned key and rest channels.
  Existing compression/Euler restrictions remain explicit admission conditions.
- Parent translation+5592, Euler+5604 and child scale+15216 feed row matrices.
  SetRotationYXZ applies Z,Y,X by left multiplication, giving Rx*Ry*Rz.
  bdMatrixMultiply3(dst,sampled,parent) computes parent*sampled. Translation is
  transformed by parent rotation only, accumulated z,y,x without contraction,
  then added to parent position. Child scale left-multiplies, not scaling that
  translation. Authored quaternions are not normalized; zero gives identity.
- Both attached modes set child slot0 time+1896 from input ticks and copy four
  parent floats+3012 to child+3004. Tail order is bdAnimationUpdate(visual,0,0),
  AnimeData_method_1A60(visual+15336,0), sub_822D3CB0(visual),
  bdVisualObjectInitBones(visual), AnimeData_helper_928(visual+5532), exactly once.
- bdGetCurrentThreadBuffer (file12:2614) selects container+4/+68 using actual
  current thread vs instance_source::kUpdateThread. InitBones (file30:2686)
  copies that world before passing it by value into bdBoneInitSkinned; it does
  not transform it before that call. Its later authored effects remain original.
- Math bodies: Multiply3 file47:9694..9989; SetRotationYXZ file62:9621..10258;
  XYZ builders file99:3421/file78:3389/file48:3395; vector transform wrapper
  file63:9230 and sub_824911F8 file22:21474; scaling wrapper file52:9718 and
  scale builder file79:21850..22068. Ordinary matrix multiplication reuses the
  established contract in20260904_2216_native-render-transforms.md. bdSinCos's
  existing host hook documents measured sine/cosine parity; full composed live
  comparison remains mandatory, not inferred from scalar trig coverage.

Direct caller sites are sub_822B8DF0 (file63:12852, object+6284 nonnull and
child+15236 nonzero) and AnimeData_method_EBA8 (file65:20386, authored branch).
The latter's complete setup condition is not yet recovered here. Those gates
plus child+15240/+15228 must be observed before another placement run. Opening
events alone are not sufficient. This does not resolve985's separate8218FC98
whole cutscene-root observation failure.

## Verification and storage

Material68/PID26572 build0; CPU65/PID20676 PASS0.11s (CTest0.13s). CPU64's new
fixture incorrectly passed3 to a one-joint transfer; the corrected fixture now
requires that rejection, then transfers pose.size(). No production check relaxed.
Coverage: both input modes, disabled/unattached short-circuits, exact thread/view
offsets, unsigned remap edges, missing entry, tick units, NaNs/overflow, source
destruction;24 noncommuting vector-reference cases with signed/nonuniform scale
and non-normalized quaternion; one-ulp late-write rejection; skeleton/instance
consumption and retirement.418 Python source/scenario checks pass0.225s; these
are not C++ behavior or pixels. Host184/PID33632/session21669 build0, codegen
0writes, no guest objects/shaders. EXE49336832B/PDB113524736B; EXE SHA256
255A033A8906F8C91C6CCAD6585A1E9E57C1C597145CBFFC972A64A08755DA41.

Run988/PID34620/session90601,22:06:03..22:07:06. Capture-free60s/400KiB/
192MiB free-drop supervisor; its exit1 means missing requested observation,
not success. No positive placement/refusal line. This cannot distinguish an
unreached callback from disabled/unattached calls; **neither attached mode nor
the new scoped root handoff is live-qualified**. Existing paths at frame2311:
109082 matching controllers,18132 unclassified admission refusals,13827samples,
23mixes,13252interior,14613advancing,13817handoffs,changed0. Late6 matching,
6samples/reuses/handoffs. Single659 matching/sampled,weighted9,refused0;
lookup1100 matching/found,refused0,owned exclusions0. Skeleton2268=14409 matching
evaluations/publications,wrong0/unavailable0. Context2285 is FieldActive bg41_01,
event0. Canonical122->121tracks/93432B import reverified,prepare-refused0.
No motion pixels, reload, both eyes, speedup or Quest qualification.

988log269258B is retained losslessly in
out/build/win-amd64-release/logs/retained-placement988.zip (45113B). Sole member
reblue_988.log full decompressed SHA256:
81FD1C911F4C9C186D7D18C3B648EDDCA82DD4A8A329212A981C4F19B88C4E65.
ZIP SHA256 ED7655643F3BCE40B912AB2876E7ABE3B9610719248F9EB09968DD3B0E9CA851.
Original profile restored116B, SHA256
2F1BC38D763A1B7BDBA31F560684FD4AA7E42A714600B8D344F19DA7F38E23B0.
All producers terminal; timestamp inventory finds no new raw/image/perf/cache/
dump outputs.988 replaces987's unchanged single-joint/lookup/import purposes;
985ZIP and all other unresolved runtime/pixel failures remain protected.
Exact measured storage/cleanup remains in the existing cumulative ledger,
20260906_0333_native-scene-state-bridge.md. No budget reset or new raw allowance.
