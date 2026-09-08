/**
 * @brief Canonical values, hostile files and source-destroyed shader inputs.
 * @copyright Copyright (c) 2026 reblue contributors
 * @license BSD 3-Clause, see LICENSE
 */
#include "gpu/scene/native_mesh_cook.h"
#include "gpu/scene/native_skin_mesh.h"
#include "gpu/scene/native_rigid_program.h"
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

static void TestMeshBounds() {
  NativeMeshData mesh;
  mesh.attributes = {{MeshSemantic::Position,0,0}};
  mesh.layout = NativeMeshLayoutId(mesh.attributes);
  mesh.base_vertex = -5; mesh.indices = {8,6,7};
  mesh.streams.push_back({0,16,std::vector<uint8_t>(5*16)});
  const std::array<std::array<float,3>,5> points{{{1e10f,1e10f,1e10f},
      {-2,1,4},{3,-5,6},{0,2,4},{-1e10f,-1e10f,-1e10f}}};
  for (unsigned n=0;n<points.size();++n) {
    for (unsigned axis=0;axis<3;++axis)
      Word(mesh.streams[0].bytes,n*16+axis*4,std::bit_cast<uint32_t>(points[n][axis]));
    Word(mesh.streams[0].bytes,n*16+12,std::bit_cast<uint32_t>(99.f)); // rigid VS uses xyz, w=1
  }
  const auto bounds = BuildNativeMeshBounds(mesh);
  Check(bounds && *bounds == NativeBounds{{-2,-5,4},{3,2,6}}, "indexed bounds ignore unused vertices and position w");
  std::vector<uint8_t> file, second_file;
  Check(EncodeNativeMesh(mesh,file), "bounds fixture persisted v2");
  const auto id = NativeMeshContentId(mesh);
  mesh = {};
  Check(DecodeNativeMesh(file,mesh) && BuildNativeMeshBounds(mesh) == bounds && NativeMeshContentId(mesh) == id,
      "source-destroyed bounds reconstructed from unchanged asset identity");
  Check(EncodeNativeMesh(mesh,second_file) && file == second_file,"derived bounds do not create a new file format/cache");
  auto positive_base = mesh; positive_base.base_vertex = 1; positive_base.indices = {2,0,1};
  Check(BuildNativeMeshBounds(positive_base) == bounds,"positive base and permuted indices");
  auto flat = mesh; flat.indices = {6,6,6};
  Check(BuildNativeMeshBounds(flat) == std::optional(NativeBounds{{-2,1,4},{-2,1,4}}),"degenerate/flat indexed bounds");
  auto invalid = mesh; invalid.base_vertex = -9;
  Check(!BuildNativeMeshBounds(invalid),"negative effective index refuses bounds");
  invalid = mesh; invalid.indices[0] = UINT32_MAX;
  Check(!BuildNativeMeshBounds(invalid),"out-of-range effective index refuses bounds");
  invalid = mesh; invalid.attributes.clear();
  Check(!BuildNativeMeshBounds(invalid),"packed v1 has no guessed native position");
  invalid = mesh; Word(invalid.streams[0].bytes,16,0x7fc00000);
  Check(!BuildNativeMeshBounds(invalid),"nonfinite native geometry refuses bounds");
  for (const auto &world : std::array<std::array<float,16>,3>{{
      {0,0,2,0, 0,3,0,0, -4,0,0,0, 100,200,300,1},
      {-2,1,0,0, .5f,3,-.75f,0, 1,0,4,0, -100,40,-60,1},
      {1,1,0,0, 1,-1,0,0, 0,0,1,0, 1e8f,-1e8f,0,1}}}) {
    const auto transformed = TransformNativeBounds(*bounds,world);
    Check(bool(transformed),"affine native bounds transform");
    for (unsigned corner=0;corner<8;++corner) {
      const float x = (corner&1) ? bounds->max[0] : bounds->min[0];
      const float y = (corner&2) ? bounds->max[1] : bounds->min[1];
      const float z = (corner&4) ? bounds->max[2] : bounds->min[2];
      for (unsigned axis=0;axis<3;++axis) {
        const float value = x*world[axis]+y*world[4+axis]+z*world[8+axis]+world[12+axis];
        Check(transformed->min[axis] <= value && value <= transformed->max[axis],
            "rotated/reflected/sheared/scaled FP32 corners remain enclosed");
      }
    }
  }
  std::array<float,16> invalid_world{};
  Check(!TransformNativeBounds(*bounds,invalid_world),"non-affine matrix refuses");
  invalid_world = {1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1};
  invalid_world[0] = std::numeric_limits<float>::infinity();
  Check(!TransformNativeBounds(*bounds,invalid_world),"nonfinite transform refuses");
  invalid_world[0] = (std::numeric_limits<float>::max)();
  Check(!TransformNativeBounds(*bounds,invalid_world),"overflowing transform refuses");
  std::cout << "native bounds: indexed positions, persistence, signed base, affine containment and hostile inputs passed\n";
}

