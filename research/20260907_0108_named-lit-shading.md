# Named lighting and fog in the live normal material

2026-09-07, EDT. Parent `e712978`, Plume unchanged `3094b35`. Previous goal turn
made verified progress and was pushed. Full desktop host-renderer goal remains.

## Change and source contract

`native_lit_shading.h` now supplies the live normal lit shader's three-light and
two-fog-layer arithmetic. Its named light/surface/fog values contain no shader
register numbers, source addresses, resource wrappers or draw templates. Shared
C++/HLSL execution makes this part of the direct rigid shader reachable in a
small numerical fixture, not only through game boot. These are semantic inputs,
not an ABI permitting raw C++ struct uploads to a GPU constant buffer.

The exact reference is `bd_normal_lit.hlsl` at parent `e712978` (ordinary
`bd_normal_ps`, source hash `FB83DD3F5E67CEB7`). Read its complete light/fog
blocks and adjacent texture/shadow/normal/export paths, the shader substitution
and build rules, and `20260903_1230_the-host-shadow-kernel-and-what-a-host-material-is-not.md`.
No generated source or game assets were changed or committed.

Replaced310 lines of repeated register-machine light/fog operations with named
calls. Preserved directional, point, spot and disabled light behavior; authored
range/cone attenuation; the coloured primary-light shadow subtraction; specular
tint/visibility and zero-shininess semantics; and two ordered fog layers with
radial/planar distance and blend/add/subtract modes. Fog colour and opacity both
include distance falloff: replacing that with a conventional linear fog would
change the authored curve. Directional vectors remain prepared inputs, not
renormalized a second time. Finite reciprocal/log clamps retain the prior math.

Detail textures1/2, normal mapping, environment reflection, the four-gather
shadow kernel/far tier, debug paths, cel export and alpha testing are untouched.
The existing slot/register bindings and texture/shadow frontend remain explicit
adapters. Wind/Toon/other shader variants do not silently inherit this rewrite.

Fresh-use observation is after queue insertion for this exact shader identity,
with host material substitution enabled and occlusion-count substitution
excluded. It counts queued normal-lit uses, not covered fragments, numerical GPU
comparisons or fully host-owned draws. Existing pipeline/shader selection still
uses the source hash; the new arithmetic itself does not.

## Verification

Material16/CPU14 passed; host68 then caught an HLSL-incompatible struct ternary.
An explicit assignment/branch preserves its semantics. Material17/CPU15 and
host69 pass after the correction. Final behavioral test0.11 s/CTest0.13 s,
host retry6.002 s. No guest objects or translated shader cache rebuilt; only
`bd_normal_lit`'s host SPIR-V header regenerated. Its new shared header is a
specific CMake dependency, avoiding a rebuild of every sibling host shader.

The CPU fixture checks1,200 light samples against an independent double-precision
reference, point/spot attenuation, disabled/zero vectors, zero shininess, all
diffuse/specular combinations, coloured shadow composition and204 fog inputs
plus their second-layer applications.52 scenario cases and170 source guards
pass. Emitted SPIR-V was inspected: fragment/multiview entry, clamps/log/exp and
five static gather instructions (one far branch, four normal taps) remain.
This is not a Vulkan numerical-render fixture or D3D12 qualification.

Run916 /owned PID780,01:04:52..01:06:02 EDT, normal flat1920x1080/native MSAA,
precache/pulling on. The prior13 run915 settings plus `bd_host_materials=true`
all took effect; material verification enabled, raw capture and perf CSV off.
The existing runner enforced75 s,400 KiB log and one quality60 JPEG <=160 KiB.

Fresh consecutive `bg41_01` windows at frames2038/2338 after the opening event,
field-state0/player1/event0/movie0/loader0/icon0:

