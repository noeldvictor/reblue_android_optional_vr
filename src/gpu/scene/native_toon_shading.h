/**
 * @brief Authored Toon surface response, independent of console shader storage.
 * @copyright Copyright (c) 2026 reblue contributors
 * @license BSD 3-Clause, see LICENSE
 */
#pragma once
#include "native_lit_shading.h"
#ifdef __cplusplus
namespace bd::gpu::scene {
#endif
struct ToonLightAdjustment {
  LitVector diffuse_scale, diffuse_add, ambient_scale, ambient_add;
};
inline LitLight AdjustToonLight(LitLight light, ToonLightAdjustment adjustment) {
  // Colour adjustment does not change distance/cone attenuation or light kind.
  light.colour = LitAdd(LitMultiply(light.colour, adjustment.diffuse_scale), adjustment.diffuse_add);
  return light;
}
inline LitVector AdjustToonAmbient(LitVector ambient, ToonLightAdjustment adjustment) {
  const LitVector boosted = LitVec(LitSaturate(ambient.x * 1.1f),
      LitSaturate(ambient.y * 1.1f), LitSaturate(ambient.z * 1.1f));
  return LitAdd(LitMultiply(boosted, adjustment.ambient_scale), adjustment.ambient_add);
}
inline float ToonDiffuseChannel(float albedo, float ambient, float primary, float lighting,
                                float shadow_colour, float visibility) {
  // bd_toon_ps and bd_toon_cs_ps share this response. It is not cel banding,
  // gamma-correct ordinary diffuse or the ordinary shader's shadow-strength rule.
  const float mixed = ambient + .65f * (.9f * (lighting - ambient) + ambient) -
      .65f * (.9f * (primary - ambient) + ambient) * (1 - visibility) * shadow_colour;
  const float response = exp2(.9f * LitFinite(log2(abs(mixed))));
  const float dark = albedo * (1 - response);
  return (dark * dark) * response + albedo * response;
}
inline LitVector ComposeToonSurface(LitSurface surface,
    LitLight light0, LitLight light1, LitLight light2,
    LitResponse response0, LitResponse response1, LitResponse response2) {
  LitVector colour = surface.albedo;
  if (surface.diffuse_enabled) {
    const LitVector primary = LitScale(light0.colour, response0.diffuse);
    const LitVector lighting = LitAdd(LitAdd(primary, LitScale(light1.colour, response1.diffuse)),
                                    LitScale(light2.colour, response2.diffuse));
    colour = LitVec(
        ToonDiffuseChannel(colour.x, surface.ambient.x, primary.x, lighting.x, surface.shadow_colour.x, surface.shadow_visibility),
        ToonDiffuseChannel(colour.y, surface.ambient.y, primary.y, lighting.y, surface.shadow_colour.y, surface.shadow_visibility),
        ToonDiffuseChannel(colour.z, surface.ambient.z, primary.z, lighting.z, surface.shadow_colour.z, surface.shadow_visibility));
  }
  // The authored Toon PS always adds material specular, regardless of the
  // ordinary family's specular-enable bit. Its exponent is at least ten.
  LitVector highlight = LitAdd(LitScale(light0.colour, response0.specular * surface.shadow_visibility),
                              LitScale(light1.colour, response1.specular));
  highlight = LitAdd(highlight, LitScale(light2.colour, response2.specular));
  highlight = LitMultiply(highlight, LitVec(1.05f, .97f, 1.27f));
  return LitAdd(colour, LitMultiply(highlight, surface.specular));
}
#ifdef __cplusplus
} // namespace bd::gpu::scene
#endif
