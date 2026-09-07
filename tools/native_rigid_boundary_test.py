"""Native rigid shader/producer wiring; behavior and pixels have separate fixtures."""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[1]


class NativeRigidBoundaryTest(unittest.TestCase):
    def test_hard_off_selection_precedes_pose_fallback_and_every_legacy_path(self):
        walk = (ROOT / "src/gpu/scene/host_walk.cpp").read_text()
        guard = walk.index("RequireNativeRigidWalkNode(route_model, instance_pose.get(), index, view_id)")
        self.assertLess(guard, walk.index("const bool native_pose ="))
        self.assertLess(guard, walk.index("if ((!native_pose && !mp)"))
        node = (ROOT / "src/gpu/hooks/scene_node.cpp").read_text().split(
            "REX_HOOK_RAW(bdSceneNodeDrawSingle) {", 1)[1].split("// A node whose material", 1)[0]
        self.assertTrue(node.lstrip().startswith(
            "bd::gpu::scene::RequireNativeRigidLegacyNode(ctx.r6.u32, ctx.r3.u32);"))
        bridge = (ROOT / "src/gpu/scene/native_instance_bridge.cpp").read_text()
        self.assertIn("REXCVAR_DEFINE_BOOL(bd_native_rigid_hard_off, false", bridge)
        for required in ("FindLoadedNativeModel(*graph)", "FindLoadedNativeModelMaterials(*graph, mesh)",
                         "PrepareNativeRigidRoute(model, pose, node, view)",
                         "NativeRigidLegacyAllowed(owned.get())", "throw std::runtime_error(reason)"):
            self.assertIn(required, bridge)
        policy = (ROOT / "src/gpu/scene/native_rigid_route.h").read_text()
        self.assertLess(policy.index("SelectedNativeRigidShadow(*program)"), policy.index("if (!pose)"))
        for forbidden in ("bd::mem::", "__imp__", "HostDrawReplay", "NodeTag"):
            self.assertNotIn(forbidden, policy)

    def test_direct_scene_has_a_live_producer_and_an_emission_gate(self):
        direct = (ROOT / "src/gpu/scene/native_rigid_draw.cpp").read_text()
        walk = (ROOT / "src/gpu/scene/host_walk.cpp").read_text()
        self.assertIn("SubmitNativeRigidScene(*instance_pose, index)", walk)
        for required in ("PrepareNativeRigidSceneForObject(pose, node, refusal)",
                         "shape->layers == 1", "first.albedo->view.get()", "first.shadow->view.get()",
                         "ResolveSamplerLocked(", "item->input = {plan->object,plan->pass}", "store.scene_retired",
                         "store.scene_emitted += instances", "draw.bindings.set_count = 3"):
            self.assertIn(required, direct)
        emitter = (ROOT / "src/gpu/draw_queue.cpp").read_text()
        self.assertIn("scene::NoteNativeRigidEmission(d.bindings, d.render_view, instance_count)", emitter)
        lights = (ROOT / "src/gpu/scene/native_selected_lights_bridge.cpp").read_text()
        preview = lights.split("PrepareNativeSelectedLightValues(uint32_t selection)", 1)[1].split(
            "std::optional<NativeSelectedLights> FindNativeSelectedLights", 1)[0]
        self.assertIn("PreviewSelectedLightValues(", preview)
        for forbidden in ("__imp__", "bd::mem::store", "REXCVAR_GET(bd_native_materials_verify)"):
            self.assertNotIn(forbidden, preview)

    def test_native_batches_use_owned_storage_and_shared_queue_without_translated_gather(self):
        direct = (ROOT / "src/gpu/scene/native_rigid_draw.cpp").read_text()
        for required in ("PackNativeRigidBatch(items,packed,FrameStatFrameCount(),slot)",
                         "PlanNativeRigidStorage(", "minStorageBufferOffsetAlignment",
                         "std::memcpy(upload.memory+prefix,packed.data(),placement->bytes)",
                         "store.batches[slot].clear()", "draw.native_indirect = indirect.ref"):
            self.assertIn(required, direct)
        queue = (ROOT / "src/gpu/draw_queue.cpp").read_text()
        native = queue.split("if (q.native_rigid) {",1)[1].split("// A run of consecutive draws",1)[0]
        for required in ("NativeRigidBatchLength(", "PrepareNativeRigidBatchDraw(", "EmitOne(cmd,d,st,n)"):
            self.assertIn(required, native)
        self.assertNotIn("CommitInstanceRecords", native)
        self.assertIn("cmd->drawIndexedIndirect(d.native_indirect.ref", queue)
        schema = (ROOT / "src/gpu/scene/native_rigid_program.h").read_text()
        self.assertIn("sets[0].addStructuredBuffer(0)", schema)
        self.assertNotIn("addConstantBufferDynamic", schema)
        shader = (ROOT / "src/gpu/scene/native_rigid_shader.h").read_text()
        self.assertIn("StructuredBuffer<NativeRigidInstanceGPU>", shader)
        self.assertIn("nointerpolation uint instance", shader)

    def test_scene_sampling_matches_production_array_views(self):
        shader = (ROOT / "src/gpu/shaders/hlsl/native_rigid_ps.hlsl").read_text()
        self.assertIn("Texture2DArray<float4> albedo_image", shader)
        self.assertIn("Texture2DArray<float> shadow_image", shader)
        self.assertIn("float3(fragment.uv, 0)", shader)
        self.assertIn("float3(uv + offset, 0)", shader)
        # Ordinary assets and the mono sun shadow share layer zero between eyes;
        # framebuffer multiview does not make these sampled inputs stereo arrays.
        fixture = (ROOT / "tools/native_scene_snapshot_test/rigid.cpp").read_text()
        for required in ("TEXTURE_2D_ARRAY", "view.mipLevels = view.arraySize = 1",
                         "albedo_view.get()", "shadow_view.get()",
                         "RenderFormat::D32_FLOAT_S8_UINT", "VK_IMAGE_ASPECT_DEPTH_BIT"):
            self.assertIn(required, fixture)
        for path in ("src/gpu/scene/native_texture_gpu.cpp", "src/gpu/native_target_images.cpp"):
            self.assertIn("TEXTURE_2D_ARRAY", (ROOT / path).read_text())

    def test_direct_caster_precedes_interpreter_and_has_bounded_fence_owners(self):
        walk = (ROOT / "src/gpu/scene/host_walk.cpp").read_text()
        self.assertIn("SubmitNativeRigidShadow(*instance_pose, index, shadow_policy)", walk)
        direct = (ROOT / "src/gpu/scene/native_rigid_draw.cpp").read_text()
        for required in ("PrepareNativeRigidShadow(", "FindNativePassCamera(1)",
                         "CreateNativeRigidPrograms(", "GetOrCreatePipeline(", "DrawQueuePush(draw)",
                         "records.size() < 4096", "store.programs.size() < 8", "store.records[slot].clear()",
                         "draw.bindings.set_count = 1", "throw std::runtime_error(reason)"):
            self.assertIn(required, direct)
        for forbidden in ("__imp__", "HostDrawReplay(", "HostDrawCommit(", "bd::mem::", "ConstantBlockBytes("):
            self.assertNotIn(forbidden, direct)
        ring = (ROOT / "src/gpu/frame_ring.cpp").read_text()
        self.assertIn("scene::DrainNativeRigidDrawsLocked(s, slot)", ring)

    def test_production_shaders_do_not_import_the_translated_abi(self):
        paths = list((ROOT / "src/gpu/shaders/hlsl").glob("native_rigid_*.hlsl"))
        paths += [ROOT / "src/gpu/scene/native_rigid_shader.h"]
        self.assertEqual(len(paths), 4)
        for path in paths:
            text = path.read_text()
            for forbidden in ("shader_common.h", "g_VSC", "g_PSC", "BD_SHARED", "BOOL_BIT", "GuestShader", "packoffset"):
                self.assertNotIn(forbidden, text)

    def test_gpu_fixture_uses_production_programs_and_real_pixels(self):
        text = (ROOT / "tools/native_scene_snapshot_test/rigid.cpp").read_text()
        for required in ("CreateNativeRigidPrograms(device, input)", "ApplyGraphicsBindings(",
                         "ApplyNativePipelineProgram(", "drawIndexedIndirect(", "instance_count = mode == 4 ? 2 : 1", "vkWaitForFences(",
                         "Rigid colour mismatch", "Rigid per-eye depth mismatch", "Rigid caster depth mismatch"):
            self.assertIn(required, text)
        self.assertNotIn("ofstream", text)
        self.assertNotIn("fopen", text)

    def test_builds_only_explicit_shader_dependencies(self):
        text = (ROOT / "cmake/shaders.cmake").read_text()
        self.assertIn('if(STEM MATCHES "^native_rigid_")', text)
        self.assertIn("src/gpu/scene/native_rigid_inputs.h", text)
        build = (ROOT / "tools/native_scene_snapshot_test/CMakeLists.txt").read_text()
        self.assertIn("../../src/gpu/scene/native_rigid_program.cpp", build)
        self.assertIn("native_rigid_pixels", build)


if __name__ == "__main__":
    unittest.main()
