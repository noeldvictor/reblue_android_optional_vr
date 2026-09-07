/**
 * @brief Canonical values, hostile files and source-destroyed shader inputs.
 * @copyright Copyright (c) 2026 reblue contributors
 * @license BSD 3-Clause, see LICENSE
 */
#include "gpu/scene/native_mesh_cook.h"
#include <algorithm>
#include <bit>
#include <cmath>
#include <iostream>
#include <stdexcept>

using namespace bd::gpu::scene;
using F = plume::RenderFormat;
namespace {
void Check(bool value, const char *message) {
  if (!value) throw std::runtime_error(message);
}
void Word(std::vector<uint8_t> &bytes, size_t offset, uint64_t value, unsigned n = 4) {
  for (unsigned i = 0; i < n; ++i) bytes.at(offset + i) = uint8_t(value >> (8 * i));
}
uint64_t Read(std::span<const uint8_t> bytes, size_t offset, unsigned n = 4) {
  uint64_t value = 0;
  for (unsigned i = 0; i < n; ++i) value |= uint64_t(bytes[offset + i]) << (8 * i);
  return value;
}
float Lane(const NativeMeshData &mesh, size_t attribute, size_t lane) {
  return std::bit_cast<float>(uint32_t(Read(mesh.streams[0].bytes, attribute * 16 + lane * 4)));
}
NativeMeshData Packed() {
  NativeMeshData mesh;
  mesh.layout = 123;
  mesh.indices = {4, 5, 6};
  mesh.base_vertex = -4;
  mesh.streams.push_back({2, 28, std::vector<uint8_t>(84)});
  for (unsigned i = 0; i < 3; ++i) {
    auto &bytes = mesh.streams[0].bytes;
    const size_t start = i * 28;
    Word(bytes, start, std::bit_cast<uint32_t>(1.25f + i));
    Word(bytes, start + 4, std::bit_cast<uint32_t>(-2.0f));
    Word(bytes, start + 8, std::bit_cast<uint32_t>(3.0f));
    // Post-bswap32 order is y,x,w,z. Normal SNORM + raw signed UV.
    Word(bytes, start + 12, 0x7fff8000);
    Word(bytes, start + 16, 0x00004000);
    Word(bytes, start + 20, 0x0005fff9);
    Word(bytes, start + 24, 0xff804020); // BGRA
  }
  return mesh;
}
std::vector<plume::RenderInputElement> Elements() {
  return {{"TEXCOORD", 0, 7, F::R16G16_SNORM, 2, 20},
          {"COLOR", 0, 10, F::B8G8R8A8_UNORM, 2, 24},
          {"POSITION", 0, 0, F::R32G32B32_FLOAT, 2, 0},
          {"NORMAL", 0, 5, F::R16G16B16A16_SNORM, 2, 12},
          {"POSITION", 1, 1, F::R32G32B32A32_FLOAT, 15, 0}};
}
void RepairChecksum(std::vector<uint8_t> &file) {
  uint64_t hash = 14695981039346656037ull;
  for (uint8_t b : std::span(file).subspan(16)) hash = (hash ^ b) * 1099511628211ull;
  Word(file, 8, hash, 8);
}
} // namespace

