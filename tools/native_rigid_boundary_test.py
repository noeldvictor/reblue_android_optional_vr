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
            "std::optional<std::vector<NativeRigidScenePlan>> PrepareNativeRigidSceneForObject",1)[1].split("\n}",1)[0]
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
        guard = walk.index("RequireNativeRigidWalkNode(route_model, instance_pose.get(), index, view_id, shadow_policy)")
        self.assertLess(guard, walk.index("const bool native_pose ="))
        self.assertLess(guard, walk.index("if ((!native_pose && !mp)"))
        node = (ROOT / "src/gpu/hooks/scene_node.cpp").read_text().split(
            "REX_HOOK_RAW(bdSceneNodeDrawSingle) {", 1)[1].split("// A node whose material", 1)[0]
        self.assertTrue(node.lstrip().startswith(
            "bd::gpu::scene::RequireNativeRigidLegacyNode(ctx.r6.u32, ctx.r3.u32);"))
        bridge = (ROOT / "src/gpu/scene/native_instance_bridge.cpp").read_text()
        self.assertIn("REXCVAR_DEFINE_BOOL(bd_native_rigid_hard_off, false", bridge)
        for required in ("FindLoadedNativeModel(*graph)", "FindLoadedNativeModelMaterials(*graph, mesh)",
                         "PrepareNativeRigidRoute(model, pose, node, view, inputs)",
                         "NativeRigidLegacyAllowed(owned.get(), *view, inputs)", "throw std::runtime_error(reason)"):
            self.assertIn(required, bridge)
        policy = (ROOT / "src/gpu/scene/native_rigid_route.h").read_text()
        self.assertLess(policy.index("SelectedNativeRigidShadow(*program)"), policy.index("if (!pose)"))
        for forbidden in ("bd::mem::", "__imp__", "HostDrawReplay", "NodeTag"):
            self.assertNotIn(forbidden, policy)

    def test_direct_scene_has_a_live_producer_and_an_emission_gate(self):
        direct = (ROOT / "src/gpu/scene/native_rigid_draw.cpp").read_text()
        walk = (ROOT / "src/gpu/scene/host_walk.cpp").read_text()
        self.assertIn("SubmitNativeRigidScene(*instance_pose, index, shadow_policy)", walk)
        for required in ("PrepareNativeRigidSceneForObject(pose, node, refusal)",
                         "shape->layers == 1", "first.albedo[layer]", "first.shadow->view.get()",
                         "ResolveSamplerLocked(", "item->input = {plan.object,plan.pass}", "store.scene_retired",
                         "store.scene_emitted += instances", "draw.bindings.set_count = 3"):
            self.assertIn(required, direct)
        emitter = (ROOT / "src/gpu/draw_queue.cpp").read_text()
        self.assertIn("scene::NoteNativeRigidEmission(d.bindings, d.render_view, instance_count,", emitter)
        self.assertIn("d.native_rigid && d.native_rigid->regression ? d.native_rigid->model_generation : 0", emitter)
        lights = (ROOT / "src/gpu/scene/native_selected_lights_bridge.cpp").read_text()
        preview = lights.split("std::optional<NativeSceneLightTicket> FindNativeSceneLights(", 1)[1].split(
            "bool CommitNativeSceneLights(", 1)[0]
        self.assertIn("scene.current.Prepare(frame, pass.light_update, instance, model_generation, node, pass.light_view)", preview)
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
            "std::optional<std::vector<NativeRigidScenePlan>> PrepareNativeRigidSceneForObject", 1)[1].split("\n}", 1)[0]
        self.assertIn("FindNativeSceneLights(pose.instance, pose.model_generation, node, packet->lighting->inputs)", consumer)
        for forbidden in ("Word(", "+3132", "+3376", "+3380", "PrepareNativeSelectedLightValues"):
            self.assertNotIn(forbidden, consumer)
        self.assertNotIn("PreviewSelectedLightValues", (ROOT / "src/gpu/scene/native_light_selection_source.h").read_text())

    def test_ordered_lights_commit_after_preparation_and_guard_compatibility_writers(self):
        bridge = (ROOT / "src/gpu/scene/native_selected_lights_bridge.cpp").read_text()
        observer = bridge.split("void ObserveCompatibilityLightSelection(", 1)[1].split("std::optional<uint32_t> Word(", 1)[0]
        self.assertIn("scene.current.Prepare(", observer)
        self.assertIn("SameNativeSelectedLights(ticket->lights,current.lights)", observer)
        self.assertNotIn("Word(", observer)
        self.assertNotIn("ticket->lights = current.lights", observer)
        mirror = bridge.split("bool CommitNativeSceneLights(", 1)[1].split("void ObserveNativeSceneLightParameters(", 1)[0]
        self.assertLess(mirror.index("CanFlushHostParameterDescriptor"), mirror.index("bd::mem::store"))
        self.assertIn("PrepareNativeLightMirror(kPublisher,ticket.lights,safe_word)", mirror)
        self.assertNotIn("__imp__", mirror)
        draw = (ROOT / "src/gpu/scene/native_rigid_draw.cpp").read_text().split("bool SubmitNativeRigidScene(", 1)[1]
        self.assertLess(draw.index("PrepareNativeRigidSceneForObject("), draw.index("CommitNativeRigidSceneLights(*plans)"))
        self.assertLess(draw.index("CommitNativeRigidSceneLights(*plans)"), draw.index("std::lock_guard lock(s.mutex)"))
        consumer = (ROOT / "src/gpu/scene/native_material_texture_bridge.cpp").read_text()
        self.assertIn("return !draws || CommitNativeSceneLights(ticket,current->stack)", consumer)
        parameters = (ROOT / "src/gpu/constant_buffers.cpp").read_text()
        for name in ("PublishNativeShaderParameters", "InvalidateNativeShaderParameters"):
            body = parameters.split("void " + name + "(", 1)[1].split("\n}", 1)[0]
            self.assertLess(body.index("ObserveNativeSceneLightParameters"), body.index("std::lock_guard"))
        self.assertIn("REX_HOOK_RAW(sub_8218ADA0)", bridge)

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
        self.assertIn("float3(fragment.uv.xy, 0)", shader)
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
                         "CreateNativeRigidPrograms(", "GetOrCreatePipeline(", "DrawQueuePush(entry.draw)",
                         "plans->size() <= 4096-records.size()", "store.programs.size() < 8", "store.records[slot].clear()",
                         "draw.bindings.set_count = 1", "throw std::runtime_error(reason)"):
            self.assertIn(required, direct)
        for forbidden in ("__imp__", "HostDrawReplay(", "HostDrawCommit(", "bd::mem::", "ConstantBlockBytes("):
            self.assertNotIn(forbidden, direct)
        ring = (ROOT / "src/gpu/frame_ring.cpp").read_text()
        self.assertIn("scene::DrainNativeRigidDrawsLocked(s, slot)", ring)

    def test_caster_family_preflights_all_siblings_without_expanding_reload_counts(self):
        direct = (ROOT / "src/gpu/scene/native_rigid_draw.cpp").read_text()
        shadow = direct.split("bool SubmitNativeRigidShadow(", 1)[1].split("bool SubmitNativeRigidScene(", 1)[0]
        self.assertIn("PrepareNativeRigidCasterAdmission(*model, inputs)", shadow)
        self.assertLess(shadow.index("pending.push_back("), shadow.index("StageNativeItem("))
        self.assertLess(shadow.index("for (const auto &plan : *plans)"), shadow.index("DrawQueuePush(entry.draw)"))
        self.assertIn("if (item->regression) NoteNativeRigidSubmitted", direct)
        self.assertIn("if (record->regression) NoteNativeRigidFenceRetired", direct)
        self.assertIn("if (generation) NoteNativeRigidEmitted", direct)
        self.assertIn("[native-caster-family]", direct)
        policy = (ROOT / "src/gpu/scene/native_rigid_shadow.h").read_text()
        self.assertNotIn("0x63B8D67932573E51", policy)
        self.assertIn("program.ranges.size() > 4096", policy)
        self.assertIn("if (!inputs) return {}", policy)

    def test_production_shaders_do_not_import_the_translated_abi(self):
        paths = list((ROOT / "src/gpu/shaders/hlsl").glob("native_rigid_*.hlsl"))
        paths += [ROOT / "src/gpu/scene/native_rigid_shader.h", ROOT / "src/gpu/scene/native_rigid_vertex.h"]
        self.assertEqual(len(paths), 6)
        for path in paths:
            text = path.read_text()
            for forbidden in ("shader_common.h", "g_VSC", "g_PSC", "BD_SHARED", "BOOL_BIT", "GuestShader", "packoffset"):
                self.assertNotIn(forbidden, text)

    def test_gpu_fixture_uses_production_programs_and_real_pixels(self):
        text = (ROOT / "tools/native_scene_snapshot_test/rigid.cpp").read_text()
        for required in ("CreateNativeRigidPrograms(device, input)", "ApplyGraphicsBindings(",
                         "ApplyNativePipelineProgram(", "drawIndexedIndirect(", "instance_count = instanced ? 2 : 1", "vkWaitForFences(",
                         "Rigid colour mismatch", "Rigid per-eye depth mismatch", "Rigid caster depth mismatch"):
            self.assertIn(required, text)
        self.assertNotIn("ofstream", text)
        self.assertNotIn("fopen", text)

    def test_layered_scene_connects_whole_node_and_retains_every_image(self):
        scene = (ROOT / "src/gpu/scene/native_rigid_scene.h").read_text()
        for required in ("PrepareNativeRigidSceneAdmission", "packet.primitive", "packet.textures.secondary_uv",
                         "geometry->layered_rigid_vertex_input", "packet.samplers[n]", "binding.slice_2d || binding.cube"):
            self.assertIn(required, scene)
        self.assertNotIn("0x63B8D67932573E51", scene)
        direct = (ROOT / "src/gpu/scene/native_rigid_draw.cpp").read_text()
        submit = direct.split("bool SubmitNativeRigidScene(", 1)[1].split("void PrepareNativeRigidBatchDraw(", 1)[0]
        self.assertLess(submit.index("PrepareNativeRigidSceneAdmission(*model, inputs)"), submit.index("PrepareNativeRigidSceneForObject"))
        self.assertLess(submit.index("pending.push_back("), submit.index("StageNativeItem("))
        self.assertIn("item->regression = geometry->id == 0x258694267A8DBAEEull", submit)
        self.assertIn("scene_family_emitted += instances", direct)
        self.assertIn("++store.scene_family_retired", direct)
        shader = (ROOT / "src/gpu/shaders/hlsl/native_rigid_ps.hlsl").read_text()
        for required in ("object_data.flags.y > 2", "fragment.uv.x < 0", "fragment.uv.z < 0",
                         "fragment.secondary_uv.x < 0", "texture_colour.rgb = lerp", "texture_colour * object_data.diffuse"):
            self.assertIn(required, shader)
        self.assertNotIn("texture_colour.a =", shader)
        vertex = (ROOT / "src/gpu/scene/native_rigid_vertex.h").read_text()
        self.assertIn("vertex.uv.zw", vertex)
        self.assertIn("vertex.secondary_uv.xy", vertex)
        schema = (ROOT / "src/gpu/scene/native_rigid_program.h").read_text()
        self.assertIn("layer < 4", schema)
        fixture = (ROOT / "tools/native_scene_snapshot_test/rigid.cpp").read_text()
        self.assertIn("mode<12", fixture)
        self.assertIn("detail_colours[layer-1]", fixture)

    def test_builds_only_explicit_shader_dependencies(self):
        text = (ROOT / "cmake/shaders.cmake").read_text()
        self.assertIn('if(STEM MATCHES "^native_rigid_")', text)
        self.assertIn("src/gpu/scene/native_rigid_inputs.h", text)
        build = (ROOT / "tools/native_scene_snapshot_test/CMakeLists.txt").read_text()
        self.assertIn("../../src/gpu/scene/native_rigid_program.cpp", build)
        self.assertIn("native_rigid_pixels", build)


if __name__ == "__main__":
    unittest.main()
