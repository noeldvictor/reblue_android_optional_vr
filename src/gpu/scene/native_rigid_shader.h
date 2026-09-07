// Production native rigid shader interface. No translated shader common header.
#include "src/gpu/scene/native_rigid_inputs.h"
#include "src/gpu/scene/native_lit_shading.h"
[[vk::binding(0, 0)]] StructuredBuffer<NativeRigidInstanceGPU> rigid_instances : register(t0, space0);
struct RigidVertex {
  [[vk::location(0)]] float4 position : POSITION;
  [[vk::location(1)]] float4 normal : NORMAL;
  [[vk::location(2)]] float4 uv : TEXCOORD0;
  [[vk::location(3)]] float4 colour : COLOR0;
#ifdef RIGID_LAYERED_INPUT
  [[vk::location(4)]] float4 secondary_uv : TEXCOORD2;
#endif
};
struct RigidFragment {
  float4 clip : SV_Position;
  float3 world : TEXCOORD0;
  float3 normal : TEXCOORD1;
  float4 uv : TEXCOORD2;
  float2 secondary_uv : TEXCOORD4;
  float4 colour : COLOR0;
  nointerpolation uint instance : TEXCOORD3;
};
float4 RigidTransform(float4 value, RigidMatrix matrix) {
  return (value.x * matrix.rows[0] + value.y * matrix.rows[1]) +
         (value.z * matrix.rows[2] + value.w * matrix.rows[3]);
}
LitVector RigidVector(float3 value) { return LitVec(value.x, value.y, value.z); }
LitLight RigidLight(RigidLightGPU packed) {
  LitLight light;
  light.position = RigidVector(packed.position_range.xyz);
  light.inverse_range = packed.position_range.w;
  light.direction = RigidVector(packed.direction_cone.xyz);
  light.cone_cosine = packed.direction_cone.w;
  light.colour = RigidVector(packed.colour_strength.xyz);
  light.cone_strength = packed.colour_strength.w;
  light.kind = int(packed.kind.x);
  return light;
}
LitFog RigidFog(RigidFogGPU packed) {
  LitFog fog;
  fog.origin = RigidVector(packed.origin_start.xyz); fog.start = packed.origin_start.w;
  fog.direction = RigidVector(packed.direction_end.xyz); fog.end = packed.direction_end.w;
  fog.colour = RigidVector(packed.colour_opacity.xyz); fog.opacity = packed.colour_opacity.w;
  fog.disabled = packed.mode.x != 0; fog.radial = packed.mode.y != 0; fog.blend = int(packed.mode.z);
  return fog;
}
