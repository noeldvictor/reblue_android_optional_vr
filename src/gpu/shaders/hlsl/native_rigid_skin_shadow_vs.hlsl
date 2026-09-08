// Native joint-local skin; ordinary opaque caster in the shared shadow queue.
// No console register array, index fractions, per-draw endian decode or world reapplication.
#include "src/gpu/scene/native_rigid_shader.h"
[[vk::binding(1, 0)]] StructuredBuffer<RigidMatrix> skin_joints : register(t1, space0);
[[vk::binding(2, 0)]] StructuredBuffer<uint4> skin_ranges : register(t2, space0);
float4 main([[vk::location(0)]] float4 p0 : POSITION0,
    [[vk::location(1)]] float4 p1 : POSITION1,
    [[vk::location(2)]] float4 p2 : POSITION2,
    [[vk::location(3)]] float4 joints : BLENDINDICES0,
    [[vk::location(4)]] float4 weights : BLENDWEIGHT0,
    uint instance : SV_InstanceID) : SV_Position {
  const uint4 range = skin_ranges[instance];
  const float3 positions[3] = {p0.xyz,p1.xyz,p2.xyz};
  float4 world = 0;
  [unroll] for (uint n = 0; n < 3; ++n) {
    const uint joint = uint(joints[n]);
    if (weights[n] > 0 && joint < range.y)
      world += RigidTransform(float4(positions[n],1),skin_joints[range.x+joint])*weights[n];
  }
  return RigidTransform(world,rigid_instances[instance].pass_data.world_to_shadow);
}
