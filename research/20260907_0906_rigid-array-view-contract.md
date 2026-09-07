# Native rigid sampling matches production array views

2026-09-07 EDT, parent ce56e24. A focused prerequisite for the direct scene
consumer, not another completed renderer-ownership milestone.

## Change and evidence

`NativeTextureGpu::Upload` and `NativeTargetImage::Create` publish explicit
`TEXTURE_2D_ARRAY` views even for single-layer images. The native rigid pixel
shader and its old fixture instead used plain `Texture2D`/default views. A new
source regression failed on that mismatch before the fix.

The production shader now samples albedo and shadow as arrays, with explicit
layer zero shared by both eyes. `SV_ViewID` still selects the per-eye camera,
not an ordinary material or mono sun-shadow layer. Descriptor-schema comments
record this requirement. No asset conversion or data-layout change is needed.

The existing GPU fixture now binds explicit single-layer array views for both
images and uses the production `D32_FLOAT_S8_UINT` sun-shadow format, followed
by the producer's `SHADER_READ` transition. Depth readback explicitly copies
only the depth aspect; the generic combined-format buffer-copy path would select
both aspects. The albedo oracle remains floating-point, not a compressed-texture
upload test. Real production programs, bindings, draws, fence wait and pixel
readback remain in use. No substitute test shader or second renderer was added.

Build25/PID4756 compiled only the changed rigid PS and two GPU-fixture objects,
then linked the existing target. Rigid04/PID23632 passes four 8x8 two-eye cases
on RTX3060: 128 pixels/case, maximum error0.0000404567, Vulkan validation
errors0/warnings0, 1.15 s fixture/1.16 s CTest. The known missing GOG overlay
manifest emitted one loader diagnostic. All276 Python boundary/scenario checks
also pass; these remain source/parser checks, not pixel evidence.

| Retained output | SHA-256 |
| --- | --- |
| GPU fixture executable | `FDEE1BB8AB67B960AE7D84CEE0A0517BE6C9BF6D3793728A4E61E3E815718783` |
| `out/verification/attachment_resolve_rigid_pixels_04.stdout.log` | `74FD31D57601597FBF1636E78FEC56CEA6AB85DAF4249A9B1AB4F137080871D6` |

No host executable rebuild or game launch: no live game scene consumer uses
this PS yet. The last field-qualified binary remains host93/run935, SHA256
`CEAADAE41F5B4D964C792F469BD1566620606E2EEEB168160603D626F85C8BF1`.
The owner's116-byte profile is unchanged. This proves the sampled-view/shader
contract in the focused fixture, not live scene routing, reload, game both-eye
qualification, source-free GPU loading or any performance improvement.

## Storage and next connection

Same cumulative3 GiB exception and62,509,998,080 B operational floor; raw0.
Preflight62,715,219,968 B free, no active producers; estimated fixture overlap
under16 MiB. Reused the bounded build wrapper with256 MiB free-drop stop,
300 s supervisor,30 s CTest and10 MiB aggregate build-log cap. No new build
tree, download, capture, cook or runtime evidence.

After replacement validation, removed four exact superseded agent-created logs:
GPU build24 and rigid03 stdout/stderr.3,997 logical B removed; immediate free
62,715,523,072 ->62,715,531,264 B: **8,192 B actually reclaimed**, credited once.
Their old exact text is gone; findings remain in the direct-caster report and
the tests can be rerun. Host93/run935/image and all distinct protected evidence
remain. Keep build25/rigid04 until their coverage is replaced.

GPU fixture tree10,433,599 B (+13,430), aggregate build logs212,743 B (-49):
known component growth13,381 B for the expanded fixture. Source, generated PS,
CMake/Git metadata and unrelated volume activity are not fully attributed.
Cleanup-end58.41 GiB free,311,296 B more free than preflight; that drive-wide
gain is not all cleanup. The existing cumulative ledger owns these numbers.

Next is the same direct object: publish fresh completed shadow/receiver values
at the actual consumption timing, run native per-node light selection before
the old shader callback, and feed the retained packet into direct scene
submission. Group those changes around one live consumer. Then prove native
batching and interpreter/template-free cold load and teardown/reload before
expanding material families. The view fix does not move those exit gates.
