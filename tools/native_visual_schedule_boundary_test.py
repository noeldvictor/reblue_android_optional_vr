"""Whole sorted scheduler wiring; CPU oracle and pixel/runtime checks also required."""
from pathlib import Path
import unittest


class VisualScheduleBoundaryTest(unittest.TestCase):
    def source(self, name):
        return (Path(__file__).resolve().parents[1] / "src/gpu" / name).read_text(encoding="utf-8")

    def test_order_is_bounded_and_address_free(self):
        core = self.source("scene/native_visual_schedule.h")
        for token in ("PPC", "REX", "bd::mem", "D3D", "Xenos", "fstream", "vector<"):
            self.assertNotIn(token, core)
        self.assertIn("std::array<uint64_t, kSortedPrimitiveLimit>", core)
        self.assertIn("count_ = 0", core)
        self.assertIn("std::greater<uint64_t>()", core)

    def test_whole_hook_does_not_import_full_parameter_blocks(self):
        state = self.source("hooks/state.cpp")
        body = state.split("REX_HOOK_RAW(Visual__DrawSortedQueues) {")[1].split("}")[0]
        self.assertIn("DrawNativeSortedVisuals(ctx, base)", body)
        self.assertNotIn("__imp__", body)
        self.assertNotIn("LegacyShaderParameterScope", body)

    def test_normal_adapter_has_no_guest_sort_or_blend_helpers(self):
        source = self.source("hooks/native_visual_schedule.cpp")
        adapter = source.split("struct Adapter {")[1].split("bool Prepare(")[0]
        for token in ("__imp__", "kQueues + 28", "bdMatrixCopy(ctx", "bdGetCurrentThreadBuffer(",
                      "sub_82425CD0", "rexcrt_memset", "LegacyShaderParameterScope"):
            self.assertNotIn(token, adapter)
        self.assertIn("PublishNativeShaderParameters(plan->device, false, 3, 1, plan->colour.data())", adapter)
        self.assertIn("Call(DrawNativeImmediateUi", adapter)
        self.assertIn("ctx.r1.u64 = saved_stack", adapter)

    def test_live_queues_and_deferred_output_are_not_dropped(self):
        core = self.source("scene/native_visual_schedule.h")
        self.assertLess(core.index("port.Model(uint32_t(key))"), core.index("port.PrimitiveCount()"))
        self.assertLess(core.index("port.SortPrimitives(order)"), core.index("port.PrepareSharedMaterial()"))
        self.assertLess(core.index("port.Primitive(index)"), core.index("deferred = port.DeferredCount();", core.index("port.Primitive(index)")))
        self.assertIn("deferred < kSortedDeferredLimit", core)
        self.assertIn("port.ResetColour();\n  port.EndPrimitives();", core)


if __name__ == "__main__":
    unittest.main()
