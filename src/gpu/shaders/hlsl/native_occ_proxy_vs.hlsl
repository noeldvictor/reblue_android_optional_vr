// Owned world-space box queried against the native scene depth attachment.
// The 96-byte push packet has no translated shader or constant-buffer ABI.
struct OcclusionPacket { float4 world_to_clip[4]; float4 center; float4 extent; };
[[vk::push_constant]] ConstantBuffer<OcclusionPacket> query : register(b0, space0);

static const float3 kCubeCorners[8] = {
    float3(-1, -1, -1), float3(1, -1, -1), float3(1, 1, -1), float3(-1, 1, -1),
    float3(-1, -1, 1),  float3(1, -1, 1),  float3(1, 1, 1),  float3(-1, 1, 1)};
// 12 triangles, both windings drawn (cull off), so the order only has to
// cover every face.
static const uint kCubeIndices[36] = {
    0, 1, 2, 0, 2, 3, 4, 6, 5, 4, 7, 6, 0, 4, 5, 0, 5, 1,
    2, 6, 7, 2, 7, 3, 1, 5, 6, 1, 6, 2, 0, 3, 7, 0, 7, 4};

void main(in uint vertexId : SV_VertexID,
          out float4 oPos : SV_Position)
{
    const float3 corner = query.center.xyz + kCubeCorners[kCubeIndices[vertexId % 36u]] * query.extent.xyz;
    const float4 p = float4(corner, 1.0);
    oPos.x = dot(query.world_to_clip[0], p);
    oPos.y = dot(query.world_to_clip[1], p);
    oPos.z = dot(query.world_to_clip[2], p);
    oPos.w = dot(query.world_to_clip[3], p);
}
