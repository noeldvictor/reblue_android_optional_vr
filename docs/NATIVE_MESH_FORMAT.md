# Native mesh payloads

`native_mesh_data.{h,cpp}` defines the checked portable format. Version 1 remains
readable for transitional packed meshes. Version 2 describes rigid vertices by
asset semantics and values, without a console declaration, shader location,
byte-swap mask, resource pointer or guest address.

## Version 2

All integers and IEEE-754 float32 values are little-endian. The topology is a
triangle list with uint32 indices and a signed base vertex. There is one
interleaved stream, slot 0, containing one float4 per attribute in schema order.
This initial full-value representation prioritizes explicit, checked conversion;
it is not a compact final headset packing or a measured bandwidth improvement.

| Offset | Field |
| --- | --- |
| 0 | Six ASCII bytes `BDMESH`, version byte 2, reserved byte 0 |
| 8 | uint64 FNV-1a checksum of all bytes starting at offset 16 |
| 16 | uint64 layout identity |
| 24 | int32 base vertex |
| 28 | uint32 stream count, exactly 1 |
| 32 | uint32 index count, nonzero and divisible by 3 |
| 36 | uint32 attribute count, 1 through 16 |
| 40 | Attribute records: uint32 semantic, index, byte offset; 12 bytes each |
| After attributes | Stream: uint32 slot, stride, payload byte count, then payload |
| After stream payload | uint32 triangle indices; no trailing bytes |

Semantic numbers are Position=1, Normal=2, Tangent=3, Binormal=4, TexCoord=5,
Color=6. Only index 0 is currently accepted except TexCoord, whose indices are
0 through 7. Attributes are strictly sorted by semantic/index, unique, densely
packed at offsets `16 * attribute_number`, and must include Position. The stride
is `16 * attribute_count`. Stream length must be a multiple of stride, every
effective index must be in range, and all stored floats must be finite.

The layout identity is FNV-1a over little-endian uint32 values: version 2,
attribute count, then each semantic/index/offset. The native content ID equals
the file's body checksum and includes the layout, base vertex, indices and vertex
values. It does not depend on source declaration identity or ordering. Neither
hash is a cryptographic authenticity check. Files are limited to 64 MiB;
decoding and cooking publish transactionally, leaving the previous result intact
on rejection. Unknown versions or schema extensions are rejected.

## Import and current consumer

`CookRigidMesh` is a temporary compatibility-boundary adapter. It reads immutable
import bytes in the existing post-word-swap order and applies supported float,
half, normalized/signed 16-bit, normalized byte-color and packed-basis decoding
once. It preserves the current shader's four-lane values, including UV recovery
and missing-input defaults. Packed missing basis vectors become explicit zero
attributes. Constrained positions, unsupported integer-class contracts, mixed
packed/unpacked bases, invalid ranges and nonfinite results are rejected; their
existing transitional import remains tracked, not relabeled as canonical.

`RigidMeshVertexInput` derives IA/pulling inputs from the decoded schema alone.
Its location/filler mapping remains an adapter to the existing shader signature,
but all unpack masks are zero. Converted dispatch also clears the packed-normal
specialization. Synthetic attributes retain their format in native pulling and
read an owned zero buffer with stride zero, so missing secondary-position
weights remain zero while missing color alpha remains one.
`LoadNativeGeometry(content_id)` loads and uploads v2 data without
source buffers or declarations; v1 cannot use that entry point. Direct native
scene/shadow object submission is still pending.

Both formats share the existing historical `native_meshes/v1` directory and
its 256 MiB/16,384-file disk budget, writer lease and 20 GiB reserve. A new version
does not grant a second cache allowance. Valid conflicting files are never
overwritten, and no bulk migration is performed. GPU storage retains its separate
256 MiB limit; metadata is bounded to 16,384 geometries and 32,768 temporary
import aliases. Direct native loads do not consult that alias map.

## Verification status

The rebuilt mesh fixture (build09/CPU08) passes canonical numeric/schema/storage
checks, source-destroyed input consumption and the synthetic-pulling correction.
All 3,510 existing v1 files were previously decoded read-only. Host64, 156 source
guards and 34 scenario cases pass. Flat run912 observes 2,206 canonical meshes,
104,787 fresh canonical draws and 117,515 fresh native pulled records, with
matching geometry/material/pose checks and an inspected full-size sanity image.
No new raw or asset-cache files. **Source-free GPU loads remain zero; direct
object submission, expanded layout coverage, compact packing, reloads, sequences,
both eyes and performance remain unqualified.** See the
[desktop qualification](../research/20260906_2316_canonical-rigid-desktop.md),
[canonical-data evidence](../research/20260906_2158_canonical-rigid-mesh.md) and
[pull-default follow-up](../research/20260906_2211_native-pull-defaults.md).
