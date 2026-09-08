"""Source guards for frame-identified screenshot ownership; pixels are separate."""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[1]


class ScreenshotBoundaryTest(unittest.TestCase):
    def test_readback_retires_at_real_slot_fence_not_present_counter(self):
        source = (ROOT / "src/gpu/screenshot.cpp").read_text()
        self.assertNotIn("g_present_index", source)
        self.assertNotIn("g_ready_at", source)
        ring = (ROOT / "src/gpu/frame_ring.cpp").read_text()
        self.assertIn("CollectScreenshotAfterFence(s, slot)", ring.split("void DrainSlot", 1)[1])
        collect = source.split("void CollectScreenshotAfterFence", 1)[1].split("void ServiceOnPresent", 1)[0]
        self.assertLess(collect.index("CollectAfterFence()"), collect.index("EncodeJpeg("))
        self.assertLess(collect.index("EncodeJpeg("), collect.index("WriteProbe("))

    def test_probe_is_opt_in_bounded_and_never_overwrites(self):
        source = (ROOT / "src/gpu/screenshot.cpp").read_text()
        for required in ("REXCVAR_DEFINE_BOOL(bd_native_frame_probe, false", "g_probe_attempted = true",
                         "std::array<char,65>", "probe->Accept(frame)", "ScreenshotPlan::kBudget-used",
                         "CREATE_NEW", "110u*1024", "capture->frame", "capture->input", "capture->output"):
            self.assertIn(required, source)
        self.assertNotIn(".raw", source)
        present = (ROOT / "src/gpu/present.cpp").read_text()
        call = present.index("ServiceOnPresent(")
        self.assertLess(present.rfind("drawInstanced(3, 1, 0, 0)", 0, call), call)
        self.assertIn("gamma_src_desc", present[call:call+200])

    def test_actual_copy_has_host_barrier_invalidation_and_row_pitch(self):
        source = (ROOT / "src/gpu/screenshot_readback.cpp").read_text()
        for required in ("VK_ACCESS_HOST_READ_BIT", "VK_PIPELINE_STAGE_HOST_BIT", "vmaInvalidateAllocation",
                         "plan_.pitch/4", "uint64_t(y)*plan_.pitch", "source[2]", "recorded_ || !buffer_",
                         "!recorded_ || collected_"):
            self.assertIn(required, source)
        encoder = (ROOT / "src/gpu/screenshot_jpeg.cpp").read_text()
        self.assertIn("InitializeFromMemory(encoded.data(),DWORD(encoded.size()))", encoder)
        self.assertIn("SetSize(capture.width,capture.height)", encoder)
        self.assertNotIn("InitializeFromFilename", encoder)

    def test_cancellation_cannot_free_pending_gpu_owner(self):
        source = (ROOT / "src/gpu/screenshot.cpp").read_text()
        cancel = source.split("void CancelScreenshot()", 1)[1].split("std::vector<uint8_t> EncodePng", 1)[0]
        self.assertNotIn("g_pending", cancel)
        self.assertIn("pending.version != g_request_version.load", source)
        self.assertIn("g_requested.store(version", source)
        self.assertIn("g_requested.exchange(0", source)

    def test_existing_cpu_gpu_fixtures_compile_production_representation(self):
        build = (ROOT / "src/CMakeLists.txt").read_text().split("set(reblue_backend_only", 1)[1].split(")", 1)[0]
        self.assertIn("gpu/screenshot_readback.cpp", build)
        self.assertIn("gpu/screenshot.cpp", build)
        for path, required in (("tools/native_texture_test/screenshot.cpp", "CopyPixels("),
                               ("tools/native_scene_snapshot_test/screenshot.cpp", "vkWaitForFences(")):
            self.assertIn(required, (ROOT / path).read_text())


if __name__ == "__main__":
    unittest.main()
