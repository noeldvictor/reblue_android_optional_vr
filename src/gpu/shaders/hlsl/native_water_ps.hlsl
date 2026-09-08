// Native water: four moving bump layers, planar/cube reflection, ordered scene
// snapshot refraction, shoreline depth, selected lights, sun shadow and authored fog.
#include "src/gpu/scene/native_water_shader.h"
[[vk::binding(0, 1)]] Texture2DArray<float4> bump_image : register(t0, space1);
[[vk::binding(1, 1)]] Texture2DArray<float4> planar_image : register(t1, space1);
[[vk::binding(2, 1)]] Texture2DArray<float4> snapshot_image : register(t2, space1);
[[vk::binding(3, 1)]] Texture2DArray<float4> bottom_image : register(t3, space1);
[[vk::binding(4, 1)]] TextureCube<float4> environment_image : register(t4, space1);
[[vk::binding(5, 1)]] Texture2DArray<float> sun_image : register(t5, space1);
[[vk::binding(0, 2)]] SamplerState bump_sampler : register(s0, space2);
[[vk::binding(1, 2)]] SamplerState planar_sampler : register(s1, space2);
[[vk::binding(2, 2)]] SamplerState snapshot_sampler : register(s2, space2);
[[vk::binding(3, 2)]] SamplerState bottom_sampler : register(s3, space2);
[[vk::binding(4, 2)]] SamplerState environment_sampler : register(s4, space2);
[[vk::binding(5, 2)]] SamplerComparisonState sun_sampler : register(s5, space2);