static void TestSkinCook() {
  NativeMeshData packed;
  packed.layout = 1; packed.base_vertex = -7; packed.indices = {7,8,9};
  packed.streams.push_back({2,96,std::vector<uint8_t>(3*96)});
  const std::array<std::array<float,4>,6> values{{
      {1,2,3,2}, {1,0,0,1.f/64}, // position0 / normal0 + palette slot1
      {4,5,6,3}, {0,1,0,0},      // position1 / normal1 + slot0
      {7,8,9,5}, {0,0,1,2.f/64}  // position2 / normal2 + slot2
  }};
  const auto set = [&](NativeMeshData &mesh, size_t vertex, size_t a, size_t lane, float value) {
    Word(mesh.streams[0].bytes,vertex*96+a*16+lane*4,std::bit_cast<uint32_t>(value));
  };
  for (size_t v = 0; v < 3; ++v)
    for (size_t a = 0; a < 6; ++a)
      for (size_t lane = 0; lane < 4; ++lane) set(packed,v,a,lane,values[a][lane]);
  const std::vector<plume::RenderInputElement> elements{
      {"POSITION",0,0,F::R32G32B32A32_FLOAT,2,0},
      {"NORMAL",0,5,F::R32G32B32A32_FLOAT,2,16},
      {"POSITION",1,1,F::R32G32B32A32_FLOAT,2,32},
      {"POSITION",2,2,F::R32G32B32A32_FLOAT,2,48},
      {"POSITION",3,3,F::R32G32B32A32_FLOAT,2,64},
      {"POSITION",4,4,F::R32G32B32A32_FLOAT,2,80}};
  const auto binding = *DecodeNativeSkinBinding(std::array<uint16_t,3>{2,0,1});
  NativeMeshData cooked;
  for (uint32_t influences = 1; influences <= 3; ++influences) {
    Check(CookSkinMesh(packed,elements,{},false,influences,binding,cooked),"joint-local skin cook");
    Check(NativeMeshSkinInfluences(cooked.attributes) == influences &&
        cooked.attributes.size() == influences*2+2,"explicit paired skin schema");
    const auto joint_attribute = influences*2, weight_attribute = joint_attribute+1;
    Check(Lane(cooked,joint_attribute,0) == 0 && Lane(cooked,0,3) == 0 &&
        Lane(cooked,influences,3) == 0,"joint IDs resolved and source index/weight packing removed");
    const float denominator = influences == 1 ? 2.f : influences == 2 ? 5.f : 10.f;
    Check(Lane(cooked,weight_attribute,0) == 2.f/denominator,"weights normalized at cook boundary");
    std::vector<uint8_t> file;
    Check(EncodeNativeMesh(cooked,file) && file[6] == 3,"skin v3 persisted");
    NativeMeshData restored;
    Check(DecodeNativeMesh(file,restored) && NativeMeshContentId(restored) == NativeMeshContentId(cooked),
        "skin persistence identity");
    const auto id = NativeMeshContentId(cooked);
    for (size_t n = 0; n < file.size(); ++n) {
      auto bad = file; bad[n] ^= 1;
      Check(!DecodeNativeMesh(bad,cooked) && NativeMeshContentId(cooked) == id,"skin corrupt file transactional");
      Check(!DecodeNativeMesh(std::span(file).first(n),cooked),"skin truncation");
    }
    auto downgraded = file; downgraded[6] = 2;
    Check(!DecodeNativeMesh(downgraded,cooked),"skin schema cannot masquerade as v2");
    NativeVertexInputLibrary library;
    Check(!RigidMeshVertexInput(cooked,library) && !BuildNativeMeshBounds(cooked),
        "skin never admitted as rigid geometry/bounds");
  }
  const auto id = NativeMeshContentId(cooked);
  for (uint32_t influences : {0u,4u})
    Check(!CookSkinMesh(packed,elements,{},false,influences,binding,cooked),"invalid influence count");
  auto missing = elements; missing.pop_back();
  Check(!CookSkinMesh(packed,missing,{},false,3,binding,cooked),"missing joint-local normal refuses");
  auto short_binding = binding; short_binding.count = 2;
  Check(!CookSkinMesh(packed,elements,{},false,3,short_binding,cooked),"palette slot checked at producer");
  Check(!CookSkinMesh(packed,elements,{},false,3,binding,packed),"skin aliased output refuses");
  auto bad = packed; set(bad,0,0,3,-1);
  Check(!CookSkinMesh(bad,elements,{},false,3,binding,cooked),"negative skin weight refuses");
  bad = packed; for (size_t a : {0u,2u,4u}) set(bad,0,a,3,0);
  Check(!CookSkinMesh(bad,elements,{},false,3,binding,cooked),"zero weight sum refuses");
  Check(NativeMeshContentId(cooked) == id,"failed skin cooks preserve prior owner");
  auto reordered = elements; std::reverse(reordered.begin(),reordered.end());
  NativeMeshData second;
  Check(CookSkinMesh(packed,reordered,{},false,3,binding,second) && NativeMeshContentId(second) == id,
      "skin stable identity independent of source declaration order");
  auto swapped = packed;
  for (size_t v = 0; v < 3; ++v)
    for (size_t a = 0; a < 6; ++a)
      for (size_t lane = 0; lane < 4; ++lane) set(swapped,v,a,lane,values[a][lane^1]);
  VertexShaderDecode swap_decode{}; swap_decode.positions = 31; swap_decode.normals = 1;
  Check(CookSkinMesh(swapped,elements,swap_decode,false,3,binding,second) && NativeMeshContentId(second) == id,
      "all joint-local position/normal pairs use the shared endian decoder");
  auto inactive = packed; set(inactive,0,0,3,0); set(inactive,0,1,3,-100);
  Check(CookSkinMesh(inactive,elements,{},false,3,binding,second) && Lane(second,6,0) == 0 &&
      Lane(second,7,0) == 0,"inactive lane ignores an unused source palette slot");
  auto other_binding = binding; other_binding.joints[1] = UINT16_MAX;
  Check(CookSkinMesh(packed,elements,{},false,3,other_binding,second) &&
      Lane(second,6,0) == UINT16_MAX && NativeMeshContentId(second) != id,"full model-local joint identity");
  // Destroy source bytes/recipes before the world-space consumer runs.
  packed = {}; second = {};
  std::array<RenderMatrix,3> pose{{
      {1,0,0,0, 0,1,0,0, 0,0,1,0, 10,0,0,1},
      {0,1,0,0, -1,0,0,0, 0,0,1,0, 0,20,0,1},
      {2,0,0,0, 0,3,0,0, 0,0,4,0, 0,0,30,1}}};
  const auto bounds = BuildNativeSkinBounds(cooked,pose);
  const auto envelopes = BuildNativeMeshJointBounds(cooked);
  const auto conservative = envelopes ? TransformNativeSkinBounds(*envelopes,pose) : std::nullopt;
  NativeVertexInputLibrary skin_inputs;
  const auto skin_input = NativeSkinShadowVertexInput(cooked,skin_inputs);
  Check(skin_input && skin_input->Elements().size() == 5 && skin_input->ShaderDecode() == VertexShaderDecode{},
      "native skin GPU signature has no source decoder state");
  Check(envelopes && envelopes->size() == 3 && conservative,"load-time native joint envelopes");
  const std::array<float,3> expected{.6f,18.4f,21.3f};
  Check(bool(bounds),"native skin consumes owned pose");
  for (size_t axis = 0; axis < 3; ++axis)
    Check(std::abs(bounds->min[axis]-expected[axis]) < 1e-4f && bounds->min[axis] == bounds->max[axis] &&
        conservative->min[axis] <= expected[axis] && conservative->max[axis] >= expected[axis],
        "joint-local weighted deformation, rotation/nonuniform scale and world translation once");
  NativeSkinVertex vertex;
  vertex.positions = {{{1,2,3},{4,5,6},{7,8,9}}};
  vertex.normals = {{{1,0,0},{0,1,0},{0,0,1}}};
  vertex.joints = {0,2,1}; vertex.weights = {.2f,.3f,.5f};
  const auto deformed = DeformNativeSkinVertex(vertex,pose);
  Check(deformed && std::abs(deformed->normal[0]-.2f) < 1e-6f &&
      std::abs(deformed->normal[1]-.9f) < 1e-6f && deformed->normal[2] == .5f,
      "independent authored normals use weighted joint linear transforms");
  auto unused = cooked;
  const auto extra = std::vector<uint8_t>(cooked.streams[0].bytes.begin(),
      cooked.streams[0].bytes.begin()+cooked.streams[0].stride);
  unused.streams[0].bytes.insert(unused.streams[0].bytes.end(),extra.begin(),extra.end());
  Word(unused.streams[0].bytes,3*unused.streams[0].stride,std::bit_cast<uint32_t>(1e20f));
  Check(BuildNativeSkinBounds(unused,pose) == bounds,"unindexed animated outlier cannot change bounds");
  unused.base_vertex = 0; unused.indices = {2,0,1};
  Check(BuildNativeSkinBounds(unused,pose) == bounds,"skin index order/base do not change effective vertices");
  Check(!BuildNativeSkinBounds(cooked,std::span(pose).first(2)),"missing model joint refuses");
  pose[0][12] += 100;
  const auto animated = BuildNativeSkinBounds(cooked,pose);
  const auto animated_envelope = TransformNativeSkinBounds(*envelopes,pose);
  Check(animated_envelope && animated_envelope->max[0] >= animated->max[0],"same immutable envelopes follow changed poses");
  Check(animated && std::abs(animated->min[0]-bounds->min[0]-20) < 1e-4f,"fresh animated pose changes bound");
  pose[0][3] = 1;
  Check(!BuildNativeSkinBounds(cooked,pose),"nonaffine palette refuses");
  auto hostile = cooked;
  Word(hostile.streams[0].bytes,6*16,std::bit_cast<uint32_t>(.5f));
  Check(!ValidateNativeMesh(hostile),"fractional native joint refuses");
  hostile = cooked; Word(hostile.streams[0].bytes,7*16,std::bit_cast<uint32_t>(-.1f));
  Check(!ValidateNativeMesh(hostile),"negative persisted weight refuses");
  hostile = cooked; Word(hostile.streams[0].bytes,12,std::bit_cast<uint32_t>(1.f));
  Check(!ValidateNativeMesh(hostile),"source packing cannot leak into native local position");
  hostile = cooked; hostile.attributes[4].index = 0; hostile.layout = NativeMeshLayoutId(hostile.attributes);
  Check(!ValidateNativeMesh(hostile),"duplicate/missing joint-local normal rejects persistence");
  std::cout << "native skin: 1/2/3 joint-local influences, exact IDs, source-destroyed persistence, deformation and animated bounds passed\n";
}

void TestMeshCook() {
  TestSkinCook();
  TestMeshBounds();
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
        default_input->Elements()[6].format == F::R32G32B32A32_FLOAT &&
        default_input->PullTable()[10] == ((1u << 24) | (15u << 16)) &&
        default_input->PullTable()[1] == ((4u << 24) | (15u << 16)),
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
