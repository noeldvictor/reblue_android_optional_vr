/**
 * @brief Canonical rigid vertices, decoded once at import, never by the GPU.
 * @copyright Copyright (c) 2026 reblue contributors
 * @license BSD 3-Clause, see LICENSE
 */
#include "gpu/scene/native_mesh_cook.h"
#include <algorithm>
#include <bit>
#include <cmath>

namespace bd::gpu::scene {
namespace {
using F = plume::RenderFormat;
struct Binding {
  MeshSemantic semantic;
  const char *name;
  uint32_t index, location;
};
// Existing translated shader ABI ONLY. Nothing in this table is serialized.
constexpr Binding kBindings[] = {
    {MeshSemantic::Position, "POSITION", 0, 0},
    {MeshSemantic::Position, "POSITION", 1, 1},
    {MeshSemantic::Position, "POSITION", 2, 2},
    {MeshSemantic::Position, "POSITION", 3, 3},
    {MeshSemantic::Position, "POSITION", 4, 4},
    {MeshSemantic::Normal, "NORMAL", 0, 5},
    {MeshSemantic::Tangent, "TANGENT", 0, 6},
    {MeshSemantic::TexCoord, "TEXCOORD", 0, 7},
    {MeshSemantic::TexCoord, "TEXCOORD", 1, 8},
    {MeshSemantic::TexCoord, "TEXCOORD", 2, 9},
    {MeshSemantic::Color, "COLOR", 0, 10}};

uint32_t Read(const uint8_t *p, uint32_t n) {
  uint32_t value = 0;
  for (uint32_t i = 0; i < n; ++i) value |= uint32_t(p[i]) << (8 * i);
  return value;
}
float Half(uint16_t bits) {
  const auto exponent = (bits >> 10) & 31;
  const auto fraction = bits & 1023;
  const float magnitude = exponent == 0 ? std::ldexp(float(fraction), -24)
      : exponent == 31 ? std::bit_cast<float>(0x7f800000u | (fraction << 13))
      : std::ldexp(float(1024 + fraction), int(exponent) - 25);
  return bits & 0x8000 ? -magnitude : magnitude;
}
uint32_t Width(F format) {
  switch (format) {
  case F::R32_FLOAT: case F::B8G8R8A8_UNORM: case F::R8G8B8A8_UNORM:
  case F::R16G16_SNORM: case F::R16G16_UNORM: case F::R16G16_FLOAT:
  case F::R16G16_SINT: return 4;
  case F::R32G32_FLOAT: case F::R16G16B16A16_SNORM:
  case F::R16G16B16A16_UNORM: case F::R16G16B16A16_FLOAT:
  case F::R16G16B16A16_SINT: return 8;
  case F::R32G32B32_FLOAT: return 12;
  case F::R32G32B32A32_FLOAT: return 16;
  default: return 0; // integer-class shader contracts require a separate path
  }
}
std::array<float, 4> Fetch(const uint8_t *p, F format) {
  std::array<float, 4> value{0, 0, 0, 1};
  switch (format) {
  case F::R32_FLOAT: case F::R32G32_FLOAT: case F::R32G32B32_FLOAT:
  case F::R32G32B32A32_FLOAT:
    for (uint32_t i = 0; i < Width(format) / 4; ++i)
      value[i] = std::bit_cast<float>(Read(p + i * 4, 4));
    break;
  case F::B8G8R8A8_UNORM: case F::R8G8B8A8_UNORM:
    for (uint32_t i = 0; i < 4; ++i) value[i] = float(p[i]) / 255.0f;
    if (format == F::B8G8R8A8_UNORM) std::swap(value[0], value[2]);
    break;
  default:
    for (uint32_t i = 0; i < Width(format) / 2; ++i) {
      const uint16_t bits = uint16_t(Read(p + i * 2, 2));
      const auto signed_value = std::bit_cast<int16_t>(bits);
      switch (format) {
      case F::R16G16_SNORM: case F::R16G16B16A16_SNORM:
        value[i] = (std::max)(-1.0f, float(signed_value) / 32767.0f); break;
      case F::R16G16_UNORM: case F::R16G16B16A16_UNORM:
        value[i] = float(bits) / 65535.0f; break;
      case F::R16G16_FLOAT: case F::R16G16B16A16_FLOAT:
        value[i] = Half(bits); break;
      default: value[i] = float(signed_value); break;
      }
    }
    break;
  }
  return value;
}
uint32_t SwapMask(MeshSemantic semantic, const VertexShaderDecode &decode) {
  switch (semantic) {
  case MeshSemantic::Position: return decode.positions;
  case MeshSemantic::Normal: return decode.normals;
  case MeshSemantic::Tangent: return decode.tangents;
  case MeshSemantic::Binormal: return decode.binormals;
  case MeshSemantic::TexCoord: return decode.texcoords;
  default: return 0;
  }
}
} // namespace

bool CookRigidMesh(const NativeMeshData &packed,
                   std::span<const plume::RenderInputElement> elements,
                   VertexShaderDecode decode, bool packed_basis,
                   NativeMeshData &result) {
  if (&packed == &result || !packed.attributes.empty() ||
      !ValidateNativeMesh(packed) || elements.empty() || elements.size() > 32)
    return false;
  struct Source {
    NativeMeshAttribute attribute;
    const NativeMeshStream *stream;
    uint32_t offset;
    F format;
  };
  std::array<Source, 16> sources{};
  uint32_t count = 0;
  for (const auto &element : elements) {
    if (!element.semanticName || count == sources.size()) return false;
    const auto binding = std::find_if(std::begin(kBindings), std::end(kBindings),
        [&](const auto &b) { return element.semanticIndex == b.index &&
            std::strcmp(element.semanticName, b.name) == 0; });
    if (binding == std::end(kBindings)) return false;
    const bool basis = binding->semantic == MeshSemantic::Normal ||
                       binding->semantic == MeshSemantic::Tangent;
    const auto stream = std::find_if(packed.streams.begin(), packed.streams.end(),
        [&](const auto &s) { return s.slot == element.slotIndex; });
    if (stream == packed.streams.end()) {
      if (element.slotIndex == 15) {
        // Packed-basis decoding turns a missing basis into float4(0), unlike
        // the regular IA float3 default w=1. Store that actual constant value
        // in the asset; do not carry a source specialization flag forward.
        if (packed_basis && basis)
          sources[count++] = {{binding->semantic, binding->index, 0}, nullptr,
                               0, F::R32G32B32A32_FLOAT};
        continue;
      }
      return false;
    }
    if (binding->semantic == MeshSemantic::Position && binding->index != 0)
      return false; // constrained/animated vertices are not rigid assets
    const uint32_t width = Width(element.format);
    if (!width || element.alignedByteOffset > stream->stride ||
        width > stream->stride - element.alignedByteOffset)
      return false;
    if (packed_basis && basis && (element.format != F::R32_FLOAT ||
        (SwapMask(binding->semantic, decode) & (1u << binding->index))))
      return false; // ambiguous mixed packed/unpacked basis: do not guess
    sources[count++] = {{binding->semantic, binding->index, 0}, &*stream,
                         element.alignedByteOffset, element.format};
  }
  if (!count) return false;
  std::sort(sources.begin(), sources.begin() + count, [](const auto &a, const auto &b) {
    return std::pair(a.attribute.semantic, a.attribute.index) <
           std::pair(b.attribute.semantic, b.attribute.index);
  });
  if (sources[0].attribute.semantic != MeshSemantic::Position) return false;
  for (uint32_t i = 1; i < count; ++i)
    if (sources[i].attribute == sources[i - 1].attribute) return false;
  const uint64_t vertices = uint64_t(int64_t(*std::max_element(
      packed.indices.begin(), packed.indices.end())) + packed.base_vertex) + 1;
  const uint32_t stride = count * 16;
  const uint64_t overhead = 52 + count * 12 + packed.indices.size() * 4;
  if (overhead > kNativeMeshMaxBytes ||
      vertices > (kNativeMeshMaxBytes - overhead) / stride) return false;
  NativeMeshData cooked;
  cooked.base_vertex = packed.base_vertex;
  cooked.indices = packed.indices;
  for (uint32_t i = 0; i < count; ++i) {
    auto a = sources[i].attribute;
    a.offset = i * 16;
    cooked.attributes.push_back(a);
  }
  cooked.layout = NativeMeshLayoutId(cooked.attributes);
  cooked.streams.push_back({0, stride, std::vector<uint8_t>(vertices * stride)});
  auto &bytes = cooked.streams[0].bytes;
  for (uint64_t vertex = 0; vertex < vertices; ++vertex) {
    for (uint32_t i = 0; i < count; ++i) {
      const auto &source = sources[i];
      const auto *p = source.stream ? source.stream->bytes.data() +
          vertex * source.stream->stride + source.offset : nullptr;
      const auto semantic = source.attribute.semantic;
      auto value = p ? Fetch(p, source.format) : std::array<float, 4>{};
      const uint32_t bit = 1u << source.attribute.index;
      if (SwapMask(semantic, decode) & bit) {
        std::swap(value[0], value[1]);
        std::swap(value[2], value[3]);
      }
      if (p && packed_basis && (semantic == MeshSemantic::Normal || semantic == MeshSemantic::Tangent)) {
        const uint32_t bits = Read(p, 4);
        value = {float(int32_t(bits & 1023) - ((bits & 1024) ? 1024 : 0)) / 1024.0f,
                 float(int32_t((bits >> 11) & 1023) - ((bits & 0x200000) ? 1024 : 0)) / 1024.0f,
                 float(int32_t((bits >> 22) & 511) - ((bits & 0x80000000) ? 512 : 0)) / 512.0f, 0};
      }
      if (semantic == MeshSemantic::TexCoord && (decode.integer_texcoords & bit))
        for (auto &lane : value) lane *= 32767.0f;
      for (uint32_t lane = 0; lane < 4; ++lane) {
        if (!std::isfinite(value[lane])) return false;
        const uint32_t bits = std::bit_cast<uint32_t>(value[lane]);
        const uint64_t offset = vertex * stride + i * 16 + lane * 4;
        for (uint32_t b = 0; b < 4; ++b) bytes[offset + b] = uint8_t(bits >> (b * 8));
      }
    }
  }
  if (!ValidateNativeMesh(cooked)) return false;
  result = std::move(cooked);
  return true;
}

NativeVertexInputHandle RigidMeshVertexInput(const NativeMeshData &mesh,
                                            NativeVertexInputLibrary &library) {
  if (mesh.attributes.empty() || !ValidateNativeMesh(mesh)) return {};
  std::array<plume::RenderInputElement, std::size(kBindings)> elements;
  size_t matched = 0;
  for (size_t i = 0; i < std::size(kBindings); ++i) {
    const auto &binding = kBindings[i];
    const auto found = std::find_if(mesh.attributes.begin(), mesh.attributes.end(),
        [&](const auto &a) { return a.semantic == binding.semantic && a.index == binding.index; });
    const bool present = found != mesh.attributes.end();
    matched += present;
    // Preserve the existing signature's missing-input defaults while that
    // shader adapter remains: zero secondary-position weights, default alpha
    // one for an absent color, and default w one for an unpacked float3 basis.
    const auto fallback = binding.semantic == MeshSemantic::Position ? F::R32G32B32A32_FLOAT
        : (binding.semantic == MeshSemantic::Normal || binding.semantic == MeshSemantic::Tangent)
              ? F::R32G32B32_FLOAT : F::R32_FLOAT;
    elements[i] = {binding.name, binding.index, binding.location,
                   present ? F::R32G32B32A32_FLOAT : fallback,
                   present ? 0u : 15u, present ? found->offset : 0u};
  }
  if (matched != mesh.attributes.size()) return {};
  return library.Resolve(elements, 1, {});
}
} // namespace bd::gpu::scene
