"""Wiring guards for owned primitive programs and whole-node replay invalidation."""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[1]


class PrimitivePolicyBoundaryTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.draw = (ROOT / "src/gpu/scene/host_draw.cpp").read_text()
        cls.bridge = (ROOT / "src/gpu/scene/native_material_texture_bridge.cpp").read_text()

    def test_load_compiles_and_budgets_owned_steps(self):
        loader = (ROOT / "src/gpu/scene/native_material.cpp").read_text()
        self.assertIn("&result.policy_steps", loader)
        owner = (ROOT / "src/gpu/scene/native_model_materials.cpp").read_text()
        self.assertIn("policy_steps.capacity()", owner)
        self.assertIn("range.policy_step_end > mesh.program.policy_steps.size()", owner)

    def test_pure_program_is_independent_of_source_and_gpu(self):
        core = (ROOT / "src/gpu/scene/native_primitive_policy.h").read_text()
        for token in ("NodeTag", "PPCContext", "REX_", "bd::mem", "plume::", "GuestTexture"):
            self.assertNotIn(token, core)
        self.assertIn("steps.size() > 65536", core)
        self.assertIn("out = std::move(values)", core)

    def test_object_publication_owns_inputs_before_consumer(self):
        producer, consumer = self.bridge.split("NativeObjectTextureState::Mesh *PrepareMaterialMesh(", 1)
        self.assertIn("ReadPrimitivePolicyInputs(context, *visual, Word)", producer)
        self.assertNotIn("ReadPrimitivePolicyInputs", consumer)
        self.assertNotIn("Word(", consumer)
        self.assertIn("ComposePrimitivePolicies", consumer)
        self.assertIn("sizeof(NativePrimitivePolicy)", consumer)

    def test_refresh_occurs_before_either_half_can_issue(self):
        hook = (ROOT / "src/gpu/hooks/scene_node.cpp").read_text()
        self.assertLess(hook.index("HostRefreshPrimitivePolicy(tag)"), hook.index("HostListBuildStatus(tag)"))
        refresh = self.draw.split("void HostRefreshPrimitivePolicy(", 1)[1].split("bool HostDrawHasDrawTemplate", 1)[0]
        for required in ("PrimitivePlanMatches(direct->second", "PrimitivePlanMatches(deferred->second",
                         "st.templates.erase(key)", "st.lists.erase(key)", "st.never.erase(key)"):
            self.assertIn(required, refresh)
        self.assertNotIn("DispatchHostNodeDraw", refresh)
        self.assertNotIn("AppendDeferredEntries", refresh)

    def test_replay_preflights_and_uses_current_cull_owner(self):
        self.assertIn("values.policy = FindNativePrimitivePolicy", self.draw)
        self.assertIn("values.policy->routing_known && !values.policy->direct", self.draw)
        self.assertIn("s.pipelineState.cullMode = cull", self.draw)
        self.assertIn("s.native_draw_pipeline = &s.pipelineState", self.draw)
        self.assertIn("NativePrimitivePolicyNoteDraw", self.draw)
        capture = self.draw.split("void HostListBuildCapture", 1)[1].split("bool HostSceneEye", 1)[0]
        self.assertIn("count_after == count_before", capture)
        self.assertIn("st.lists.erase(KeyOf(tag))", capture)


if __name__ == "__main__":
    unittest.main()
