/**
 * @brief Conservative native geometry bounds, independent of source memory/GPU APIs.
 * @copyright Copyright (c) 2026 reblue contributors
 * @license BSD 3-Clause, see LICENSE
 */
#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <optional>

namespace bd::gpu::scene {
struct NativeBounds {
  std::array<float, 3> min{}, max{};
  bool operator==(const NativeBounds &) const = default;
  bool Valid() const {
    for (unsigned n = 0; n < 3; ++n)
      if (!std::isfinite(min[n]) || !std::isfinite(max[n]) || min[n] > max[n]) return false;
    return true; // Flat triangles are valid; the query proxy inflates every axis.
  }
};
// Row-vector affine transform, exactly the native rigid vertex contract. Interval
// arithmetic handles reflection, shear and nonuniform scale. Include an FP32
// multiply/add error envelope and round outwards, never towards the geometry.
inline std::optional<NativeBounds> TransformNativeBounds(
    const NativeBounds &local, const std::array<float, 16> &world) {
  if (!local.Valid() || world[3] != 0 || world[7] != 0 || world[11] != 0 || world[15] != 1) return {};
  for (float value : world) if (!std::isfinite(value)) return {};
  NativeBounds result;
  for (unsigned axis = 0; axis < 3; ++axis) {
    double lo = world[12+axis], hi = lo, magnitude = std::abs(lo);
    for (unsigned row = 0; row < 3; ++row) {
      const double a = double(local.min[row])*world[row*4+axis];
      const double b = double(local.max[row])*world[row*4+axis];
      lo += (std::min)(a,b); hi += (std::max)(a,b);
      magnitude += (std::max)(std::abs(a),std::abs(b));
    }
    const double error = 8*std::numeric_limits<float>::epsilon()*magnitude +
        16*double((std::numeric_limits<float>::min)()); // includes GPU flush-to-zero
    result.min[axis] = std::nextafter(float(lo-error), -std::numeric_limits<float>::infinity());
    result.max[axis] = std::nextafter(float(hi+error), std::numeric_limits<float>::infinity());
  }
  return result.Valid() ? std::optional(result) : std::nullopt;
}
} // namespace bd::gpu::scene
