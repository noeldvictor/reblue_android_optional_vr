// Textured depth coverage only. The zero-layer variant declares no image slots.
#include "src/gpu/scene/native_rigid_shader.h"
#ifndef RIGID_SHADOW_UNTEXTURED
[[vk::binding(0, 1)]] Texture2DArray<float4> albedo_image : register(t0, space1);
[[vk::binding(0, 2)]] SamplerState albedo_sampler : register(s0, space2);
#endif
void main(float2 uv : TEXCOORD0, float vertex_alpha : TEXCOORD1,
    nointerpolation uint instance : TEXCOORD2) {
  const NativeRigidObjectGPU object_data = rigid_instances[instance].object_data;
  float texture_alpha = 1;
#ifndef RIGID_SHADOW_UNTEXTURED
  const float sampled = albedo_image.Sample(albedo_sampler, float3(uv, 0)).a;
  texture_alpha = uv.x < 0 ? 0 : sampled;
#endif
  const float alpha = (texture_alpha * object_data.diffuse.w) * vertex_alpha;
  if (!RigidCutoutPasses(object_data.flags.z, alpha, asfloat(object_data.flags.w))) discard;
}
