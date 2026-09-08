// Shared ONLY by the three current-depth compute stages. Each output covers
// every input pixel/sample in its tile, including odd right/bottom dimensions.
struct DepthPush { uint width, height, offset, next_width, next_height, next_offset, samples, reserved; };
[[vk::push_constant]] ConstantBuffer<DepthPush> input : register(b0,space0);
#if !defined(NATIVE_REDUCE)
#if defined(NATIVE_MSAA)
Texture2DMSArray<float> depth : register(t0,space0);
#else
Texture2DArray<float> depth : register(t0,space0);
#endif
#endif
RWStructuredBuffer<float> pyramid : register(u1,space0);
[numthreads(8,8,1)]
void main(uint3 id : SV_DispatchThreadID) {
    if (id.x >= input.next_width || id.y >= input.next_height) return;
    float farthest = 0;
    for (uint y=0;y<2;++y) for (uint x=0;x<2;++x) {
        uint2 pixel = min(id.xy*2+uint2(x,y),uint2(input.width,input.height)-1);
#if defined(NATIVE_REDUCE)
        float z = pyramid[input.offset+pixel.y*input.width+pixel.x];
        farthest = max(farthest,isfinite(z) && z >= 0 && z <= 1 ? z : 1);
#else
        for (uint sample_index=0;sample_index<input.samples;++sample_index) {
#if defined(NATIVE_MSAA)
            float z = depth.Load(int3(pixel,0),sample_index);
#else
            float z = depth.Load(int4(pixel,0,0));
#endif
            farthest = max(farthest,isfinite(z) && z >= 0 && z <= 1 ? z : 1);
        }
#endif
    }
    pyramid[input.next_offset+id.y*input.next_width+id.x] = farthest;
}
