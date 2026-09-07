#include "gpu/scene/native_material_data.h"
#include "gpu/scene/native_material_texture_source.h"
#include <iostream>
#include <stdexcept>
#include <unordered_map>
using namespace bd::gpu::scene;
namespace {
void Require(bool good, const char *message) {
  if (!good) throw std::runtime_error(message);
}
using Image = uint32_t;
using Selection = MaterialImageSelection<Image>;
using Values = MaterialTextureValues<Image>;
Selection Bind(Image image) { return {MaterialImageAction::Bind, image}; }
}
void TestNativeMaterialTextures() {
  std::vector<NativeMaterialRange> ranges;
  std::vector<MaterialImageAssignment> assignments;
  std::vector<uint16_t> words{0x6001, 0x6102, 0x1000, 1, 0,
      0x6000, 0x1000, 1, 3, 0x6003, 0x1000, 1, 6, 0xff};
  Require(DecodeMeshMaterials(words, ranges, &assignments) && assignments.size() == 4 &&
          ranges[0].texture_assignment_end == 2 && ranges[1].texture_assignment_end == 3 &&
          ranges[2].texture_assignment_end == 4, "ordered assignment ordinals decoded at load");
  auto bad_words = words; bad_words.pop_back();
  Require(!DecodeMeshMaterials(bad_words, ranges, &assignments) && assignments.size() == 4,
          "failed texture decode is transactional");
  words.clear(); // compiled native program survives source destruction
  std::unordered_map<uint8_t, Selection> images{{0, {MaterialImageAction::Keep}},
      {1, Bind(101)}, {2, Bind(202)}, {3, {}}};
  auto lookup = [&](uint8_t key) { return images.at(key); };
  MaterialTextureInputs<Image> inputs;
  inputs.owns_uv = true; inputs.initial_uv = {1, 2, 3, 4}; inputs.reset_uv = {5, 6, 7, 8};
  std::vector<Values> out;
  auto compose = [&] { return ComposeMaterialTextures(std::span<const MaterialImageAssignment>(assignments),
      std::span<const NativeMaterialRange>(ranges), inputs, lookup, out); };
  Require(compose() && out[0].images[0] == 101 && out[0].images[1] == 202 &&
          out[1].images[0] == 101 && !(out[2].image_mask & 1),
          "known null preserves prior image, unavailable non-null invalidates it");
  const auto before = out;
  auto bad_ranges = ranges; bad_ranges[1].texture_assignment_end = 1;
  Require(!ComposeMaterialTextures(std::span<const MaterialImageAssignment>(assignments),
      std::span<const NativeMaterialRange>(bad_ranges), inputs, lookup, out) && out == before,
      "backward assignment frontier refused without partial output");
  Require(!ComposeMaterialTextures(std::span<const MaterialImageAssignment>(assignments),
      std::span<const NativeMaterialRange>(ranges), inputs, lookup, out, 2), "primitive output budget");

  inputs.overrides = {{1, 1, {}, true, Bind(301)}, {1, 1, std::array<float, 2>{9, 10}, true, Bind(302)},
                      {1, 0, {}, true, Bind(303)}};
  inputs.late_images = {{1, 0, {}, true, Bind(401)}};
  Require(compose() && out[0].images[0] == 301 && out[0].uv == std::array<float, 4>{9, 10, 3, 4} &&
          out[1].uv == std::array<float, 4>{5, 6, 3, 4},
          "first UV match stops scan, skips its own image, earlier image skips late override, UV resets on next token");
  inputs.overrides.erase(inputs.overrides.begin());
  Require(compose() && out[0].images[0] == 401, "UV-only early match still allows late image");
  inputs.late_images.push_back({1, 0, {}, true, Bind(402)});
  Require(compose() && out[0].images[0] == 401, "first matching late image wins");
  inputs.late_images.clear(); inputs.overrides.clear();
  inputs.overrides = {{1, 2, {}, true, Bind(501)}, {1, 0, {}, true, Bind(502)}};
  Require(compose() && out[0].images[0] == 502, "channel matching and wildcard order");
  inputs.skip_overrides = true;
  Require(compose() && out[0].images[0] == 101, "special material families skip early overrides");
  inputs.special_selector = 1;
  Require(compose() && !(out[0].image_mask & 1), "unknown special image cannot inherit the base binding");
  inputs.late_images = {{1, 0, {}, true, Bind(601)}};
  Require(compose() && out[0].images[0] == 601, "late animation can replace special selection");
  inputs.special_selector.reset(); inputs.late_images.clear();
  images[1] = Bind(701);
  Require(compose() && out[0].images[0] == 701, "new object publication uses current images, not prior draw values");
  inputs.overrides.resize(257);
  Require(!compose(), "override scan has an aggregate bound");

  const uint16_t reflection[]{0x6501, 0x1000, 1, 0, 0x0602, 0x1000, 1, 3,
      0x6501, 0x0602, 0x06ff, 0x1000, 1, 6, 0xff};
  Require(DecodeMeshMaterials(reflection, ranges, &assignments) && assignments.size() == 3 &&
          assignments[1].source == MaterialImageSource::Unknown &&
          ranges.back().texture_assignment_end == 3,
          "reflection writer invalidates ordinary ownership, repetition/disable do not rebind");

  // The real import helper consumes bounded host-endian fields once. Erasing
  // every source word afterwards does not invalidate the owned values.
  constexpr uint32_t visual = 10000, early = 20000, late = 30000;
  std::unordered_map<uint64_t, uint32_t> memory{
      {visual + 3000, 0}, {visual + 3128, 0}, {visual + 3440, 1},
      {visual + 3560, early}, {visual + 3564, 1}, {visual + 3572, late}, {visual + 3576, late + 84},
      {visual + 3680, 0}, {visual + 3712, 0xffffffff},
      {early + 4, 1}, {early + 8, 0}, {early + 20, 1}, {early + 24, 1}, {early + 84, 77},
      {late + 8, 2}, {late + 12, 88}, {late + 80, 1}};
  for (uint32_t i = 0; i < 4; ++i) memory[visual + 3444 + i * 4] = std::bit_cast<uint32_t>(float(i));
  memory[early + 28] = std::bit_cast<uint32_t>(9.0f); memory[early + 32] = std::bit_cast<uint32_t>(10.0f);
  auto read = [&](uint64_t address) -> std::optional<uint32_t> {
    const auto it = memory.find(address);
    return it == memory.end() ? std::nullopt : std::optional(it->second);
  };
  const auto imported = ReadMaterialTextureInputs<Image>(visual, read, Bind);
  Require(imported && imported->owns_uv && imported->overrides[0].image.image == 77 &&
          imported->late_images[0].image.image == 88, "live object import owns named UV/image values");
  memory[visual + 3564] = 257;
  Require(!ReadMaterialTextureInputs<Image>(visual, read, Bind), "early input count bounded");
  memory[visual + 3564] = 1; memory[visual + 3576] = late + 83;
  Require(!ReadMaterialTextureInputs<Image>(visual, read, Bind), "late input extent validated");
  memory[visual + 3576] = late + 84; memory[early + 28] = 0x7fc00000;
  Require(!ReadMaterialTextureInputs<Image>(visual, read, Bind), "nonfinite UV is unconverted");
  memory[early + 28] = 0; memory[visual + 3128] = 1;
  Require(!ReadMaterialTextureInputs<Image>(visual, read, Bind), "special callback route not guessed");
  Require(!ReadMaterialTextureInputs<Image>(UINT32_MAX - 3, read, Bind), "object extent overflow");
  memory.clear();
  Require(imported->overrides[0].uv == std::array<float, 2>{9, 10} &&
          imported->late_images[0].image.image == 88, "owned publication survives all source storage destruction");
  std::cout << "native material texture order, null inheritance, live overrides, source-free ownership and bounds passed\n";
}
