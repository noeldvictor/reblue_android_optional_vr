"""Native rigid shader/producer wiring; behavior and pixels have separate fixtures."""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[1]


class NativeRigidBoundaryTest(unittest.TestCase):
    def test_receiver_is_native_and_draw_consumes_a_retained_packet(self):
        source = (ROOT / "src/gpu/scene/native_shadow_receiver_bridge.cpp").read_text()
        self.assertIn("REXCVAR_DEFINE_BOOL(bd_native_shadow_receiver, true", source)
        hook = source.split("REX_HOOK_RAW(sub_82176708) {",1)[1]
        native = hook.split("const auto technique =",1)[1]
        self.assertIn("RunNativeReceiverSetup(enabled,adapter)",native)
        self.assertNotIn("__imp__",native)
        self.assertNotIn("D3DDevice_SetTexture",source)
        self.assertIn("CanFlushHostParameterDescriptor(*descriptor,stack-96)",source)
        self.assertIn("FlushHostParameterDescriptor(ReadWord(uint64_t(source)+356),stack-96)",source)
        reader = source.split("std::optional<NativePrimaryReceiver> FindNativePrimaryReceiver",1)[1].split("} // namespace",1)[0]
        for forbidden in ("Word(","ResolveGuestTexture","FindCompletedNativePrimaryShadow"):
            self.assertNotIn(forbidden,reader)
        consumer = (ROOT / "src/gpu/scene/native_material_texture_bridge.cpp").read_text().split(
            "std::optional<NativeRigidScenePlan> PrepareNativeRigidSceneForObject",1)[1].split("\n}",1)[0]
        self.assertIn("receiver->image, receiver->world_to_shadow, receiver->colour",consumer)
        self.assertNotIn("FindCompletedNativePrimaryShadow",consumer)
        self.assertNotIn("value_or(ReadColour",source) # Eager fallback reads can outlive the validated late-read boundary.

    def test_reload_uses_actual_source_and_gpu_lifetimes(self):
        material = (ROOT / "src/gpu/scene/native_material.cpp").read_text()
        retire = material.split("REX_HOOK_RAW(sub_8227EBE8) {", 1)[1]
        self.assertLess(retire.index("__imp__sub_8227EBE8(ctx, base)"),
                        retire.index("NoteNativeRigidSourceRetired(generation)"))
        direct = (ROOT / "src/gpu/scene/native_rigid_draw.cpp").read_text()
        self.assertIn("NoteNativeRigidSubmitted(item->model_generation,item->instance,item->view)", direct)
        self.assertIn("NoteNativeRigidFenceRetired(record->model_generation,record->view)", direct)
        bridge = (ROOT / "src/gpu/scene/native_rigid_lifecycle_bridge.cpp").read_text()
        self.assertIn("REXCVAR_DEFINE_BOOL(bd_native_rigid_reload, false", bridge)
        self.assertIn("REX_HOOK_RAW(SequenceControl_vf02)", bridge)
        self.assertIn("RigidFindSequenceByName(sequence,0x82065008)", bridge)
        for forbidden in ("Models().Retire", "DrainNativeRigidDrawsLocked", "HostDrawReplay", "CreateProcess"):
            self.assertNotIn(forbidden, bridge)

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
        self.assertIn("scene::NoteNativeRigidEmission(d.bindings, d.render_view, instance_count,", emitter)
        self.assertIn("d.native_rigid ? d.native_rigid->model_generation : 0", emitter)
        lights = (ROOT / "src/gpu/scene/native_selected_lights_bridge.cpp").read_text()
        preview = lights.split("std::optional<NativeSelectedLights> FindNativeSceneLights(", 1)[1].split(
            "std::optional<NativeSelectedLights> FindNativeSelectedLights", 1)[0]
        self.assertIn("scene.current.Select(frame, pass.light_update, instance, model_generation, node, pass.light_view)", preview)
        for forbidden in ("__imp__", "bd::mem::", "Word(", "PrepareSelection(", "PrepareSelectedLights("):
            self.assertNotIn(forbidden, preview)

    def test_scene_lights_publish_at_handoff_without_dirty_or_shader_cache_inputs(self):
        producer = (ROOT / "src/engine/frame_interp.cpp").read_text().split(
            "REX_HOOK_RAW(bdLightListUpdateSnapshot) {", 1)[1].split("\n}", 1)[0]
        self.assertLess(producer.index("__imp__bdLightListUpdateSnapshot(ctx, base)"),
                        producer.index("PublishNativeSceneLights(manager)"))
        self.assertNotIn("return;", producer)
        source = (ROOT / "src/gpu/scene/native_scene_lights_source.h").read_text()
        for forbidden in ("selection+4)", "selection+8", "selection+216", "+276", "PrepareSelectedLights"):
            self.assertNotIn(forbidden, source)
        consumer = (ROOT / "src/gpu/scene/native_material_texture_bridge.cpp").read_text().split(
            "std::optional<NativeRigidScenePlan> PrepareNativeRigidSceneForObject", 1)[1].split("\n}", 1)[0]
        self.assertIn("FindNativeSceneLights(pose.instance, pose.model_generation, node, packet->lighting->inputs)", consumer)
        for forbidden in ("Word(", "+3132", "+3376", "+3380", "PrepareNativeSelectedLightValues"):
            self.assertNotIn(forbidden, consumer)
        self.assertNotIn("PreviewSelectedLightValues", (ROOT / "src/gpu/scene/native_light_selection_source.h").read_text())

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
