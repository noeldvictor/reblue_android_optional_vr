"""Whole deferred pass boundary; CPU scheduling and GPU snapshot tests complement these guards."""
from pathlib import Path
import unittest


class DeferredVisualBoundaryTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        root = Path(__file__).resolve().parents[1]
        cls.core = (root / "src/gpu/scene/native_deferred_visuals.h").read_text(encoding="utf-8")
        cls.bridge = (root / "src/gpu/hooks/native_deferred_visuals.cpp").read_text(encoding="utf-8")

    def test_scheduler_has_native_bounds_and_live_reads(self):
        for token in ("PPC", "REX", "bd::mem", "GuestTexture", "1280", "720", "D3D"):
            self.assertNotIn(token, self.core)
        self.assertIn("kDeferredVisualLimit = 512", self.core)
        self.assertIn("const auto live_count = port.Count()", self.core)
        self.assertLess(self.core.index("port.SelectMode(requested)"), self.core.index("port.Primitive(index, mode == 6)"))
        self.assertLess(self.core.index("port.End()"), self.core.index("port.Clear()"))

    def test_native_snapshot_uses_scene_extent_without_emulated_resolve(self):
        snapshot = self.bridge.split("bool Snapshot()", 1)[1].split("uint32_t Count()", 1)[0]
        for token in ("1280", "720", "D3DDevice_Resolve", "sourceSurface", "ResolveRtToTexture", "copyTexture"):
            self.assertNotIn(token, snapshot)
        for token in ("scope->ColorReadImage()", "AcquireNativePostImage(source.width, source.height, source.layers)",
                      "NativeImageExtentPolicy::AdoptSource", "CopySceneSnapshot(", "Video::SetTexture(8, destination)"):
            self.assertIn(token, snapshot)
        self.assertLess(snapshot.index("DrawQueueFlushAt("), snapshot.index("Check(CopySceneSnapshot("))
        self.assertLess(snapshot.index("Check(CopySceneSnapshot("), snapshot.index("Video::PublishNativeImage("))
        self.assertNotIn("return false", snapshot.split("BeginCommandList(s)", 1)[1])

    def test_full_original_replay_is_only_at_the_preflight_boundary(self):
        adapter = self.bridge.split("struct Adapter", 1)[1].split("bool Prepare(", 1)[0]
        for token in ("__imp__", "sub_82425220", "sub_82425CD0", "LegacyShaderParameterScope"):
            self.assertNotIn(token, adapter)
        self.assertIn("ctx.r1.u64 = saved_stack", adapter)
        self.assertIn("REX_HOOK_RAW(sub_824252D0)", self.bridge)
        self.assertEqual(self.bridge.count("__imp__sub_824252D0(ctx, base)"), 1)
        self.assertNotIn("catch", self.bridge)

    def test_primitive_reloads_after_shader_callbacks_and_uses_native_submission(self):
        primitive = self.bridge.split("void Primitive(", 1)[1].split("void End()", 1)[0]
        self.assertIn("const uint64_t entry = Entry(index)", primitive)
        self.assertIn("ImportVisualBlend(Read(entry + 36))", primitive)
        self.assertIn("Call(DrawNativeImmediateUi", primitive)
        self.assertIn("translated ? entry + 40 : 0", primitive)
        self.assertIn("Read(kQueues + 1064)", primitive)
        self.assertIn("{8, 0, 1}", self.bridge)
        self.assertIn("{8, 4, 1}", self.bridge)


if __name__ == "__main__":
    unittest.main()
