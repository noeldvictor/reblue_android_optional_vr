// Shared native vertex program; the three-layer variant has an explicit fifth
// asset attribute. Missing TexCoord2 is never silently replaced by TexCoord0.
#include "src/gpu/scene/native_rigid_shader.h"
RigidFragment main(RigidVertex vertex, uint eye : SV_ViewID, uint instance : SV_InstanceID) {
  const NativeRigidObjectGPU object_data = rigid_instances[instance].object_data;
  const NativeRigidPassGPU pass_data = rigid_instances[instance].pass_data;
  RigidFragment result;
  result.instance = instance;
  const float4 world = RigidTransform(float4(vertex.position.xyz, 1), object_data.world);
  result.clip = RigidTransform(world, pass_data.world_to_clip[eye]);
  result.world = world.xyz;
  result.normal = normalize(vertex.normal.x * object_data.normal_rows[0].xyz +
      vertex.normal.y * object_data.normal_rows[1].xyz + vertex.normal.z * object_data.normal_rows[2].xyz);
  result.uv.xy = vertex.uv.xy * object_data.uv_scale_offset.xy + object_data.uv_scale_offset.zw;
  result.uv.zw = vertex.uv.zw * object_data.detail_uv_scale_offset[0].xy + object_data.detail_uv_scale_offset[0].zw;
#ifdef RIGID_LAYERED_INPUT
  result.secondary_uv = vertex.secondary_uv.xy * object_data.detail_uv_scale_offset[1].xy + object_data.detail_uv_scale_offset[1].zw;
#else
  result.secondary_uv = 0; // Unused output padding; CPU admission forbids layer 2.
#endif
  result.colour = (object_data.flags.x & RigidVertexColour) ? vertex.colour : float4(1,1,1,1);
  return result;
}
