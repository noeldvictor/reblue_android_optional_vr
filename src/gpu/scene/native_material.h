/**
 * @file    gpu/scene/native_material.h
 * @brief   Temporary guest asset boundary for native material properties.
 * @copyright Copyright (c) 2026 Tom Clay <tomc@tctechstuff.com>
 * @license BSD 3-Clause, see LICENSE
 */
#pragma once
#include "gpu/scene/native_material_library.h"
#include "gpu/scene/node_tag.h"
#include <optional>

namespace bd::gpu::scene {
struct NativeGeometry;
struct ModelMaterialImport;
std::shared_ptr<const ModelMaterialImport> FindLoadedNativeModelMaterials(
    uint32_t source_model, uint32_t source_mesh);
uint64_t LoadedNativeModelGeneration(uint32_t source_model);
std::shared_ptr<const NativeGeometry> FindLoadedNativeGeometry(
    const NodeTag &tag, uint32_t index_va, uint32_t stream_va,
    uint32_t first_index, uint32_t index_count, uint64_t layout, uint32_t stride);
void NativeModelGeometryCheck(bool same);
void NativeModelGeometryNoteDraw(bool load_owned);
bool ModelOwnsReflectionBinding(const NodeTag &tag);
std::optional<NativeReflectionRecipe> ImportNativeReflectionRecipe(
    const NodeTag &tag, uint32_t index_va, uint32_t stream_va,
    uint32_t first_index, uint32_t index_count);
std::optional<NativeSkinBinding> ImportNativeSkinBinding(
    const NodeTag &tag, uint32_t index_va, uint32_t stream_va,
    uint32_t first_index, uint32_t index_count);
NativeMaterialHandle ImportNativeMaterial(
    const NodeTag &tag, uint32_t index_va, uint32_t stream_va,
    uint32_t first_index, uint32_t index_count);
// Named immutable shadow policy from mesh commands/model controls. This is
// still discovered through the model bridge, not persisted in .bdmat v1.
std::optional<bool> ImportMaterialDisablesShadow(
    const NodeTag &tag, uint32_t index_va, uint32_t stream_va,
    uint32_t first_index, uint32_t index_count);
uint32_t EvaluateNativeMaterial(const NodeTag &tag,
                               const NativeMaterialAsset &material,
                               std::array<float, 4> values[3]);
void NativeMaterialCheck(uint32_t mask, const std::array<float, 4> values[3],
                         const uint8_t *pixel_constants);
void NativeMaterialNoteReplay(uint32_t mask);
} // namespace bd::gpu::scene
