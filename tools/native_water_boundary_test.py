"""Architecture guards only; the production --water GPU fixture checks behavior."""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[1]


class NativeWaterBoundaryTest(unittest.TestCase):
    def test_shaders_consume_owned_values_not_translated_constants(self):
        files = ("src/gpu/scene/native_water_shader.h",
                 "src/gpu/shaders/hlsl/native_water_vs.hlsl",
                 "src/gpu/shaders/hlsl/native_water_ps.hlsl")
        source = "\n".join((ROOT / file).read_text() for file in files)
        for forbidden in ("g_PSC", "BD_VSC", "BOOL_BIT", "BD_SHARED", "shader_common.h", "tfetch", "packoffset"):
            self.assertNotIn(forbidden, source)
        self.assertIn("StructuredBuffer<NativeWaterInstanceGPU>", source)
        self.assertIn("EvaluateLitLight", source)
        self.assertIn("ApplyLitFog", source)

    def test_camera_and_scene_images_are_explicitly_per_eye(self):
        vertex = (ROOT / "src/gpu/shaders/hlsl/native_water_vs.hlsl").read_text()
        pixel = (ROOT / "src/gpu/shaders/hlsl/native_water_ps.hlsl").read_text()
        self.assertIn("world_to_clip[eye]", vertex)
        self.assertIn("world_to_bottom[eye]", pixel)
        self.assertIn("cameras[eye]", pixel)
        for lane in "xyz":
            self.assertIn(f"WaterImageLayer(data.image_layers.{lane},eye)", pixel)
        self.assertNotIn("g_StereoSeparation", vertex)

    def test_temporary_import_is_outside_program_and_native_data(self):
        native = (ROOT / "src/gpu/scene/native_water_inputs.h").read_text()
        program = (ROOT / "src/gpu/scene/native_water_program.cpp").read_text()
        for forbidden in ("REX_", "PPCContext", "bd::mem", "ResolveGuest", "WaterUpdateBuilder", "source_va"):
            self.assertNotIn(forbidden, native + program)
        self.assertIn("NativePipelineProgram::Create", program)
        source = (ROOT / "src/gpu/scene/native_water_material_source.h").read_text()
        self.assertIn("boundary.Destination", source)
        self.assertNotIn("boundary.Store", source)

    def test_gpu_fixture_uses_real_program_snapshot_and_indirect_draw(self):
        fixture = (ROOT / "tools/native_scene_snapshot_test/water.cpp").read_text()
        for required in ("CreateNativeWaterProgram", "ReadNativeWaterMaterial", "BuildNativeWaterInstance",
                         "CopySceneSnapshot", "drawIndexedIndirect", "Water per-eye depth oracle",
                         "Water authored animation must change both eyes", "HalfStoreMatches"):
            self.assertIn(required, fixture)
        cmake = (ROOT / "tools/native_scene_snapshot_test/CMakeLists.txt").read_text()
        self.assertIn("native_water_program.cpp", cmake)
        self.assertIn("NAME native_water_pixels", cmake)

    def test_water_reuses_native_queue_emit_and_fence_owners(self):
        direct = (ROOT / "src/gpu/scene/native_rigid_draw.cpp").read_text()
        submit = direct.split("bool SubmitNativeWaterScenePackets(", 1)[1].split(
            "void PrepareNativeRigidBatchDraw(", 1)[0]
        for required in ("NativeWaterWorldBounds", "CreateNativeWaterProgram", "StageNativeItem",
                         "DrawQueuePush", "draw.reorderable = false", "ResolveSamplerLocked", "commands->WritesImage"):
            self.assertIn(required, submit)
        for forbidden in ("bd::mem", "__imp__", "ResolveGuest", "ReadNativeWaterMaterial"):
            self.assertNotIn(forbidden, submit)
        emit = direct.split("void PrepareNativeRigidBatchDraw(", 1)[1]
        for required in ("PackNativeWaterBatch", "BindNativeWaterImages", "first.Pass()",
                         "record->output.Retire()", "++store.water_retired"):
            self.assertIn(required, emit)
        fixture = (ROOT / "tools/native_scene_snapshot_test/water.cpp").read_text()
        for required in ("PackNativeWaterBatch", "BindNativeWaterImages", "weak_geometry.expired()",
                         "weak_snapshot.expired()", "queue->waitForCommandFence"):
            self.assertIn(required, fixture)
        queue = (ROOT / "src/gpu/draw_queue.cpp").read_text()
        self.assertIn("q.reorderable || q.native_rigid->water", queue)
        self.assertIn("q.native_rigid->water && candidate.native_rigid->water", queue)
        self.assertEqual(queue.count("StableSortDrawRuns(std::span(g_queue), ordered_native"), 2)
        self.assertIn("prepass_end = DrawOrderRunEnd", queue)
        self.assertIn("emit_prepass_run(i,prepass_end)", queue)
        self.assertIn("if (e.native_rigid && !e.reorderable) break", queue)

    def test_wave_metadata_is_produced_from_native_cpu_asset_before_upload(self):
        upload = (ROOT / "src/gpu/scene/native_mesh.cpp").read_text().split(
            "std::shared_ptr<const NativeGeometry> Upload(", 1)[1].split(
            "std::shared_ptr<const NativeGeometry> Import(", 1)[0]
        self.assertLess(upload.index("BuildNativeMeshWaveWeight"), upload.index("buffer->map()"))
        self.assertIn("result->water_vertex_input = std::move(water_input)", upload)
        self.assertIn("result->wave_weight = wave_weight", upload)
        bounds = (ROOT / "src/gpu/scene/native_water_scene.h").read_text()
        for required in ("TransformNativeBounds", "1.5 * std::abs", "std::nextafter"):
            self.assertIn(required, bounds)

    def test_completed_writer_connects_to_native_sorted_draw_before_legacy_bind(self):
        bridge = (ROOT / "src/gpu/scene/native_refraction_material_bridge.cpp").read_text()
        direct = bridge.split("void PublishWaterOutput()", 1)[1].split("void SubmitWater()", 1)[0]
        self.assertLess(direct.index("ReadNativeWaterMaterial"), direct.index("scope.Publish"))
        submit = bridge.split("bool NativeWaterMaterialScope::Submit", 1)[1]
        self.assertNotIn("ReadNativeWaterMaterial", submit)
        for required in ("publication_.Read", "FindNativeInstancePose", "FindLoadedNativeModelNodeImport",
                         "FindCompletedNativeWaterBottom", "pass.lights = lights_",
                         "commands->WritesImage", "SubmitNativeWaterScenePackets"):
            self.assertIn(required, submit)
        consumer = (ROOT / "src/gpu/scene/deferred_consumer.cpp").read_text()
        start = consumer.index("NativeWaterMaterialScope water(")
        section = consumer[start:]
        self.assertLess(section.index("water.Draw"), section.index("bridge.Material(36"))
        self.assertLess(section.index("water.Draw"), section.index("BindEntry("))
        self.assertIn("if (item.native || water_entry) CloseDeferredCompatibilityCapture()", consumer)
        walk = (ROOT / "src/gpu/scene/host_walk.cpp").read_text()
        self.assertIn("if (late_water_bounds)", walk)
        self.assertIn("if (have_eye && !late_water_bounds)", walk)
        self.assertIn("if (have_light && !late_water_bounds)", walk)

    def test_direct_water_has_single_ordered_light_commit_and_no_material_dispatch(self):
        bridge = (ROOT / "src/gpu/scene/native_refraction_material_bridge.cpp").read_text()
        direct = bridge.split("bool NativeWaterMaterialScope::Draw(", 1)[1].split("void NativeWaterMaterialScope::Publish(", 1)[0]
        for required in ("CheckNativeWaterModelContract", "ResolveNativeSceneLights", "CommitNativeSceneLights",
                         "ConsumeNativeWaterMaterial", "RefreshNativeVisualInputsAfterWriter", "Video::SetTexture(7,nullptr)"):
            self.assertIn(required, direct)
        for forbidden in ("__imp__", "function_dispatcher", "sub_82174270(", "sub_82286228(", "sub_82454720(", "sub_824548A8("):
            self.assertNotIn(forbidden, direct)
        submit = bridge.split("bool NativeWaterMaterialScope::Submit(", 1)[1]
        self.assertNotIn("CommitNativeSceneLights", submit)
        self.assertNotIn("ResolveNativeSceneLights", submit)

    def test_water_reuses_native_visual_scope_and_owned_late_object_inputs(self):
        bridge = (ROOT / "src/gpu/scene/native_refraction_material_bridge.cpp").read_text()
        self.assertNotIn("water_output", bridge) # retired callback-capture side channel
        publish = bridge.split("void NativeWaterMaterialScope::Publish(", 1)[1].split("bool NativeWaterMaterialScope::Submit(", 1)[0]
        self.assertLess(publish.index("ReadMaterialObjectInputs"), publish.index("publication_.Publish"))
        submit = bridge.split("bool NativeWaterMaterialScope::Submit(", 1)[1]
        self.assertNotIn("ReadMaterialObjectInputs", submit)
        self.assertIn("features,output->object", submit)
        consumer = (ROOT / "src/gpu/scene/deferred_consumer.cpp").read_text()
        self.assertIn("item.native || (water_entry && visual_inputs.Read", consumer)
        scope = consumer.split("struct NativeDeferredVisualScope", 1)[1].split("ReadNativeDeferredEffects", 1)[0]
        self.assertLess(scope.index("PrepareNativePrimaryReceiver"), scope.index("publication.ReadAfterWriter"))
        self.assertLess(scope.index("publication.ReadAfterWriter"), scope.index("BeginDeferredMaterialCompatibility"))
        self.assertIn("if (!requested.empty())", consumer) # water-only batches also publish

    def test_native_water_preserves_sorted_alpha_and_coverage(self):
        native = (ROOT / "src/gpu/scene/native_water_inputs.h").read_text()
        pixel = (ROOT / "src/gpu/shaders/hlsl/native_water_ps.hlsl").read_text()
        backend = (ROOT / "src/gpu/scene/native_rigid_draw.cpp").read_text().split(
            "bool SubmitNativeWaterScenePackets(", 1)[1].split("void PrepareNativeRigidBatchDraw(", 1)[0]
        self.assertIn("SetNativeWaterCutout", native)
        self.assertIn("RigidCutoutPasses(material.modes.z,opacity,asfloat(material.modes.w))", pixel)
        self.assertIn("pipeline_state.enableAlphaToCoverage = plan.alpha_to_coverage && shape->samples > 1", backend)
        fixture = (ROOT / "tools/native_scene_snapshot_test/water.cpp").read_text()
        for required in ("NativeWaterMaterialPublication", "Invalid water alpha update is transactional",
                         "if (discarded) expected = {9,8,7,1}", "mode <= 17"):
            self.assertIn(required, fixture)

    def test_dynamic_images_retain_actual_live_producer_views(self):
        paths = ("src/gpu/scene/native_scene_snapshot_bridge.cpp", "src/gpu/hooks/native_deferred_visuals.cpp",
                 "src/gpu/scene/native_scene_pass_bridge.cpp", "src/gpu/scene/native_shadow_pass_bridge.cpp",
                 "src/gpu/resolve.cpp")
        for path in paths:
            self.assertIn("NativeImageLease::From(", (ROOT / path).read_text())
        water = (ROOT / "src/gpu/scene/native_water_scene.h").read_text()
        self.assertIn("NativeImageLease planar, snapshot, bottom, shadow", water)
        self.assertIn("lease.ArrayView()", water)
        for forbidden in ("NativeTargetImageHandle", "GuestTexture", "textureView", "static_pointer_cast",
                          "AcquireNativePostImage", "AcquireNativeTargetImage", "copyTexture"):
            self.assertNotIn(forbidden, water)
        fixture = (ROOT / "tools/native_scene_snapshot_test/water.cpp").read_text()
        self.assertIn("NativePostImagePool snapshot_pool", fixture)
        self.assertIn("NativeImageLease::From(snapshot)", fixture)
        self.assertIn("Queued water prevents a new snapshot writer", fixture)

    def test_bottom_pass_replaces_surface_allocation_and_console_resolve(self):
        bridge = (ROOT / "src/gpu/scene/native_water_bottom_bridge.cpp").read_text()
        for required in ("REX_HOOK_RAW(sub_82187878)", "REX_HOOK_RAW(sub_82187A00)",
                         "HostTargetClass::WaterBottomDepth", "NativeSceneCommands::CreateDepthOnly",
                         "FinishNativeWaterBottom", "ReadNativeWaterBottom", "Video::PublishNativeImage",
                         "DrawQueueFlush", "RetainResourceAdapter", "LeaveNativePass"):
            self.assertIn(required, bridge)
        for forbidden in ("D3DDevice_", "hcgD3DCreateSurface", "bdSurfaceSetMSAA(", "bdDestroySurface(",
                          "HostTargetDropLinks", "copyTexture", "ResolveGuestTexture", "SetTexture(9"):
            self.assertNotIn(forbidden, bridge)
        core = (ROOT / "src/gpu/scene/native_water_bottom.h").read_text()
        for forbidden in ("PPCContext", "bd::mem", "sourceSurface", "GuestTexture", "copyTexture"):
            self.assertNotIn(forbidden, core)
        self.assertIn("camera->world_to_clip", core)
        scene = (ROOT / "src/gpu/scene/native_scene_pass_bridge.cpp").read_text()
        self.assertEqual(scene.count("ActiveNativeWaterBottomCommands(color, depth)"), 2)
        water = (ROOT / "src/gpu/scene/native_water_scene.h").read_text()
        self.assertIn("target(bottom,layers.z,true)", water)

    def test_reflection_pass_renders_into_exclusive_native_output_owner(self):
        bridge = (ROOT / "src/gpu/scene/native_reflection_pass_bridge.cpp").read_text()
        for required in ("REX_HOOK_RAW(sub_821875F8)", "REX_HOOK_RAW(sub_821877C8)",
                         "AcquireNativePostImage", "CreateNativeColorAttachmentAdapter", "HostTargetClass::ReflectionDepth",
                         "AcquireNativeLeasedColorFramebuffer", "NativeSceneCommands::CreateLeasedColor",
                         "FinishNativeReflection", "Video::PublishNativeImage", "DrawQueueFlush", "LeaveNativePass"):
            self.assertIn(required, bridge)
        for forbidden in ("D3DDevice_", "hcgD3DCreateSurface", "bdSurfaceSetMSAA(", "bdDestroySurface(",
                          "copyTexture", "HostTargetClass::ReflectionColor"):
            self.assertNotIn(forbidden, bridge)
        core = (ROOT / "src/gpu/scene/native_reflection_pass.h").read_text()
        for forbidden in ("PPCContext", "bd::mem", "sourceSurface", "GuestTexture", "copyTexture"):
            self.assertNotIn(forbidden, core)
        scene = (ROOT / "src/gpu/scene/native_scene_pass_bridge.cpp").read_text()
        self.assertEqual(scene.count("ActiveNativeReflectionCommands(color, depth)"), 2)
        fixture = (ROOT / "tools/native_scene_snapshot_test/water.cpp").read_text()
        for required in ("AcquireLeasedColor", "CreateLeasedColor", "FinishNativeReflection", "NativeImageLease::From(reflection)",
                         "weak_reflection.expired()", "Reflection cannot publish before its pending clear"):
            self.assertIn(required, fixture)

    def test_water_frame_images_are_direct_completed_producer_results(self):
        bridge = (ROOT / "src/gpu/scene/native_refraction_material_bridge.cpp").read_text()
        planar = bridge.split("void BindPlanarReflection()", 1)[1].split("void BindSceneImage()", 1)[0]
        self.assertIn("FindCompletedNativeWaterReflection()", planar)
        self.assertIn("outgoing->nativeImage == output.planar", planar)
        self.assertNotIn("ResolveGuestTexture", planar)
        snapshot = bridge.split("void Snapshot()", 1)[1].split("void Prepare(", 1)[0]
        self.assertIn("ProduceNativeSceneSnapshot(material,output.snapshot)", snapshot)
        for forbidden in ("scene_getter", "reflection_getter", "ResolveGuestTexture", "nativeImage"):
            self.assertNotIn(forbidden, snapshot)
        reflection = (ROOT / "src/gpu/scene/native_reflection_pass_bridge.cpp").read_text()
        self.assertIn("pass.plane == kWaterPlane", reflection)
        self.assertLess(reflection.index("FinishNativeReflection("), reflection.index("water_reflection.Complete("))
        self.assertIn("water_reflection.Begin(bd::gpu::FrameStatFrameCount())", reflection)
        self.assertIn("water_reflection.Reset()", reflection)
        snapshot_producer = (ROOT / "src/gpu/scene/native_scene_snapshot_bridge.cpp").read_text()
        self.assertIn("output = destination->nativeImage", snapshot_producer)
        self.assertLess(snapshot_producer.index("Check(CopySceneSnapshot("), snapshot_producer.index("output = lease"))


if __name__ == "__main__":
    unittest.main()
