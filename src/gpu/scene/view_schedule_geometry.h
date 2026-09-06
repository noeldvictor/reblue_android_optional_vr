/**
 * @brief   Native camera-ray and reflection-candidate geometry for view scheduling.
 * @copyright Copyright (c) 2026 reblue contributors
 * @license BSD 3-Clause, see LICENSE
 */
#pragma once
#include "gpu/scene/native_frustum.h"
#include "gpu/scene/native_transform.h"
namespace bd::gpu::scene {
struct ScheduledViewRay {
  std::array<float, 3> origin{}, direction{};
  float length = 0;
};
inline ScheduledViewRay BuildScheduledViewRay(std::array<float, 3> eye,
                                             std::array<float, 3> target,
                                             float zero_min, float zero_max) {
#pragma clang fp contract(off)
  ScheduledViewRay ray{eye};
  for (size_t i = 0; i < 3; ++i) ray.direction[i] = target[i] - eye[i];
  const auto [x, y, z] = ray.direction;
  ray.length = std::sqrt((x * x + y * y) + z * z);
  // The authored near-zero interval preserves the unnormalised delta. NaNs
  // likewise retain their IEEE values; don't invent a replacement camera.
  if (ray.length < zero_min || ray.length > zero_max) {
    const float reciprocal = 1.0f / ray.length;
    for (auto &value : ray.direction) value *= reciprocal;
  }
  return ray;
}
inline std::array<float, 4> TransformReflectionSphere(std::array<float, 4> sphere,
                                                     const RenderMatrix &world) {
#pragma clang fp contract(off)
  const auto [x, y, z, radius] = sphere;
  for (size_t c = 0; c < 3; ++c)
    sphere[c] = (z * world[8 + c] + world[12 + c]) + y * world[4 + c] + x * world[c];
  sphere[3] = radius; // the imported radius is already authored for this query
  return sphere;
}
inline bool ReflectionSphereVisible(const std::array<float, 4> &sphere,
                                    const RenderFrustum &frustum) {
#pragma clang fp contract(off)
  for (const auto &plane : frustum.planes) {
    const float distance = (sphere[0] * plane[0] + sphere[1] * plane[1]) +
                           (sphere[2] * plane[2] + plane[3]);
    if (distance > sphere[3]) return false;
  }
  return true;
}
inline std::array<float, 3> ScheduledFocusPoint(const ScheduledViewRay &ray,
                                              std::array<float, 3> target,
                                              float minimum_distance,
                                              float fallback_distance) {
#pragma clang fp contract(off)
  if (ray.length < minimum_distance)
    for (size_t c = 0; c < 3; ++c)
      target[c] = ray.direction[c] * fallback_distance + ray.origin[c];
  return target;
}
} // namespace bd::gpu::scene
