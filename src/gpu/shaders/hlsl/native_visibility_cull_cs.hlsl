// Current-frame maximum depth -> native indexed command. No previous camera,
// queries, source addresses or CPU result readback. Ordinary forward-Z / LEQUAL.
struct CullPush { float4 rows[4]; float4 center, extent; uint4 draw, output; };
[[vk::push_constant]] ConstantBuffer<CullPush> input : register(b0,space0);
StructuredBuffer<float> pyramid : register(t0,space0);
RWByteAddressBuffer commands : register(u1,space0);
bool Hidden() {
    if (input.extent.w != 1) return false;
    float2 lo = 1e30, hi = -1e30;
    float nearest = 1;
    for (uint corner=0;corner<8;++corner) {
        float3 sign = float3((corner&1) ? 1 : -1,(corner&2) ? 1 : -1,(corner&4) ? 1 : -1);
        float4 p = float4(input.center.xyz+sign*input.extent.xyz,1);
        float4 clip = float4(dot(input.rows[0],p),dot(input.rows[1],p),dot(input.rows[2],p),dot(input.rows[3],p));
        if (!all(isfinite(clip)) || clip.w <= 1e-5 || clip.z <= 0 || clip.z > clip.w) return false;
        float3 ndc = clip.xyz/clip.w;
        if (!all(isfinite(ndc))) return false;
        lo = min(lo,ndc.xy); hi = max(hi,ndc.xy); nearest = min(nearest,ndc.z);
    }
    // No guessed frustum decision outside the viewport or around clip planes.
    if (any(lo > 1) || any(hi < -1)) return false;
    lo = max(lo,-1); hi = min(hi,1);
    uint2 dimensions = input.output.zw;
    // Native vertex shaders use D3D clip Y; Vulkan VS compilation inverts it.
    // Include a full extra pixel on every edge for raster/FP32 rounding.
    float2 pixel_lo = (float2(lo.x,-hi.y)*.5+.5)*dimensions;
    float2 pixel_hi = (float2(hi.x,-lo.y)*.5+.5)*dimensions;
    uint2 begin = uint2(clamp(floor(pixel_lo)-1,0,float2(dimensions-1)))/2;
    uint2 end = uint2(clamp(ceil(pixel_hi)+1,0,float2(dimensions-1)))/2;
    uint2 level = (dimensions+1)/2;
    uint offset = 0;
    while (any(end-begin > 1)) {
        offset += level.x*level.y; level = (level+1)/2;
        begin /= 2; end /= 2;
    }
    float farthest = max(max(pyramid[offset+begin.y*level.x+begin.x],pyramid[offset+begin.y*level.x+end.x]),
                         max(pyramid[offset+end.y*level.x+begin.x],pyramid[offset+end.y*level.x+end.x]));
    return isfinite(farthest) && farthest >= 0 && farthest <= 1 && nearest-1e-5 > farthest;
}
[numthreads(1,1,1)]
void main() {
    uint4 draw = input.draw;
    if (Hidden()) draw.y = 0;
    commands.Store4(input.output.y,draw);
    commands.Store(input.output.y+16,input.output.x);
}
