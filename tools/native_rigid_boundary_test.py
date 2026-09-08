"""Native rigid shader/producer wiring; behavior and pixels have separate fixtures."""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[1]


class NativeRigidBoundaryTest(unittest.TestCase):
    def test_skin_scene_uses_owned_palette_normals_bounds_and_fences(self):
        direct = (ROOT / "src/gpu/scene/native_rigid_draw.cpp").read_text()
        for required in ("CreateNativeSkinSceneProgram(*s.device,plan.vertex_input)",
                         "item->skin_pose = plan.skin_pose", "plan.skin_pose ? plan.skin_bounds",
                         "store.skin_scene_emitted : store.skin_emitted", "store.skin_scene_retired : store.skin_retired"):
            self.assertIn(required, direct)
        scene = (ROOT / "src/gpu/scene/native_rigid_scene.h").read_text()
        for required in ("FindNativeInstanceNode(*packet.pose,packet.node) != &program",
                         "plan.skin_pose = packet.pose", "TransformNativeSkinBounds(",
                         "skin_scene_layered_vertex_input"):
            self.assertIn(required, scene)
        shader = (ROOT / "src/gpu/scene/native_skin_scene_vertex.h").read_text()
        self.assertIn("NativeSkinNormal(", shader)
        self.assertIn("world_to_clip[eye]", shader)
        self.assertNotIn("object_data.world", shader)
        self.assertNotIn("object_data.normal_rows", shader)
        walk = (ROOT / "src/gpu/scene/host_walk.cpp").read_text()
        self.assertIn("view_id == 3 && NativeSkinSceneEnabled()", walk)

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
        self.assertIn("SubmitNativeRigidScene(*instance_pose, index, shadow_policy, ctx.r1.u32)", walk)
        for required in ("PrepareNativeRigidSceneForObject(pose, node, refusal)",
                         "shape->layers == 1", "first.albedo[layer]", "first.shadow->view.get()",
                         "ResolveSamplerLocked(", "item->input = {plan.object,plan.pass}", "store.scene_retired",
                         "store.scene_emitted += instances", "draw.bindings.set_count = 3"):
            self.assertIn(required, direct)
        emitter = (ROOT / "src/gpu/draw_queue.cpp").read_text()
        self.assertIn("scene::NoteNativeRigidEmission(d,native_items)", emitter)
        self.assertIn("items[0]->regression", direct)
        lights = (ROOT / "src/gpu/scene/native_selected_lights_bridge.cpp").read_text()
        preview = lights.split("std::optional<NativeSceneLightRecipe> CaptureNativeSceneLights(", 1)[1].split(
            "bool CommitNativeSceneLights(", 1)[0]
        self.assertIn("scene.current.Capture(frame, pass.light_update, instance, model_generation, node, pass.light_view)", preview)
        self.assertIn("scene.current.Resolve(FrameStatFrameCount(), recipe)", preview)
        for forbidden in ("__imp__", "bd::mem::", "Word(", "PrepareSelection(", "PrepareSelectedLights("):
            self.assertNotIn(forbidden, preview)

    def test_cutouts_are_counted_per_emitted_instance_and_retired_record(self):
        direct = (ROOT / "src/gpu/scene/native_rigid_draw.cpp").read_text()
        emitter = (ROOT / "src/gpu/draw_queue.cpp").read_text()
        self.assertIn("EmitOne(cmd,d,st,n,0,std::span(items).first(n))", emitter)
        self.assertIn("scene::NoteNativeRigidEmission(d,native_items)", emitter)
        note = direct.split("void ResolveNativeRigidEmission(", 1)[1].split("void DrainNativeRigidDrawsLocked", 1)[0]
        self.assertIn("item->output.Resolve(visible)", note)
        self.assertIn("for (const auto *item : items)", note)
        self.assertIn("++cutouts.emitted", note)
        self.assertIn("cutouts.textured_emitted +=", note)
        retire = direct.split("void DrainNativeRigidDrawsLocked", 1)[1]
        self.assertIn("record->Cutout()", retire)
        batch = (ROOT / "src/gpu/scene/native_rigid_batch.h").read_text()
        self.assertIn("!water && (input.object_data.flags.x & RigidCutout)", batch)
        self.assertIn("cutouts.textured_retired +=", retire)
        self.assertIn("[native-cutout-family]", direct)

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
        self.assertIn("CaptureNativeSceneLights(pose.instance, pose.model_generation, node, packet->lighting->inputs)", consumer)
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
        self.assertLess(draw.index("PrepareNativeRigidSceneForObject("), draw.index("SubmitNativeRigidScenePackets(std::move(submission), stack)"))
        self.assertLess(draw.index("ResolveNativeSceneLights("), draw.index("FinalizeNativeRigidSceneLights("))
        self.assertLess(draw.index("FinalizeNativeRigidSceneLights("), draw.index("CommitNativeSceneLights(*ticket, stack)"))
        self.assertLess(draw.index("CommitNativeSceneLights(*ticket, stack)"), draw.index("std::lock_guard lock(s.mutex)"))
        consumer = (ROOT / "src/gpu/scene/native_material_texture_bridge.cpp").read_text()
        self.assertNotIn("CommitNativeSceneLights", consumer)
        parameters = (ROOT / "src/gpu/constant_buffers.cpp").read_text()
        for name in ("PublishNativeShaderParameters", "InvalidateNativeShaderParameters"):
            body = parameters.split("void " + name + "(", 1)[1].split("\n}", 1)[0]
            self.assertLess(body.index("ObserveNativeSceneLightParameters"), body.index("std::lock_guard"))
        self.assertIn("REX_HOOK_RAW(sub_8218ADA0)", bridge)
        self.assertIn("unowned observations {}; no guessed defaults", bridge)

    def test_native_batches_use_owned_storage_and_shared_queue_without_translated_gather(self):
        direct = (ROOT / "src/gpu/scene/native_rigid_draw.cpp").read_text()
        for required in ("PackNativeRigidBatch(items,output,FrameStatFrameCount(),slot)",
                         "PlanNativeRigidStorage(", "minStorageBufferOffsetAlignment",
                         "std::memcpy(upload.memory+prefix,packed,placement->bytes)",
                         "store.batches[slot].clear()", "draw.native_indirect = indirect.ref"):
            self.assertIn(required, direct)
        queue = (ROOT / "src/gpu/draw_queue.cpp").read_text()
        native = queue.split("if (q.native_rigid) {",1)[1].split("// A run of consecutive draws",1)[0]
        for required in ("NativeRigidBatchLength(", "PrepareNativeRigidBatchDraw(", "EmitOne(cmd,d,st,n,0,std::span(items).first(n))"):
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
        self.assertIn("SubmitNativeRigidShadow(*instance_pose, index, shadow_policy, instance_pose)", walk)
        direct = (ROOT / "src/gpu/scene/native_rigid_draw.cpp").read_text()
        for required in ("PrepareNativeRigidShadow(", "FindNativePassCamera(1)",
                         "CreateNativeRigidPrograms(", "GetOrCreatePipeline(", "DrawQueuePush(entry.draw)",
                         "plans->size() <= 4096-records.size()", "store.programs.size() < 16", "store.records[slot].clear()",
                         "draw.bindings.set_count = 1", "throw std::runtime_error(reason)"):
            self.assertIn(required, direct)
        for forbidden in ("__imp__", "HostDrawReplay(", "HostDrawCommit(", "bd::mem::", "ConstantBlockBytes("):
            self.assertNotIn(forbidden, direct)
        ring = (ROOT / "src/gpu/frame_ring.cpp").read_text()
        self.assertIn("scene::DrainNativeRigidDrawsLocked(s, slot)", ring)

    def test_caster_family_preflights_all_siblings_without_expanding_reload_counts(self):
        direct = (ROOT / "src/gpu/scene/native_rigid_draw.cpp").read_text()
        shadow = direct.split("bool SubmitNativeRigidShadow(", 1)[1].split("bool SubmitNativeRigidScene(", 1)[0]
        self.assertIn("PrepareNativeRigidShadowAdmission(*model, inputs, skin,policies)", shadow)
        self.assertLess(shadow.index("pending.push_back("), shadow.index("StageNativeItem("))
        self.assertLess(shadow.index("for (const auto &plan : *plans)"), shadow.index("DrawQueuePush(entry.draw)"))
        self.assertIn("if (item->regression) NoteNativeRigidSubmitted", direct)
        self.assertIn("if (record->regression) NoteNativeRigidFenceRetired", direct)
        resolved = direct.split("void ResolveNativeRigidEmission(", 1)[1].split("void NoteNativeRigidEmission(", 1)[0]
        self.assertIn("if (items[0]->regression) NoteNativeRigidEmitted(items[0]->model_generation,render_view,instances)", resolved)
        self.assertLess(resolved.index("if (!visible) { store.scene_culled += instances; return; }"),
                        resolved.index("NoteNativeRigidEmitted("))
        self.assertIn("[native-caster-family]", direct)
        policy = (ROOT / "src/gpu/scene/native_rigid_shadow.h").read_text()
        self.assertNotIn("0x63B8D67932573E51", policy)
        self.assertIn("program.ranges.size() > 4096", policy)
        self.assertIn("if (!inputs) return {}", policy)

    def test_toon_surface_uses_owned_inputs_and_shared_consumers(self):
        packet = (ROOT / "src/gpu/scene/native_object_primitive.h").read_text()
        self.assertIn("std::optional<NativeToonSurface> toon", packet)
        scene = (ROOT / "src/gpu/scene/native_rigid_scene.h").read_text()
        self.assertIn("toon != packet.toon.has_value()", scene)
        self.assertIn("SetRigidToonSurface(*object,*packet.toon)", scene)
        shader = (ROOT / "src/gpu/shaders/hlsl/native_rigid_ps.hlsl").read_text()
        for text in ("ComposeToonSurface", "AdjustToonAmbient", "AdjustToonLight", "max(10.f,object_data.specular.w)",
                     "object_data.texture_colours[2]", "RigidIgnoreTextureAlpha"):
            self.assertIn(text, shader)
        # Authored visual/scene inputs and classified siblings feed the same
        # admission for pre-culling and submission. No shader-register fallback.
        bridge = (ROOT / "src/gpu/scene/native_material_texture_bridge.cpp").read_text()
        self.assertIn("ReadNativeToonSurface(*visual, Word)", bridge)
        self.assertIn("FindNativePassCamera(scope->render_view),scope->toon", bridge)
        self.assertIn("toon && mesh ? std::span<const NativePrimitivePolicy>(mesh->policies)", bridge)
        source = (ROOT / "src/gpu/scene/native_toon_source.h").read_text()
        for required in ("+3652", "+3660", "+3668", "scene+132", "3752+n*16", "x+y : x*y"):
            self.assertIn(required, source)
        for forbidden in ("device", "g_PSC", "PublishNativeShaderParameters", "ReadVS"):
            self.assertNotIn(forbidden, source)
        for path in ("native_toon_surface.h", "native_toon_shading.h"):
            text = (ROOT / "src/gpu/scene" / path).read_text()
            for forbidden in ("g_PSC", "g_VSC", "BD_SHARED", "BOOL_BIT", "bd::mem::", "PPCContext"):
                self.assertNotIn(forbidden, text)

    def test_production_shaders_do_not_import_the_translated_abi(self):
        paths = list((ROOT / "src/gpu/shaders/hlsl").glob("native_rigid_*.hlsl"))
        paths += [ROOT / "src/gpu/scene/native_rigid_shader.h", ROOT / "src/gpu/scene/native_rigid_vertex.h",
                  ROOT / "src/gpu/scene/native_skin_vertex.h"]
        self.assertEqual(len(paths), 11)
        for path in paths:
            text = path.read_text()
            for forbidden in ("shader_common.h", "g_VSC", "g_PSC", "BD_SHARED", "BOOL_BIT", "GuestShader", "packoffset"):
                self.assertNotIn(forbidden, text)

    def test_skin_caster_reuses_owned_load_pose_queue_and_native_program(self):
        load = (ROOT / "src/gpu/scene/native_material.cpp").read_text()
        self.assertIn("request.skin = &*range.skin", load)
        self.assertIn("program.skin_geometries[i] = ImportNativeMesh(request)", load)
        direct = (ROOT / "src/gpu/scene/native_rigid_draw.cpp").read_text()
        for required in ("CreateNativeSkinShadowProgram(", "item->skin_pose = owned_pose",
                         "PlanNativeSkinPalette(", "pose->transforms.data()",
                         "NativeRigidDescriptorSchema schema(bool(first.skin_pose))"):
            self.assertIn(required, direct)
        walk = (ROOT / "src/gpu/scene/host_walk.cpp").read_text()
        self.assertLess(walk.index("NativeSkinCasterBounds("), walk.index("PublishNodeSphere(out, radius)"))
        shader = (ROOT / "src/gpu/scene/native_skin_vertex.h").read_text()
        self.assertIn("skin_joints[range.x+joint]", shader)
        self.assertNotIn("object_data.world", shader)
        self.assertNotIn("exMatrix", shader)
        bridge = (ROOT / "src/gpu/scene/native_material_texture_bridge.cpp").read_text()
        self.assertIn("camera, cutouts,skin ? &pose : nullptr", bridge)
        self.assertIn("*scope->policy_inputs != inputs", bridge)
        self.assertIn("FindNativeShadowPoliciesForObject(*instance_pose,index,*shadow_policy)", walk)
        self.assertIn("FindNativeShadowPoliciesForObject(pose,node,*inputs)", direct)
        admission = (ROOT / "src/gpu/scene/native_rigid_shadow.h").read_text()
        self.assertIn("needs_owned && owned_policies.empty()", admission)
        self.assertIn("!owned_policies[n].routing_known", admission)
        self.assertIn("owned_policies[n] != policies[n]", admission)
        cutout = (ROOT / "src/gpu/shaders/hlsl/native_rigid_skin_shadow_cutout_vs.hlsl").read_text()
        self.assertIn("NativeSkinWorld(", cutout)
        self.assertIn("object_data.uv_scale_offset", cutout)
        self.assertNotIn("object_data.world", cutout)

    def test_gpu_fixture_uses_production_programs_and_real_pixels(self):
        text = (ROOT / "tools/native_scene_snapshot_test/rigid.cpp").read_text()
        for required in ("CreateNativeRigidPrograms(device, input)", "ApplyGraphicsBindings(",
                         "ApplyNativePipelineProgram(", "drawIndexedIndirect(", "instance_count = instanced || deferred ? 2 : 1", "vkWaitForFences(",
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
        self.assertLess(submit.index("FindNativeSceneAdmissionForObject(pose,node,inputs)"), submit.index("PrepareNativeRigidSceneForObject"))
        self.assertLess(submit.index("pending.push_back("), submit.index("StageNativeItem("))
        self.assertIn("item->regression = geometry->id == 0x258694267A8DBAEEull", submit)
        self.assertIn("scene_family_emitted += instances", direct)
        self.assertIn("++store.scene_family_retired", direct)
        shader = (ROOT / "src/gpu/shaders/hlsl/native_rigid_ps.hlsl").read_text()
        for required in ("object_data.flags.y > 2", "fragment.uv.x < 0", "fragment.uv.z < 0",
                         "fragment.secondary_uv.x < 0", "texture_colour.rgb = lerp", "texture_colour * object_data.diffuse"):
            self.assertIn(required, shader)
        # Only the explicitly owned Toon ignore-alpha policy may replace base
        # alpha. Detail layering and the ordinary family must still preserve it.
        alpha_override = "if (toon && (flags & RigidIgnoreTextureAlpha)) texture_colour.a = 1;"
        self.assertIn(alpha_override, shader)
        self.assertNotIn("texture_colour.a =", shader.replace(alpha_override, ""))
        vertex = (ROOT / "src/gpu/scene/native_rigid_vertex.h").read_text()
        self.assertIn("vertex.uv.zw", vertex)
        self.assertIn("vertex.secondary_uv.xy", vertex)
        schema = (ROOT / "src/gpu/scene/native_rigid_program.h").read_text()
        self.assertIn("layer < 4", schema)
        fixture = (ROOT / "tools/native_scene_snapshot_test/rigid.cpp").read_text()
        self.assertIn("mode<46", fixture)
        self.assertIn("if (cutout_receiver) sampler_desc.minFilter = sampler_desc.magFilter = RenderFilter::LINEAR", fixture)
        self.assertIn("compare_z <= caster_depth(", fixture)
        self.assertIn("shadowed_receivers > 0 && filtered_receivers > 0", fixture)
        self.assertIn("detail_colours[layer-1]", fixture)

    def test_cutouts_use_owned_recipe_and_shader_not_draw_state(self):
        alpha = (ROOT / "src/gpu/scene/native_material_alpha_source.h").read_text()
        self.assertNotIn("override_byte", alpha)
        self.assertNotIn("26711", alpha)
        self.assertIn("read(defaults + 60)", alpha)
        self.assertIn("read(uint64_t(visual) + 3124)", alpha)
        source = (ROOT / "src/gpu/scene/native_material_texture_bridge.cpp").read_text()
        producer = source.split("NativeObjectTextureScope::NativeObjectTextureScope", 1)[1].split(
            "NativeObjectTextureScope::~NativeObjectTextureScope", 1)[0]
        self.assertIn("ReadMaterialAlphaInputs(*visual, Word)", producer)
        self.assertIn("publication->cutout_pass = CaptureCutoutPass()", producer)
        consumer = source.split("std::optional<std::vector<NativeRigidScenePlan>> PrepareNativeRigidSceneForObject", 1)[1].split(
            "\n}", 1)[0]
        self.assertIn("ComposeMaterialAlphaReferences(program->ranges, admission.policies", consumer)
        self.assertIn("cutout->reference = references[primitive]", consumer)
        for forbidden in ("Word(", "CurrentAlphaIntent", "CurrentBlendIntent", "Video::AlphaThreshold", "pipelineState"):
            self.assertNotIn(forbidden, consumer)
        draw = (ROOT / "src/gpu/scene/native_rigid_draw.cpp").read_text()
        self.assertIn("ApplyBlendState(plan.blend, pipeline_state, blend_dirty)", draw)
        self.assertIn("draw.reorderable = !plan.blend.alphaBlendEnable", draw)
        self.assertIn("plan.alpha_to_coverage && shape->samples > 1", draw)
        shader = (ROOT / "src/gpu/shaders/hlsl/native_rigid_ps.hlsl").read_text()
        self.assertIn("RigidCutoutPasses(object_data.flags.z, albedo.a, asfloat(object_data.flags.w))", shader)
        self.assertLess(shader.index("const float4 albedo ="), shader.index("RigidCutoutPasses("))
        for path, function in (("native_alpha_bridge.cpp", "FindNativeAlphaIntent"),
                               ("native_blend_bridge.cpp", "FindNativeEnabledBlendIntent")):
            body = (ROOT / "src/gpu/scene" / path).read_text().split(function + "() {", 1)[1].split("\n}", 1)[0]
            for forbidden in ("Bootstrap", "ReadShadow", "ReadImport", "bd::mem::"):
                self.assertNotIn(forbidden, body)

    def test_scene_packet_consumer_has_no_object_scope_or_pose_dependency(self):
        source = (ROOT / "src/gpu/scene/native_rigid_draw.cpp").read_text()
        consumer = source.split("bool SubmitNativeRigidScenePackets(", 1)[1].split(
            "void PrepareNativeRigidBatchDraw(", 1)[0]
        for forbidden in ("pose.", "*model", "FindNativePassCamera", "FindNativeObjectPrimitive",
                          "PrepareNativeRigidSceneForObject", "NodeTag", "bd::mem::", "current->stack"):
            self.assertNotIn(forbidden, consumer)
        for required in ("submission.frame == FrameStatFrameCount()", "plan.light_ticket",
                         "for (const auto &eye : plan.pass.world_to_clip)", "pipeline_state.zWriteEnable = plan.depth_write",
                         "draw.zwrite = plan.depth_write", "StageNativeItem", "DrawQueuePush"):
            self.assertIn(required, consumer)
        self.assertLess(consumer.index("pending.push_back"), consumer.index("StageNativeItem"))

    def test_deferred_packets_connect_to_the_same_consumer_without_entry_imports(self):
        source = (ROOT / "src/gpu/scene/deferred_consumer.cpp").read_text()
        branch = source.split("auto submission = native_queue.Take(item.index);", 1)[1].split("++native_consumed", 1)[0]
        for required in ("ReadNativeDeferredEffects(identity)", "FinalizeNativeDeferredEffects(*submission, *effects, FrameStatFrameCount())",
                         "SubmitNativeRigidScenePackets(std::move(*submission), ctx.r1.u32)"):
            self.assertIn(required, branch)
        for forbidden in ("Read<", "bd::mem::", "Material(", "BindEntry", "D3DDevice_", "ResolveGuestTexture",
                          "FindNativeVisualIdentity", "CheckNativeDeferredContract"):
            self.assertNotIn(forbidden, branch)
        for required in ("MergeDeferredWork(legacy_depths, insertions, merged)", "CloseDeferredCompatibilityCapture()",
                         "visual_inputs.Begin(requested, FrameStatFrameCount())", "visual_inputs.Read(identity, FrameStatFrameCount())",
                         "CheckDeferredBatchResource(visual, CheckedWord)", "native_queue.EndDrain()"):
            self.assertIn(required, source)
        self.assertNotIn("native_visuals", source)
        self.assertNotIn("VisualTransition", source)
        self.assertLess(source.index("visual_inputs.Begin(requested, FrameStatFrameCount())"), source.index("native_queue.BeginDrain("))
        queue = (ROOT / "src/gpu/scene/native_deferred_queue.h").read_text()
        for forbidden in ("NodeTag", "visual", "bd::mem::", "816", "DeferredEntryRecipe"):
            self.assertNotIn(forbidden, queue)

    def test_native_visual_scope_exports_legacy_state_without_dispatching_callbacks(self):
        source = (ROOT / "src/gpu/scene/deferred_consumer.cpp").read_text()
        scope = source.split("struct NativeDeferredVisualScope {", 1)[1].split("} // namespace", 1)[0]
        for forbidden in ("PPCContext", "bridge.Call", "GetFunction", "__imp__", "sub_8221DBE0(", "sub_82174648("):
            self.assertNotIn(forbidden, scope)
        for required in ("PrepareNativePrimaryReceiver(inputs.identity, stack)", "PublishNativeDeferredBlend(uint32_t(inputs.blend))",
                         "BeginDeferredMaterialCompatibility(port)", "EndDeferredMaterialCompatibility(port)",
                         "PrepareEffectParticipants(*this)", "FinishEffectParticipants(*this)", "ValidateRoster()"):
            self.assertIn(required, scope)
        pure = (ROOT / "src/gpu/scene/native_deferred_effects.h").read_text()
        for forbidden in ("bd::mem::", "PPCContext", "Read<uint", "ResolveGuest", "Staging("):
            self.assertNotIn(forbidden, pure)
        blend = (ROOT / "src/gpu/scene/native_blend_bridge.cpp").read_text().split("bool PublishNativeDeferredBlend", 1)[1]
        self.assertIn("REXCVAR_GET(bd_native_blend_verify)", blend)
        self.assertNotIn("Bootstrap(", blend)
        self.assertNotIn("__imp__", blend)
        self.assertIn("NativeUpdate(context, nullptr, 2, false)", blend)
        self.assertIn("NativeUpdate(context, nullptr, 3, false)", blend)

    def test_visual_publication_uses_native_keys_and_existing_source_index(self):
        pure = (ROOT / "src/gpu/scene/native_visual_inputs.h").read_text()
        for required in ("uint64_t instance = 0, model_generation = 0", "kMaxVisuals", "kMaxBytes",
                         "inputs.capacity()", "frame != frame_", "it->identity == identity"):
            self.assertIn(required, pure)
        for forbidden in ("bd::mem::", "PPCContext", "Read<uint", "visual_va", "ResolveGuest", "unordered_map"):
            self.assertNotIn(forbidden, pure)
        bridge = (ROOT / "src/gpu/scene/native_instance_bridge.cpp").read_text()
        producer = bridge.split("bool CollectNativeVisualInputs(", 1)[1].split("bool CollectNativeInstanceLightInputs(", 1)[0]
        for required in ("std::lock_guard lock(store.mutex)", "store.sources", "ReadNativeDeferredVisualInputs(identity, visual, Word)",
                         "pose->model_generation != identity.model_generation", "out.size() != requested.size()"):
            self.assertIn(required, producer)
        receiver = (ROOT / "src/gpu/scene/native_shadow_receiver_bridge.cpp").read_text()
        native = receiver.split("bool PrepareNativePrimaryReceiver(", 1)[1].split("} // namespace", 1)[0]
        self.assertIn("NativeVisualIdentity identity", native)
        self.assertIn("Adapter adapter{kPrimary, 0, stack, identity}", native)
        for forbidden in ("FindNativeVisualIdentity", "visual +", "visual)+", "uint32_t visual"):
            self.assertNotIn(forbidden, native)

    def test_known_writers_republish_after_completion_and_failures_do_not_resume(self):
        water = (ROOT / "src/gpu/scene/native_refraction_material_bridge.cpp").read_text()
        for symbol, mode in (("sub_82454720", "true"), ("sub_82455150", "false")):
            hook = water.split(f"REX_HOOK_RAW({symbol}) {{", 1)[1].split("\n}", 1)[0]
            self.assertLess(hook.index(f"Prepare(ctx, base, {mode})"), hook.index("RefreshNativeVisualInputsAfterWriter()"))
        bridge = (ROOT / "src/gpu/scene/native_instance_bridge.cpp").read_text()
        refresh = bridge.split("void RefreshNativeVisualInputsAfterWriter() {", 1)[1].split("bool CollectNativeVisualInputs(", 1)[0]
        for required in ("if (!active_visual_inputs) return", "scope.frame_ != FrameStatFrameCount()",
                         "CollectNativeVisualInputs(scope.requested_, inputs)", "scope.inputs_.Publish(", "throw std::runtime_error"):
            self.assertIn(required, refresh)
        self.assertIn("if (active_visual_inputs == this) active_visual_inputs = nullptr", bridge)
        hook = (ROOT / "src/gpu/hooks/scene_node.cpp").read_text().split("REX_HOOK_RAW(sub_8227F360)", 1)[1]
        failure = hook.split("catch (const std::exception &error)", 1)[1]
        for required in ("BD_ERROR(", "rex::FlushLogging()", "ShutdownReason::Fatal, 1", "TerminateProcessNow(1)"):
            self.assertIn(required, failure)
        for forbidden in ("__imp__", "RecordDeferredConsumerFallback", "return true", "return false"):
            self.assertNotIn(forbidden, failure)

    def test_builds_only_explicit_shader_dependencies(self):
        text = (ROOT / "cmake/shaders.cmake").read_text()
        self.assertIn('if(STEM MATCHES "^native_rigid_")', text)
        self.assertIn("src/gpu/scene/native_rigid_inputs.h", text)
        build = (ROOT / "tools/native_scene_snapshot_test/CMakeLists.txt").read_text()
        self.assertIn("../../src/gpu/scene/native_rigid_program.cpp", build)
        self.assertIn("native_rigid_pixels", build)
        generated = (ROOT / "cmake/generated.cmake").read_text()
        ordering = generated.split("add_custom_target(native_rigid_shader_headers DEPENDS", 1)[1].split("add_dependencies", 1)[0]
        for path in (ROOT / "src/gpu/shaders/hlsl").glob("native_rigid_*.hlsl"):
            self.assertIn(path.name + ".spirv.h", ordering)

    def test_shadow_cutouts_use_phase_owned_inputs_and_no_attachment_feedback(self):
        source = (ROOT / "src/gpu/scene/native_material_texture_bridge.cpp").read_text()
        consumer = source.split("PrepareNativeRigidShadowForObject(", 1)[1].split(
            "std::optional<NativeObjectPrimitiveInputs>", 1)[0]
        for required in ("scope->shadow_phase", "scope->pose.get() != &pose", "range.shadow_uses_texture",
                         "admission.policies[n].deferred", "MaterialSampleAddress::Wrap", "mesh->values[n].images[0]"):
            self.assertIn(required, consumer)
        for forbidden in ("Word(", "BuildNativeObjectPrimitive", "FindNativeLightingPass", "ReadMaterial", "pipelineState",
                          "ComposeMaterialAlphaReferences", "cutout_pass", "cutout.alpha"):
            self.assertNotIn(forbidden, consumer)
        replay = source.split("PrepareReplayMaterialMesh(const NodeTag &tag)", 1)[1].split("} // namespace", 1)[0]
        self.assertIn("scope->shadow_phase", replay)  # No phase0 replay consumers see phase1 recipes.
        draw = (ROOT / "src/gpu/scene/native_rigid_draw.cpp").read_text()
        descriptors = draw.split("if (first.view == 1 && first.albedo[0])", 1)[1].split("else if (first.view == 3)", 1)[0]
        self.assertIn("first.albedo[0]->image.get()", descriptors)
        self.assertNotIn("first.shadow", descriptors)
        for required in ("program->shaders.shadow_cutout", "program->shaders.shadow", "item->albedo[0] = plan.albedo"):
            self.assertIn(required, draw)
        shader = (ROOT / "src/gpu/shaders/hlsl/native_rigid_shadow_cutout_ps.hlsl").read_text()
        for required in ("sampled < .6f", "discard", "float3(uv, 0)"):
            self.assertIn(required, shader)
        for forbidden in ("RigidCutoutPasses", "diffuse", "vertex_alpha", "uv.x < 0", "rigid_instances"):
            self.assertNotIn(forbidden, shader)
        self.assertNotIn("SV_Target", shader)


if __name__ == "__main__":
    unittest.main()
