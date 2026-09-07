"""Native program cache wiring; C++ fixture checks ownership/selection behavior."""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[1]


class NativePipelineBoundaryTest(unittest.TestCase):
    def read(self, name):
        return (ROOT / name).read_text()

    def test_native_build_precedes_and_bypasses_translated_selection(self):
        source = self.read("src/gpu/pipeline/pipeline_cache.cpp")
        build = source.split("Build(const PipelineState &state)", 1)[1]
        self.assertLess(build.index("ApplyNativePipelineProgram("), build.index("Video::MainPipelineLayout()"))
        self.assertLess(build.index('return CreateHostGraphicsPipeline(device, desc, "native pipeline")'),
                        build.index("u32 specMask = 0"))
        core = self.read("src/gpu/pipeline/native_pipeline_program.h")
        apply = core.split("inline void ApplyNativePipelineProgram", 1)[1]
        for forbidden in ("Video::", "GuestShader", "GetOrLinkShader", "Occlusion::", "shaderCacheEntry"):
            self.assertNotIn(forbidden, apply)
        self.assertIn("if (!NativePipelineStateValid(state)) return nullptr;", source)

    def test_async_and_cache_pin_program_and_retry_native_failures(self):
        queue = self.read("src/gpu/pipeline/pso_precache.cpp")
        cache = self.read("src/gpu/pipeline/pipeline_cache.cpp")
        self.assertIn("NativePipelineHandle program;", queue)
        self.assertIn("state.native_program->Lease()", queue)
        self.assertEqual(queue.count("WorkItem{state, std::move(token), std::move(program)}"), 2)
        self.assertIn("if (item.program)", queue)
        self.assertIn("if (!succeeded) g_queuedOrDone.erase", queue)
        self.assertIn("g_queuedOrDone.erase(HashPipelineState(item.state));", queue)
        self.assertIn("g_native_pending >= kMaxNativePending", queue)
        self.assertIn("if (pending_added) rollback_token->ReleasePending();", queue)
        self.assertIn("PipelineEntry{std::move(program), std::move(p)}", cache)
        self.assertEqual(cache.count("g_native_pipelines >= kMaxNativePipelines"), 2)

    def test_native_identity_stays_out_of_csv_and_engine_intent(self):
        capture = self.read("src/gpu/pipeline/pso_recorder.cpp").split("void RecordPipelineState(", 1)[1]
        self.assertLess(capture.index("if (state.native_program) return;"), capture.index("ShaderHash("))
        self.assertIn("assign(state.pipelineState.native_program, nullptr);", self.read("src/gpu/draw_intent.h"))
        self.assertIn("bindings.cpp native_pipeline.cpp", self.read("tools/native_texture_test/CMakeLists.txt"))


if __name__ == "__main__":
    unittest.main()
