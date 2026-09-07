// Same object owner and light camera as the receiving shader; no emulated resolve.
#include "src/gpu/scene/native_rigid_shader.h"
float4 main([[vk::location(0)]] float4 position : POSITION) : SV_Position {
  return RigidTransform(RigidTransform(float4(position.xyz, 1), object_data.world), pass_data.world_to_shadow);
}
