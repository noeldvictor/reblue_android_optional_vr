/**
 * @brief Retained native image handoff and one shared image-layout record.
 * @copyright Copyright (c) 2026 reblue contributors
 * @license BSD 3-Clause, see LICENSE
 */
#pragma once
#include "gpu/sampled_image.h"
#include <memory>

namespace bd::gpu {
// The producing native store still owns fence-gated destruction. This handle
// keeps the image, view, descriptor and layout record alive for boundary readers.
struct NativeImageLease {
  std::shared_ptr<const void> owner;
  SampledImage image;
  explicit operator bool() const { return owner && bool(image); }
  bool Fits(uint32_t width, uint32_t height, uint32_t layers) const {
    return bool(*this) && image.width == width && image.height == height && image.layers == layers;
  }
};

// A remaining adapter either owns a local record or borrows its native owner's
// record. All reads AND writes go through it; copying a record copies its value,
// never its binding. Unbind before releasing the owner of the shared record.
class ImageLayoutRecord {
public:
  using Layout = plume::RenderTextureLayout;
  ImageLayoutRecord(Layout value = Layout::UNKNOWN) : local_(value) {}
  ImageLayoutRecord(const ImageLayoutRecord &other) : local_(other.Get()) {}
  ImageLayoutRecord &operator=(const ImageLayoutRecord &other) { return *this = other.Get(); }
  ImageLayoutRecord &operator=(Layout value) { Get() = value; return *this; }
  Layout &Get() { return shared_ ? *shared_ : local_; }
  const Layout &Get() const { return shared_ ? *shared_ : local_; }
  operator Layout &() { return Get(); }
  operator const Layout &() const { return Get(); }
  void Bind(Layout &record) { shared_ = &record; }
  void Unbind() { local_ = Get(); shared_ = nullptr; }
private:
  Layout local_;
  Layout *shared_ = nullptr;
};
} // namespace bd::gpu
