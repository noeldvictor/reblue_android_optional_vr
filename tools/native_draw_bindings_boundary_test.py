"""Queue wiring guards; behavior lives in host_draw_intent_test, pixels are separate."""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[1]


class DrawBindingsBoundaryTest(unittest.TestCase):
    def test_emitter_has_no_global_descriptor_or_three_offset_abi_lookup(self):
        queue = (ROOT / "src/gpu/draw_queue.cpp").read_text()
        emitter = queue.split("bool EmitBindings(", 1)[1].split("void EmitOne(", 1)[0]
        self.assertIn("ApplyGraphicsBindings(*cmd, d.bindings, st.bindings)", emitter)
        for forbidden in ("ConstantDescriptorSet", "constant_offsets", "kConstantDescriptorSetIndex", "state()"):
            self.assertNotIn(forbidden, emitter)
        core = (ROOT / "src/gpu/draw_bindings.h").read_text()
        for forbidden in ("Video::", "PPCContext", "bd::mem", "GuestShader", "NodeTag"):
            self.assertNotIn(forbidden, core)

    def test_snapshot_is_produced_before_queue_and_used_for_grouping(self):
        producer = (ROOT / "src/gpu/draw.cpp").read_text()
        queue = (ROOT / "src/gpu/draw_queue.cpp").read_text()
        self.assertIn("s.pending.bindings = EngineGraphicsBindings(s);", producer)
        self.assertIn("b.binding_key = d.bindings.Key(0);", queue)
        self.assertIn("b.binding_key = q.bindings.Key(0);", queue)
        self.assertGreaterEqual(queue.count(".bindings.Matches("), 3)
        self.assertIn("!draw.translated_instance_records", queue)

    def test_flush_restores_entry_bindings_and_fixture_is_built(self):
        queue = (ROOT / "src/gpu/draw_queue.cpp").read_text().split("void DrawQueueFlush(", 1)[1]
        self.assertLess(queue.index("const auto resume_bindings = EngineGraphicsBindings(state());"),
                        queue.index("EmitState st;"))
        self.assertIn("ApplyGraphicsBindings(*cmd, resume_bindings, st.bindings)", queue)
        build = (ROOT / "tools/native_texture_test/CMakeLists.txt").read_text()
        self.assertIn("host_draw_intent_test draw_intent.cpp draw_bindings.cpp", build)

    def test_immediate_geometry_does_not_use_logical_dirty_flags_as_gpu_state(self):
        producer = (ROOT / "src/gpu/draw.cpp").read_text()
        self.assertIn("ApplyImmediateGeometryBindings(*s.command_list, s.current_pso", producer)
        for old_gate in ("s.command_list->setPipeline(pso)", "s.dirtyStates.indices &&",
                         "s.command_list->setVertexBuffers("):
            self.assertNotIn(old_gate, producer)
        core = (ROOT / "src/gpu/draw_geometry_bindings.h").read_text()
        for forbidden in ("dirtyStates", "state()", "PPCContext", "bd::mem"):
            self.assertNotIn(forbidden, core)
        self.assertIn("draw_geometry.cpp", (ROOT / "tools/native_texture_test/CMakeLists.txt").read_text())
        pixels = (ROOT / "tools/native_scene_snapshot_test/rigid.cpp").read_text()
        self.assertIn("ApplyImmediateGeometryBindings(*commands", pixels)
        self.assertIn("Run(device,0,stale,false)", pixels)
        self.assertIn("Run(device,0,stale,true)", pixels)


if __name__ == "__main__":
    unittest.main()
