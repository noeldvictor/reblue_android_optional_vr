/**
 * @copyright Copyright (c) 2026 reblue contributors
 * @license BSD 3-Clause, see LICENSE
 */
#pragma once
#include <cstddef>
#include <cstdint>
namespace bd::gpu::scene {
struct NativeRigidIndexedCommand {
  uint32_t index_count, instance_count, first_index;
  int32_t base_vertex;
  uint32_t first_instance;
};
static_assert(sizeof(NativeRigidIndexedCommand) == 20 && offsetof(NativeRigidIndexedCommand, first_instance) == 16);
}
