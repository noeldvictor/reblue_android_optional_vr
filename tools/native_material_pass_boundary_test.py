"""Native material scheduling/binding boundaries; CPU/live desktop checks are also required."""
from pathlib import Path
import unittest


class MaterialPassBoundaryTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        root = Path(__file__).resolve().parents[1]
        cls.core = (root / "src/gpu/scene/native_material_pass.h").read_text(encoding="utf-8")
        cls.bridge = (root / "src/gpu/scene/native_material_pass_bridge.cpp").read_text(encoding="utf-8")
        cls.binding = (root / "src/gpu/draw_bindings.cpp").read_text(encoding="utf-8")
        cls.state = (root / "src/gpu/hooks/state.cpp").read_text(encoding="utf-8")

    def test_native_core_has_no_console_or_resource_header_inputs(self):
        for token in ("REX", "PPC", "Guest", "D3D", "bd::mem", "fstream"):
            self.assertNotIn(token, self.core)
        self.assertIn("kMaterialParticipantLimit = 256", self.core)
        self.assertIn("port.ActiveMode()", self.core)
        self.assertLess(self.core.index("SelectMaterialRecipe(port, mode);"), self.core.index("InvokeMaterialParticipant(port, false);"))

    def test_all_five_original_bodies_are_replaced(self):
        for name in ("bdBeginRenderPass", "bdEndRenderPass", "bdRenderPassSetTextureState",
                     "bdInitDefaultTextures", "bdSetVertexDeclarationCached"):
            self.assertIn(f"NATIVE_MATERIAL_HOOK({name},", self.bridge)
        adapter = self.bridge.split("struct Adapter", 1)[1].split("bool Prepare(", 1)[0]
        for token in ("D3DDevice_", "__imp__", "LegacyShaderParameterScope"):
            self.assertNotIn(token, adapter)
        for token in ("Video::SetVertexShader(shader)", "Video::SetPixelShader(shader)",
                      "Video::SetVertexDeclaration(declaration)", "ctx.r1.u64 = saved_stack"):
            self.assertIn(token, adapter)
        self.assertIn("++stats.callbacks", adapter)
        self.assertNotIn("catch", self.bridge)

    def test_shader_and_declaration_publication_follow_the_bind(self):
        self.assertLess(self.core.index("port.BindDeclaration(declaration)"), self.core.index("port.CacheDeclaration(declaration)"))
        self.assertLess(self.core.index("port.BindShader(stage, port.Shader(recipe))"), self.core.index("port.CacheShader(stage, port.Shader(recipe))"))
        end = self.core.split("void EndMaterialPass(", 1)[1]
        self.assertLess(end.index("InvokeMaterialParticipant(port, true)"), end.index("port.SavedShaderRecipe(0)"))
        self.assertNotIn("port.SetActiveMode", end)

    def test_declaration_specialization_is_shared_by_native_and_compatibility_callers(self):
        binding = self.binding.split("void Video::SetVertexDeclaration(", 1)[1].split("void Video::NoteStreamSource", 1)[0]
        for token in ("kSpecConstantR11G11B10Normal", "decl->hasR11G11B10Normal", "SetDirtyValue<u32>"):
            self.assertIn(token, binding)
        self.assertLess(binding.index("std::lock_guard lock"), binding.index("s.pipelineState.specConstants"))
        adapter = self.state.split("void D3DDevice_SetVertexDeclaration_hook(", 1)[1].split("void D3DDevice_SetTexture_hook", 1)[0]
        self.assertIn("Video::SetVertexDeclaration(decl)", adapter)
        self.assertNotIn("kSpecConstantR11G11B10Normal", adapter)


if __name__ == "__main__":
    unittest.main()
