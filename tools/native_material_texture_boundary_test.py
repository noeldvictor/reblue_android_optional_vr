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
        self.assertLess(walk.index("const auto instance_pose = FindNativeInstancePose("),
                        walk.index("NativeObjectTextureScope textures(ctx_va, instance_pose)"))
        self.assertLess(walk.index("NativeObjectTextureScope textures(ctx_va, instance_pose)"),
                        walk.index("const u32 saved_r1"))
        self.assertIn("previous_(current)", self.bridge)
        self.assertIn("current = previous_; --depth;", self.bridge)
        self.assertIn("depth > kScopeDepth", self.bridge)

    def test_native_packet_uses_exact_pose_and_shared_preparation_without_source_keys(self):
        prepare = self.bridge.split("NativeObjectTextureState::Mesh *PrepareMaterialMesh(", 1)[1].split(
            "NativeObjectTextureState::Mesh *PrepareReplayMaterialMesh(", 1)[0]
        packet = self.bridge.split("std::optional<NativeObjectPrimitiveInputs> FindNativeObjectPrimitive(", 1)[1].split(
            "std::optional<NativeMaterialObjectInputs>", 1)[0]
        for part in (prepare, packet):
            for forbidden in ("NodeTag", "LoadedNativeModelGeneration", "FindLoadedNativeModelMaterials",
                              "source_meshes", "ModelPrimitiveMatches", "bd::mem", "Word(", "Video::"):
                self.assertNotIn(forbidden, part)
        self.assertIn("scope->pose.get() != &pose", packet)
        self.assertIn("scope->model != pose.model", packet)
        self.assertIn("BuildNativeObjectPrimitive(scope->pose, node, primitive", packet)
        self.assertIn("PrepareMaterialMesh(owner->program)", self.bridge)
        self.assertIn("owner.owner_before(scope->model)", self.bridge)
        self.assertIn("alias_bytes = 128", self.bridge)

    def test_colour_consumer_reads_source_only_for_comparison_or_missing_publication(self):
        material = (ROOT / "src/gpu/scene/native_material.cpp").read_text()
        begin = material.split("uint32_t EvaluateNativeMaterial(", 1)[1]
        owned = begin.split("if (const auto object = FindNativeMaterialObjectInputs(tag))", 1)[1].split(
            "if (bd::mem::try_load<uint32_t>(tag.ctx_va + 16", 1)[0]
        self.assertIn("if (REXCVAR_GET(bd_native_materials_verify))", owned)
        self.assertIn("NativeMaterialObjectInputCheck(original && *original == *object)", owned)
        self.assertIn("ComposeNativeMaterialAsset(material, object->colour, object->writes_shininess", owned)
        core = (ROOT / "src/gpu/scene/native_object_primitive.h").read_text()
        for forbidden in ("NodeTag", "bd::mem", "PPCContext", "Video::", "REX_", "source_bindings"):
            self.assertNotIn(forbidden, core)

    def test_consumer_only_uses_owned_program_table_and_override_values(self):
        consumer = self.bridge.split("NativeObjectTextureState::Mesh *PrepareMaterialMesh(")[1]
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
