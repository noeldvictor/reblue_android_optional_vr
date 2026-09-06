"""Whole-callback and bounded-publication guards; runtime comparisons remain required."""
from pathlib import Path
import unittest


class WaterUpdateBoundaryTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        root = Path(__file__).resolve().parents[1]
        cls.core = (root / "src/gpu/scene/native_water_update.h").read_text(encoding="utf-8")
        cls.bridge = (root / "src/gpu/scene/native_water_update_bridge.cpp").read_text(encoding="utf-8")
        cls.importer = (root / "src/gpu/scene/water_material_import.h").read_text(encoding="utf-8")

    def test_native_policy_has_no_console_addresses_or_dispatch(self):
        for token in ("PPC", "REX", "bd::mem", "D3D", "Xenos", "0x82"):
            self.assertNotIn(token, self.core)

    def test_whole_callbacks_have_no_normal_original_execution(self):
        for name in ("sub_82454398", "sub_8221D460"):
            self.assertIn(f"REX_HOOK_RAW({name})", self.bridge)
        native = self.bridge.split("plan->Apply(", 1)[1]
        self.assertNotIn("original(ctx", native)
        self.assertNotIn("catch", self.bridge)
        self.assertIn("ReferenceScope reference", self.bridge)
        self.assertIn("if (reference_execution) { original(ctx, base); return; }", self.bridge)

    def test_time_uses_existing_tick_gate_and_single_strict_wrap(self):
        self.assertIn("bd::engine::TickDue()", self.bridge)
        self.assertIn("if (tick)", self.importer)
        self.assertIn("if (next > limit)", self.importer)
        self.assertNotIn("fmod", self.importer)

    def test_overlay_is_bounded_and_does_not_allocate_or_dump(self):
        self.assertIn("std::array<WaterWordWrite, 32>", self.importer)
        self.assertIn("plan.count == plan.writes.size()", self.importer)
        self.assertIn("for (uint32_t i = plan.count; i > 0; --i)", self.importer)
        for token in ("std::vector", "ofstream", "memcpy", "REX_CALL", "D3DDevice_"):
            self.assertNotIn(token, self.importer + self.bridge)

    def test_diagnostics_compare_final_aliases_and_return_value(self):
        self.assertIn("ctx.r3.u64 != result || !plan->Matches(ReadWord)", self.bridge)
        self.assertIn("REXCVAR_DEFINE_BOOL(bd_native_water_verify, false", self.bridge)
        self.assertIn("writes[j].address != writes[i].address", self.importer)
        self.assertIn("checked {} wrong {}", self.bridge)


if __name__ == "__main__":
    unittest.main()
