"""Toon producer wiring; exact CPU/reference and pixel qualification remain required."""
from pathlib import Path
import unittest


class ToonMaterialBoundaryTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        root = Path(__file__).resolve().parents[1]
        cls.core = (root / 'src/gpu/scene/native_toon_material.h').read_text(encoding='utf-8')
        cls.bridge = (root / 'src/gpu/scene/native_toon_material_bridge.cpp').read_text(encoding='utf-8')
        cls.passes = (root / 'src/gpu/scene/native_material_pass_bridge.cpp').read_text(encoding='utf-8')

    def test_native_core_has_no_resource_or_console_dependencies(self):
        for token in ('PPC', 'REX', 'bd::mem', 'D3D', 'Guest', 'fstream', 'vector<'):
            self.assertNotIn(token, self.core)
        self.assertIn('std::bit_cast<int32_t>(counter) / 6', self.core)
        self.assertLess(self.core.index('port.BindTexture(6'), self.core.index('port.BindTexture(7'))
        self.assertLess(self.core.index('port.BindTexture(7'), self.core.index('port.SetCounter'))

    def test_complete_callbacks_and_direct_participant_dispatch(self):
        for name in ('sub_821837B0', 'Visual__Shader__Toon__vf04', 'sub_82183990'):
            self.assertIn(f'NATIVE_TOON_HOOK({name},', self.bridge)
        invoke = self.passes.split('void Invoke(', 1)[1].split('bool Prepare(', 1)[0]
        self.assertLess(invoke.index('TryNativeToonMaterial'), invoke.index('GetFunction'))
        self.assertIn('++stats.native_callbacks', invoke)
        self.assertIn('++stats.callbacks', invoke)

    def test_direct_texture_and_computed_native_parameter_publication(self):
        animation = self.bridge.split('struct AnimationPort', 1)[1].split('bool Ready(', 1)[0]
        self.assertNotIn('D3DDevice_', animation)
        self.assertIn('Video::SetTexture(slot, texture)', animation)
        self.assertIn('GetOrCreateDebugTexture()', animation)
        self.assertIn('PublishNativeShaderParameters(device, true, 50, 2, words.data())', self.bridge)
        self.assertIn('BuildToonEdgeParameters(authored,', self.bridge)
        self.assertIn('two inherited edge words', self.bridge)

    def test_original_leaf_comparison_and_explicit_fallback(self):
        self.assertIn('bd_native_toon_material_verify, false', self.bridge)
        self.assertIn('__imp__Visual__Shader__Toon__vf04(ctx, base)', self.bridge)
        self.assertIn('if (!same) { ++stats.wrong; Check(false); }', self.bridge)
        fallback = self.bridge.split('void ToonFallback(', 1)[1]
        self.assertIn('InvalidateNativeShaderParameters(true, 50, 2)', fallback)
        self.assertNotIn('catch', self.bridge)


if __name__ == '__main__':
    unittest.main()
