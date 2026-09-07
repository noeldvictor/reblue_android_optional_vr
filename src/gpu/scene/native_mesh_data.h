/**
 * @file    native_mesh_data.h
 * @brief   Portable native mesh payloads, independent of guest memory and Vulkan.
 * @copyright Copyright (c) 2026 reblue contributors
 * @license   BSD 3-Clause License
 */
#pragma once

#include <cstdint>
#include <span>
#include <vector>

namespace bd::gpu::scene {

enum class MeshTopology { Triangles, Strip };

// The import boundary accepts big-endian indices; everything after it is a
// native triangle list. Restart resets parity; degenerate strips advance it.
bool ImportMeshIndices(std::span<const uint8_t> source, bool index32,
                       MeshTopology topology, std::vector<uint32_t> &triangles);

struct NativeMeshStream {
  uint32_t slot = 0;
  uint32_t stride = 0;
  std::vector<uint8_t> bytes;
};

// BDMESH v2 rigid vertices: named float4 attributes in one interleaved stream.
// Values and all file metadata are little-endian. These are asset semantics,
// not shader locations, console declaration types or decoder masks.
enum class MeshSemantic : uint32_t {
  Position = 1, Normal = 2, Tangent = 3, Binormal = 4, TexCoord = 5, Color = 6
};
struct NativeMeshAttribute {
  MeshSemantic semantic = MeshSemantic::Position;
  uint32_t index = 0, offset = 0;
  bool operator==(const NativeMeshAttribute &) const = default;
};
uint64_t NativeMeshLayoutId(std::span<const NativeMeshAttribute> attributes);

struct NativeMeshData {
  // Empty attributes identify the transitional v1 packed payload. Nonempty
  // attributes are self-describing v2; layout is derived from that schema.
  uint64_t layout = 0;
  int32_t base_vertex = 0;
  std::vector<uint32_t> indices;
  std::vector<NativeMeshStream> streams;
  std::vector<NativeMeshAttribute> attributes;
};

constexpr uint64_t kNativeMeshMaxBytes = 64ull << 20;
bool ValidateNativeMesh(const NativeMeshData &mesh);
// Stable identity of a valid self-describing payload; zero rejects v1/invalid.
uint64_t NativeMeshContentId(const NativeMeshData &mesh);
bool EncodeNativeMesh(const NativeMeshData &mesh, std::vector<uint8_t> &file);
bool DecodeNativeMesh(std::span<const uint8_t> file, NativeMeshData &mesh);

} // namespace bd::gpu::scene
