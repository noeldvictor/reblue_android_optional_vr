// Native water interface. No translated common header, shader IDs or register file.
#include "src/gpu/scene/native_water_inputs.h"
#include "src/gpu/scene/native_lit_shading.h"
[[vk::binding(0, 0)]] StructuredBuffer<NativeWaterInstanceGPU> water_instances : register(t0, space0);
struct WaterVertex {
  [[vk::location(0)]] float4 position : POSITION;
  [[vk::location(1)]] float4 normal : NORMAL;
  [[vk::location(2)]] float4 uv : TEXCOORD0;
  [[vk::location(3)]] float4 colour : COLOR0;
  [[vk::location(4)]] float4 tangent : TANGENT;
};
struct WaterFragment {
  float4 clip : SV_Position;
  float3 world : TEXCOORD0;
  float3 normal : TEXCOORD1;
  float3 tangent : TEXCOORD2;
  float2 uv : TEXCOORD3;
  float4 projected : TEXCOORD4;
  float opacity : TEXCOORD5;
  nointerpolation uint instance : TEXCOORD6;
};
float4 WaterTransform(float4 value, RigidMatrix matrix) {
  return (value.x * matrix.rows[0] + value.y * matrix.rows[1]) +
         (value.z * matrix.rows[2] + value.w * matrix.rows[3]);
}
float3 WaterNormalize(float3 value) { return value * rsqrt(max(dot(value,value), 1e-20)); }
LitVector WaterVector(float3 value) { return LitVec(value.x, value.y, value.z); }
float3 WaterRGB(LitVector value) { return float3(value.x, value.y, value.z); }
LitLight WaterLight(RigidLightGPU packed) {
  LitLight light;
  light.position = WaterVector(packed.position_range.xyz); light.inverse_range = packed.position_range.w;
  light.direction = WaterVector(packed.direction_cone.xyz); light.cone_cosine = packed.direction_cone.w;
  light.colour = WaterVector(packed.colour_strength.xyz); light.cone_strength = packed.colour_strength.w;
  light.kind = int(packed.kind.x); return light;
}
LitFog WaterFogValue(RigidFogGPU packed) {
  LitFog fog;
  fog.origin = WaterVector(packed.origin_start.xyz); fog.start = packed.origin_start.w;
  fog.direction = WaterVector(packed.direction_end.xyz); fog.end = packed.direction_end.w;
  fog.colour = WaterVector(packed.colour_opacity.xyz); fog.opacity = packed.colour_opacity.w;
  fog.disabled = packed.mode.x != 0; fog.radial = packed.mode.y != 0; fog.blend = int(packed.mode.z);
  return fog;
}
