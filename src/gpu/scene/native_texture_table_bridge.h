/**
 * @brief Temporary source association for load-owned native texture tables.
 * @copyright Copyright (c) 2026 reblue contributors
 * @license BSD 3-Clause License
 */
#pragma once
#include "gpu/scene/native_texture_table.h"
#include <optional>
namespace bd::gpu::scene {
std::optional<NativeTextureBinding> FindLoadedNativeTableTexture(
    uint32_t source_table, uint32_t slot);
// Object publication pins one immutable table generation for all primitives.
NativeTextureTableHandle FindLoadedNativeTextureTable(uint32_t source_table);
// Called at image replacement/eviction, never by a draw. No resource lookup or
// Video lock may occur here: the mirror producer calls with its own lock held.
void NativeTextureTableImageChanged(uint32_t source_image,
                                    NativeTextureTableSlot slot) noexcept;
} // namespace bd::gpu::scene
