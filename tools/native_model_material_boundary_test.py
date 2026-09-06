"""Loader/consumer wiring guards. CPU lifetimes and live pixels remain required."""
from pathlib import Path
import unittest


class ModelMaterialBoundaryTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        root = Path(__file__).resolve().parents[1]
        cls.bridge = (root / "src/gpu/scene/native_material.cpp").read_text(encoding="utf-8")
        cls.core = (root / "src/gpu/scene/native_model_materials.cpp").read_text(encoding="utf-8")

    def test_load_publishes_after_complete_builder_without_pso_switch(self):
        hook = self.bridge.split("REX_HOOK_RAW(bdSceneGraphBuild) {", 1)[1].split("REX_HOOK_RAW", 1)[0]
        self.assertLess(hook.index("__imp__bdSceneGraphBuild(ctx, base)"),
                        hook.index("PublishModelMaterials(ctx.r3.u32)"))
        self.assertNotIn("PrecacheEnabled", self.bridge)
        self.assertNotIn("bdModelLoadEndHook", self.bridge)

    def test_retirement_covers_complete_nonphysical_destructor(self):
        hook = self.bridge.split("REX_HOOK_RAW(sub_8227EBE8) {", 1)[1]
        self.assertLess(hook.index("Models().Retire(ctx.r3.u32)"),
                        hook.index("__imp__sub_8227EBE8(ctx, base)"))
        self.assertNotIn("XPhysicalFree", hook)

    def test_draw_lookup_cannot_rebuild_or_read_commands(self):
        lookup = self.bridge.split("FindCommands(const NodeTag &tag) {", 1)[1].split("bool Matches", 1)[0]
        self.assertIn("Models().Find", lookup)
        for forbidden in ("ReadCommands", "DecodeMeshMaterials", "Resolve(",
                          "PhysicalBufferGeneration", "emplace", "tag.mesh_va +"):
            self.assertNotIn(forbidden, lookup)
        self.assertEqual(self.bridge.count("const auto commands = FindCommands(tag);"), 4)
        self.assertNotIn("thread_local std::unordered_map", self.bridge)

    def test_owner_core_has_no_guest_memory_gpu_or_disk_operations(self):
        for forbidden in ("REX_", "PPCContext", "bd::mem", "GuestBuffer", "Video::",
                          "ifstream", "ofstream", "PhysicalBufferGeneration"):
            self.assertNotIn(forbidden, self.core)

    def test_diagnostic_context_uses_existing_scene_state_not_water_proxy(self):
        for required in ("game.Mode()", "game.FieldState()", "game.Stage().Name()",
                         "game.Field().HasPlayer()", "bd::engine::EventScenePlaying()",
                         "bd::engine::SofdecMoviePlaying()"):
            self.assertIn(required, self.bridge)
        self.assertIn("if (REXCVAR_GET(bd_native_materials_verify))", self.bridge)


if __name__ == "__main__":
    unittest.main()
