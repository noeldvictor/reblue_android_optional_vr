"""Wiring checks complement the production C++ owner/selector and runtime tests."""
from pathlib import Path
import re
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

    def test_canonical_shader_adapter_matches_existing_signature(self):
        shader = self.read("thirdparty/XenosRecomp/XenosRecomp/shader_common.h")
        macro = shader.split("#define REBLUE_VERTEX_INPUT_LOCATIONS(X)", 1)[1].split("\n\n", 1)[0]
        expected = re.findall(r"X\((\w+),\s*(\d+),\s*(\d+)\)", macro)
        adapter = self.read("src/gpu/scene/native_mesh_cook.cpp")
        actual = re.findall(r'\{MeshSemantic::(\w+), "\w+", (\d+), (\d+)\}', adapter)
        self.assertGreater(len(expected), 0)
        self.assertEqual(actual, expected)

    def test_canonical_dispatch_disables_old_packed_normal_specialization(self):
        text = self.read("src/gpu/scene/host_draw.cpp")
        self.assertIn("if (mesh->canonical_vertices)\n          s.pipelineState.specConstants &= ~kSpecConstantR11G11B10Normal;", text)

    def test_source_free_load_and_native_file_contract(self):
        load = self.read("src/gpu/scene/native_mesh.cpp").split("LoadNativeGeometry(u64 content_id)", 1)[1].split("void NativeMeshNoteDraw", 1)[0]
        for required in ("DiskCache().Read(content_id, data)", "NativeMeshContentId(data) != content_id",
                         "RigidMeshVertexInput(data, s.vertex_inputs)", "Upload(s, data, content_id"):
            self.assertIn(required, load)
        for forbidden in ("Guest", "import_aliases", "ImportNativeMesh", "declaration"):
            self.assertNotIn(forbidden, load)
        data = self.read("src/gpu/scene/native_mesh_data.h")
        for forbidden in ("RenderFormat", "VertexShaderDecode", "GuestVertex", "semanticName", "location"):
            self.assertNotIn(forbidden, data.replace("not shader locations", "not shader slots"))

    def test_synthetic_pulling_uses_owned_storage_and_zero_stride(self):
        text = self.read("src/gpu/vertex_pull.cpp")
        self.assertIn("plume::RenderBufferFlag::VERTEX | plume::RenderBufferFlag::STORAGE", text)
        self.assertIn("native->PullStreams()", text)
        self.assertIn("synthetic ? p.dummy_view : s.vertex_views[i]", text)
        self.assertIn("synthetic ? 0 : s.input_slots[i].stride", text)
        self.assertIn("if (!mapped)\n      return false;", text)
        owner = self.read("src/gpu/scene/native_vertex_input.h")
        self.assertIn("input->pull_[element.location] = entry;", owner)
        self.assertIn("element.alignedByteOffset != 0", owner)


if __name__ == "__main__":
    unittest.main()
