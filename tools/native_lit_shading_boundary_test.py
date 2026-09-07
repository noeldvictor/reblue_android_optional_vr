"""Named light/fog core is consumed by the live normal material shader."""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[1]


class LitShadingBoundaryTest(unittest.TestCase):
    def test_owned_fog_and_late_publication_invalidation(self):
        core = (ROOT / "src/gpu/scene/native_fog.h").read_text()
        for forbidden in ("g_PSC", "PPCContext", "bd::mem", "NodeTag", "uint32_t", "plume::"):
            self.assertNotIn(forbidden, core)
        source = (ROOT / "src/gpu/scene/native_fog_source.h").read_text()
        self.assertIn("std::array<NativeFogWrite, 16>", source)
        self.assertIn("result.fog = previous", source)
        self.assertNotIn("bd::mem::store", source)
        bridge = (ROOT / "src/gpu/scene/native_fog_bridge.cpp").read_text()
        self.assertEqual(bridge.count("__imp__sub_82179270(ctx, base)"), 2)
        self.assertIn("if (REXCVAR_GET(bd_native_materials_verify))", bridge)
        self.assertIn("expected == revision", bridge)
        self.assertIn("current[0].frame == FrameStatFrameCount()", bridge)
        self.assertIn("current[1].frame == FrameStatFrameCount()", bridge)
        owner = (ROOT / "src/gpu/scene/native_material_texture_bridge.cpp").read_text()
        self.assertIn("publication->fog = FindNativeFogLayers()", owner)
        self.assertEqual(owner.count("NativeFogIsCurrent(scope->fog_revision)"), 2)
        packet = (ROOT / "src/gpu/scene/native_object_primitive.h").read_text()
        self.assertIn("std::optional<NativeFogLayers> fog", packet)
        draw = (ROOT / "src/gpu/scene/host_draw.cpp").read_text()
        self.assertLess(draw.index("d.bools[4 + i] ="), draw.index("CheckNativeFogLayers(*fog, t_ps_block, d.bools[4])"))

    def test_selected_lights_are_semantic_owned_values(self):
        core = (ROOT / "src/gpu/scene/native_selected_lights.h").read_text()
        for forbidden in ("g_PSC", "PPCContext", "bd::mem", "NodeTag", "uint32_t", "plume::"):
            self.assertNotIn(forbidden, core)
        packet = (ROOT / "src/gpu/scene/native_object_primitive.h").read_text()
        self.assertIn("std::optional<NativeSelectedLights> lights", packet)
        source = (ROOT / "src/gpu/scene/native_selected_lights_source.h").read_text()
        self.assertIn("std::array<SelectedLightWrite, 42>", source)
        self.assertIn("std::array<uint32_t, 128>", source)
        self.assertIn("record_count > 300", source)
        self.assertIn("if (id == old_id)", source)
        self.assertNotIn("bd::mem::store", source)

    def test_source_publication_is_checked_and_scoped_before_consumption(self):
        bridge = (ROOT / "src/gpu/scene/native_selected_lights_bridge.cpp").read_text()
        self.assertEqual(bridge.count("__imp__sub_8218B0F0(ctx, base)"), 2)
        self.assertIn("if (REXCVAR_GET(bd_native_materials_verify))", bridge)
        self.assertIn("current.known != 7 || current_selection != selection", bridge)
        self.assertIn("current_frame != FrameStatFrameCount()", bridge)
        self.assertIn("current = {}; current_selection = 0; current_frame = ~0u", bridge)
        owner = (ROOT / "src/gpu/scene/native_material_texture_bridge.cpp").read_text()
        self.assertIn("per_node && !*per_node", owner)
        self.assertIn("FindNativeSelectedLights(*visual+3132)", owner)
        self.assertIn("SelectNativeObjectLights(scope->lights, scope->node_lights, node)", owner)
        self.assertIn("if (expected != selection) return false", owner)
        self.assertIn("PublishNativeMaterialLights(selection, current.lights)", bridge)
        self.assertEqual(bridge.count("InvalidateNativeMaterialLights();"), 2)
        self.assertIn("current->lights.reset(); current->node_lights.reset();", owner)
        draw = (ROOT / "src/gpu/scene/host_draw.cpp").read_text()
        self.assertIn("0xFB83DD3F5E67CEB7ull", draw)
        self.assertIn("CheckNativeSelectedLights(*lights, t_ps_block)", draw)

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
