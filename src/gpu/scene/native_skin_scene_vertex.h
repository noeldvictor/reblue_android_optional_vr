// Joint-local normals use the palette's linear transform, not an object inverse
// transpose. Positions already become world-space; never apply object.world again.
#include "src/gpu/scene/native_skin_vertex.h"
struct NativeSkinSceneVertex {
  [[vk::location(0)]] float4 p0 : POSITION0;
  [[vk::location(1)]] float4 p1 : POSITION1;
  [[vk::location(2)]] float4 p2 : POSITION2;
  [[vk::location(3)]] float4 n0 : NORMAL0;
  [[vk::location(4)]] float4 n1 : NORMAL1;
  [[vk::location(5)]] float4 n2 : NORMAL2;
  [[vk::location(6)]] float4 joints : BLENDINDICES0;
  [[vk::location(7)]] float4 weights : BLENDWEIGHT0;
  [[vk::location(8)]] float4 uv : TEXCOORD0;
  [[vk::location(9)]] float4 colour : COLOR0;
#ifdef RIGID_LAYERED_INPUT
  [[vk::location(10)]] float4 secondary_uv : TEXCOORD2;
#endif
};
RigidFragment main(NativeSkinSceneVertex vertex, uint eye : SV_ViewID, uint instance : SV_InstanceID) {
  const NativeRigidObjectGPU object_data = rigid_instances[instance].object_data;
  const NativeRigidPassGPU pass_data = rigid_instances[instance].pass_data;
  RigidFragment result;
  result.instance = instance;
  const float4 world = NativeSkinWorld(vertex.p0.xyz,vertex.p1.xyz,vertex.p2.xyz,vertex.joints,vertex.weights,instance);
  result.clip = RigidTransform(world,pass_data.world_to_clip[eye]);
  result.world = world.xyz;
  result.normal = NativeSkinNormal(vertex.n0.xyz,vertex.n1.xyz,vertex.n2.xyz,vertex.joints,vertex.weights,instance);
  result.uv.xy = vertex.uv.xy*object_data.uv_scale_offset.xy + object_data.uv_scale_offset.zw;
  result.uv.zw = vertex.uv.zw*object_data.detail_uv_scale_offset[0].xy + object_data.detail_uv_scale_offset[0].zw;
#ifdef RIGID_LAYERED_INPUT
  result.secondary_uv = vertex.secondary_uv.xy*object_data.detail_uv_scale_offset[1].xy + object_data.detail_uv_scale_offset[1].zw;
#else
  result.secondary_uv = 0;
#endif
  result.colour = (object_data.flags.x & RigidVertexColour) ? vertex.colour : float4(1,1,1,1);
  return result;
}
