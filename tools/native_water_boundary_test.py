"""Architecture guards only; the production --water GPU fixture checks behavior."""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[1]


class NativeWaterBoundaryTest(unittest.TestCase):
    def test_shaders_consume_owned_values_not_translated_constants(self):
        files = ("src/gpu/scene/native_water_shader.h",
                 "src/gpu/shaders/hlsl/native_water_vs.hlsl",
                 "src/gpu/shaders/hlsl/native_water_ps.hlsl")
        source = "\n".join((ROOT / file).read_text() for file in files)
        for forbidden in ("g_PSC", "BD_VSC", "BOOL_BIT", "BD_SHARED", "shader_common.h", "tfetch", "packoffset"):
            self.assertNotIn(forbidden, source)
        self.assertIn("StructuredBuffer<NativeWaterInstanceGPU>", source)
        self.assertIn("EvaluateLitLight", source)
        self.assertIn("ApplyLitFog", source)

    def test_camera_and_scene_images_are_explicitly_per_eye(self):
        vertex = (ROOT / "src/gpu/shaders/hlsl/native_water_vs.hlsl").read_text()
        pixel = (ROOT / "src/gpu/shaders/hlsl/native_water_ps.hlsl").read_text()
        self.assertIn("world_to_clip[eye]", vertex)
        self.assertIn("world_to_bottom[eye]", pixel)
        self.assertIn("cameras[eye]", pixel)
        for lane in "xyz":
            self.assertIn(f"WaterImageLayer(data.image_layers.{lane},eye)", pixel)
        self.assertNotIn("g_StereoSeparation", vertex)

    def test_temporary_import_is_outside_program_and_native_data(self):
        native = (ROOT / "src/gpu/scene/native_water_inputs.h").read_text()
        program = (ROOT / "src/gpu/scene/native_water_program.cpp").read_text()
        for forbidden in ("REX_", "PPCContext", "bd::mem", "ResolveGuest", "WaterUpdateBuilder", "source_va"):
            self.assertNotIn(forbidden, native + program)
        self.assertIn("NativePipelineProgram::Create", program)
        source = (ROOT / "src/gpu/scene/native_water_material_source.h").read_text()
        self.assertIn("boundary.Destination", source)
        self.assertNotIn("boundary.Store", source)

    def test_gpu_fixture_uses_real_program_snapshot_and_indirect_draw(self):
        fixture = (ROOT / "tools/native_scene_snapshot_test/water.cpp").read_text()
        for required in ("CreateNativeWaterProgram", "ReadNativeWaterMaterial", "BuildNativeWaterInstance",
                         "CopySceneSnapshot", "drawIndexedIndirect", "Water per-eye depth oracle",
                         "Water authored animation must change both eyes", "HalfStoreMatches"):
            self.assertIn(required, fixture)
        cmake = (ROOT / "tools/native_scene_snapshot_test/CMakeLists.txt").read_text()
        self.assertIn("native_water_program.cpp", cmake)
        self.assertIn("NAME native_water_pixels", cmake)


if __name__ == "__main__":
    unittest.main()