| Check | Observed delta |
| --- | --- |
| Updated normal lit shader | 56,835 queued draws |
| Primitive policy | 44,729 known plans;15,488 matching checks;58,733 native-cull replays |
| Policy limits | 0 cull changes/compound refreshes/unknown plans; runtime policy changes still unqualified |
| Material image/UV | 33,600 publications,300 with overrides;58,621 matching checks;61,510 image slots/58,733 UV draws |
| Scope budget | Peak111,300 B,0 refusals;167,670 cumulative unsupported scopes |
| Canonical geometry/pulling | 2,206 canonical owners,+136,774 draws;149,745 pulled records;0 source-free GPU loads |
| Geometry | 45,021 load-owned draws,1,368 matching checks;942 cumulative unavailable lookups |
| Material values | Diffuse15,488/specular14,748 matching;reflection0 |
| Instance poses | 109,184 reads/checks;0 unavailable/refused/drift |
| Normal image tables | 20,920 lookups,0 original fallback/comparison calls |
| Shadow receivers | 15,488 checks,14,873 receiving;2,973 policies,209 disabled,0 unknown |
| Movement | Same episode,+31 observations,+38.587541 world units;12.802 s walking |

Inspected one full-resolution image of Shu running beside the fence/building.
Terrain/material lighting, foliage and shadows look coherent. Thin black cliff
artifacts and distant blur remain. Neither counters nor this image prove
numerical GPU parity, complete sequences, authored light/fog variation, reloads
or both eyes. No speedup is claimed. Source object/pass setup, lights/fog input
ownership, shader-binding ABI, source index and captured templates still block
the direct static-object outcome. Reuse this evaluator while completing that
path; do not substitute more generic helper conversions for its acceptance test.

No new raw/perf/cache/dump files, no owned producer remains, original116-byte
profile restored exactly. Existing mesh cache unchanged. Artifact SHA-256:

- Host exe48,264,704 B: `c3fb67d09a41fe59830bdba38edb844cfa60b1290adc5114b11eb48049e88208`.
- Material test exe: `6b471c18e018a37d3832ee140cc8b3434db6c253ae72811189e59ac8d2fb94c8`.
- Normal host shader header298,505 B:
  `8b49814741b70271479037b19e959ad336d4e22fb2d31a0350917c853b9bc99b`.
- `out/build/win-amd64-release/logs/reblue_916.log`,231,076 B:
  `b4387756bf1c37f7c4c9d9ca5e0d1108724d0800db34d2c6d54b245992e5ce1f`.
- `out/verification/native_lit_shading_window.jpg`,136,607 B:
  `2e757619350abb343ff0369e3d06952cb4def5755a8bbf8437e86254d502493c`.
- Restored profile: `2f1bc38d763a1b7bdba31f560684fd4aa7e42a714600b8d344f19da7f38e23b0`.

## Storage

Same cumulative ledger `20260906_0333_native-scene-state-bridge.md`, original
65,462,788,096 B baseline, approved3 GiB checkpoint exception only, operational
floor62,509,998,080 B,100 MiB diagnostics/10 MiB logs/10 MiB images/raw0 unchanged.

After validating the replacement, removed14 exact superseded outputs: run915
log/primitive sanity image; material15/CPU13/host67 logs; intermediate material16/
CPU14 and resolved failed-host68 logs.363,770 logical B; immediate free
63,352,668,160 ->63,353,044,992 B, measured376,832 B (368 KiB) recovered once.
Small prior reports/hashes remain, exact old runtime image/log does not. Build
logs can be reproduced. No protected raw/baseline/failure/motion evidence touched.

Comparable fixture/log/image growth59,550 B: fixture+46,673 B, aggregate logs-4,769 B
(now146,229), runtime log+14,779 B, image+2,867 B. New numerical/live-shader coverage
justifies this retained growth; replace by purpose at next equivalent check.
Normal shader header shrinks16,426 B, exe/PDB shrink5,632 B. Other object/metadata/
source/docs/helpers/Git sizes lack complete byte baselines, not zero. Images total
10,243,007 B;242,753 B overlap headroom. Cleanup ending free63,353,044,992 B
(~59.00 GiB), drive-wide gain336,257,024 B from this turn's first measurement;
only376,832 B is attributed to this cleanup.
