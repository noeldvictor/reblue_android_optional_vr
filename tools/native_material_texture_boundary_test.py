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
                        walk.index("NativeObjectTextureScope textures(ctx_va, instance_pose, ctx.r1.u32)"))
        self.assertLess(walk.index("NativeObjectTextureScope textures(ctx_va, instance_pose, ctx.r1.u32)"),
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

    def test_uv_failure_provenance_is_bounded_and_does_not_replace_comparison(self):
        diagnostic = self.bridge.split("void ReportNativeMaterialUvMismatch(", 1)[1].split(
            "\nnamespace {", 1)[0]
        self.assertIn("stats.wrong > 4", diagnostic)
        self.assertIn("std::min<size_t>(8,current->inputs.overrides.size())", diagnostic)
        self.assertIn("const auto &entry=current->inputs.overrides[n]", diagnostic)
        for forbidden in ("Publish", "REX_STORE", "bd::mem::store", "current->inputs =", "values.uv ="):
            self.assertNotIn(forbidden, diagnostic)
        self.assertIn("std::memcmp(textures->uv.data(), t_vs_block + 2 * 16, 16) == 0", self.draw)
        self.assertIn("if (!same) ReportNativeMaterialUvMismatch(tag, *textures, t_vs_block + 2 * 16)", self.draw)

    def test_native_samplers_have_owned_filter_and_address_producers(self):
        core = (ROOT / "src/gpu/scene/native_material_sampler.h").read_text()
        producer = (ROOT / "src/gpu/scene/native_sampler_bridge.cpp").read_text()
        for forbidden in ("be_u32", "bd::mem", "PPCContext", "fetch", "NodeTag"):
            self.assertNotIn(forbidden, core.split("#pragma once", 1)[1])
        self.assertIn("filters.Publish(ImportMaterialFilterDefaults(min, mag, mip)", producer)
        self.assertIn("TrackFilter(slot, field, requested)", producer)
        self.assertIn("filters.Reset(); // unowned production", producer)
        getter = producer.split("std::optional<NativeSamplerFilterPass> FindNativeSamplerFilters(", 1)[1].split("\n}", 1)[0]
        self.assertNotIn("bd::mem", getter)
        self.assertIn("filters.Read(FrameStatFrameCount(), render_view)", getter)
        getter = self.bridge.split("std::optional<NativeMaterialSamplers> FindNativeMaterialSamplers(", 1)[1].split(
            "void NativeMaterialSamplerCheck", 1)[0]
        for forbidden in ("bd::mem", "Word(", "DecodeSamplerRecipe", "ReadCommands", "Video::"):
            self.assertNotIn(forbidden, getter)
        self.assertIn("ComposeMaterialSamplers(range.sampler_addresses, *filters)", getter)
        self.assertIn("if (found && *found != value) return {}", getter)
        self.assertIn("if (!(*samplers)[slot] || !MaterialSamplerImage2D(binding)) return false", self.draw)
        self.assertLess(self.draw.index("values.samplers[slot] = (*samplers)[slot]"),
                        self.draw.index("native_samplers[slot] = MaterialSamplerDesc(*sampler)"))
        self.assertIn("NativeMaterialSamplerNoteDraw()", self.draw)
        self.assertIn("e.native_material_sampler_mask = d.native_material_sampler_mask", self.draw)

    def test_ordered_features_use_live_owners_and_preflight_before_replay(self):
        data = (ROOT / "src/gpu/scene/native_material_data.h").read_text()
        decode = (ROOT / "src/gpu/scene/native_material_data.cpp").read_text()
        packet = (ROOT / "src/gpu/scene/native_object_primitive.h").read_text()
        source = (ROOT / "src/gpu/scene/native_material_texture_source.h").read_text()
        for required in ("NativeMaterialFeatureRecipe", "MaterialDiffuseMode::Unknown", "object.diffuse_enabled",
                         "object.writes_shininess", "pass.specular_enabled", "pass.normal_mapping", "pass.fog_enabled"):
            self.assertIn(required, data)
        self.assertIn("current.features.specular_requested = false", decode)
        self.assertIn("ComposeNativeMaterialFeatures", packet)
        self.assertIn("read(uint64_t(visual) + 3052)", source)
        getter = self.bridge.split("std::optional<NativeMaterialFeatures> FindNativeMaterialFeatures(", 1)[1].split(
            "void NativeMaterialFeatureCheck", 1)[0]
        for forbidden in ("bd::mem", "Word(", "Video::", "DecodeMeshMaterials", "d.bools"):
            self.assertNotIn(forbidden, getter)
        self.assertIn("NativeNodeLightingPass(tag)", getter)
        self.assertIn("if (tag.tech != 0)", getter)
        self.assertIn("if (!value || (found && *found != *value)) return {}", getter)
        self.assertIn("NativeMaterialFeatureCheck(same)", self.draw)
        self.assertIn("values.features = FindNativeMaterialFeatures", self.draw)
        self.assertLess(self.draw.index("values.features = FindNativeMaterialFeatures"),
                        self.draw.index("ApplyNativeMaterialFeatures(*native_values[di].features"))
        self.assertIn("NativeMaterialFeatureNoteDraw()", self.draw)


if __name__ == "__main__":
    unittest.main()
