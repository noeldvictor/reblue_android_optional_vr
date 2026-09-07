"""Wiring checks complement the production C++ owner/selector and runtime tests."""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[1]


class VertexInputBoundaryTest(unittest.TestCase):
    def read(self, path):
        return (ROOT / path).read_text(encoding="utf-8")

    def test_import_owns_input_before_geometry_publication(self):
        text = self.read("src/gpu/scene/native_mesh.cpp")
        self.assertLess(text.index("s.vertex_inputs.Resolve("), text.index("auto result = Upload("))
        self.assertIn("result->vertex_input = std::move(vertex_input);", text)
        self.assertIn("if (!vertex_input) return {};", text)

    def test_native_dispatch_clears_declaration_and_publishes_owned_strides(self):
        text = self.read("src/gpu/scene/host_draw.cpp").split("if (const auto &mesh = d.native_geometry)", 1)[1]
        for statement in ("s.pipelineState.vertexDeclaration = nullptr;",
                          "s.pipelineState.native_vertex_input = mesh->vertex_input.get();",
                          "s.native_draw_pipeline = &s.pipelineState;",
                          "s.input_slots[slot].stride = mesh->strides[slot];"):
            self.assertIn(statement, text)

    def test_all_terminal_consumers_select_owned_input(self):
        self.assertIn("scene::VertexInputElements(state.native_vertex_input", self.read("src/gpu/pipeline/pipeline_cache.cpp"))
        self.assertIn("scene::VertexInputDecode(", self.read("src/gpu/constant_buffers.cpp"))
        self.assertIn("native ? VertexPullInputId(native) : VertexPullDeclId(", self.read("src/gpu/vertex_pull.cpp"))
        self.assertIn("(!s.pipelineState.native_vertex_input && !s.pipelineState.vertexDeclaration)", self.read("src/gpu/draw.cpp"))

    def test_native_rows_never_enter_console_pso_csv(self):
        text = self.read("src/gpu/pipeline/pso_recorder.cpp").split("void RecordPipelineState(", 1)[1]
        self.assertLess(text.index("if (state.native_vertex_input) return;"), text.index("ShaderHash("))

    def test_engine_intent_clears_native_input(self):
        self.assertIn("assign(state.pipelineState.native_vertex_input, nullptr);", self.read("src/gpu/draw_intent.h"))

    def test_core_has_no_resource_memory_or_disk_dependency(self):
        text = self.read("src/gpu/scene/native_vertex_input.h")
        for token in ("GuestVertex", "REX_LOAD", "bd::mem", "PPCContext", "ofstream", "filesystem", "Video::"):
            self.assertNotIn(token, text)


if __name__ == "__main__":
    unittest.main()