void TestMeshCook() {
  const VertexShaderDecode decode{1, 1, 0, 0, 0, 0, 1};
  NativeMeshData cooked;
  std::vector<uint8_t> file;
  {
    auto packed = Packed();
    auto elements = Elements();
    Check(CookRigidMesh(packed, elements, decode, false, cooked), "rigid cook");
    Check(cooked.attributes.size() == 4 && cooked.streams[0].slot == 0 &&
          cooked.streams[0].stride == 64 && cooked.base_vertex == -4,
          "explicit canonical interleaving");
    Check(Lane(cooked, 0, 0) == 1.25f && Lane(cooked, 0, 1) == -2 &&
          Lane(cooked, 0, 2) == 3 && Lane(cooked, 0, 3) == 1, "float3 values/default w");
    Check(Lane(cooked, 1, 0) == 1 && Lane(cooked, 1, 1) == -1 &&
          Lane(cooked, 1, 2) == 0 && std::abs(Lane(cooked, 1, 3) - 16384.f / 32767) < 1e-6f,
          "SNORM pair order and saturation");
    Check(std::abs(Lane(cooked, 2, 0) - 5) < 1e-5f &&
          std::abs(Lane(cooked, 2, 1) + 7) < 1e-5f &&
          Lane(cooked, 2, 2) == 32767 && Lane(cooked, 2, 3) == 0,
          "raw signed UV and full four-lane shader value");
    Check(Lane(cooked, 3, 0) == 128.f / 255 && Lane(cooked, 3, 2) == 32.f / 255 &&
          Lane(cooked, 3, 3) == 1, "BGRA to canonical RGBA");
    Check(EncodeNativeMesh(cooked, file) && file[6] == 2 &&
          NativeMeshContentId(cooked) == Read(file, 8, 8), "v2 independent identity/checksum");
    auto reordered = elements;
    std::reverse(reordered.begin(), reordered.end());
    NativeMeshData second;
    packed.layout = 999; // source declaration identity is not an asset identity
    Check(CookRigidMesh(packed, reordered, decode, false, second) &&
          NativeMeshContentId(second) == NativeMeshContentId(cooked), "source order/hash independence");
  }
  // No source bytes, declaration, names or decode recipe remain in scope.
  cooked = {};
  Check(DecodeNativeMesh(file, cooked), "source-free v2 decoding");
  NativeVertexInputHandle input;
  {
    NativeVertexInputLibrary library;
    input = RigidMeshVertexInput(cooked, library);
    Check(input && input->Streams() == 1 && input->ShaderDecode() == VertexShaderDecode{} &&
          input->Pullable() && input->PullTable()[7] == ((4u << 24) | 32),
          "source-free IA/pulling with zero decoder masks");
  }
  Check(input->Elements()[0].format == F::R32G32B32A32_FLOAT &&
        input->Elements()[1].slotIndex == 15, "source-free owned binding lifetime");
  const auto good_id = NativeMeshContentId(cooked);
  for (size_t i = 0; i < file.size(); ++i) {
    auto bad = file;
    bad[i] ^= 1;
    Check(!DecodeNativeMesh(bad, cooked) && NativeMeshContentId(cooked) == good_id,
          "v2 corruption is transactional");
    Check(!DecodeNativeMesh(std::span(file).first(i), cooked), "v2 truncation");
  }
  for (size_t offset : {size_t(36), size_t(40), size_t(44), size_t(48)}) {
    auto bad = file;
    Word(bad, offset, UINT32_MAX); // count/semantic/index/offset with valid checksum
    RepairChecksum(bad);
    Check(!DecodeNativeMesh(bad, cooked), "hostile v2 schema");
  }
  auto invalid = cooked;
  invalid.attributes[1] = invalid.attributes[0];
  invalid.layout = NativeMeshLayoutId(invalid.attributes);
  Check(!ValidateNativeMesh(invalid), "duplicate semantics");
  invalid = cooked;
  Word(invalid.streams[0].bytes, 0, 0x7fc00000);
  Check(!ValidateNativeMesh(invalid), "native payload rejects nonfinite float");
  invalid = cooked;
  invalid.streams[0].bytes.push_back(0);
  Check(!ValidateNativeMesh(invalid), "canonical stride divisibility");
  auto packed = Packed();
  auto elements = Elements();
  elements[4].slotIndex = 2;
  Check(!CookRigidMesh(packed, elements, decode, false, cooked), "constrained input rejected");
  elements = Elements();
  elements[0].alignedByteOffset = UINT32_MAX;
  Check(!CookRigidMesh(packed, elements, decode, false, cooked), "attribute offset overflow");
  elements = Elements();
  elements[0].format = F::R8G8B8A8_UINT;
  Check(!CookRigidMesh(packed, elements, decode, false, cooked), "integer class not guessed");
  Check(!CookRigidMesh(packed, Elements(), decode, true, cooked), "mixed packed basis rejected");
  Check(NativeMeshContentId(cooked) == good_id, "failed cooks preserve prior owner");
  Check(!CookRigidMesh(packed, Elements(), decode, false, packed), "aliased output rejected");
  // DEC3N bit patterns, including a NaN when misinterpreted as a scalar float.
  elements = {{"POSITION", 0, 0, F::R32G32B32_FLOAT, 2, 0},
              {"NORMAL", 0, 5, F::R32_FLOAT, 2, 12},
              {"TANGENT", 0, 6, F::R32G32B32_FLOAT, 15, 0}};
  Word(packed.streams[0].bytes, 12, 0xffc00400);
  Check(CookRigidMesh(packed, elements, {}, true, cooked) && Lane(cooked, 1, 0) == -1 &&
        Lane(cooked, 1, 1) == 0 && Lane(cooked, 1, 2) == -1.f / 512 &&
        Lane(cooked, 1, 3) == 0, "packed 11/11/10 decoded as bits, not float");
  Check(cooked.attributes.size() == 3 && Lane(cooked, 2, 3) == 0,
        "missing packed basis is an explicit zero native value");
  NativeVertexInputLibrary defaults;
  const auto default_input = RigidMeshVertexInput(cooked, defaults);
  Check(default_input && default_input->Elements()[10].format == F::R32_FLOAT &&
        default_input->Elements()[6].format == F::R32G32B32A32_FLOAT,
        "absent color retains alpha one, packed tangent retains w zero");
  // Half subnormals, sign and one; then nonfinite rejection.
  elements = {{"POSITION", 0, 0, F::R32G32B32_FLOAT, 2, 0},
              {"TEXCOORD", 0, 7, F::R16G16B16A16_FLOAT, 2, 12}};
  for (unsigned vertex = 0; vertex < 3; ++vertex) {
    Word(packed.streams[0].bytes, vertex * 28 + 12, 0x00018000);
    Word(packed.streams[0].bytes, vertex * 28 + 16, 0x3c00c000);
  }
  Check(CookRigidMesh(packed, elements, {}, false, cooked) &&
        std::signbit(Lane(cooked, 1, 0)) && Lane(cooked, 1, 1) == std::ldexp(1.f, -24) &&
        Lane(cooked, 1, 2) == -2 && Lane(cooked, 1, 3) == 1, "half values/subnormals/signed zero");
  Word(packed.streams[0].bytes, 12, 0x7c00);
  Check(!CookRigidMesh(packed, elements, {}, false, cooked), "half infinity rejected");
  NativeVertexInputLibrary tiny(NativeVertexInputLibrary::kOwnerBytes - 1);
  Check(!RigidMeshVertexInput(cooked, tiny), "source-free input byte budget");
  std::cout << "canonical mesh: numeric conversion, schema/identity, rejection, "
               "source-destroyed IA/pulling and lifetimes passed\n";
}
