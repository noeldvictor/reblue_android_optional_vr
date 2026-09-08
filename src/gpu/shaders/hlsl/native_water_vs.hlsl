// Authored animation drives a native directional wave basis. The console's
// angular polynomial approximation is deliberately not the native deformation API.
#include "src/gpu/scene/native_water_shader.h"
WaterFragment main(WaterVertex vertex, uint eye : SV_ViewID, uint instance : SV_InstanceID) {
  const NativeWaterInstanceGPU data = water_instances[instance];
  const NativeWaterObjectGPU object_data = data.object_data;
  const NativeWaterMaterialGPU material = object_data.material;
  float3 world = WaterTransform(vertex.position, object_data.world).xyz;
  float3 normal = WaterNormalize(vertex.normal.x*object_data.normal_rows[0].xyz +
      vertex.normal.y*object_data.normal_rows[1].xyz + vertex.normal.z*object_data.normal_rows[2].xyz);
  float3 tangent = WaterNormalize(WaterTransform(float4(vertex.tangent.xyz,0), object_data.world).xyz);
  if (material.waves.x != 0) {
    const float2 direction = float2(cos(material.waves.z), sin(material.waves.z));
    const float2 cross_direction = float2(-direction.y, direction.x);
    const float time = material.scroll.w * material.waves.y;
    const float a = dot(world.xz, direction) * material.waves.w + time * 20;
    const float b = dot(world.xz, cross_direction) * material.waves.w * 1.93 - time * 8;
    const float amplitude = material.waves.x * vertex.colour.r;
    world.y += amplitude * (sin(a) + .5 * sin(b));
    const float2 slope = amplitude * material.waves.w *
        (cos(a)*direction + .965*cos(b)*cross_direction);
    normal = WaterNormalize(normal + float3(-slope.x,0,-slope.y));
  }
  // Reorthogonalize the tangent after nonuniform transform and displacement.
  tangent = WaterNormalize(tangent - normal*dot(tangent,normal));
  WaterFragment result;
  result.clip = WaterTransform(float4(world,1), data.pass_data.world_to_clip[eye]);
  result.projected = result.clip; // per-eye projected UV, not a post-VS stereo skew
  result.world = world; result.normal = normal; result.tangent = tangent;
  result.uv = vertex.uv.xy; result.opacity = vertex.colour.a; result.instance = instance;
  return result;
}
