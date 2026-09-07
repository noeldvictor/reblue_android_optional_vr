"""Ownership wiring guards; pure assignment fixtures and live pixels are separate."""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[1]


class MaterialTextureBoundaryTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.bridge = (ROOT / "src/gpu/scene/native_material_texture_bridge.cpp").read_text()
        cls.draw = (ROOT / "src/gpu/scene/host_draw.cpp").read_text()

    def test_object_publication_precedes_traversal_and_restores_nested_scope(self):
        walk = (ROOT / "src/gpu/scene/host_walk.cpp").read_text()
        self.assertLess(walk.index("NativeObjectTextureScope textures(ctx.r4.u32)"),
                        walk.index("Walk(ctx, base, ctx.r3.u32, ctx.r4.u32)"))
        self.assertIn("previous_(current)", self.bridge)
        self.assertIn("current = previous_; --depth;", self.bridge)
        self.assertIn("depth > kScopeDepth", self.bridge)

    def test_consumer_only_uses_owned_program_table_and_override_values(self):
        consumer = self.bridge.split("const NativeMaterialTextureValues *FindNativeMaterialTextures(")[1]
        consumer = consumer.split("void NativeMaterialTextureCheck")[0]
        for forbidden in ("Word(", "ReadMaterialTextureInputs", "ResolveGuestTexture", "CaptureNativeTexture",
                          "REX_LOAD", "bd::mem", "Video::"):
            self.assertNotIn(forbidden, consumer)
        for required in ("LoadedNativeModelGeneration", "FindLoadedNativeModelMaterials", "ComposeMaterialTextures",
                         "scope->table->slots", "kScopeBytes", "mesh.owner", "scope->inputs"):
            self.assertIn(required, consumer)

    def test_table_and_model_getters_do_not_import_under_consumer_locks(self):
        table = (ROOT / "src/gpu/scene/native_texture_table_bridge.cpp").read_text()
        table = table.split("NativeTextureTableHandle FindLoadedNativeTextureTable(")[1].split(
            "void NativeTextureTableImageChanged")[0]
        model = (ROOT / "src/gpu/scene/native_material.cpp").read_text()
        model = model.split("FindLoadedNativeModelMaterials(")[1].split("\n}", 1)[0]
        for getter in (table, model):
            for forbidden in ("bd::mem", "ResolveGuestTexture", "CaptureNativeTexture", "ReadCommands", "Video::"):
                self.assertNotIn(forbidden, getter)

    def test_pure_values_cannot_retain_source_layout(self):
        core = (ROOT / "src/gpu/scene/native_material_textures.h").read_text()
        for forbidden in ("be_u32", "NodeTag", "PPCContext", "visual_va", "REX_", "bd::mem"):
            self.assertNotIn(forbidden, core)
        registry = (ROOT / "src/gpu/scene/native_model_materials.cpp").read_text()
        self.assertIn("texture_assignments.capacity()", registry)
        self.assertIn("texture_assignment_end", registry)

    def test_capture_validates_and_replay_recomposes_native_values(self):
        for required in ("native_texture_recipe_mask", "native_uv_recipe", "NativeMaterialTextureCheck",
                         "values.textures = FindNativeMaterialTextures", "NativeMaterialTextureNoteDraw"):
            self.assertIn(required, self.draw)
        self.assertGreaterEqual(self.draw.count("d.native_uv_recipe"), 4)
        self.assertIn("p.replayable = false", self.draw)
        self.assertIn("channel == 5", self.draw)  # reflection owns its separate producer
        self.assertIn("p.scene_texture_recipe.UsesSlot(channel)", self.draw)


if __name__ == "__main__":
    unittest.main()
