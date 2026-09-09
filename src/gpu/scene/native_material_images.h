/**
 * @brief Immutable instance material images; no source wrappers or table addresses.
 * @copyright Copyright (c) 2026 reblue contributors
 * @license BSD 3-Clause, see LICENSE
 */
#pragma once
#include "gpu/scene/native_texture_binding_data.h"
#include "gpu/scene/native_material_textures.h"

namespace bd::gpu::scene {
struct NativeMaterialImages {
  struct Entry {
    uint32_t selector=0, channel=0;
    bool enabled=false, replaces_image=false;
    MaterialImageSelection<NativeTextureBinding> image;
    bool Same(const Entry &other) const {
      return selector == other.selector && channel == other.channel && enabled == other.enabled &&
          replaces_image == other.replaces_image && image.action == other.image.action && image.image == other.image.image;
    }
  };
  // Dense authored slots, including disabled/no-op slots. Order is semantic.
  std::vector<Entry> entries;
  bool Valid() const {
    if (entries.empty() || entries.size() > 256) return false;
    for (const auto &entry : entries)
      if (uint32_t(entry.image.action) > uint32_t(MaterialImageAction::Bind) ||
          (entry.image.action == MaterialImageAction::Bind && !entry.image.image.primary) ||
          (!entry.enabled && entry.replaces_image)) return false;
    return true;
  }
  bool Same(const NativeMaterialImages &other) const {
    if (entries.size() != other.entries.size()) return false;
    for (size_t n=0; n<entries.size(); ++n) if (!entries[n].Same(other.entries[n])) return false;
    return true;
  }
};
} // namespace bd::gpu::scene
