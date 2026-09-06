"""Ownership wiring guards; CPU contracts and independent runtime checks also required."""
from pathlib import Path
import unittest


class ParameterStorageBoundaryTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.root = Path(__file__).resolve().parents[1]

    def source(self, name):
        return (self.root / "src/gpu" / name).read_text(encoding="utf-8")

    def test_native_owner_has_no_engine_or_disk_dependencies(self):
        owner = self.source("native_parameter_buffer.h")
        for token in ("PPC", "REX", "bd::mem", "D3D", "Xenos", "fstream", "vector<"):
            self.assertNotIn(token, owner)
        self.assertIn("std::array<uint32_t, Words>", owner)
        self.assertIn("std::bitset<Words>", owner)

    def test_upload_and_replay_consume_native_data(self):
        source = self.source("constant_buffers.cpp")
        fetch = source.split("void FetchVertexBlock(")[1].split("void CopyGuestVertexBlock(")[0]
        self.assertIn("CopyNativeParameterBlock(device_guest, true, s.scratchVS)", fetch)
        self.assertIn("CopyNativeParameterBlock(device_guest, false, s.scratchPS)", fetch)
        self.assertNotIn("CopyByteSwap32FlushNaN", fetch)
        replay = self.source("scene/host_draw.cpp").split("static thread_local u64 copied_gen")[1].split("for (size_t di")[0]
        self.assertIn("ForceShaderParameterCopy()", replay)
        self.assertIn("CopyRenderVertexBlock", replay)
        self.assertIn("CopyRenderPixelBlock", replay)
        self.assertNotIn("CopyGuest", replay)

    def test_reference_is_independent_and_fails_mismatch(self):
        source = self.source("constant_buffers.cpp")
        reference = source.split("void CopyGuestVertexBlock(")[1].split("void SnapshotVertexShaderConstants(")[0]
        self.assertEqual(reference.count("CopyByteSwap32FlushNaN"), 2)
        self.assertNotIn("CopyNativeParameterBlock", reference)
        self.assertIn("std::memcmp(reference.data(), out, kConstantBlockBytes)", source)
        self.assertIn('throw std::runtime_error("Native shader parameter storage mismatch")', source)

    def test_producers_publish_their_computed_words(self):
        bridge = self.source("scene/host_parameter_bridge.cpp")
        publish = bridge.split("void Publish() const {")[1].split("void Count() const")[0]
        self.assertIn("words[size_t(offset) / 4] = word", publish)
        self.assertIn("PublishNativeShaderParameters(device, vertex, first, count, words.data())", publish)
        for name in ("scene/native_transform_bridge.cpp", "scene/deferred_consumer.cpp"):
            self.assertIn("PublishNativeShaderParameters(", self.source(name))

    def test_inline_writers_and_ui_are_explicit(self):
        state = self.source("hooks/state.cpp")
        for row in (21, 53):
            for stage in ("true", "false"):
                self.assertIn(f"InvalidateNativeShaderParameters({stage}, {row}, 1)", state)
        # Sorted scheduling remains an explicit original scope; the immediate
        # producer now owns its parameters and geometry, not another import.
        self.assertEqual(state.count("LegacyShaderParameterScope parameter_scope"), 1)
        self.assertIn("DrawNativeImmediateUi(ctx, base)", state)
        self.assertIn("REBLUE_CONSTANT_DIRTY_HOOK(bdVisualObjectSetShaderConstants", state)
        self.assertIn("start <= 31 && count > 31 - start", state)
        self.assertIn("InvalidateNativeShaderParameters(true, 57, 1)", state)
        tweaks = self.source("hooks/tweaks.cpp")
        self.assertIn("InvalidateNativeShaderParameters(true, 50, 2)", tweaks)
        self.assertIn("REBLUE_CONSTANT_DIRTY_HOOK(Visual__Shader__Toon__vf04", state)
        self.assertIn("InvalidateNativeShaderParameters(true, 50, 2)", state)
        for stage in ("true", "false"):
            self.assertIn(f"InvalidateNativeShaderParameters({stage}, kScreenUVScaleReg, 1)", tweaks)

    def test_lifetime_reset_tracks_actual_creation(self):
        source = self.source("hooks/device.cpp")
        self.assertIn("memory->Zero(device_guest, kGuestDeviceSize);\n  bd::gpu::InitializeNativeShaderParameters(device_guest);", source)
        reset = source.split("u32 D3DDevice_SetResolution_hook")[1]
        self.assertNotIn("InitializeNativeShaderParameters(", reset)


if __name__ == "__main__":
    unittest.main()
