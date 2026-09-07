// Authored shadowmap coverage: fixed cutoff, no scene alpha/negative-U sentinel.
// Discard instead of writing far depth preserves min-depth and earlier casters.
[[vk::binding(0, 1)]] Texture2DArray<float4> albedo_image : register(t0, space1);
[[vk::binding(0, 2)]] SamplerState albedo_sampler : register(s0, space2);
void main(float2 uv : TEXCOORD0) {
  const float sampled = albedo_image.Sample(albedo_sampler, float3(uv, 0)).a;
  if (sampled < .6f) discard;
}
