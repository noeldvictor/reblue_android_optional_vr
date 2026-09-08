// Same owned deformation as opaque skin; authored shadow UV and alpha-only PS.
#include "src/gpu/scene/native_skin_vertex.h"
struct ShadowFragment {
  float4 clip : SV_Position;
  float2 uv : TEXCOORD0;
};
ShadowFragment main([[vk::location(0)]] float4 p0 : POSITION0,
    [[vk::location(1)]] float4 p1 : POSITION1,
    [[vk::location(2)]] float4 p2 : POSITION2,
    [[vk::location(3)]] float4 joints : BLENDINDICES0,
    [[vk::location(4)]] float4 weights : BLENDWEIGHT0,
    [[vk::location(5)]] float4 uv : TEXCOORD0,
    uint instance : SV_InstanceID) {
  ShadowFragment result;
  result.clip = RigidTransform(NativeSkinWorld(p0.xyz,p1.xyz,p2.xyz,joints,weights,instance),
      rigid_instances[instance].pass_data.world_to_shadow);
  const float4 transform = rigid_instances[instance].object_data.uv_scale_offset;
  result.uv = uv.xy*transform.xy+transform.zw;
  return result;
}
