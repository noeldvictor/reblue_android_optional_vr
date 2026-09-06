"""Scene ownership source guards, not an independent GPU or ABI comparison."""
from pathlib import Path
import unittest


class NativeSceneBoundaryTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        root = Path(__file__).resolve().parents[1]
        cls.bridge = (root / "src/gpu/scene/native_scene_pass_bridge.cpp").read_text(encoding="utf-8")
        source = (root / "src/gpu/resolve.cpp").read_text(encoding="utf-8")
        start = source.index("bool Video::PublishSceneOutput(")
        cls.output = source[start:source.index("void Video::ResolveRtToTexture(", start)]
        cls.view = (root / "src/gpu/scene/native_view_bridge.cpp").read_text(encoding="utf-8")
        cls.view_math = (root / "src/gpu/scene/native_view.h").read_text(encoding="utf-8")
        cls.shadow = (root / "src/gpu/scene/native_shadow_pass_bridge.cpp").read_text(encoding="utf-8")
        cls.sun = (root / "src/gpu/scene/native_sun_camera_bridge.cpp").read_text(encoding="utf-8")
        cls.sun_math = (root / "src/gpu/scene/native_sun_camera.h").read_text(encoding="utf-8")
        cls.sun_fit = (root / "src/gpu/shadow_fit.cpp").read_text(encoding="utf-8")
        cls.tweaks = (root / "config/hooks/render_tweaks.toml").read_text(encoding="utf-8")
        cls.targets = (root / "src/gpu/host_targets.cpp").read_text(encoding="utf-8")
        cls.interp = (root / "src/engine/frame_interp.cpp").read_text(encoding="utf-8")

    def test_whole_native_pair_does_not_use_console_allocation_or_resolve(self):
        native = self.bridge[self.bridge.index("bool Begin("):self.bridge.index("REX_HOOK_RAW(")]
        for name in ("__imp__", "hcgD3DCreateSurface", "bdRenderTargetRelease",
                     "ClassifyHostTarget", "bdResolveToTexture", "D3DDevice_Resolve",
                     "D3DDevice_BeginTiling", "D3DDevice_EndTiling",
                     "kStack + 232", "kStack + 236", "kStack + 240"):
            self.assertNotIn(name, native)
        self.assertIn("HostTargetClass::SceneColor", native)
        self.assertIn("HostTargetClass::SceneDepth", native)
        self.assertIn("PublishSceneOutput(pass.depth, depth_output, 1.0f, true, &sampled_depth)", native)
        self.assertIn("PublishSceneOutput(pass.color, color_output, exposure, true, &sampled_color)", native)
        self.assertLess(native.index("LeaveNativePass(result)"),
                        native.index("ReleaseResourceAdapter(pass.color->selfVa)"))

    def test_explicit_output_never_selects_a_source_by_binding_or_dimensions(self):
        for name in ("TrackResolveSource", "ResolveSourceForFlagsLocked",
                     "last_drawn", "s.render_target", "s.depth_stencil",
                     "resolveClearToFar = true", "HostResourceHeap", "ResolveGuestTexture"):
            self.assertNotIn(name, self.output)
        self.assertIn("dst->sourceSurface = src", self.output)
        self.assertIn("dst->resolveScale = exposure", self.output)
        # Keep the remaining downstream compatibility dependency visible.
        self.assertIn("NoteTileContentLocked", self.output)

    def test_native_depth_handoff_finishes_resolve_before_borrowing_without_copy(self):
        end = self.bridge.split("bool End(", 1)[1].split("} // namespace", 1)[0]
        self.assertLess(end.index("DrawQueueFlush(s.command_list)"), end.index("FinishNativeSceneResolves(s, *pass.resolves)"))
        self.assertLess(end.index("FinishNativeSceneResolves(s, *pass.resolves)"), end.index("const NativeImageLease depth_image"))
        depth = end.split("const NativeImageLease depth_image", 1)[1].split("++stats.outputs", 1)[0]
        native, compatibility = depth.split("} else {", 1)
        self.assertIn("pass.resolves->Sampled(1.f).depth", native)
        self.assertIn("Video::CanPublishNativeImage(depth_image, depth_output)", native)
        self.assertIn("Video::PublishNativeImage(depth_image, depth_output)", native)
        self.assertIn("++stats.native_depth_publications", native)
        for forbidden in ("PublishSceneOutput(", "copyTexture", "ResolveRtToTexture", "NoteTileContentLocked"):
            self.assertNotIn(forbidden, native)
        self.assertIn("++stats.compatibility_depth_publications", compatibility)
        self.assertIn("Video::PublishSceneOutput(pass.depth, depth_output", compatibility)

    def test_output_receipt_carries_exposure_exactly_once_and_only_on_success(self):
        self.assertIn("SceneImage result{src, exposure}", self.output)
        self.assertIn("result = {dst, 1.0f}", self.output)
        self.assertIn("if (sampled) *sampled = result", self.output)
        self.assertGreater(self.output.index("*sampled = result"), self.output.rindex("return false"))
        self.assertGreater(self.output.index("result = {dst, 1.0f}"), self.output.index("CopySurfaceToTextureLocked("))

    def test_completion_is_view_scoped_single_use_and_keeps_explicit_ownership(self):
        scope = self.interp.split("REX_HOOK_RAW(bdRenderViewSubmit)", 1)[1].split("REX_HOOK_RAW(bdBuildViewMatrix)", 1)[0]
        self.assertLess(scope.index("NativeSceneResultScope scene_result(ctx.r3.u32)"),
                        scope.index("__imp__bdRenderViewSubmit(ctx, base)"))
        complete = self.bridge.split("bool NativeSceneResultScope::Complete(", 1)[1].split("NativeSceneResultScope::Take(", 1)[0]
        for check in ("frame_ != FrameStatFrameCount()", "color_getter != color_getter_",
                      "depth_getter != depth_getter_", "HostTargetPin(sources[i])",
                      "RetainResourceAdapter(outputs[i]->selfVa)", "result_.Complete(frame_, std::move(result))"):
            self.assertIn(check, complete)
        take = self.bridge.split("NativeSceneResultScope::Take(", 1)[1].split("TakeCompletedSceneImages(", 1)[0]
        self.assertIn("view != view_", take)
        self.assertIn("result_.Take(FrameStatFrameCount())", take)
        self.assertIn("current_result = previous_", self.bridge)
        for name in ("HostPostImportInputs", "sourceSurface", "resolveScale", "Content("):
            self.assertNotIn(name, complete + take)
        reset = self.bridge.split("void CompletedSceneImages::Reset()", 1)[1].split("NativeSceneResultScope::NativeSceneResultScope", 1)[0]
        self.assertIn("ReleaseResourceAdapter(std::exchange(address, 0))", reset)
        self.assertIn("HostTargetUnpin(std::exchange(image, nullptr))", reset)

    def test_deferred_colour_never_drops_an_unconsumed_normal_result(self):
        end = self.bridge.split("bool End(", 1)[1].split("} // namespace", 1)[0]
        self.assertIn("const bool deferred_color = complete_scene &&", end)
        self.assertIn("bd::mem::load<uint32_t>(source + 8), native_inputs,", end)
        self.assertIn("if (color_output && !deferred_color)", end)
        self.assertIn("if (depth_output)", end)  # external depth readers still get publication
        clear = self.bridge.split("void NativeSceneResultScope::Clear()", 1)[1].split("bool NativeSceneResultScope::Complete(", 1)[0]
        self.assertIn("result_.Take(frame_)", clear)
        self.assertIn("pending->PublishPendingColor()", clear)
        view = self.interp.split("REX_HOOK_RAW(bdRenderViewSubmit)", 1)[1].split("REX_HOOK_RAW(bdBuildViewMatrix)", 1)[0]
        self.assertLess(view.index("__imp__bdRenderViewSubmit(ctx, base)"), view.index("scene_result.Clear()"))
        publish = self.bridge.split("void CompletedSceneImages::PublishPendingColor()", 1)[1].split("void CompletedSceneImages::Reset()", 1)[0]
        self.assertIn("if (!pending_scene_color) return", publish)
        self.assertIn("Video::PublishSceneOutput(source_pins[0], output, inputs.exposure)", publish)
        self.assertLess(publish.index("PublishSceneOutput("), publish.index("pending_scene_color = false"))

    def test_resolve_owner_is_native_and_fence_retired_before_image_graveyards(self):
        root = Path(__file__).resolve().parents[1]
        owner = (root / "src/gpu/scene/native_scene_resolves.cpp").read_text(encoding="utf-8")
        for name in ("GuestTexture", "HostResourceHeap", "D3DDevice_", "sourceSurface", "copyTexture", "drawInstanced"):
            self.assertNotIn(name, owner)
        self.assertIn("FencedAssetCache<NativeSceneResolves>", owner)
        self.assertIn("fb.depthResolveMode = plume::RenderResolveMode::MIN", owner)
        self.assertIn("fb.stencilResolveMode = plume::RenderResolveMode::NONE", owner)
        self.assertLess(owner.index("slot = AllocateSlot(s)"), owner.index("WriteTextureDescriptor(s, result->descriptors[i]"))
        frame = (root / "src/gpu/frame_ring.cpp").read_text(encoding="utf-8")
        self.assertLess(frame.index("scene::DrainNativeSceneResolvesLocked(s, slot)"), frame.index("s.texture_view_graveyard[slot].clear()"))
        self.assertLess(frame.index("scene::DrainNativeSceneResolvesLocked(s, slot)"), frame.index("scene::MarkUnusedNativeSceneResolvesLocked(s, slot)"))

    def test_single_sample_images_have_native_ownership_and_direct_post_inputs(self):
        root = Path(__file__).resolve().parents[1]
        owner = (root / "src/gpu/native_target_images.h").read_text(encoding="utf-8")
        allocator = (root / "src/gpu/native_target_images.cpp").read_text(encoding="utf-8")
        self.assertIn("FencedAssetCache<NativeTargetImage>", owner)
        for forbidden in ("GuestTexture", "HostResourceHeap", "ConvertGuestFormat", "SurfacePool::",
                          "copyTexture", "sourceSurface", "NoteTileContentLocked", "AdoptNativeTargetImage"):
            self.assertNotIn(forbidden, owner + allocator)
        self.assertIn("CreateHostTexture(s.device.get(), desc", allocator)
        self.assertIn("getSampleCountsSupported(shape.format) & shape.samples", allocator)
        self.assertIn("desc.arraySize = shape.layers", allocator)
        self.assertIn("shape.format, descriptor, shape.samples", owner)
        self.assertIn("NativeImageLease{color->nativeTarget, color->nativeTarget->Sampled()}", self.bridge)
        end = self.bridge.split("bool End(", 1)[1].split("} // namespace", 1)[0]
        self.assertNotIn("BorrowPostImage(", end)
        self.assertIn("HostPostInputs{pass.source_images[0].image, pass.source_images[1].image, exposure}", end)
        self.assertIn("pass.resolves->Sampled(1.f).depth} : pass.source_images[1]", end)
        self.assertLess(end.index("DrawQueueFlush(s.command_list)"), end.index("s.command_list->setFramebuffer(nullptr)"))
        self.assertLess(end.index("s.command_list->setFramebuffer(nullptr)"), end.index("Video::PublishNativeImage("))
        frame = (root / "src/gpu/frame_ring.cpp").read_text(encoding="utf-8")
        self.assertLess(frame.index("s.framebuffer_graveyard[slot].clear()"), frame.index("DrainNativeTargetImagesLocked(s, slot)"))
        self.assertLess(frame.index("DrainNativeTargetImagesLocked(s, slot)"), frame.index("s.texture_view_graveyard[slot].clear()"))
        self.assertLess(frame.index("DrainNativeTargetImagesLocked(s, slot)"), frame.index("MarkUnusedNativeTargetImagesLocked(s, slot)"))
        pool = (root / "src/gpu/surface_pool.cpp").read_text(encoding="utf-8")
        returned = pool.split("bool SurfacePool::Return(", 1)[1].split("void SurfacePool::Tick(", 1)[0]
        self.assertLess(returned.index("surface->nativeImage.owner"), returned.index("ReturnLocked("))
        self.assertLess(returned.index("surface->nativeTarget"), returned.index("ReturnLocked("))

    def test_scene_allocator_has_native_inputs_and_header_only_adapter(self):
        begin = self.bridge.split("bool Begin(", 1)[1].split("bool End(", 1)[0]
        self.assertIn("HostTargetAcquireNative(HostTargetClass::SceneColor", begin)
        self.assertIn("HostTargetAcquireNative(HostTargetClass::SceneDepth", begin)
        for forbidden in ("HostTargetAcquire(", "kColorFormat", "kDepthFormat", "0x1A2201BF", "0x2D200196"):
            self.assertNotIn(forbidden, begin)
        adapter = self.targets.split("GuestTexture *HostTargetAcquireNative(", 1)[1].split("void HostTargetReleased(", 1)[0]
        self.assertLess(adapter.index("slot.readers"), adapter.index("Disown(target)"))
        self.assertLess(adapter.index("AcquireNativeTargetImage(identity, shape)"), adapter.index("HostResourceHeap::Alloc<GuestTexture>"))
        for forbidden in ("SurfacePool::", "ConvertGuestFormat", "CreateHostTexture", "textureHolder =", "textureView ="):
            self.assertNotIn(forbidden, adapter)
        self.assertIn("target->nativeTarget = image", adapter)
        self.assertIn("target->layout.Bind(image->layout)", adapter)

    def test_native_resolve_retains_its_actual_source_images(self):
        root = Path(__file__).resolve().parents[1]
        owner = (root / "src/gpu/scene/native_scene_resolves.h").read_text(encoding="utf-8")
        allocation = (root / "src/gpu/scene/native_scene_resolves.cpp").read_text(encoding="utf-8")
        self.assertLess(owner.index("std::array<NativeTargetImageHandle, 2> source_owners"), owner.index("std::unique_ptr<plume::RenderFramebuffer> framebuffer"))
        self.assertIn("source_owners[0]->image.get() != sources.color", allocation)
        self.assertIn("source_owners[1]->image.get() != sources.depth", allocation)
        self.assertIn("result->source_owners = source_owners", allocation)
        self.assertIn("AcquireNativeSceneResolves(sources, {color->nativeTarget, depth->nativeTarget})", self.bridge)

    def test_single_sample_scene_framebuffer_is_native_and_fence_retained(self):
        root = Path(__file__).resolve().parents[1]
        owner = (root / "src/gpu/scene/native_scene_framebuffer.h").read_text(encoding="utf-8")
        factory = (root / "src/gpu/scene/native_scene_framebuffer.cpp").read_text(encoding="utf-8")
        for forbidden in ("GuestTexture", "HostResourceHeap", "SurfacePool", "sourceSurface",
                          "guestFormat", "copyTexture", "drawInstanced", "GetFramebuffer("):
            self.assertNotIn(forbidden, owner + factory)
        self.assertIn("FencedAssetCache<NativeSceneFramebuffer>", owner)
        self.assertLess(owner.index("std::array<NativeTargetImageHandle, 2> sources"),
                        owner.index("std::unique_ptr<plume::RenderFramebuffer> framebuffer"))
        self.assertIn("owner->sources == sources && owner->density_map == density_map", owner)
        self.assertIn("AcquireNativeSceneFramebuffer({color->nativeTarget, depth->nativeTarget}, density_map)", self.bridge)
        frame = (root / "src/gpu/frame_ring.cpp").read_text(encoding="utf-8")
        self.assertLess(frame.index("scene::DrainNativeSceneFramebuffersLocked(s, slot)"),
                        frame.index("DrainNativeTargetImagesLocked(s, slot)"))
        self.assertLess(frame.index("scene::DrainNativeSceneFramebuffersLocked(s, slot)"),
                        frame.index("scene::MarkUnusedNativeSceneFramebuffersLocked(s, slot)"))

    def test_scene_uses_prebuilt_framebuffer_before_compatibility_cache(self):
        root = Path(__file__).resolve().parents[1]
        draw = (root / "src/gpu/draw_framebuffer.cpp").read_text(encoding="utf-8")
        get = draw.split("plume::RenderFramebuffer *GetFramebuffer(", 1)[1].split("void ResolveEffectiveTargets(", 1)[0]
        self.assertLess(get.index("scene::ActiveNativeSceneFramebuffer("), get.index("container->framebuffers.find"))
        active = self.bridge.split("plume::RenderFramebuffer *ActiveNativeSceneFramebuffer(", 1)[1].split("CompletedSceneImages::", 1)[0]
        self.assertIn("ActiveNativeSceneResolves(color, depth)", active)
        self.assertIn("pass.framebuffer->Matches(color, depth)", active)
        for forbidden in ("width", "height", "tile", "createFramebuffer", "HostResourceHeap"):
            self.assertNotIn(forbidden, active)

    def test_native_scene_clear_is_scope_owned_not_a_console_request(self):
        root = Path(__file__).resolve().parents[1]
        core = (root / "src/gpu/scene/native_scene_commands.h").read_text(encoding="utf-8")
        for forbidden in ("GuestTexture", "HostResourceHeap", "hostClearFlags", "clear_flags",
                          "RequestClear", "EDRAM", "sourceSurface", "copyTexture", "drawInstanced"):
            self.assertNotIn(forbidden, core)
        begin = self.bridge.split("bool Begin(", 1)[1].split("bool End(", 1)[0]
        self.assertIn("NativeSceneCommands::Create", begin)
        self.assertNotIn("Video::RequestClear", begin)
        self.assertIn("BeginCommandList(s)", begin)
        end = self.bridge.split("bool End(", 1)[1].split("} // namespace", 1)[0]
        self.assertLess(end.index("pass.commands->ClearPending()"), end.index("DrawQueueFlush(s.command_list)"))
        self.assertIn("clear_.reset()", core)

    def test_native_writes_skip_seed_alias_and_compatibility_layout_selection(self):
        root = Path(__file__).resolve().parents[1]
        draw = (root / "src/gpu/draw_framebuffer.cpp").read_text(encoding="utf-8")
        legacy = draw.split("if (!native_commands) {", 1)[1].split("// What does the depth-tested", 1)[0]
        for operation in ("AliasFreshTargetToChainHeadLocked", "TransitionTargetsToWrite",
                          "SeedFreshColorTarget", "fullscreen_chain_head"):
            self.assertIn(operation, legacy)
        bind = draw.split("plume::RenderFramebuffer *fb = native_commands", 1)[1]
        self.assertLess(bind.index("DrawQueueFlushAt"), bind.index("scene::BindNativeSceneCommands"))
        self.assertLess(bind.index("scene::BindNativeSceneCommands"), bind.index("scene::ApplyNativeSceneClear"))
        self.assertNotIn("PrepareNativeSceneResolveWrites", draw)

    def test_native_scene_precision_is_format_owned_with_only_getter_publication(self):
        root = Path(__file__).resolve().parents[1]
        adapter = (root / "src/gpu/scene/scene_precision_import.h").read_text(encoding="utf-8")
        begin = self.bridge.split("bool Begin(", 1)[1].split("bool End(", 1)[0]
        self.assertNotIn("SetState(ctx, base, 308", begin)
        self.assertIn("PublishScenePrecisionGetters(precision_device", begin)
        self.assertLess(begin.index("kScenePrecisionDeviceSlot"), begin.index("HostTargetAcquireNative"))
        self.assertIn("plume::RenderFormat::R16G16B16A16_FLOAT", begin)
        for forbidden in ("bdSetRenderState", "__imp__", "PPCContext", "GuestTexture",
                          "REX_", "10244", "dirty24", "copyTexture", "drawInstanced"):
            self.assertNotIn(forbidden, adapter)
        self.assertEqual(adapter.count("  write("), 2)

    def test_pin_blocks_native_target_reuse_and_recreation_before_mutation(self):
        acquire = self.targets.split("GuestTexture *HostTargetAcquire(", 1)[1].split("void HostTargetReleased(", 1)[0]
        self.assertLess(acquire.index("if (slot.readers)"), acquire.index("Disown(t)"))
        self.assertLess(acquire.index("if (slot.readers)"), acquire.index("InitResourceHeader("))
        pin = self.targets.split("bool HostTargetPin(", 1)[1].split("bool HostTargetRequestClear(", 1)[0]
        for name in ("std::lock_guard lock(g_mutex)", "slot.target != target", "++slot.readers", "--slot.readers"):
            self.assertIn(name, pin)
        begin = self.bridge.split("REX_HOOK_RAW(sub_82186BA0)", 1)[1].split("REX_HOOK_RAW(sub_82187010)", 1)[0]
        self.assertLess(begin.index("current_result->Clear()"), begin.index("Begin(ctx, base, source)"))
        end = self.bridge.split("bool End(", 1)[1].split("} // namespace", 1)[0]
        self.assertIn("bd::mem::load<int32_t>(kPhase) == 3", end)
        self.assertLess(end.index("current_result->Complete("), end.index("ReleaseResourceAdapter(pass.color->selfVa)"))

    def test_view_producer_does_not_delegate_its_math(self):
        native = self.view[self.view.index("bool Produce("):self.view.index("__imp__sub_82186840(ctx, base);")]
        for name in ("__imp__", "sub_822873E0", "sub_82287478", "sub_821CCC78",
                     "sub_82491748", "bdMatrixInverse4x4", "sub_82277198", "sub_8217A8D0"):
            self.assertNotIn(name, native)
        self.assertIn("GetNativeRenderTransforms()", native)
        self.assertIn("BuildViewFrustumShape", native)
        self.assertIn("views.Get(view)", native)
        self.assertIn("PublishCachedViewFrustum(ctx, 1)", self.bridge)
        # No address-based memory access, PPC context or GPU SDK in the core.
        for name in ("PPCContext", "bd::mem::", "REX_", "plume::", "kCache"):
            self.assertNotIn(name, self.view_math)

    def test_view_comparison_precedes_native_publication(self):
        native = self.view[self.view.index("bool Produce("):self.view.index("} // namespace")]
        self.assertLess(native.index("__imp__sub_82186840(ctx, base)"),
                        native.index("CompareWords(kShape"))
        self.assertLess(native.index("CompareWords(kShape"), native.index("Publish(shape, frustum)"))
        self.assertIn('CompareWords(slot + 4, Pack(shape), "cache")', native)

    def test_shadow_lifecycle_has_explicit_depth_and_output_ownership(self):
        native = self.shadow[self.shadow.index("bool Begin("):self.shadow.index("REX_HOOK_RAW(")]
        for name in ("__imp__", "hcgD3DCreateSurface", "bdRenderTargetRelease",
                     "ClassifyHostTarget", "bdSurfaceSetMSAA", "bdDestroySurface",
                     "D3DDevice_Resolve", "TrackResolveSource", "surfaceDrawn"):
            self.assertNotIn(name, native)
        self.assertIn("HostTargetClass::Shadow", native)
        self.assertIn("EnterNativePass(nullptr, depth, result)", native)
        self.assertIn("shadows.push_back({source, depth, output, NativePassDepth()})", native)
        self.assertIn("RetainResourceAdapter(output->selfVa)", native)
        end = native[native.index("bool End("):]
        self.assertIn("Video::BindDrawFramebuffer()", end)
        self.assertIn("PublishSceneOutput(pass.depth, pass.output, 1.0f, false)", end)
        self.assertLess(end.index("LeaveNativePass(result)"),
                        end.index("ReleaseResourceAdapter(pass.depth->selfVa)"))
        self.assertNotIn("bd_native_shadow_passes", end)  # scopes outlive setting changes

    def test_shadow_output_does_not_publish_a_post_chain(self):
        self.assertIn("if (publish_post_chain)\n    NoteTileContentLocked", self.output)
        self.assertIn("PublishSceneOutput(pass.depth, pass.output, 1.0f, false)", self.shadow)
        # These two producers are deliberately still counted, never called native.
        self.assertIn("++stats.camera_snapshots", self.shadow)
        self.assertIn("++stats.light_fits", self.shadow)

    def test_native_sun_camera_has_no_guest_fitting_execution(self):
        for name in ("__imp__", "sub_", "REX_EXTERN", "PPCContext", "ClassifyPass"):
            self.assertNotIn(name, self.sun)
        for name in ("PPCContext", "bd::mem::", "REX_", "plume::"):
            self.assertNotIn(name, self.sun_math)
        self.assertIn("BuildNativeSunCamera(transforms->inputs.view", self.sun)
        self.assertIn("if (!ProduceNativeSunCamera", self.shadow)
        self.assertIn("PublishNativeViewVolume(1, sun->frustum)", self.shadow)

    def test_native_sun_consumers_bypass_old_register_fit(self):
        start = self.sun_fit.index("void ShadowFitOnVertexBlock(")
        native = self.sun_fit[start:self.sun_fit.index("const Pass pass =", start)]
        self.assertIn("scene::GetNativeSunCamera()", native)
        self.assertIn("return;", native)
        self.assertNotIn("WriteRegs", native)
        self.assertIn("views.Volume(view)", self.view)

    def test_native_sun_culling_uses_owned_scope_and_volume(self):
        native = self.shadow[self.shadow.index("REX_HOOK_RAW(sub_82287788)"):]
        self.assertIn("NativePassDepth() == shadows.back().nesting", native)
        self.assertIn("IntersectsSunVolume(sun->frustum, center, radius)", native)
        self.assertIn("if (REXCVAR_GET(bd_shadow_fit_diag))", native)
        self.assertNotIn("REX_HOOK_RAW(bdVisualObjectFrustumCull)", self.shadow)

    def test_native_character_visibility_does_not_use_light_eye_distance(self):
        hooks = [h for h in self.tweaks.split("[[midasm_hook]]")
                 if 'name = "bdNativeSunCharacterDistanceHook"' in h]
        self.assertEqual(len(hooks), 1)
        for field in ('address = 0x822D3AF8', 'jump_address_on_true = 0x822D3B14',
                      'registers = ["r26", "f0"]', 'after_instruction = false'):
            self.assertIn(field, hooks[0])
        native = self.shadow[self.shadow.index("bool bdNativeSunCharacterDistanceHook("):]
        for condition in ("view.u32 != 1", "shadows.empty()", "!shadows.back().depth",
                          "NativePassDepth() != shadows.back().nesting", "!GetNativeSunCamera()"):
            self.assertIn(condition, native)
        self.assertIn("++stats.character_depth_skipped", native)


if __name__ == "__main__":
    unittest.main()
