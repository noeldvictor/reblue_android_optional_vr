"""Whole-view source boundaries; CPU/runtime checks remain separate evidence."""
from pathlib import Path
import unittest


class ViewScheduleBoundaryTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        root = Path(__file__).resolve().parents[1]
        cls.bridge = (root / "src/gpu/scene/native_view_schedule_bridge.cpp").read_text(encoding="utf-8")
        cls.core = (root / "src/gpu/scene/native_view_schedule.h").read_text(encoding="utf-8")
        cls.interp = (root / "src/engine/frame_interp.cpp").read_text(encoding="utf-8")

    def test_parent_fallback_only_precedes_host_effects(self):
        entry = self.bridge.split("bool TryScheduleRenderView(", 1)[1]
        self.assertLess(entry.index("!Prepare(ctx)"), entry.index("ViewAdapter adapter"))
        self.assertLess(entry.index("return false"), entry.index("ScheduleRenderView(adapter)"))
        self.assertNotIn("__imp__bdRenderViewSubmit", self.bridge)
        self.assertIn("if (!bd::gpu::scene::TryScheduleRenderView(ctx, base))", self.interp)

    def test_core_has_no_engine_or_console_dependency(self):
        for name in ("PPC", "REX", "bd::mem", "D3D", "vtable", "kPassTables"):
            self.assertNotIn(name, self.core)
        for name in ("D3DDevice_SetShaderGPRAllocation", "D3DDevice_CreateTexture",
                     "D3DDevice_Resolve", "BeginTiling", "EndTiling"):
            self.assertNotIn(name, self.bridge)

    def test_special_views_are_not_silently_replaced_by_main_only(self):
        self.assertEqual(self.bridge.count("face < 6"), 2)
        self.assertIn("i < 10", self.bridge)
        for method in ("RenderIndexedView", "RenderSunShadow", "RenderCubeShadow",
                       "RenderAuxiliary", "RenderShadowVolume", "RenderReflections",
                       "RenderEnvironment", "RenderAdditionalScene", "RenderMainScene", "RenderPost"):
            self.assertIn(method, self.core)
            self.assertIn(method, self.bridge)
        self.assertLess(self.bridge.index("++stats.legacy_exports"), self.bridge.index("struct ViewAdapter"))

    def test_post_refusal_keeps_its_complete_isolated_cleanup(self):
        post = self.bridge.split("void RenderPost()", 1)[1]
        self.assertLess(post.index("bdNativeScenePostHook"), post.index("Call(sub_8221C9A0"))
        self.assertEqual(post.count("Call(sub_8221C9A0"), 2)
        self.assertIn("DestroyPostContainer(color); DestroyPostContainer(depth)", post)
        self.assertNotIn("return false", post.split("};", 1)[0])

    def test_camera_composition_scope_wraps_native_and_fallback(self):
        scope = self.interp.split("REX_HOOK_RAW(bdRenderViewSubmit)", 1)[1].split("REX_HOOK_RAW(bdBuildViewMatrix)", 1)[0]
        self.assertLess(scope.index("ViewCompositionScope composition"), scope.index("TryScheduleRenderView"))
        self.assertLess(scope.index("NativeSceneResultScope scene_result"), scope.index("TryScheduleRenderView"))
        self.assertGreater(scope.index("scene_result.Clear()"), scope.index("__imp__bdRenderViewSubmit"))


if __name__ == "__main__":
    unittest.main()
