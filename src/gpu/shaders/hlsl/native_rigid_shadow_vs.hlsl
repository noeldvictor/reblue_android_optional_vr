// Same object owner and light camera as the receiving shader; no emulated resolve.
#include "src/gpu/scene/native_rigid_shader.h"
float4 main([[vk::location(0)]] float4 position : POSITION, uint instance : SV_InstanceID) : SV_Position {
  return RigidTransform(RigidTransform(float4(position.xyz, 1), rigid_instances[instance].object_data.world),
      rigid_instances[instance].pass_data.world_to_shadow);
}
