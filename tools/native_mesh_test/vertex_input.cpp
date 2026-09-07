/**
 * @brief Production input ownership, resource-free consumers and bounded reuse.
 * @copyright Copyright (c) 2026 reblue contributors
 * @license BSD 3-Clause, see LICENSE
 */
#include "gpu/scene/native_vertex_input.h"
#include <iostream>
#include <stdexcept>
#include <vector>

using namespace bd::gpu::scene;
using F = plume::RenderFormat;
static void Check(bool value, const char *message) {
  if (!value)
    throw std::runtime_error(message);
}
void TestVertexInput() {
  NativeVertexInputHandle owned;
  const VertexShaderDecode decode{1, 2, 4, 8, 16, 32, 64};
  {
    char name[] = "POSITION";
    std::vector<plume::RenderInputElement> elements{
        {name, 0, 0, F::R32G32B32_FLOAT, 0, 0},
        {"TEXCOORD", 0, 7, F::R16G16_FLOAT, 2, 12},
        {"POSITION", 1, 1, F::R32G32B32A32_FLOAT, 15, 0}};
    NativeVertexInputLibrary library(2 * NativeVertexInputLibrary::kOwnerBytes,
                                     2);
    owned = library.Resolve(elements, 5, decode);
    Check(bool(owned) && owned->Elements().size() == 3 && owned->Streams() == 5,
          "owned input");
    Check(library.Resolve(elements, 5, decode) == owned && library.Size() == 1,
          "content reuse");
    auto changed = decode;
    ++changed.normals;
    const auto second = library.Resolve(elements, 5, changed);
    Check(second && second->Id() != owned->Id(),
          "decode participates in identity");
    ++changed.normals;
    Check(!library.Resolve(elements, 5, changed) && library.Size() == 2,
          "bounded new owners");
    Check(library.Resolve(elements, 5, decode) == owned, "budget-full reuse");
    Check(library.Bytes() == 2 * NativeVertexInputLibrary::kOwnerBytes,
          "owner accounting");
    name[0] = 'X';
    elements.clear();
    Check(std::strcmp(owned->Elements()[0].semanticName, "POSITION") == 0,
          "names not borrowed");
  }
  // Destroy the source AND library, then exercise the production PSO/constant
  // input selectors. A native consumer must not call its legacy reader.
  const auto elements = VertexInputElements(
      owned.get(), []() -> std::span<const plume::RenderInputElement> {
        throw std::runtime_error("native pipeline read old declaration");
      });
  Check(elements.size() == 3 && elements[1].slotIndex == 2,
        "source-free pipeline inputs");
  Check(VertexInputDecode(owned.get(),
                          []() -> VertexShaderDecode {
                            throw std::runtime_error(
                                "native constants read old declaration");
                          }) == decode,
        "source-free shader bridge");
  Check(owned->Pullable() && owned->PullTable()[0] == (3u << 24) &&
            owned->PullTable()[7] == ((13u << 24) | (2u << 16) | 12) &&
            owned->PullTable()[1] == 0,
        "real streams and synthetic zero attributes");
  NativeVertexInputLibrary library;
  std::vector<plume::RenderInputElement> bad(elements.begin(), elements.end());
  bad[1].location = 0;
  Check(!library.Resolve(bad, 5, {}), "duplicate location");
  bad.assign(elements.begin(), elements.end());
  bad[0].slotIndex = 16;
  Check(!library.Resolve(bad, 5, {}), "invalid slot");
  bad[0].slotIndex = 0;
  bad[0].semanticName = "1234567890123456";
  Check(!library.Resolve(bad, 5, {}), "bounded semantic name");
  bad[0].semanticName = nullptr;
  Check(!library.Resolve(bad, 5, {}), "null semantic");
  Check(!library.Resolve({}, 5, {}) && !library.Resolve(elements, 65536, {}),
        "empty and invalid mask");
  NativeVertexInputLibrary tiny(NativeVertexInputLibrary::kOwnerBytes - 1);
  Check(!tiny.Resolve(elements, 5, {}), "byte-budget refusal");
  Check(VertexInputPullEntry(F::R32_FLOAT, 16, 0) == 0 &&
            VertexInputPullEntry(F::R32_FLOAT, 0, 65536) == 0 &&
            VertexInputPullEntry(F::UNKNOWN, 0, 0) == 0,
        "pulling bounds/unknown");
  std::cout << "native vertex input: ownership, independent consumers, "
               "pulling, reuse and budgets passed\n";
}
