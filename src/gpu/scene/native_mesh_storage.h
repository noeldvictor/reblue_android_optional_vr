/**
 * @file    native_mesh_storage.h
 * @brief   Bounded native mesh persistence, independent of GPU and source memory.
 * @copyright Copyright (c) 2026 reblue contributors
 * @license BSD 3-Clause, see LICENSE
 */
#pragma once

#include "gpu/scene/native_mesh_data.h"
#include <filesystem>
#include <mutex>

namespace bd::gpu::scene {

struct NativeMeshDiskBudget {
  uint64_t max_bytes = 256ull << 20;
  size_t max_files = 16384;
  uint64_t min_free_bytes = 20ull << 30;
};

struct NativeMeshDiskStats {
  uint64_t loaded = 0, written = 0, reused = 0, invalid = 0;
  uint64_t write_failures = 0, budget_refusals = 0, conflicts = 0;
  uint64_t bytes = 0;
  size_t files = 0;
  // Last write inventory, not live volume use. A refused scan may be partial.
  bool inventory_complete = false;
};

constexpr bool NativeMeshWriteFitsReserve(uint64_t available, uint64_t bytes,
                                         uint64_t reserve) {
  return available >= reserve && available - reserve >= bytes &&
         available - reserve - bytes >= (64ull << 10);
}

// Keys/BDMESH v1 bytes are unchanged. The legacy importer still derives the key
// from canonical geometry inputs; this is not a new address-keyed draw cache.
// Writes serialize across cooperating instances/processes via .bdmesh-writer.
// Never steal a stale lease, follow reparse paths, evict files, or overwrite a
// different valid payload. Reads remain available when persistence is disabled.
class NativeMeshDiskCache {
public:
  explicit NativeMeshDiskCache(std::filesystem::path directory,
                               NativeMeshDiskBudget budget = {});
  bool Read(uint64_t key, NativeMeshData &mesh);
  bool Write(uint64_t key, const NativeMeshData &mesh);
  NativeMeshDiskStats Stats() const;
  static std::filesystem::path FileName(uint64_t key);

private:
  std::filesystem::path directory_;
  NativeMeshDiskBudget budget_;
  mutable std::mutex mutex_;
  NativeMeshDiskStats stats_;
};

} // namespace bd::gpu::scene
