# Fence-owned frame capture and bounded JPEG correction

Desktop Vulkan; full host rendering, scene/stereo acceptance and Quest remain
incomplete. No FPS or culling-correctness claim. Continues
[run962's image discrepancy](20260907_2256_current-depth-runtime.md); uses the
[same cumulative storage ledger](20260906_0333_native-scene-state-bridge.md).

## Connected ownership

The existing post-gamma screenshot consumer retains request, frame, input image,
output image and descriptor identities. Its readback buffer belongs to the actual
submission slot until `DrainSlot` has waited its fence. Cancellation invalidates
publication but cannot free an in-flight GPU buffer. Present-index arithmetic is
removed. Vulkan copy/host-read visibility, row pitch and BGRA-to-RGBA conversion
are shared with the GPU fixture. External XR images without a copy-usage contract
refuse; this is not stereo qualification.

The default-off `bd_native_frame_probe` accepts one <=64-byte request, restricted
to a <=120-frame interval. It permits a <=2048x1200 image, shared <=64 MiB readback
storage and an exclusive, <=110 KiB JPEG output. The existing scenario checker
requires matching recorded/saved identities and ordering, actual JPEG decode,
dimensions/bytes and fresh current-depth field context. That is provenance,
not pixel or reload acceptance. The inspected local supervisor keeps those gates
separate, restores the exact owner profile, and writes no raw frames or perf CSV.

## Evidence and failure

- Screenshot1: eight Vulkan pixel cases, RTX3060, validation errors/warnings0/0.
  Widths1/65/257/2048 exercise padding, exact channels, later source overwrites,
  release after the actual fence and exactly-once collection. No disk images.
- Host124 failed on undeclared `kCvarGroup`. Adding its owning settings header
  fixes the build; host125 links with actual stamp08d7525 dirty, no guest objects.
- Run963/PID36428/session57817, September7 23:53:46..23:55:56: the full strict
  cold/reload receiver/lighting/caster/cutout chain passes. Generation93/instance144
  retires before207/384. Fresh4683..4983 yields3,281 generated/draw-recorded and
  collected commands,3,274 visible and7 culled instances. These counts do not
  qualify visibility correctness or speedup.
- Request `1 4983 5103` records frame5070/slot0, input0170E4397050,
  output017046440570, descriptor99,1920x1080. A matching real-fence saved receipt
  follows. However, the65,536 B JPEG fails strict decoding as truncated. The
  supervisor exits1, preserves the failure and restores the116-byte profile.
  No image was accepted or reconstructed from partial data.

| Retained run963 evidence | Bytes | SHA256 |
| --- | ---: | --- |
| `logs/reblue_963.log` in the build tree | 535,227 | `DF96F670913A3F5FEA1AD64332AD9BB4D9E136371A0930D902FF3BDD9B116780` |
| `out/verification/native_frame_probe_window.jpg` (truncated) | 65,536 | `F0402D31CD1A085747F697E27AA789E47D9092AEA38DEC30C9BBCECA88C12772` |
| `out/verification/native_frame_probe.request` | 11 | Request text above |
| Restored owner profile | 116 | `2F1BC38D763A1B7BDBA31F560684FD4AA7E42A714600B8D344F19DA7F38E23B0` |

## Encoder correction and limits of the tests

The old encoder used its current seek position as the encoded length. A seek
cursor is not a stream's written extent. The fixed-capacity output now tracks
both separately, rejects any overflow even if a codec continues after a failed
write, checks JPEG beginning/end markers and releases stream consumers before
moving the backing storage. Neither capacity nor a backward seek determines EOF.
[Microsoft IStream::Seek](https://learn.microsoft.com/en-us/windows/win32/api/objidl/nf-objidl-istream-seek),
[WIC fixed-memory stream contract](https://learn.microsoft.com/en-us/windows/win32/api/wincodec/nf-wincodec-iwicstream-initializefrommemory).

The first1920x1080 test pattern was too compressible: it produced57,462 bytes,
so its initial >64 KiB assertion did not prove reproduction of the live bug.
That failed expectation is retained in CPU27/28 evidence, not relabelled a
causal reproduction. The strengthened8-pixel-block pattern reaches larger
output. CPU29 fully decodes it but fails the unchanged colour-error limit12.

Explicit4:4:4 subsampling then passes that same case and threshold without
changing dimensions, quality attempts or byte cap. WIC defaults to4:2:0; the
new setting preserves full chroma resolution. It may require more bytes and
must refuse if none of the fixed quality levels fits the caller's cap.
[Microsoft JPEG encoder options](https://learn.microsoft.com/en-us/windows/win32/wic/jpeg-format-overview).

Output47/PID29472 and CPU30/PID36840 pass:153,370 encoded bytes under the fixture's
256 KiB cap, valid tail, full1920x1080 decode and maximum block-centre RGB error4.
The tiny red-channel round trip and deliberately too-small output refusals pass.
Codec messages about too few scanlines occur during intentional short-budget
refusal tests. No fixture image is exported.350 Python source/scenario checks
pass. The fixture's256 KiB bound does **not** increase the live probe's110 KiB cap
or prove the next live frame will fit.

Host126/PID33008/session31734 links successfully with stampa6727b6 dirty; codegen
writes0 files,1 module is current, and no guest objects rebuild.

| Host126 artifact | Bytes | SHA256 |
| --- | ---: | --- |
| `reblue_vk.exe` | 48,760,320 | `44C7F7719115FAF6512AE3AC4E7996072EA03BEA2DF66B4DA3D7CB986B966773` |
| `reblue_vk.pdb` | 109,948,928 | `DFFFD90D8349248BEA7DD33A762B36A9C7246A7F2DCC5033AAB28EA5551D37CD` |

No new live frame has used this correction. Preserve963,962's title-logo image,
945's accepted mono baseline and956's tree gaps. Reserve/verify image overlap
before the next frame probe; its existing fixed output/request paths must not
be overwritten. The original presentation-vs-window-capture question, controlled
culling images, wider scene families/animation, every desktop event/both-eye gate
and Quest readiness remain open.

## Storage

All jobs reuse existing trees and the original budget exception/floor. Initial
free79,953,272,832 B; after host126,79,948,718,080 B (drive-wide decrease4,554,752 B,
not wholly attributed to this task). No raw capture, asset cook or game-data
deletion. Lossless PNG recompression preserves exact decoded pixels; logs960/961
remain SHA256-recoverable in `retained-native-runtime-960-961.zip` and count in
the supervisor. Together with superseded-log cleanup before963,1,219,185 logical
bytes were reclaimed. Final fixture/log cleanup and final free space are recorded
in the cumulative ledger. The image archive is10,378,927 B, leaving106,833 B:
another110 KiB export does not yet fit. Available drive space does not override
that separate archive limit.
