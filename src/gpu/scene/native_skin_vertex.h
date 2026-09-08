// Native joint-local deformation shared by scene and shadow vertices.
#include "src/gpu/scene/native_rigid_shader.h"
[[vk::binding(1, 0)]] StructuredBuffer<RigidMatrix> skin_joints : register(t1, space0);
[[vk::binding(2, 0)]] StructuredBuffer<uint4> skin_ranges : register(t2, space0);
float4 NativeSkinWorld(float3 p0, float3 p1, float3 p2, float4 joints, float4 weights, uint instance) {
  const uint4 range = skin_ranges[instance];
  const float3 positions[3] = {p0,p1,p2};
  float4 world = 0;
  [unroll] for (uint n = 0; n < 3; ++n) {
    const uint joint = uint(joints[n]);
    if (weights[n] > 0 && joint < range.y)
      world += RigidTransform(float4(positions[n],1),skin_joints[range.x+joint])*weights[n];
  }
  return world;
}
float3 NativeSkinNormal(float3 n0, float3 n1, float3 n2, float4 joints, float4 weights, uint instance) {
  const uint4 range = skin_ranges[instance];
  const float3 normals[3] = {n0,n1,n2};
  float3 normal = 0;
  [unroll] for (uint n = 0; n < 3; ++n) {
    const uint joint = uint(joints[n]);
    if (weights[n] > 0 && joint < range.y)
      normal += RigidTransform(float4(normals[n],0),skin_joints[range.x+joint]).xyz*weights[n];
  }
  return normalize(normal);
}
