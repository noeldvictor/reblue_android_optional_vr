// Opaque rigid geometry: named attributes, object transform and complete per-eye views.
#include "src/gpu/scene/native_rigid_shader.h"
RigidFragment main(RigidVertex vertex, uint eye : SV_ViewID) {
  RigidFragment result;
  const float4 world = RigidTransform(float4(vertex.position.xyz, 1), object_data.world);
  result.clip = RigidTransform(world, pass_data.world_to_clip[eye]);
  result.world = world.xyz;
  result.normal = normalize(vertex.normal.x * object_data.normal_rows[0].xyz +
      vertex.normal.y * object_data.normal_rows[1].xyz + vertex.normal.z * object_data.normal_rows[2].xyz);
  result.uv = vertex.uv.xy * object_data.uv_scale_offset.xy + object_data.uv_scale_offset.zw;
  result.colour = (object_data.flags.x & RigidVertexColour) ? vertex.colour : float4(1,1,1,1);
  return result;
}
