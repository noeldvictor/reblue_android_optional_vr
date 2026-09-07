/**
 * @file    native_mesh_data.cpp
 * @brief   Checked, little-endian native mesh files and triangle conversion.
 * @copyright Copyright (c) 2026 reblue contributors
 * @license   BSD 3-Clause License
 */
#include "gpu/scene/native_mesh_data.h"

#include <algorithm>
#include <bit>
#include <cmath>
#include <cstring>
#include <limits>

namespace bd::gpu::scene {
namespace {
constexpr uint8_t kMagic[8] = {'B', 'D', 'M', 'E', 'S', 'H', 1, 0};

uint64_t Checksum(std::span<const uint8_t> bytes) {
  uint64_t hash = 14695981039346656037ull;
  for (uint8_t b : bytes)
    hash = (hash ^ b) * 1099511628211ull;
  return hash;
}
void Put(std::vector<uint8_t> &v, uint64_t n, unsigned bytes = 4) {
  for (unsigned i = 0; i < bytes; ++i)
    v.push_back(uint8_t(n >> (8 * i)));
}
struct Reader {
  std::span<const uint8_t> bytes;
  bool ok = true;
  uint64_t Get(unsigned n = 4) {
    if (bytes.size() < n) {
      ok = false;
      return 0;
    }
    uint64_t value = 0;
    for (unsigned i = 0; i < n; ++i)
      value |= uint64_t(bytes[i]) << (8 * i);
    bytes = bytes.subspan(n);
    return value;
  }
};
} // namespace

uint64_t NativeMeshLayoutId(std::span<const NativeMeshAttribute> attributes) {
  uint64_t hash = 14695981039346656037ull;
  const auto word = [&](uint32_t value) {
    for (unsigned i = 0; i < 4; ++i)
      hash = (hash ^ uint8_t(value >> (8 * i))) * 1099511628211ull;
  };
  word(2); // format version, including float4/interleaved packing
  word(uint32_t(attributes.size()));
  for (const auto &a : attributes) {
    word(uint32_t(a.semantic));
    word(a.index);
    word(a.offset);
  }
  return hash;
}

bool ImportMeshIndices(std::span<const uint8_t> source, bool index32,
                       MeshTopology topology, std::vector<uint32_t> &triangles) {
  triangles.clear();
  const size_t width = index32 ? 4 : 2;
  const size_t count = source.size() / width;
  if (source.size() % width || count > kNativeMeshMaxBytes / 12 ||
      (topology == MeshTopology::Triangles && count % 3))
    return false;
  const uint32_t restart = index32 ? UINT32_MAX : UINT16_MAX;
  uint32_t a = 0, b = 0;
  size_t run = 0;
  triangles.reserve(count * (topology == MeshTopology::Strip ? 3 : 1));
  for (size_t i = 0; i < count; ++i) {
    uint32_t c = 0;
    for (size_t j = 0; j < width; ++j)
      c = (c << 8) | source[i * width + j];
    if (topology == MeshTopology::Triangles) {
      triangles.push_back(c);
      continue;
    }
    if (c == restart) {
      run = 0;
      continue;
    }
    if (run >= 2 && a != b && b != c && a != c) {
      triangles.push_back(a);
      triangles.push_back((run & 1) ? c : b);
      triangles.push_back((run & 1) ? b : c);
    }
    a = b;
    b = c;
    ++run;
  }
  return true;
}

bool ValidateNativeMesh(const NativeMeshData &mesh) {
  if (mesh.indices.empty() || mesh.indices.size() % 3 ||
      mesh.indices.size() > kNativeMeshMaxBytes / 4 || mesh.streams.empty() ||
      mesh.streams.size() > 16)
    return false;
  const auto [lo, hi] = std::minmax_element(mesh.indices.begin(), mesh.indices.end());
  const int64_t first = int64_t(*lo) + mesh.base_vertex;
  const int64_t last = int64_t(*hi) + mesh.base_vertex;
  if (first < 0 || last > UINT32_MAX)
    return false;
  uint32_t slots = 0;
  uint64_t bytes = 36 + mesh.indices.size() * 4;
  if (!mesh.attributes.empty()) {
    if (mesh.attributes.size() > 16 || mesh.streams.size() != 1 ||
        mesh.streams[0].slot != 0 ||
        mesh.streams[0].stride != mesh.attributes.size() * 16 ||
        mesh.streams[0].bytes.size() % mesh.streams[0].stride ||
        mesh.layout != NativeMeshLayoutId(mesh.attributes))
      return false;
    uint32_t previous = 0;
    bool position = false;
    for (size_t i = 0; i < mesh.attributes.size(); ++i) {
      const auto &a = mesh.attributes[i];
      const auto semantic = uint32_t(a.semantic);
      if (semantic < 1 || semantic > 6 ||
          a.index > (a.semantic == MeshSemantic::TexCoord ? 7u : 0u) ||
          a.offset != i * 16)
        return false;
      const uint32_t key = semantic * 8 + a.index;
      if (key <= previous) return false;
      previous = key;
      position |= a.semantic == MeshSemantic::Position;
    }
    if (!position) return false;
    bytes += 4 + mesh.attributes.size() * 12;
  }
  for (const auto &s : mesh.streams) {
    if (s.slot >= 16 || (slots & (1u << s.slot)) || s.stride == 0 ||
        (uint64_t(last) + 1) * s.stride > s.bytes.size())
      return false;
    slots |= 1u << s.slot;
    bytes += 12 + s.bytes.size();
    if (bytes > kNativeMeshMaxBytes)
      return false;
  }
  if (!mesh.attributes.empty()) {
    // The new native contract never stores raw packed bits as a float, NaNs
    // or infinities. Check byte order explicitly, including on big-endian CPUs.
    Reader r{mesh.streams[0].bytes};
    while (!r.bytes.empty())
      if (!std::isfinite(std::bit_cast<float>(uint32_t(r.Get())))) return false;
  }
  return true;
}

bool EncodeNativeMesh(const NativeMeshData &mesh, std::vector<uint8_t> &file) {
  file.clear();
  if (!ValidateNativeMesh(mesh))
    return false;
  file.insert(file.end(), std::begin(kMagic), std::end(kMagic));
  if (!mesh.attributes.empty()) file[6] = 2;
  Put(file, 0, 8);
  Put(file, mesh.layout, 8);
  Put(file, std::bit_cast<uint32_t>(mesh.base_vertex));
  Put(file, mesh.streams.size());
  Put(file, mesh.indices.size());
  if (!mesh.attributes.empty()) {
    Put(file, mesh.attributes.size());
    for (const auto &a : mesh.attributes) {
      Put(file, uint32_t(a.semantic));
      Put(file, a.index);
      Put(file, a.offset);
    }
  }
  for (const auto &s : mesh.streams) {
    Put(file, s.slot);
    Put(file, s.stride);
    Put(file, s.bytes.size());
    file.insert(file.end(), s.bytes.begin(), s.bytes.end());
  }
  for (uint32_t i : mesh.indices)
    Put(file, i);
  const uint64_t sum = Checksum(std::span(file).subspan(16));
  for (unsigned i = 0; i < 8; ++i)
    file[8 + i] = uint8_t(sum >> (8 * i));
  return true;
}

uint64_t NativeMeshContentId(const NativeMeshData &mesh) {
  if (mesh.attributes.empty() || !ValidateNativeMesh(mesh)) return 0;
  uint64_t hash = 14695981039346656037ull;
  const auto byte = [&](uint8_t value) { hash = (hash ^ value) * 1099511628211ull; };
  const auto word = [&](uint64_t value, unsigned n = 4) {
    for (unsigned i = 0; i < n; ++i) byte(uint8_t(value >> (8 * i)));
  };
  // Same field order as the file checksum, without allocating a second copy.
  word(mesh.layout, 8);
  word(std::bit_cast<uint32_t>(mesh.base_vertex));
  word(mesh.streams.size());
  word(mesh.indices.size());
  word(mesh.attributes.size());
  for (const auto &a : mesh.attributes) {
    word(uint32_t(a.semantic)); word(a.index); word(a.offset);
  }
  for (const auto &s : mesh.streams) {
    word(s.slot); word(s.stride); word(s.bytes.size());
    for (auto value : s.bytes) byte(value);
  }
  for (auto index : mesh.indices) word(index);
  return hash;
}

bool DecodeNativeMesh(std::span<const uint8_t> file, NativeMeshData &mesh) {
  // Parse into a temporary: a rejected cache cannot leave a partially usable
  // mesh in the renderer. Counts are bounded by the remaining file first.
  if (file.size() < 36 || file.size() > kNativeMeshMaxBytes ||
      std::memcmp(file.data(), kMagic, 6) != 0 || file[7] != 0 ||
      (file[6] != 1 && file[6] != 2))
    return false;
  Reader r{file.subspan(8)};
  if (r.Get(8) != Checksum(file.subspan(16)))
    return false;
  NativeMeshData result;
  result.layout = r.Get(8);
  result.base_vertex = std::bit_cast<int32_t>(uint32_t(r.Get()));
  const uint32_t streams = uint32_t(r.Get());
  const uint32_t indices = uint32_t(r.Get());
  if (streams == 0 || streams > 16 || indices > r.bytes.size() / 4)
    return false;
  if (file[6] == 2) {
    const auto count = r.Get();
    if (!count || count > 16 || count > r.bytes.size() / 12) return false;
    for (uint64_t i = 0; i < count; ++i) {
      NativeMeshAttribute a;
      a.semantic = MeshSemantic(uint32_t(r.Get()));
      a.index = uint32_t(r.Get());
      a.offset = uint32_t(r.Get());
      result.attributes.push_back(a);
    }
  }
  for (uint32_t i = 0; i < streams; ++i) {
    NativeMeshStream s;
    s.slot = uint32_t(r.Get());
    s.stride = uint32_t(r.Get());
    const uint32_t size = uint32_t(r.Get());
    if (!r.ok || size > r.bytes.size())
      return false;
    s.bytes.assign(r.bytes.begin(), r.bytes.begin() + size);
    r.bytes = r.bytes.subspan(size);
    result.streams.push_back(std::move(s));
  }
  if (uint64_t(indices) * 4 != r.bytes.size())
    return false;
  result.indices.reserve(indices);
  for (uint32_t i = 0; i < indices; ++i)
    result.indices.push_back(uint32_t(r.Get()));
  if (!r.ok || !ValidateNativeMesh(result))
    return false;
  mesh = std::move(result);
  return true;
}
} // namespace bd::gpu::scene
