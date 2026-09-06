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
        cls.lifecycle = (root / "src/gpu/scene/native_effect_lifecycle.h").read_text(encoding="utf-8")

    def test_all_three_whole_function_hooks_exist(self):
        for function in ("sub_82173DF8", "sub_8221D678", "sub_8221D9A8"):
            self.assertIn(f"REX_HOOK_RAW({function})", self.bridge)
        self.assertIn("ApplyRenderFeature(adapter, feature, value)", self.bridge)
        self.assertIn("RegisterEffectParticipant(adapter, participant)", self.bridge)
        self.assertIn("UnregisterEffectParticipant(adapter, participant)", self.bridge)

    def test_core_does_not_import_engine_or_console_state(self):
        for text in (self.core, self.array, self.lifecycle):
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

    def test_preparation_cleanup_and_teardown_are_whole_function_hooks(self):
        for function, work in (("sub_8221D530", "TryPrepareEffect"),
                               ("sub_8221DB00", "TryPrepareEffect"),
                               ("sub_8221D548", "TryFinishEffect"),
                               ("sub_8221DBE0", "TryPrepareEffect"),
                               ("sub_8221DCA0", "TryFinishEffect"),
                               ("sub_8221D5E0", "TryDestroyEffectRegistry")):
            body = self.bridge.split(f"REX_HOOK_RAW({function})", 1)[1].split("REX_HOOK_RAW", 1)[0]
            self.assertIn(f"if (!{work}(", body)
            self.assertEqual(body.count(f"__imp__{function}(ctx, base)"), 1)
            self.assertNotIn("catch", body)

    def test_lifecycle_refuses_only_before_execution(self):
        for function, next_function, work in (
                ("TryPrepareEffect", "TryFinishEffect", "PreparationAdapter adapter"),
                ("TryFinishEffect", "TryDestroyEffectRegistry", "PreparationAdapter adapter"),
                ("TryDestroyEffectRegistry", "struct ActivationAdapter", "RegistryAdapter adapter")):
            body = self.bridge.split(f"bool {function}(", 1)[1].split(next_function, 1)[0]
            self.assertEqual(body.count("return false;"), 1)
            self.assertLess(body.index("return false;"), body.index(work))
            self.assertNotIn("catch", body)

    def test_lifecycle_imports_are_counted_and_native_core_is_wired(self):
        for function in ("PrepareEffectParticipants", "PrepareEffectModel", "FinishEffectParticipants",
                         "FinishEffectModel", "DestroyEffectRegistry"):
            self.assertIn(f"{function}(adapter)", self.bridge)
        self.assertIn("[native-effect-lifecycle]", self.bridge)
        self.assertIn("imported callbacks, identities and shared storage", self.bridge)
        self.assertIn("ctx.last_indirect_target = address", self.bridge)


if __name__ == "__main__":
    unittest.main()
