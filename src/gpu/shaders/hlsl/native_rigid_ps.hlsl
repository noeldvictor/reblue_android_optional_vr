// Native ordinary/Toon zero-to-three-layer surfaces share rigid and skin inputs.
// Normal maps, reflections, wind and specialized passes need explicit owners.
#include "src/gpu/scene/native_rigid_shader.h"
#include "src/gpu/scene/native_toon_shading.h"
// The native texture uploader and native target owner publish array views even
// for mono images. Both eyes sample layer zero of this ordinary material and
// the shared sun shadow; SV_ViewID selects cameras, not these image layers.
[[vk::binding(0, 1)]] Texture2DArray<float4> albedo_image : register(t0, space1);
[[vk::binding(1, 1)]] Texture2DArray<float4> detail1_image : register(t1, space1);
[[vk::binding(2, 1)]] Texture2DArray<float4> detail2_image : register(t2, space1);
[[vk::binding(3, 1)]] Texture2DArray<float> shadow_image : register(t3, space1);
[[vk::binding(0, 2)]] SamplerState albedo_sampler : register(s0, space2);
[[vk::binding(1, 2)]] SamplerState detail1_sampler : register(s1, space2);
[[vk::binding(2, 2)]] SamplerState detail2_sampler : register(s2, space2);
[[vk::binding(3, 2)]] SamplerComparisonState shadow_sampler : register(s3, space2);

float RigidShadow(float3 world, float3 normal, NativeRigidObjectGPU object_data, NativeRigidPassGPU pass_data) {
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
    visibility += .25 * shadow_image.SampleCmpLevelZero(shadow_sampler, float3(uv + offset, 0), reference);
  }
  return visibility;
}

float4 main(RigidFragment fragment, uint eye : SV_ViewID) : SV_Target0 {
  const NativeRigidObjectGPU object_data = rigid_instances[fragment.instance].object_data;
  const NativeRigidPassGPU pass_data = rigid_instances[fragment.instance].pass_data;
  const uint flags = object_data.flags.x;
  const bool toon = (flags & RigidToon) != 0;
  float4 texture_colour = 1;
  if (object_data.flags.y > 0) {
    float4 base = albedo_image.Sample(albedo_sampler, float3(fragment.uv.xy, 0));
    if (toon) base *= object_data.texture_colours[0];
    // Negative U is an authored absent-layer sentinel, independent of sampler
    // addressing. Reflective families have a different fallback and are refused.
    texture_colour = fragment.uv.x < 0 ? 0 : base;
    if (object_data.flags.y > 1) {
      float4 detail = detail1_image.Sample(detail1_sampler, float3(fragment.uv.zw, 0));
      if (toon) detail *= object_data.texture_colours[1];
      if (fragment.uv.z < 0) detail = 0;
      texture_colour.rgb = lerp(texture_colour.rgb, detail.rgb, detail.a);
    }
    if (object_data.flags.y > 2) {
      float4 detail = detail2_image.Sample(detail2_sampler, float3(fragment.secondary_uv, 0));
      if (toon) detail *= object_data.texture_colours[2];
      if (fragment.secondary_uv.x < 0) detail = 0;
      texture_colour.rgb = lerp(texture_colour.rgb, detail.rgb, detail.a);
    }
  }
  if (toon && (flags & RigidIgnoreTextureAlpha)) texture_colour.a = 1;
  const float4 albedo = texture_colour * object_data.diffuse * fragment.colour;
  if ((flags & RigidCutout) && !RigidCutoutPasses(object_data.flags.z, albedo.a, asfloat(object_data.flags.w))) discard;
  const float3 normal = normalize(fragment.normal);
  const LitVector position = RigidVector(fragment.world);
  const LitVector camera = RigidVector(pass_data.cameras[eye].xyz);
  const LitVector view = LitNormalize(LitSubtract(camera, position));
  LitLight light0 = RigidLight(pass_data.lights[0]);
  LitLight light1 = RigidLight(pass_data.lights[1]);
  LitLight light2 = RigidLight(pass_data.lights[2]);
  ToonLightAdjustment adjustment;
  adjustment.diffuse_scale = RigidVector(object_data.toon_diffuse_scale.xyz);
  adjustment.diffuse_add = RigidVector(object_data.toon_diffuse_add.xyz);
  adjustment.ambient_scale = RigidVector(object_data.toon_ambient_scale.xyz);
  adjustment.ambient_add = RigidVector(object_data.toon_ambient_add.xyz);
  if (toon) {
    light0 = AdjustToonLight(light0, adjustment);
    light1 = AdjustToonLight(light1, adjustment);
    light2 = AdjustToonLight(light2, adjustment);
  }
  const LitVector n = RigidVector(normal);
  const float shininess = toon ? max(10.f,object_data.specular.w) : object_data.specular.w;
  const LitResponse a = EvaluateLitLight(light0, position, n, view, shininess);
  const LitResponse b = EvaluateLitLight(light1, position, n, view, shininess);
  const LitResponse c = EvaluateLitLight(light2, position, n, view, shininess);
  LitSurface surface;
  surface.albedo = RigidVector(albedo.rgb); surface.specular = RigidVector(object_data.specular.rgb);
  surface.ambient = RigidVector(pass_data.ambient.rgb);
  if (toon) surface.ambient = AdjustToonAmbient(surface.ambient, adjustment);
  surface.shadow_colour = RigidVector(pass_data.shadow_colour_strength.rgb);
  surface.shadow_strength = pass_data.shadow_colour_strength.w;
  surface.shadow_visibility = RigidShadow(fragment.world, normal, object_data, pass_data);
  surface.diffuse_enabled = (flags & RigidDiffuse) != 0;
  surface.specular_enabled = (flags & RigidSpecular) != 0;
  LitVector colour;
  if (toon) colour = ComposeToonSurface(surface, light0, light1, light2, a, b, c);
  else colour = ComposeLitSurface(surface, light0, light1, light2, a, b, c);
  if (flags & RigidFogEnabled) {
    colour = ApplyLitFog(colour, position, camera, RigidFog(pass_data.fog[0]));
    colour = ApplyLitFog(colour, position, camera, RigidFog(pass_data.fog[1]));
  }
  return float4((float3(colour.x, colour.y, colour.z) + pass_data.colour_grade.xyz) *
                pass_data.colour_grade.w, albedo.a);
}
