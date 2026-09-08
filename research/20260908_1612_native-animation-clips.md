# Native keyed animation ownership prerequisite

2026-09-08, after2e7745f. Source/CPU work only: the game still produces animation
channels through the original routines. No new host binary, live run, speedup,
motion/stereo qualification or completed host frame is claimed.

## Delivered connection and remaining runtime boundary

`NativeAnimationClip` owns model-bound T/R/S tracks, seconds and Euler turn
fractions; it retains no source pointer, name hash, packed record or callbacks.
An explicit remaining-byte budget bounds retained vector capacities. Import has
4096-joint/descriptor limits, checked 64-bit address arithmetic and transactional
refusal; temporary binding maps are bounded by those counts. There is no new
registry, disk format/cache, output asset or duplicate animation framework.

The existing CPU fixture exercises import -> source destruction -> native joint
channels -> `EvaluateNativeSkeleton` -> `NativeInstanceRegistry` publication and
completed handoff. Authored hierarchy, scale compensation and pose leases reuse
the production types. This is a useful prerequisite, **not runtime removal** of
guest animation. The existing game bridge still calls `ReadChannels` on original
48-byte records. Load/slot binding must retain clips under the existing model/
instance lifetime and aggregate budget before switching that producer.

## Recovered source contract (reuse this audit)

Read guest-source/devloop and relevant census/frame-interpolation hook TOMLs.
No existing host replacement for the skeletal clip dispatch/samplers was found.
The similarly named `bdAnimCurveSample3` uses an unrelated curve layout at+52;
it is not the skeletal clip entry. D2Anime loading is not the skeletal loader.

| Boundary | Translated source | Contract |
| --- | --- | --- |
| Visual animation update | `bdAnimationUpdate`,0x821414A0,file53:2663 | Clears per-channel dirty128, advances active56-byte slots, dispatches whole/subtree sampling and layer blend, then collision/effect and UV side effects. Keep non-render side effects; do not replace this entire body just to remove a scalar sampler. |
| Slot selection | `bdVisualObjectSetAnimation`,0x82141298,file86:2636 | Visual+2248 clip lookup; slot stride56, ID+1872,loop+1876,wrap count+1880,blend+1884,time+1896,clip object+1920. Same selected object is a no-op unless forced; replacing selection resets time/wrap. |
| Loaded clip lookup | `bdAnimClipLookup`,0x8218BA98,file22:4256;`sub_8218FC10`,file21:4425 | List entry+8 ID,+4 next; poll asynchronous state+36. On completion, request object+184 is published to entry+12. Null/in-flight must not be treated as a ready clip. |
| Standalone load | `sub_8217BD70`,file85:3829 | Parse source via`sub_82198DE0`, write parsed clip to loader+184, then initialize with`sub_82288680`. Exact retirement callback still needs audit before runtime ownership. |
| Packed load | `sub_8217C5E8`,file71:3908 | Initializes pack index;96-byte entries. Actual data is parsed and initialized, aliases reuse an earlier matching entry's clip pointer. Runtime ownership must cover both load paths and shared aliases. |
| Header/fixup | `sub_82198DE0`,file98:4732;`sub_82198F30`,file56:4524;`sub_82198FF8`,file12:4789 | Header pointer+0,durationu16+4,typeu16+6,recordcountu16+8,ratefloat+12. Type2 records36B: T/R/S pointers+0/+4/+8,counts u16+12/+14/+16,name bytes/hash+18. Fixup uses field-relative offsets before they become pointers. Import only the completed relocated BE representation. |
| Load initialization | `sub_82288680`,file66:9955 | If header+16>0, integer rate+12/30, else1; duration divided by this rate and+12 rewritten as samples per30Hz tick. Types2/3 names at record+18 become hashes through`bdSceneNodeBuildTransform`. |
| Key count writer | `sub_821995B8`,file4:4358;`sub_82199628`,file105:4595 | Swap exactly count records, strides16 T/S or8 R. They do NOT allocate/swap count+1. |
| Whole clip | `sub_82289888`,file50:9867 | Clamp time0..header duration, clear48B*graph+12 channels, multiply by header+12; graph+16 root. Dispatch type0/1 to dense routes,2/3 to keyed routes with compressed-mode global. |
| Weighted/subtree | `bdVisualObjectAnimSlotUpdate`,file89:2624;`sub_8228A3E8`,file43:9917 | Slot time/weight clamp; unweighted mode uses whole clip. Weight1/no forced subtree/no exclusion may use keyed route preserving flags; otherwise weighted recursive route. Not replaced by the new sampler. |
| Keyed application | `bdAnimKeyframeSample`,0x82289150,file89:9299 | Dense output by node+0; match node+4 hash in36B descriptors with traversal cursor; optional global subtree name/depth restriction. T+8,R quaternion+20,S+36;flags1/2/4,dirty128 for multi-key. Writes node hash+4; follows child+56 and sibling+60. No new pure sampler claims those side effects. |
| T/R interpolation | `sub_82288760`,file68:10124;`sub_82288898`,file99:9817 | Signed16 key time. Binary search upper bound is count, but loader count is authoritative. Exact key copies; interior weighted interpolation; R corrects >32768 or <-32768 unit differences, ties do not wrap. Preserve endpoint quaternion sign. |

