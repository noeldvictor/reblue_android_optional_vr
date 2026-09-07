"""Named light/fog core is consumed by the live normal material shader."""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[1]


class LitShadingBoundaryTest(unittest.TestCase):
    def test_core_has_no_shader_register_or_source_binding_dependency(self):
        core = (ROOT / "src/gpu/scene/native_lit_shading.h").read_text()
        for forbidden in ("g_PSC", "BOOL_BIT", "BD_SHARED", "PPCContext", "bd::mem", "NodeTag", "plume::"):
            self.assertNotIn(forbidden, core)
        for entry in ("EvaluateLitLight", "ComposeLitSurface", "ApplyLitFog"):
            self.assertIn(entry, core)

    def test_live_shader_uses_core_and_preserves_material_families(self):
        shader = (ROOT / "src/gpu/shaders/hlsl/bd_normal_lit.hlsl").read_text()
        self.assertIn('#include "src/gpu/scene/native_lit_shading.h"', shader)
        self.assertEqual(shader.count("EvaluateLitLight("), 3)
        self.assertEqual(shader.count("ApplyLitFog("), 2)
        self.assertIn("ComposeLitSurface(", shader)
        for feature in ("g_bTexture1", "g_bTexture2", "g_bEnvMap", "g_bShadowMap", "g_bDebug0", "SPEC_CONSTANT_ALPHA_TEST"):
            self.assertIn(feature, shader)
        self.assertNotIn("r7.x = c250.y > g_vLightPos1.w", shader)

    def test_precise_shader_dependency_and_actual_queue_observation(self):
        build = (ROOT / "cmake/shaders.cmake").read_text()
        self.assertIn('STEM STREQUAL "bd_normal_lit"', build)
        self.assertIn("src/gpu/scene/native_lit_shading.h", build)
        dispatch = (ROOT / "src/gpu/hooks/draw.cpp").read_text()
        self.assertLess(dispatch.index("bd::gpu::DrawQueuePush(q)"), dispatch.index("NoteNativeLitQueuedDraw()"))
        self.assertIn("!s.pipelineState.occlusionCounting", dispatch)


if __name__ == "__main__":
    unittest.main()
