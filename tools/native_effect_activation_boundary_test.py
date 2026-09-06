"""Source boundaries complement the actual CPU core and desktop runtime tests."""
from pathlib import Path
import unittest


class EffectActivationBoundaryTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        root = Path(__file__).resolve().parents[1]
        cls.bridge = (root / "src/gpu/scene/native_effect_activation_bridge.cpp").read_text(encoding="utf-8")
        cls.core = (root / "src/gpu/scene/native_effect_activation.h").read_text(encoding="utf-8")
        cls.array = (root / "src/gpu/scene/native_registry_array.h").read_text(encoding="utf-8")

    def test_all_three_whole_function_hooks_exist(self):
        for function in ("sub_82173DF8", "sub_8221D678", "sub_8221D9A8"):
            self.assertIn(f"REX_HOOK_RAW({function})", self.bridge)
        self.assertIn("ApplyRenderFeature(adapter, feature, value)", self.bridge)
        self.assertIn("RegisterEffectParticipant(adapter, participant)", self.bridge)
        self.assertIn("UnregisterEffectParticipant(adapter, participant)", self.bridge)

    def test_core_does_not_import_engine_or_console_state(self):
        for text in (self.core, self.array):
            for token in ("PPC", "REX", "bd::mem", "D3D", "kRegistry", "vtable"):
                self.assertNotIn(token, text)

    def test_array_mutation_does_not_call_old_container_algorithms(self):
        for function in ("sub_8221DD10", "sub_8221DDA8", "sub_8221DE20", "sub_8221DF78", "bdEffectSlotArrayResize"):
            self.assertNotIn(function, self.bridge)
        for function in ("InsertRegistryEntry", "AppendRegistryEntry", "EraseRegistryEntry"):
            self.assertIn(function, self.bridge)
        self.assertIn("imported flags, metadata and array storage", self.bridge)

    def test_refusal_does_not_replay_partly_executed_effects(self):
        for function, work in (("sub_82173DF8", "ApplyRenderFeature"),
                               ("sub_8221D678", "RegisterEffectParticipant"),
                               ("sub_8221D9A8", "UnregisterEffectParticipant")):
            body = self.bridge.split(f"REX_HOOK_RAW({function})", 1)[1].split("REX_HOOK_RAW", 1)[0]
            self.assertLess(body.index("++stats.compatibility"), body.index(f"__imp__{function}"))
            self.assertLess(body.index(f"__imp__{function}"), body.index(work))
            self.assertNotIn("catch", body)


if __name__ == "__main__":
    unittest.main()
