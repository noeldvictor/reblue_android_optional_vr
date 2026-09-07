"""Native rigid shader/producer wiring; behavior and pixels have separate fixtures."""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[1]


class NativeRigidBoundaryTest(unittest.TestCase):
    def test_scene_sampling_matches_production_array_views(self):
        shader = (ROOT / "src/gpu/shaders/hlsl/native_rigid_ps.hlsl").read_text()
        self.assertIn("Texture2DArray<float4> albedo_image", shader)
        self.assertIn("Texture2DArray<float> shadow_image", shader)
        self.assertIn("float3(fragment.uv, 0)", shader)
        self.assertIn("float3(uv + offset, 0)", shader)
        # Ordinary assets and the mono sun shadow share layer zero between eyes;
        # framebuffer multiview does not make these sampled inputs stereo arrays.
        fixture = (ROOT / "tools/native_scene_snapshot_test/rigid.cpp").read_text()
        for required in ("TEXTURE_2D_ARRAY", "view.mipLevels = view.arraySize = 1",
                         "albedo_view.get()", "shadow_view.get()",
                         "RenderFormat::D32_FLOAT_S8_UINT", "VK_IMAGE_ASPECT_DEPTH_BIT"):
            self.assertIn(required, fixture)
        for path in ("src/gpu/scene/native_texture_gpu.cpp", "src/gpu/native_target_images.cpp"):
            self.assertIn("TEXTURE_2D_ARRAY", (ROOT / path).read_text())

    def test_direct_caster_precedes_interpreter_and_has_bounded_fence_owners(self):
        walk = (ROOT / "src/gpu/scene/host_walk.cpp").read_text()
        self.assertIn("SubmitNativeRigidShadow(*instance_pose, index, shadow_policy)", walk)
        direct = (ROOT / "src/gpu/scene/native_rigid_draw.cpp").read_text()
        for required in ("PrepareNativeRigidShadow(", "FindNativePassCamera(1)",
                         "CreateNativeRigidPrograms(", "GetOrCreatePipeline(", "DrawQueuePush(draw)",
                         "records.size() < 4096", "store.programs.size() < 8", "store.records[slot].clear()",
                         "draw.bindings.set_count = 1", "throw std::runtime_error(reason)"):
            self.assertIn(required, direct)
        for forbidden in ("__imp__", "HostDrawReplay(", "HostDrawCommit(", "bd::mem::", "ConstantBlockBytes("):
            self.assertNotIn(forbidden, direct)
        ring = (ROOT / "src/gpu/frame_ring.cpp").read_text()
        self.assertIn("scene::DrainNativeRigidDrawsLocked(s, slot)", ring)

    def test_production_shaders_do_not_import_the_translated_abi(self):
        paths = list((ROOT / "src/gpu/shaders/hlsl").glob("native_rigid_*.hlsl"))
        paths += [ROOT / "src/gpu/scene/native_rigid_shader.h"]
        self.assertEqual(len(paths), 4)
        for path in paths:
            text = path.read_text()
            for forbidden in ("shader_common.h", "g_VSC", "g_PSC", "BD_SHARED", "BOOL_BIT", "GuestShader", "packoffset"):
                self.assertNotIn(forbidden, text)

    def test_gpu_fixture_uses_production_programs_and_real_pixels(self):
        text = (ROOT / "tools/native_scene_snapshot_test/rigid.cpp").read_text()
        for required in ("CreateNativeRigidPrograms(device, input)", "ApplyGraphicsBindings(",
                         "ApplyNativePipelineProgram(", "drawIndexedInstanced(", "vkWaitForFences(",
                         "Rigid colour mismatch", "Rigid per-eye depth mismatch", "Rigid caster depth mismatch"):
            self.assertIn(required, text)
        self.assertNotIn("ofstream", text)
        self.assertNotIn("fopen", text)

    def test_builds_only_explicit_shader_dependencies(self):
        text = (ROOT / "cmake/shaders.cmake").read_text()
        self.assertIn('if(STEM MATCHES "^native_rigid_")', text)
        self.assertIn("src/gpu/scene/native_rigid_inputs.h", text)
        build = (ROOT / "tools/native_scene_snapshot_test/CMakeLists.txt").read_text()
        self.assertIn("../../src/gpu/scene/native_rigid_program.cpp", build)
        self.assertIn("native_rigid_pixels", build)


if __name__ == "__main__":
    unittest.main()
