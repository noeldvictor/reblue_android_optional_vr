// Native single-albedo rigid family. Detail/normal maps, reflections, wind, skin
// and alpha-tested/translucent recipes require their own explicit eligibility.
#include "src/gpu/scene/native_rigid_shader.h"
[[vk::binding(0, 1)]] Texture2D<float4> albedo_image : register(t0, space1);
[[vk::binding(1, 1)]] Texture2D<float> shadow_image : register(t1, space1);
[[vk::binding(0, 2)]] SamplerState albedo_sampler : register(s0, space2);
[[vk::binding(1, 2)]] SamplerComparisonState shadow_sampler : register(s1, space2);

float RigidShadow(float3 world, float3 normal) {
  if (!(object_data.flags.x & RigidReceiveShadow)) return 1;
  const float4 shadow = RigidTransform(float4(world, 1), pass_data.world_to_shadow);
  if (shadow.w <= 0) return 1;
  const float3 projected = shadow.xyz / shadow.w;
  const float2 uv = projected.xy * float2(.5, -.5) + .5;
  if (any(uv < 0) || any(uv > 1) || projected.z < 0 || projected.z > 1) return 1;
  const float slope = 1 - saturate(dot(normal, -pass_data.lights[0].direction_cone.xyz));
  const float reference = projected.z - pass_data.shadow_filter.x - slope * pass_data.shadow_filter.y;
  float visibility = 0;
  [unroll] for (uint tap = 0; tap < 4; ++tap) {
    const float2 offset = float2((tap & 1) ? 1 : -1, (tap & 2) ? 1 : -1) * pass_data.shadow_filter.z;
    visibility += .25 * shadow_image.SampleCmpLevelZero(shadow_sampler, uv + offset, reference);
  }
  return visibility;
}

float4 main(RigidFragment fragment, uint eye : SV_ViewID) : SV_Target0 {
  const uint flags = object_data.flags.x;
  float4 albedo = object_data.diffuse * fragment.colour;
  if (flags & RigidAlbedo) albedo *= albedo_image.Sample(albedo_sampler, fragment.uv);
  const float3 normal = normalize(fragment.normal);
  const LitVector position = RigidVector(fragment.world);
  const LitVector camera = RigidVector(pass_data.cameras[eye].xyz);
  const LitVector view = LitNormalize(LitSubtract(camera, position));
  const LitLight light0 = RigidLight(pass_data.lights[0]);
  const LitLight light1 = RigidLight(pass_data.lights[1]);
  const LitLight light2 = RigidLight(pass_data.lights[2]);
  const LitVector n = RigidVector(normal);
  const LitResponse a = EvaluateLitLight(light0, position, n, view, object_data.specular.w);
  const LitResponse b = EvaluateLitLight(light1, position, n, view, object_data.specular.w);
  const LitResponse c = EvaluateLitLight(light2, position, n, view, object_data.specular.w);
  LitSurface surface;
  surface.albedo = RigidVector(albedo.rgb); surface.specular = RigidVector(object_data.specular.rgb);
  surface.ambient = RigidVector(pass_data.ambient.rgb);
  surface.shadow_colour = RigidVector(pass_data.shadow_colour_strength.rgb);
  surface.shadow_strength = pass_data.shadow_colour_strength.w;
  surface.shadow_visibility = RigidShadow(fragment.world, normal);
  surface.diffuse_enabled = (flags & RigidDiffuse) != 0;
  surface.specular_enabled = (flags & RigidSpecular) != 0;
  LitVector colour = ComposeLitSurface(surface, light0, light1, light2, a, b, c);
  if (flags & RigidFogEnabled) {
    colour = ApplyLitFog(colour, position, camera, RigidFog(pass_data.fog[0]));
    colour = ApplyLitFog(colour, position, camera, RigidFog(pass_data.fog[1]));
  }
  return float4((float3(colour.x, colour.y, colour.z) + pass_data.colour_grade.xyz) *
                pass_data.colour_grade.w, albedo.a);
}