float WaterImageLayer(uint layers, uint eye) { return layers == 2 ? eye : 0; }
float WaterSun(float3 world, float3 normal, NativeRigidPassGPU pass_data) {
  const float4 projected = WaterTransform(float4(world,1), pass_data.world_to_shadow);
  if (projected.w <= 0) return 1;
  const float3 ndc = projected.xyz / projected.w;
  const float2 uv = ndc.xy*float2(.5,-.5)+.5;
  if (any(uv < 0) || any(uv > 1) || ndc.z < 0 || ndc.z > 1) return 1;
  const float reference = ndc.z - pass_data.shadow_filter.x - pass_data.shadow_filter.y *
      (1-saturate(dot(normal,-pass_data.lights[0].direction_cone.xyz)));
  float visibility = 0;
  [unroll] for (uint tap = 0; tap < 4; ++tap) {
    const float2 delta = float2((tap&1) ? 1 : -1, (tap&2) ? 1 : -1)*pass_data.shadow_filter.z;
    visibility += .25*sun_image.SampleCmpLevelZero(sun_sampler,float3(uv+delta,0),reference);
  }
  return visibility;
}
float4 main(WaterFragment fragment, uint eye : SV_ViewID) : SV_Target0 {
  const NativeWaterInstanceGPU data = water_instances[fragment.instance];
  const NativeWaterMaterialGPU material = data.object_data.material;
  const NativeRigidPassGPU pass_data = data.pass_data;
  const uint flags = material.modes.y;
  const float3 view = WaterNormalize(pass_data.cameras[eye].xyz-fragment.world);
  const float3 geometric_normal = WaterNormalize(fragment.normal);
  const float3 tangent = WaterNormalize(fragment.tangent);
  const float3 bitangent = WaterNormalize(cross(geometric_normal,tangent));
  const float reciprocal_depth = rcp(max(fragment.projected.w, 1e-8));
  const float2 screen_uv = fragment.projected.xy*reciprocal_depth*float2(.5,-.5)+.5;
  float depth_opacity = 1;
  if (flags & WaterShore) {
    const float4 bottom = WaterTransform(float4(fragment.world,1),data.world_to_bottom[eye]);
    if (bottom.w > 0) {
      const float3 ndc = bottom.xyz/bottom.w;
      const float2 uv = ndc.xy*float2(.5,-.5)+.5;
      const float depth = bottom_image.Sample(bottom_sampler,float3(uv,WaterImageLayer(data.image_layers.z,eye))).r;
      depth_opacity = saturate((depth-ndc.z)*material.shore.x);
    }
  }
  const float distance_fade = saturate(reciprocal_depth*material.surface.z);
  const float2 drift = material.scroll.xy*material.scroll.w;
  const float2 uv = fragment.uv*material.scroll.z;
  const float shallow_scale = pow(max(depth_opacity,1e-20),.01);
  float3 sampled = bump_image.Sample(bump_sampler,float3((uv+2*drift)*shallow_scale,0)).xyz;
  sampled += bump_image.Sample(bump_sampler,float3(2*uv+4*drift,0)).xyz;
  sampled += bump_image.Sample(bump_sampler,float3((4*uv+2*drift)*shallow_scale,0)).xyz;
  sampled += bump_image.Sample(bump_sampler,float3(8*uv+drift,0)).xyz;
  const float3 bump = WaterNormalize(sampled*2-4);
  const float3 perturbed = WaterNormalize(tangent*bump.x + bitangent*bump.y + geometric_normal*bump.z);
  const float3 normal = WaterNormalize(lerp(geometric_normal,perturbed,1+material.surface.w-depth_opacity));
  const float facing = saturate(dot(WaterNormalize(tangent*(bump.x*material.surface.x) +
      bitangent*(bump.y*material.surface.x) + geometric_normal*bump.z),view));
  const float fresnel = .2+.8*pow(1-facing,3.5);
  const float4 tint = material.tint*float4(1,1,1,fragment.opacity)*(1-distance_fade*facing);
  float3 reflection = 0;
  if (material.modes.x == WaterReflectionPlanar)
    reflection = planar_image.Sample(planar_sampler,float3(screen_uv+bump.xy*material.surface.x,
        WaterImageLayer(data.image_layers.x,eye))).rgb;
  else if (material.modes.x == WaterReflectionEnvironment)
    reflection = environment_image.Sample(environment_sampler,reflect(-view,normal)).rgb;
  float3 colour = .9*(tint.rgb+fresnel*reflection);
  float opacity = tint.a*depth_opacity;
  float3 snapshot = 0;
  if (flags & WaterRefraction) {
    snapshot = snapshot_image.Sample(snapshot_sampler,float3(screen_uv-
        bump.xy*material.surface.y*distance_fade*depth_opacity,WaterImageLayer(data.image_layers.y,eye))).rgb;
    colour = lerp(snapshot,colour,tint.a);
    opacity = 1; // snapshot already includes the background; do not blend it twice
  }
  const LitVector position = WaterVector(fragment.world), camera = WaterVector(pass_data.cameras[eye].xyz);
  const LitVector n = WaterVector(normal), v = WaterVector(view);
  float3 diffuse = 0, specular = 0, primary = 0;
  const float visibility = flags & WaterShadow ? WaterSun(fragment.world,normal,pass_data) : 1;
  [unroll] for (uint i = 0; i < 3; ++i) {
    const LitLight light = WaterLight(pass_data.lights[i]);
    const LitResponse response = EvaluateLitLight(light,position,n,v,material.highlight.x);
    const float3 rgb = WaterRGB(light.colour);
    diffuse += response.diffuse*rgb;
    specular += response.specular*rgb*(i == 0 ? visibility : 1);
    if (i == 0) primary = response.diffuse*rgb;
  }
  if (flags & WaterDiffuse)
    colour *= .5*diffuse+pass_data.ambient.xyz-primary*(1-visibility)*pass_data.shadow_colour_strength.xyz;
  colour += 2*material.highlight.y*specular*float3(1.05,.97,1.27);
  opacity += dot(specular,float3(.223,.693,.091566));
  if (flags & WaterShore) {
    const float shallow = 1-depth_opacity;
    const float3 transmitted = lerp(tint.rgb,1,shallow*shallow*shallow)*snapshot;
    const float3 foam = (colour+shallow*shallow*material.highlight.z)*
        (1+pow(shallow,1.5)*material.highlight.w);
    colour = lerp(transmitted,foam,depth_opacity);
  }
  LitVector fogged = WaterVector(colour);
  if (flags & WaterFog) {
    fogged = ApplyLitFog(fogged,position,camera,WaterFogValue(pass_data.fog[0]));
    fogged = ApplyLitFog(fogged,position,camera,WaterFogValue(pass_data.fog[1]));
  }
  colour = (WaterRGB(fogged)+pass_data.colour_grade.xyz)*pass_data.colour_grade.w;
  if (flags & WaterCel) {
    const float luminance = dot(max(colour,0),float3(.2126,.7152,.0722));
    if (luminance > 1e-6) colour *= (floor(luminance*4)+.5)/(4*luminance);
  }
  return float4(colour,opacity);
}
