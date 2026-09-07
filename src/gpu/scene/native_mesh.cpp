/**
 * @file    native_mesh.cpp
 * @brief   Persistent model geometry and shared buffers for indirect drawing.
 * @copyright Copyright (c) 2026 reblue contributors
 * @license   BSD 3-Clause License
 */
#include "gpu/scene/native_mesh.h"
#include "gpu/scene/native_mesh_data.h"
#include "gpu/scene/native_mesh_cook.h"
#include "gpu/scene/native_mesh_storage.h"

#include <algorithm>
#include <bit>
#include <cstring>
#include <filesystem>
#include <mutex>
#include <unordered_map>
#include <fmt/format.h>
#include <xxhash.h>
#include <rex/graphics/xenos.h>
#include <rex/runtime.h>

#include "core/logging.h"
#include "core/memory_helpers.h"
#include "gpu/byte_swap.h"
#include "gpu/device.h"
#include "gpu/frame_stats.h"
#include "gpu/resources.h"

namespace bd::gpu::scene {
namespace {
static_assert(std::endian::native == std::endian::little);
constexpr u32 kChunkBytes = 32u << 20;
// Geometry's share of the project's 1.5 GB asset budget. A full arena is
// reported explicitly; it must never grow with every scene reload forever.
constexpr u64 kGeometryBudget = 256ull << 20;
constexpr size_t kMaxGeometries = 16384, kMaxImportAliases = 32768;
struct Chunk {
  std::unique_ptr<plume::RenderBuffer> buffer;
  u32 size = 0, used = 0;
};
struct Store {
  std::mutex mutex;
  std::vector<Chunk> chunks;
  std::unordered_map<u64, std::shared_ptr<const NativeGeometry>> meshes;
  // Temporary import-key -> native content identity. Direct asset loads do
  // not consult this adapter; bound it independently of GPU byte residency.
  std::unordered_map<u64, u64> import_aliases;
  NativeVertexInputLibrary vertex_inputs;
  u64 allocated = 0;
  u32 built = 0, loaded = 0, refused = 0, budget_refused = 0;
  u32 native_draws = 0, legacy_draws = 0, last_frame = 0;
  u32 canonical_meshes = 0, canonical_draws = 0, source_free_loads = 0;
};
Store &store() {
  static Store s;
  return s;
}

std::filesystem::path CacheDir() {
  std::filesystem::path root;
  if (auto *runtime = rex::Runtime::instance())
    root = runtime->cache_root();
  if (root.empty())
    root = std::filesystem::current_path();
  // Historical directory name, shared by both checked file versions. Keeping
  // it avoids a duplicate cache or a reset of its byte/file accounting.
  return root / "native_meshes" / "v1";
}

NativeMeshDiskCache &DiskCache() {
  static NativeMeshDiskCache cache(CacheDir());
  return cache;
}

u32 Align(u32 n) { return (n + 15u) & ~15u; }

std::shared_ptr<const NativeGeometry> Upload(Store &s, const NativeMeshData &data, u64 key,
                                          NativeVertexInputHandle vertex_input) {
  u32 bytes = Align(u32(data.indices.size() * 4));
  for (const auto &stream : data.streams)
    bytes += Align(u32(stream.bytes.size()));
  if (s.chunks.empty() || s.chunks.back().size - s.chunks.back().used < bytes) {
    const u32 size = std::max(kChunkBytes, bytes);
    if (s.allocated + size > kGeometryBudget) {
      ++s.budget_refused;
      return {};
    }
    auto *device = Video::HostDevice();
    plume::RenderBufferDesc desc;
    desc.size = size;
    desc.heapType = GeometryHeapType(device, GeometryClass::Static);
    desc.flags = plume::RenderBufferFlag::VERTEX | plume::RenderBufferFlag::INDEX |
                 plume::RenderBufferFlag::STORAGE;
    auto buffer = CreateHostBuffer(device, desc, "native-mesh-arena");
    if (!buffer)
      return {};
    s.chunks.push_back(Chunk{std::move(buffer), size, 0});
    s.allocated += size;
  }
  auto &chunk = s.chunks.back();
  auto *mapped = static_cast<u8 *>(chunk.buffer->map());
  if (!mapped)
    return {};
  auto result = std::make_shared<NativeGeometry>();
  result->id = key;
  result->layout = data.layout;
  result->canonical_vertices = !data.attributes.empty();
  result->vertex_input = std::move(vertex_input);
  result->count = u32(data.indices.size());
  result->base_vertex = data.base_vertex;
  result->start_index = chunk.used / 4;
  // Every mesh in a chunk binds the same index view. Its range lives in the
  // indirect command, allowing distinct meshes to share a multi-draw call.
  result->index = plume::RenderIndexBufferView(chunk.buffer->at(0), chunk.size,
                                               plume::RenderFormat::R32_UINT);
  std::memcpy(mapped + chunk.used, data.indices.data(), data.indices.size() * 4);
  chunk.used += Align(u32(data.indices.size() * 4));
  for (const auto &stream : data.streams) {
    result->strides[stream.slot] = stream.stride;
    result->streams[stream.slot] = plume::RenderVertexBufferView(
        chunk.buffer->at(chunk.used), u32(stream.bytes.size()));
    result->stream_mask |= 1u << stream.slot;
    std::memcpy(mapped + chunk.used, stream.bytes.data(), stream.bytes.size());
    chunk.used += Align(u32(stream.bytes.size()));
  }
  chunk.buffer->unmap();
  return result;
}

std::shared_ptr<const NativeGeometry> Import(Store &s, const NativeMeshImport &r) {
  if (!r.declaration || !r.index || r.index->ownsMirror || !r.count)
    return {};
  NativeMeshData data;
  data.layout = r.declaration->hash;
  data.base_vertex = r.base_vertex;
  if (!r.lod_indices.empty()) {
    if (r.lod_indices.size() > kNativeMeshMaxBytes / 4)
      return {};
    data.indices.assign(r.lod_indices.begin(), r.lod_indices.end());
  } else {
    using rex::graphics::xenos::PrimitiveType;
    const auto primitive = static_cast<PrimitiveType>(r.primitive_type);
    if (primitive != PrimitiveType::kTriangleList &&
        primitive != PrimitiveType::kTriangleStrip)
      return {};
    const bool index32 = r.index->format == plume::RenderFormat::R32_UINT;
    if (!index32 && r.index->format != plume::RenderFormat::R16_UINT)
      return {};
    const u32 width = index32 ? 4 : 2;
    const u64 offset = u64(r.start_index) * width;
    const u64 size = u64(r.count) * width;
    if (offset + size > r.index->dataSize || size > kNativeMeshMaxBytes)
      return {};
    const auto *source = bd::mem::try_at<const u8>(r.index->guestMirrorVa);
    if (!source || !ImportMeshIndices({source + offset, size}, index32,
          primitive == PrimitiveType::kTriangleStrip ? MeshTopology::Strip
                                                     : MeshTopology::Triangles,
          data.indices))
      return {};
  }
  if (data.indices.empty())
    return {};
  const auto [lo, hi] = std::minmax_element(data.indices.begin(), data.indices.end());
  const i64 first = i64(*lo) + data.base_vertex;
  const i64 last = i64(*hi) + data.base_vertex;
  if (first < 0 || last > UINT32_MAX)
    return {};

  // Hash bytes and layout, never allocation addresses. Re-loading a model
  // resolves the same native mesh, including across separate desktop runs.
  u64 key = XXH3_64bits(data.indices.data(), data.indices.size() * 4);
  const u64 identity[] = {1, data.layout, u64(u32(data.base_vertex))};
  key = XXH3_64bits_withSeed(identity, sizeof(identity), key);
  struct Source { const u8 *bytes; u32 size, slot, stride; } sources[16];
  u32 n = 0;
  u64 total = 36 + data.indices.size() * 4;
  for (u32 slot = 0; slot < 16; ++slot) {
    if (!r.declaration->vertexStreams[slot])
      continue;
    const auto *buffer = r.streams[slot];
    if (!buffer || buffer->ownsMirror || !r.strides[slot])
      return {};
    const u64 size = (u64(last) + 1) * r.strides[slot];
    total += 12 + size;
    if (total > kNativeMeshMaxBytes || u64(r.offsets[slot]) + size > buffer->dataSize ||
        size % 4)
      return {};
    const auto *source = bd::mem::try_at<const u8>(buffer->guestMirrorVa);
    if (!source)
      return {};
    source += r.offsets[slot];
    const u32 meta[] = {slot, r.strides[slot], u32(size)};
    key = XXH3_64bits_withSeed(meta, sizeof(meta), key);
    key = XXH3_64bits_withSeed(source, size, key);
    sources[n++] = Source{source, u32(size), slot, r.strides[slot]};
  }
  if (!n)
    return {};
  if (auto it = s.import_aliases.find(key); it != s.import_aliases.end())
    return s.meshes.at(it->second);
  if (s.import_aliases.size() >= kMaxImportAliases) {
    ++s.budget_refused;
    return {};
  }
  const u64 import_key = key;

  NativeMeshData cached;
  bool loaded = DiskCache().Read(key, cached) && cached.layout == data.layout &&
                cached.base_vertex == data.base_vertex &&
                cached.indices == data.indices && cached.streams.size() == n;
  for (u32 i = 0; loaded && i < n; ++i)
    loaded = cached.streams[i].slot == sources[i].slot &&
             cached.streams[i].stride == sources[i].stride &&
             cached.streams[i].bytes.size() == sources[i].size;
  if (loaded) {
    data = std::move(cached);
  } else {
    for (u32 i = 0; i < n; ++i) {
      const auto &source = sources[i];
      NativeMeshStream stream;
      stream.slot = source.slot;
      stream.stride = source.stride;
      stream.bytes.resize(source.size);
      // Match the current GPU vertex layout at this import boundary. The
      // persistent bytes never require guest endian conversion at draw time.
      ByteSwapElements(stream.bytes.data(), source.bytes, source.size, 4);
      data.streams.push_back(std::move(stream));
    }
    if (!ValidateNativeMesh(data))
      return {};
  }
  const auto &decl = *r.declaration;
  const VertexShaderDecode decode{
      decl.swappedTexcoords, decl.swappedNormals, decl.swappedBinormals,
      decl.swappedTangents, decl.swappedBlendWeights, decl.swappedPositions,
      decl.sintTexcoords};
  NativeMeshData canonical;
  if (CookRigidMesh(data, {decl.inputElements.get(), decl.inputElementCount},
                    decode, decl.hasR11G11B10Normal, canonical)) {
    data = std::move(canonical);
    key = NativeMeshContentId(data);
    if (!key) return {};
    // Keep the existing aggregate disk budget/path: a second format must not
    // silently create a second cache allowance. Existing v1 files stay valid.
    NativeMeshData persisted;
    loaded = DiskCache().Read(key, persisted) &&
             NativeMeshContentId(persisted) == key &&
             persisted.attributes == data.attributes &&
             persisted.base_vertex == data.base_vertex &&
             persisted.indices == data.indices && persisted.layout == data.layout &&
             persisted.streams[0].bytes == data.streams[0].bytes;
    if (loaded) data = std::move(persisted);
  }
  if (auto it = s.meshes.find(key); it != s.meshes.end()) {
    s.import_aliases.emplace(import_key, key);
    return it->second;
  }
  if (s.meshes.size() >= kMaxGeometries) {
    ++s.budget_refused;
    return {};
  }
  uint32_t stream_mask = 0;
  for (uint32_t slot = 0; slot < 16; ++slot)
    if (decl.vertexStreams[slot]) stream_mask |= 1u << slot;
  auto vertex_input = data.attributes.empty() ? s.vertex_inputs.Resolve(
      {decl.inputElements.get(), decl.inputElementCount}, stream_mask,
      decode) : RigidMeshVertexInput(data, s.vertex_inputs);
  if (!vertex_input) return {};
  auto result = Upload(s, data, key, std::move(vertex_input));
  if (!result)
    return {};
  if (loaded)
    ++s.loaded;
  else {
    // Persistence refusal never discards usable native GPU geometry or routes
    // this draw back through the guest. Keep one bounded resident result.
    if (r.persist)
      DiskCache().Write(key, data);
    ++s.built;
  }
  s.meshes.emplace(key, result);
  s.import_aliases.emplace(import_key, key);
  s.canonical_meshes += result->canonical_vertices;
  return result;
}
} // namespace

std::shared_ptr<const NativeGeometry> ImportNativeMesh(const NativeMeshImport &r) {
  auto &s = store();
  std::lock_guard lock(s.mutex);
  auto result = Import(s, r);
  if (!result)
    ++s.refused;
  return result;
}

std::shared_ptr<const NativeGeometry> LoadNativeGeometry(u64 content_id) {
  auto &s = store();
  std::lock_guard lock(s.mutex);
  if (auto it = s.meshes.find(content_id); it != s.meshes.end())
    return it->second->canonical_vertices ? it->second : nullptr;
  NativeMeshData data;
  if (!content_id || s.meshes.size() >= kMaxGeometries ||
      !DiskCache().Read(content_id, data) || NativeMeshContentId(data) != content_id)
    return {};
  auto input = RigidMeshVertexInput(data, s.vertex_inputs);
  if (!input) return {};
  auto result = Upload(s, data, content_id, std::move(input));
  if (!result) return {};
  s.meshes.emplace(content_id, result);
  ++s.loaded;
  ++s.canonical_meshes;
  ++s.source_free_loads;
  return result;
}

void NativeMeshNoteDraw(bool native, bool canonical) {
  auto &s = store();
  std::lock_guard lock(s.mutex);
  ++(native ? s.native_draws : s.legacy_draws);
  s.canonical_draws += native && canonical;
  const u32 frame = FrameStatFrameCount();
  if (frame - s.last_frame < 300)
    return;
  const u32 frames = frame - s.last_frame;
  BD_INFO("[native-mesh] {} cooked, {} loaded, {} live meshes, {} MiB; indexed "
          "replays/frame {:.1f} native / {:.1f} unconverted; import refusals {}, "
          "budget refusals {}",
          s.built, s.loaded, s.meshes.size(), s.allocated >> 20,
          double(s.native_draws) / frames, double(s.legacy_draws) / frames,
          s.refused, s.budget_refused);
  const auto disk = DiskCache().Stats();
  BD_INFO("[native-mesh-canonical] {} meshes, {} draws, {} source-free disk loads; "
          "named float4 vertices, zero shader unpack masks",
          s.canonical_meshes, s.canonical_draws, s.source_free_loads);
  BD_INFO("[native-vertex-input] {} owned inputs / {} bytes; no declaration needed by native dispatch",
          s.vertex_inputs.Size(), s.vertex_inputs.Bytes());
  const auto &uses = NativeVertexInputUses();
  BD_INFO("[native-vertex-input-use] {} pipeline binds, {} decode blocks, {} pulled records",
          uses.pipelines, uses.decode_blocks, uses.pulled_records);
  BD_INFO("[native-mesh-disk] {} writes, {} reused, {} failures / {} budget refusals, "
          "{} conflicts; last inventory {} files / {} bytes, complete {}; "
          "disk-full retains native geometry, never evicts files",
          disk.written, disk.reused, disk.write_failures, disk.budget_refusals,
          disk.conflicts, disk.files, disk.bytes, disk.inventory_complete);
  s.native_draws = s.legacy_draws = 0;
  s.last_frame = frame;
}
} // namespace bd::gpu::scene