Native importer type2 only; unsupported type0/1 dense and type3 compressed
families stay explicit. T/S values are finite float3; R signed16/65536 becomes
exact turn fractions. Active multi-key curves must start at0 and bracket the
clip's full duration; otherwise refuse before any output, rather than reading
an imaginary extra key through the source search's inclusive bound. Constant
keys ignore their time. Missing channel pointers mean absent even if their
inactive count is nonzero. Duplicate record/model hashes are refused rather
than guessing traversal-cursor ambiguity. Unmatched records are not inputs to
the bound model. Clocks are seconds; duration=header duration/30 and key time=
source sample/(30*samples-per-tick). This sampler clamps; loop/state advancement
belongs to the pending slot controller, not an implicit sampler modulo.

Read two constants from the owned executable in memory only:0x8208EA70 is
9.58738019107841e-05 radians/unit (`0x1.921fb6p-14f`);0x82063D84 is0.5.
No decrypted executable, key, image or asset was printed/written. Full runtime
comparison is still required for time conversion, blending and SIMD differences.

## Causal regression and verification

Material47/CPU45 passed the initial fixture. A stronger independent source-unit
reference then FAILED material48/CPU46: converting each packed key to float
radians before comparing its angular span can make an exact half-turn slightly
greater than pi and reverse the interpolation arc. Endpoint-only matrix checks
miss this. The fix retains exact binary turn fractions through interpolation,
then converts to radians and composes qZ*qY*qX. No tolerance was loosened.

Material49/CPU47 passes0.12s (CTest0.14s), including384 half-turn/adjacent-span
cases across nonzero key offsets, both wrap directions, exact endpoint signs,
noncommuting axes, clock conversion, constant/missing channels, unaligned hashes,
count/bounds/truncation/nonfinite/duplicate refusal, source destruction and
completed-pose lifetime. Exact retained-capacity budget and budget-minus-one
cases pass.403 Python source/scenario checks pass in0.246s, including the native
clip ownership guard; they do not replace these C++ behavior checks.

No runtime clip hooks or defaults changed; host167/run975 remains the latest
host binary/runtime with its preserved full-reload failure. Do not retry it
unchanged or infer motion pixels from these CPU results. Native slot/layer
production, types0/1/3, late writers, secondary/view-dependent bones, outgoing
palettes, full desktop events/sequences/both eyes and Quest remain open.

## Storage

Existing material tree and bounded build operator reused; no game/host/shader/
guest build, new tree, raw/image/perf/cache output or cooked asset. All build/test
handles are terminal. Exact sizes, retained-current/failure logs, scoped cleanup
and ending free space are in the cumulative scene-state ledger; no budget reset.
