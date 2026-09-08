"""Source wiring guards; C++ behavior and real Vulkan query fixtures are separate."""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[1]


class NativeOcclusionBoundaryTest(unittest.TestCase):
    def test_native_end_flushes_before_depth_retirement_without_queries(self):
        source = (ROOT / "src/gpu/scene/native_scene_pass_bridge.cpp").read_text()
        flush = source.index("DrawQueueFlush(s.command_list);", source.index("// Complete native resolves"))
        retire = source.index("FinishNativeSceneResolves(s, *pass.resolves)", flush)
        self.assertLess(flush, retire)
        self.assertNotIn("OcclusionCullEmit", source)
        self.assertNotIn("OcclusionCullEmit", (ROOT / "src/gpu/draw_framebuffer.cpp").read_text())

    def test_temporal_query_production_is_removed(self):
        self.assertFalse((ROOT / "src/gpu/occlusion_cull.cpp").exists())
        self.assertFalse((ROOT / "src/gpu/occlusion_cull.h").exists())
        build = (ROOT / "src/CMakeLists.txt").read_text()
        self.assertNotIn("gpu/occlusion_cull.cpp", build)
        backend = build.split("set(reblue_backend_only", 1)[1].split(")", 1)[0]
        self.assertIn("gpu/native_depth_visibility.cpp", backend)
        source = (ROOT / "src/gpu/native_depth_visibility.cpp").read_text()
        for forbidden in ("GuestTexture", "ShadowFitCamera", "UploadHostConstants", "ConstantDescriptorSet",
                          "dynamicOffset", "matrix_va", "mesh_va"):
            self.assertNotIn(forbidden, source)
        self.assertIn("setComputePushConstants", source)
        self.assertIn("cmd.drawIndexedIndirect", source)
        for path in ("frame_ring.cpp", "present.cpp"):
            self.assertNotIn("OcclusionCull", (ROOT / "src/gpu" / path).read_text())

    def test_native_consumer_preserves_ordered_lights_and_sibling_preflight(self):
        source = (ROOT / "src/gpu/scene/native_rigid_draw.cpp").read_text()
        start = source.index("bool SubmitNativeRigidScene(")
        light = source.index("CommitNativeSceneLights", start)
        siblings = source.index("pending.push_back", light)
        draw = source.index("DrawQueuePush", siblings)
        self.assertLess(light, siblings)
        self.assertLess(siblings, draw)
        self.assertNotIn("std::erase_if(pending", source)
        self.assertNotIn("OcclusionCullRequest", source)
        self.assertIn("item->scene_depth = commands->DepthOwner()", source)
        self.assertNotIn("OcclusionCullRequest", (ROOT / "src/gpu/hooks/draw.cpp").read_text())

    def test_owned_bounds_and_current_view_feed_queries(self):
        source = (ROOT / "src/gpu/scene/host_walk.cpp").read_text()
        self.assertIn("SubmitNativeRigidScene(*instance_pose, index, shadow_policy, ctx.r1.u32)", source)
        self.assertNotIn("world_bounds", source)
        self.assertNotIn("OcclusionCullNote", source)
        self.assertNotIn("FindNativePassOcclusionView", source)
        self.assertNotIn("r_near", source)

    def test_bounds_come_from_indexed_cpu_asset_and_exact_draw_transform(self):
        mesh = (ROOT / "src/gpu/scene/native_mesh.cpp").read_text()
        self.assertLess(mesh.index("BuildNativeMeshBounds(data)"), mesh.index("chunk.buffer->map()"))
        self.assertIn("result->bounds = bounds", mesh)
        draw = (ROOT / "src/gpu/scene/native_rigid_draw.cpp").read_text()
        self.assertIn("TransformNativeBounds(*geometry->bounds,std::bit_cast<RenderMatrix>(plan.object.world))", draw)
        self.assertIn("plan.primitive = packet.primitive", (ROOT / "src/gpu/scene/native_rigid_scene.h").read_text())

    def test_current_visibility_seals_and_collects_through_real_slot_lifetime(self):
        ring = (ROOT / "src/gpu/frame_ring.cpp").read_text()
        submit = ring.split("void SubmitOpenListLocked", 1)[1]
        self.assertLess(submit.index("SealNativeRigidVisibilityLocked"), submit.index("s.command_lists[cur]->end()"))
        self.assertLess(ring.index("DrainNativeRigidDrawsLocked"), ring.index("DrainNativeTargetImagesLocked"))
        source = (ROOT / "src/gpu/scene/native_rigid_draw.cpp").read_text().split("void DrainNativeRigidDrawsLocked", 1)[1]
        self.assertLess(source.index("CollectAfterFence"), source.index("ResolveNativeRigidEmission"))
        self.assertLess(source.index("ResolveNativeRigidEmission"), source.index("record->output.Retire()"))
        self.assertLess(source.index("record->output.Retire()"), source.index("store.visibility[slot].clear()"))

    def test_queue_restores_bindings_and_refreshes_after_non_native_depth(self):
        source = (ROOT / "src/gpu/draw_queue.cpp").read_text()
        prepare = source.index("scene::PrepareNativeRigidBatchDraw(")
        self.assertLess(prepare, source.index("if (d.native_visibility) st = {}", prepare))
        self.assertIn("refresh_native_depth = true", source)
        self.assertIn("d.native_visibility->DrawCommand", source)
        direct = (ROOT / "src/gpu/scene/native_rigid_draw.cpp").read_text()
        self.assertIn("NativeRigidBatchBounds(items)", direct)
        self.assertIn("store.visibility_budget", direct)

    def test_shader_and_behavior_fixtures_are_connected(self):
        shader = (ROOT / "src/gpu/shaders/hlsl/native_occ_proxy_vs.hlsl").read_text()
        self.assertIn("vk::push_constant", shader)
        self.assertNotIn("shader_common", shader)
        self.assertFalse((ROOT / "src/gpu/shaders/hlsl/occ_proxy_vs.hlsl").exists())
        cmake = (ROOT / "tools/native_scene_snapshot_test/CMakeLists.txt").read_text()
        self.assertIn("native_occlusion_program.cpp", cmake)
        self.assertIn("native_occlusion_pixels", cmake)
        self.assertIn("native_occlusion_tests::Run()", (ROOT / "tools/native_texture_test/post_output.cpp").read_text())

    def test_current_depth_has_no_temporal_or_translated_inputs(self):
        source = (ROOT / "src/gpu/native_depth_visibility.cpp").read_text()
        for forbidden in ("GuestTexture", "UploadHostConstants", "OcclusionCullRequest", "queryResults", "waitForCommandFence"):
            self.assertNotIn(forbidden, source)
        shader = (ROOT / "src/gpu/shaders/hlsl/native_visibility_depth.h").read_text()
        self.assertIn("Texture2DMSArray<float>", shader)
        self.assertIn("sample_index<input.samples", shader)
        self.assertIn("farthest = max", shader)
        self.assertNotIn("RenderResolveMode::MIN", source)

    def test_current_depth_budget_precedes_allocations_and_releases_last(self):
        source = (ROOT / "src/gpu/native_depth_visibility.cpp").read_text()
        self.assertLess(source.index("budget->Acquire(buffer_bytes)"), source.index("device.createBuffer("))
        self.assertIn("plan->bytes+2*capacity*NativeDepthPyramid::kCommandStride", source)
        header = (ROOT / "src/gpu/native_depth_visibility.h").read_text()
        self.assertLess(header.index("} reservation_;"), header.index("pyramid_, indirect_, readback_"))
        self.assertIn("bytes > byte_limit_-bytes_", header)
        self.assertIn("owners_ >= owner_limit_", header)

    def test_receipts_require_real_draw_and_validate_gpu_fields(self):
        source = (ROOT / "src/gpu/native_depth_visibility.cpp").read_text()
        draw = source.split("bool NativeDepthVisibilityWork::DrawCommand", 1)[1].split("bool NativeDepthVisibilityWork::Seal", 1)[0]
        self.assertLess(draw.index("cmd.drawIndexedIndirect"), draw.index("draw_recorded = true"))
        collect = source.split("NativeDepthVisibilityWork::CollectAfterFence()", 1)[1]
        self.assertIn("!sealed_ || collected_", collect)
        self.assertLess(collect.index("vmaInvalidateAllocation"), collect.index("readback_->map()"))
        self.assertIn("std::memcmp(&actual,&expected_[n],sizeof(actual))", collect)
        self.assertLess(collect.index("if (!valid) return {}"), collect.index("collected_ = true"))

    def test_current_depth_fixture_exercises_ordered_reuse_and_faults(self):
        fixture = (ROOT / "tools/native_scene_snapshot_test/visibility.cpp").read_text()
        for required in ("RefreshDepth(*cmd,moved)", "original_pyramid", "scene.reset(); depth.reset();",
                         "weak_depth.expired()", "Receipt fault buffer", "perspective", "EmittedInstances()"):
            self.assertIn(required, fixture)
        self.assertIn("native_depth_visibility_pixels", (ROOT / "tools/native_scene_snapshot_test/CMakeLists.txt").read_text())


if __name__ == "__main__":
    unittest.main()
