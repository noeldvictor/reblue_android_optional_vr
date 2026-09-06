"""UI ownership wiring; executable publication/geometry fixtures also required."""
from pathlib import Path
import unittest


class ImmediateUiBoundaryTest(unittest.TestCase):
    def source(self, name):
        return (Path(__file__).resolve().parents[1] / "src/gpu" / name).read_text(encoding="utf-8")

    def test_owner_is_bounded_address_free_cpu_data(self):
        source = self.source("native_ui_vertices.h")
        for forbidden in ("PPC", "REX", "bd::mem", "D3D", "fstream", "vector<"):
            self.assertNotIn(forbidden, source)
        self.assertIn("std::array<uint32_t, size_t(Capacity) * kImmediateUiWords>", source)
        self.assertIn("count_ = 0", source)

    def test_normal_path_publishes_owned_values_and_draws_directly(self):
        source = self.source("hooks/native_ui.cpp").split("plan->Apply(")[1]
        self.assertIn("PublishNativeShaderParameters(plan->device, false, 3, 1, plan->colour.data())", source)
        self.assertIn("PublishNativeShaderParameters(plan->device, true, 20, 1, plan->translation.data())", source)
        self.assertIn("DispatchHostImmediateUi(plan->device, vertices->Words())", source)
        for forbidden in ("__imp__", "BeginVertices", "EndVertices", "SystemHeap", "CopyGuest"):
            self.assertNotIn(forbidden, source)

    def test_host_upload_has_no_guest_scratch_or_cpu_readback(self):
        source = self.source("hooks/draw.cpp").split("bool DispatchHostImmediateUi(")[1].split("void DispatchHostNodeDraw")[0]
        self.assertIn("UploadHostBytes(words.data()", source)
        self.assertIn('"NativeImmediateUI"', source)
        for forbidden in ("UploadAndBindUpVertices", "bd::mem", "SystemHeap", "memcpy", "upload.memory["):
            self.assertNotIn(forbidden, source)

    def test_original_observer_runs_before_upload(self):
        source = self.source("hooks/draw.cpp").split("u32 D3DDevice_EndVertices_hook")[1].split("u32 D3DDevice_DrawIndexedVertices_hook")[0]
        self.assertLess(source.index("ObserveOriginalImmediateUi"), source.index("UploadAndBindUpVertices"))
        observer = self.source("hooks/native_ui.cpp").split("void ObserveOriginalImmediateUi(")[1].split("void DrawNativeImmediateUi(")[0]
        self.assertIn("reference->plan.Matches(Word)", observer)
        self.assertIn("Compare(original == reference->vertices[i])", observer)
        self.assertNotIn("plan->Apply", observer)

    def test_scope_restores_overlay_on_every_exit(self):
        source = self.source("hooks/native_ui.cpp")
        self.assertIn("~Scope()", source)
        self.assertIn("state().overlay2DScope = outer_overlay", source)
        self.assertIn("active = outer_active; reference = outer_reference", source)
        self.assertIn("throw std::runtime_error", source)
        self.assertIn("stats.refused += enabled", source)


if __name__ == "__main__":
    unittest.main()
