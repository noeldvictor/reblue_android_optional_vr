// Native joint-local skin; ordinary opaque caster in the shared shadow queue.
// No console register array, index fractions, per-draw endian decode or world reapplication.
#include "src/gpu/scene/native_skin_vertex.h"
float4 main([[vk::location(0)]] float4 p0 : POSITION0,
    [[vk::location(1)]] float4 p1 : POSITION1,
    [[vk::location(2)]] float4 p2 : POSITION2,
    [[vk::location(3)]] float4 joints : BLENDINDICES0,
    [[vk::location(4)]] float4 weights : BLENDWEIGHT0,
    uint instance : SV_InstanceID) : SV_Position {
  const float4 world = NativeSkinWorld(p0.xyz,p1.xyz,p2.xyz,joints,weights,instance);
  return RigidTransform(world,rigid_instances[instance].pass_data.world_to_shadow);
}
