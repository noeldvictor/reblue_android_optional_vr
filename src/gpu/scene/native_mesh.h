/**
 * @file    native_mesh.h
 * @brief   Loaded-model import boundary and shared native geometry buffers.
 * @copyright Copyright (c) 2026 reblue contributors
 * @license   BSD 3-Clause License
 */
#pragma once

#include <memory>
#include <span>
#include <rex/types.h>
#include "plume_render_interface.h"
#include "gpu/scene/native_vertex_input.h"

namespace bd::gpu {
struct GuestBuffer;
struct GuestVertexDeclaration;
}
namespace bd::gpu::scene {

struct NativeMeshImport {
  const GuestVertexDeclaration *declaration = nullptr;
  const GuestBuffer *streams[16]{};
  u32 offsets[16]{};
  u32 strides[16]{};
  const GuestBuffer *index = nullptr;
  u32 start_index = 0, count = 0, primitive_type = 0;
  i32 base_vertex = 0;
  // Read existing cooked data but suppress new disk outputs during bounded
  // correctness diagnostics. Normal load/cook paths keep persistence enabled.
  bool persist = true;
  // An already cooked LOD replaces the source indices, keeping its winding
  // and the same vertex layout. Empty means import the original mesh.
  std::span<const u32> lod_indices;
};

struct NativeGeometry {
  // Content identity and explicit native stream strides travel with the data,
  // independently of the temporary source-buffer/declaration lookup.
  u64 id = 0, layout = 0;
  bool canonical_vertices = false;
  NativeVertexInputHandle vertex_input;
  u32 strides[16]{};
  plume::RenderVertexBufferView streams[16]{};
  u32 stream_mask = 0;
  plume::RenderIndexBufferView index{};
  u32 count = 0, start_index = 0;
  i32 base_vertex = 0;
};

// Resolves a content-keyed native file, importing on first sight. Only this
// boundary touches guest buffers. The resulting GPU geometry owns its bytes
// independently of model allocations, stream VAs, and physical-block mirrors.
std::shared_ptr<const NativeGeometry> ImportNativeMesh(const NativeMeshImport &r);
// A native asset reference loads v2 geometry without source buffers, a
// declaration, renderer warm-up, or import-source storage. Rejects v1 files.
std::shared_ptr<const NativeGeometry> LoadNativeGeometry(u64 content_id);
void NativeMeshNoteDraw(bool native, bool canonical = false);

} // namespace bd::gpu::scene
