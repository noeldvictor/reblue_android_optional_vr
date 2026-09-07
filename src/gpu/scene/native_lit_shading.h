/**
 * @brief Named lighting/fog arithmetic shared by native shaders and CPU tests.
 * @copyright Copyright (c) 2026 reblue contributors
 * @license BSD 3-Clause, see LICENSE
 */
#pragma once
#ifdef __cplusplus
#include <algorithm>
#include <cmath>
namespace bd::gpu::scene {
using std::abs;
using std::exp2;
using std::log2;
using std::sqrt;
#endif

struct LitVector { float x, y, z; };
inline LitVector LitVec(float x, float y, float z) {
  LitVector result; result.x = x; result.y = y; result.z = z; return result;
}
inline LitVector LitAdd(LitVector a, LitVector b) { return LitVec(a.x+b.x, a.y+b.y, a.z+b.z); }
inline LitVector LitSubtract(LitVector a, LitVector b) { return LitVec(a.x-b.x, a.y-b.y, a.z-b.z); }
inline LitVector LitScale(LitVector a, float b) { return LitVec(a.x*b, a.y*b, a.z*b); }
inline LitVector LitMultiply(LitVector a, LitVector b) { return LitVec(a.x*b.x, a.y*b.y, a.z*b.z); }
inline float LitDot(LitVector a, LitVector b) { return (a.x*b.x + a.y*b.y) + a.z*b.z; }
inline float LitClamp(float x, float lo, float hi) {
#ifdef __cplusplus
  return std::clamp(x, lo, hi);
#else
  return clamp(x, lo, hi);
#endif
}
inline float LitSaturate(float x) { return LitClamp(x, 0.0f, 1.0f); }
inline float LitFinite(float x) { return LitClamp(x, -3.402823466e+38f, 3.402823466e+38f); }
inline float LitReciprocal(float x) { return LitFinite(1.0f / x); }
inline LitVector LitNormalize(LitVector v) {
  return LitScale(v, LitFinite(1.0f / sqrt(abs(LitDot(v, v)))));
}
inline float LitShininess(float cosine, float exponent) {
  // Retain black^0 and negative-exponent behavior without a register machine.
  const float positive = cosine > 0.0f ? cosine : 0.0f;
  return exp2(exponent * LitFinite(log2(positive)));
}

// Named light kinds, independent of the original position.w encoding.
static const int LitDisabled = 0, LitDirectional = 1, LitSpot = 2, LitPoint = 3;
struct LitLight {
  LitVector position, direction, colour;
  float inverse_range, cone_cosine, cone_strength;
  int kind;
};
struct LitResponse { float diffuse, specular; };
inline LitResponse EvaluateLitLight(LitLight light, LitVector position,
                                    LitVector normal, LitVector view, float shininess) {
  LitResponse result; result.diffuse = 0; result.specular = 0;
  if (light.kind == LitDisabled) return result;
  LitVector direction;
  float attenuation = 1;
  if (light.kind == LitDirectional) {
    // Authored directional vectors are already prepared; do not normalize twice.
    direction = LitScale(light.direction, -1);
  } else {
    const LitVector delta = LitSubtract(light.position, position);
    const float distance = sqrt(abs(LitDot(delta, delta)));
    direction = LitNormalize(delta);
    attenuation = 1 - LitSaturate(light.inverse_range * distance);
    if (light.kind == LitSpot) {
      const float cone = LitSaturate(LitDot(LitScale(direction, -1), light.direction) - light.cone_cosine);
      attenuation *= LitSaturate((light.cone_strength * LitReciprocal(1 - light.cone_cosine)) * cone);
    }
  }
  const LitVector halfway = LitNormalize(LitAdd(view, direction));
  const float facing = LitDot(normal, direction);
  result.diffuse = (facing > 0 ? facing : 0) * attenuation;
  result.specular = LitShininess(LitDot(halfway, normal), shininess) * attenuation;
  return result;
}

struct LitSurface {
  LitVector albedo, specular, ambient, shadow_colour;
  float shadow_strength, shadow_visibility;
  bool diffuse_enabled, specular_enabled;
};
inline LitVector ComposeLitSurface(LitSurface surface,
    LitLight light0, LitLight light1, LitLight light2,
    LitResponse response0, LitResponse response1, LitResponse response2) {
  LitVector colour = surface.albedo;
  if (surface.diffuse_enabled) {
    const LitVector primary = LitScale(light0.colour, response0.diffuse);
    const float shade = 1 - surface.shadow_visibility;
    // Authored shadow subtraction is coloured and depends on the primary light;
    // it is not ordinary multiplication by a grey visibility value.
    const LitVector subtraction = LitMultiply(surface.shadow_colour,
        LitAdd(LitScale(LitSubtract(LitScale(primary, shade), LitVec(shade,shade,shade)),
                        surface.shadow_strength), LitVec(shade,shade,shade)));
    LitVector lighting = LitAdd(primary, surface.ambient);
    lighting = LitAdd(LitScale(light1.colour, response1.diffuse), lighting);
    lighting = LitAdd(LitScale(light2.colour, response2.diffuse), lighting);
    colour = LitMultiply(colour, LitSubtract(lighting, subtraction));
  }
  if (surface.specular_enabled) {
    LitVector highlight = LitAdd(LitScale(light0.colour, response0.specular * surface.shadow_visibility),
                                 LitScale(light1.colour, response1.specular));
    highlight = LitAdd(LitScale(light2.colour, response2.specular), highlight);
    highlight = LitMultiply(highlight, LitVec(1.05f, .97f, 1.27f));
    colour = LitAdd(LitMultiply(highlight, surface.specular), colour);
  }
  return colour;
}

static const int LitFogBlend = 0, LitFogAdd = 1, LitFogSubtract = 2;
struct LitFog {
  LitVector origin, direction, colour;
  float start, end, opacity;
  bool disabled, radial;
  int blend;
};
inline LitVector ApplyLitFog(LitVector colour, LitVector position, LitVector camera, LitFog fog) {
  float amount = 0;
  if (!fog.disabled) {
    LitVector origin = fog.origin;
    if (fog.radial) origin = camera;
    const LitVector relative = LitSubtract(position, origin);
    const float distance = fog.radial ? sqrt(abs(LitDot(relative, relative))) : LitDot(relative, fog.direction);
    amount = LitSaturate((distance - fog.start) * LitReciprocal(fog.end - fog.start));
  }
  const LitVector fog_colour = LitScale(fog.colour, amount);
  const float opacity = amount * fog.opacity;
  // Colour AND opacity contain distance falloff. Preserve that authored curve.
  if (fog.blend == LitFogBlend)
    return LitAdd(LitScale(LitSubtract(fog_colour, colour), opacity), colour);
  return LitAdd(colour, LitScale(fog_colour, fog.blend == LitFogAdd ? opacity : -opacity));
}

#ifdef __cplusplus
} // namespace bd::gpu::scene
#endif
