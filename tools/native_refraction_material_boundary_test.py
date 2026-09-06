"""Structural migration guards, not a substitute for authored runtime/pixel checks."""
from pathlib import Path
import re
import unittest


class RefractionMaterialBoundaryTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        root = Path(__file__).resolve().parents[1]
        cls.core = (root / "src/gpu/scene/native_refraction_material.h").read_text(encoding="utf-8")
        cls.bridge = (root / "src/gpu/scene/native_refraction_material_bridge.cpp").read_text(encoding="utf-8")

    def test_core_has_no_console_resource_or_register_inputs(self):
        for token in ("PPC", "REX", "GuestTexture", "bd::mem", "D3D", "Xenos", "kActiveTextureTable"):
            self.assertNotIn(token, self.core)

    def test_whole_hooks_only_fall_back_before_effects(self):
        for name in ("sub_82454720", "sub_82455150"):
            self.assertIn(f"REX_HOOK_RAW({name})", self.bridge)
            self.assertEqual(self.bridge.count(f"__imp__{name}(ctx, base)"), 1)
        active = self.bridge.split("Adapter adapter(ctx, base, water);", 1)[1]
        self.assertNotIn("__imp__", active)
        self.assertNotIn("catch", self.bridge)
        self.assertIn("~Adapter() { ctx.r1.u64 = saved_stack; }", self.bridge)

    def test_state_changes_preserve_depth_write_and_other_blend_terms(self):
        self.assertEqual(re.findall(r"State\((\d+), (\d+)\)", self.bridge),
                         [("60", "1"), ("72", "6"), ("76", "7"), ("40", "1")])
        self.assertIn("state adapters", self.bridge)

    def test_binding_does_not_dispatch_console_texture_or_resolve(self):
        for token in ("D3DDevice_SetTexture(", "D3DDevice_Resolve(", "ResolveRtToTexture("):
            self.assertNotIn(token, self.bridge)
        self.assertIn("ResolveGuestTexture(address)", self.bridge)
        self.assertIn("GetOrCreateDebugTexture()", self.bridge)
        self.assertIn("Video::SetTexture(slot, image)", self.bridge)
        self.assertIn("else ++stats.null_bindings", self.bridge)
        self.assertIn("debug images", self.bridge)

    def test_snapshot_decision_follows_live_image_binding(self):
        sequence = self.core.split("void PrepareWaterMaterial", 1)[1].split("void PrepareRefractionMaterial", 1)[0]
        ordered = ("PublishSceneFactor()", "FlushWaterParameters(0)", "FlushWaterParameters(1)",
                   "EnableSourceAlphaBlending()", "EnableDepthTest()", "BindPlanarReflection()",
                   "BindSceneImage()", "WantsSnapshot()", "Snapshot()")
        positions = [sequence.index(token) for token in ordered]
        self.assertEqual(positions, sorted(positions))
        self.assertIn("int32_t(ReadWord(uint64_t(material) + 4700)) > 0", self.bridge)
        self.assertIn("sub_8221D2C8(ctx, base)", self.bridge)

    def test_parameter_clamp_and_live_descriptor_validation_remain(self):
        self.assertIn("ClampWaterHighlight(before)", self.bridge)
        self.assertIn("Check(DescriptorReady(descriptor,", self.bridge)
        self.assertIn("Flush(index ? 4952 : 4760, true)", self.bridge)
        self.assertIn("Flush(4968, false)", self.bridge)
        self.assertIn("first <= 51 && end > 51 && data", self.bridge)
        self.assertIn("parameter adapters", self.bridge)


if __name__ == "__main__":
    unittest.main()
