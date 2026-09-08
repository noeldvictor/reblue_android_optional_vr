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
        query = source.index("OcclusionCullRequest", siblings)
        draw = source.index("DrawQueuePush", query)
        self.assertLess(light, siblings)
        self.assertLess(siblings, query)
        self.assertLess(query, draw)
        self.assertIn("std::erase_if(pending", source)
        self.assertIn("node, entry.primitive}, occlusion_view, entry.bounds", source)
        self.assertNotIn("OcclusionCullRequest", (ROOT / "src/gpu/hooks/draw.cpp").read_text())

    def test_owned_bounds_and_current_view_feed_queries(self):
        source = (ROOT / "src/gpu/scene/host_walk.cpp").read_text()
        self.assertIn("SubmitNativeRigidScene(*instance_pose, index, shadow_policy)", source)
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

    def test_empty_pass_does_not_create_pipeline_or_switch_bindings(self):
        source = (ROOT / "src/gpu/occlusion_cull.cpp").read_text().split("void OcclusionCullEmit", 1)[1]
        self.assertLess(source.index("!o.tracker.HasQueries(*view)"), source.index("PipelineFor(o"))
        self.assertLess(source.index("!o.tracker.HasQueries(*view)"), source.index("EngineGraphicsBindings"))

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
