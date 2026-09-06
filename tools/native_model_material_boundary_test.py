"""Loader/consumer wiring guards. CPU lifetimes and live pixels remain required."""
from pathlib import Path
import unittest


class ModelMaterialBoundaryTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        root = Path(__file__).resolve().parents[1]
        cls.bridge = (root / "src/gpu/scene/native_material.cpp").read_text(encoding="utf-8")
        cls.core = (root / "src/gpu/scene/native_model_materials.cpp").read_text(encoding="utf-8")
        cls.draw = (root / "src/gpu/scene/host_draw.cpp").read_text(encoding="utf-8")

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
        lookup = self.bridge.split("FindCommands(const NodeTag &tag) {", 1)[1].split("} // namespace", 1)[0]
        self.assertIn("Models().Find", lookup)
        for forbidden in ("ReadCommands", "DecodeMeshMaterials", "Resolve(",
                          "PhysicalBufferGeneration", "emplace", "tag.mesh_va +"):
            self.assertNotIn(forbidden, lookup)
        self.assertEqual(self.bridge.count("const auto commands = FindCommands(tag);"), 4)
        self.assertNotIn("thread_local std::unordered_map", self.bridge)

    def test_load_owns_geometry_before_publication(self):
        publish = self.bridge.split("bool PublishModelMaterials(uint32_t graph)", 1)[1].split("FindCommands", 1)[0]
        self.assertLess(publish.index("LoadModelGeometry(meshes.back());"),
                        publish.index("Models().Publish"))
        loader = self.bridge.split("void LoadModelGeometry", 1)[1].split("bool PublishModelMaterials", 1)[0]
        self.assertIn("ImportNativeMesh(request)", loader)
        self.assertIn("ReadModelGeometrySource", loader)
        self.assertNotIn("PrecacheEnabled", loader)

    def test_consumers_do_not_read_buffer_association_tables(self):
        consumers = self.bridge.split("bool ModelOwnsReflectionBinding", 1)[1]
        for forbidden in ("tag.mesh_va +", "range.index_record *", "range.vertex_record *"):
            self.assertNotIn(forbidden, consumers)
        self.assertEqual(consumers.count("ModelPrimitiveMatches("), 4)

    def test_geometry_lookup_cannot_import_or_touch_source_tables(self):
        lookup = self.bridge.split("FindLoadedNativeGeometry(", 1)[1].split("bool ModelOwnsReflectionBinding", 1)[0]
        for forbidden in ("bd::mem", "ImportNativeMesh", "ResolveGuestBuffer", "ReadModelGeometrySource"):
            self.assertNotIn(forbidden, lookup)
        self.assertIn("binding.layout != layout", lookup)
        self.assertIn("binding.stride != stride", lookup)

    def test_replay_uses_loaded_geometry_before_legacy_import(self):
        replay = self.draw.split("const auto import_geometry =", 1)[1]
        self.assertLess(replay.index("FindLoadedNativeGeometry(tag"),
                        replay.index("d.native_geometry = import_geometry();"))
        self.assertIn("if (!d.native_geometry)", replay)
        self.assertIn("d.stream_offset[0] == 0", replay)
        self.assertIn("NativeModelGeometryNoteDraw", replay)

    def test_geometry_verification_samples_warm_frames_without_disk_writes(self):
        replay = self.draw.split("const auto import_geometry =", 1)[1]
        self.assertIn("request.persist = !REXCVAR_GET(bd_native_materials_verify);", replay)
        self.assertIn("new_geometry || t_geometry_checked_frame != frame", replay)
        self.assertIn("d.geometry_load_owned && REXCVAR_GET(bd_native_materials_verify)", replay)
        self.assertIn("NativeModelGeometryCheck(imported == d.native_geometry)", replay)

    def test_registry_accounts_for_geometry_and_source_association_leases(self):
        for field in ("mesh.program.geometries.capacity()", "mesh.source_bindings.capacity()",
                      "mesh.program.geometries.size()", "mesh.source_bindings.size()"):
            self.assertIn(field, self.core)

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
