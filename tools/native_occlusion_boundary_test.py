"""Source wiring guards; C++ behavior and real Vulkan query fixtures are separate."""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[1]


class NativeOcclusionBoundaryTest(unittest.TestCase):
    def test_native_end_queries_before_depth_retirement(self):
        source = (ROOT / "src/gpu/scene/native_scene_pass_bridge.cpp").read_text()
        flush = source.index("DrawQueueFlush(s.command_list);", source.index("// Complete native resolves"))
        query = source.index("OcclusionCullEmit(s, *pass.commands);", flush)
        retire = source.index("FinishNativeSceneResolves(s, *pass.resolves)", flush)
        self.assertLess(flush, query)
        self.assertLess(query, retire)
        self.assertNotIn("OcclusionCullEmit", (ROOT / "src/gpu/draw_framebuffer.cpp").read_text())

    def test_queries_have_no_translated_binding_or_source_address(self):
        source = (ROOT / "src/gpu/occlusion_cull.cpp").read_text()
        for forbidden in ("GuestTexture", "ShadowFitCamera", "UploadHostConstants", "ConstantDescriptorSet",
                          "dynamicOffset", "matrix_va", "mesh_va"):
            self.assertNotIn(forbidden, source)
        self.assertIn("setGraphicsPushConstants", source)
        self.assertIn("WithNativeOcclusionBindings(*cmd", source)
        self.assertIn("sl.queries.push_back(query)", source)
        self.assertIn("o.tracker.Collect(sl.queries[n]", source)

    def test_native_consumer_preserves_ordered_lights_and_sibling_preflight(self):
        source = (ROOT / "src/gpu/scene/native_rigid_draw.cpp").read_text()
        start = source.index("bool SubmitNativeRigidScene(")
        light = source.index("CommitNativeRigidSceneLights", start)
        siblings = source.index("pending.push_back", light)
        query = source.index("OcclusionCullOccluded", siblings)
        draw = source.index("DrawQueuePush", query)
        self.assertLess(light, siblings)
        self.assertLess(siblings, query)
        self.assertLess(query, draw)
        self.assertNotIn("OcclusionCullOccluded", (ROOT / "src/gpu/hooks/draw.cpp").read_text())

    def test_owned_bounds_and_current_view_feed_queries(self):
        source = (ROOT / "src/gpu/scene/host_walk.cpp").read_text()
        self.assertIn("visible && occlusion && native_pose && bounds", source)
        self.assertIn("instance_pose->instance, instance_pose->model_generation, index", source)
        self.assertIn("FindNativePassOcclusionView()", source)
        self.assertNotIn("r_near", source)

    def test_shader_and_behavior_fixtures_are_connected(self):
        shader = (ROOT / "src/gpu/shaders/hlsl/native_occ_proxy_vs.hlsl").read_text()
        self.assertIn("vk::push_constant", shader)
        self.assertNotIn("shader_common", shader)
        self.assertFalse((ROOT / "src/gpu/shaders/hlsl/occ_proxy_vs.hlsl").exists())
        cmake = (ROOT / "tools/native_scene_snapshot_test/CMakeLists.txt").read_text()
        self.assertIn("native_occlusion_program.cpp", cmake)
        self.assertIn("native_occlusion_pixels", cmake)
        self.assertIn("native_occlusion_tests::Run()", (ROOT / "tools/native_texture_test/post_output.cpp").read_text())


if __name__ == "__main__":
    unittest.main()
