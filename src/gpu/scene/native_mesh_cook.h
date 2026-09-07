/**
 * @brief Transitional import of rigid attributes; source-free native input binding.
 * @copyright Copyright (c) 2026 reblue contributors
 * @license BSD 3-Clause, see LICENSE
 */
#pragma once
#include "gpu/scene/native_mesh_data.h"
#include "gpu/scene/native_vertex_input.h"

namespace bd::gpu::scene {

// The compatibility recipe exists only at the load/cook boundary. The v2
// result contains values and asset semantics, never this recipe or locations.
// Reject unsupported/dynamic/constrained inputs; leave result unchanged.
bool CookRigidMesh(const NativeMeshData &packed,
                   std::span<const plume::RenderInputElement> elements,
                   VertexShaderDecode decode, bool packed_basis,
                   NativeMeshData &result);

// Temporary adapter to the existing shader signature. Derives IA and pulling
// from the decoded asset alone, with no declaration or per-draw decode masks.
NativeVertexInputHandle RigidMeshVertexInput(const NativeMeshData &mesh,
                                            NativeVertexInputLibrary &library);
} // namespace bd::gpu::scene
