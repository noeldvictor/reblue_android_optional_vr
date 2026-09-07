// Alpha-only caster inputs: no normals, lighting, receiver or translated state.
#include "src/gpu/scene/native_rigid_shader.h"
struct ShadowFragment {
  float4 clip : SV_Position;
  float2 uv : TEXCOORD0;
  float alpha : TEXCOORD1;
  nointerpolation uint instance : TEXCOORD2;
};
ShadowFragment main([[vk::location(0)]] float4 position : POSITION,
    [[vk::location(2)]] float4 uv : TEXCOORD0,
    [[vk::location(3)]] float4 colour : COLOR0, uint instance : SV_InstanceID) {
  const NativeRigidObjectGPU object_data = rigid_instances[instance].object_data;
  ShadowFragment result;
  result.clip = RigidTransform(RigidTransform(float4(position.xyz, 1), object_data.world),
      rigid_instances[instance].pass_data.world_to_shadow);
  result.uv = uv.xy * object_data.uv_scale_offset.xy + object_data.uv_scale_offset.zw;
  result.alpha = (object_data.flags.x & RigidVertexColour) ? colour.a : 1;
  result.instance = instance;
  return result;
}
