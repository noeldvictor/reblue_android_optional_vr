"""DoF source ownership guards, not independent GPU qualification."""
from pathlib import Path
import re
import unittest


class NativePostBoundaryTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        root = Path(__file__).resolve().parents[1]
        cls.root = root
        cls.hooks = (root / "config/hooks/render_tweaks.toml").read_text(encoding="utf-8")
        cls.bridge = (root / "src/gpu/native_dof_bridge.cpp").read_text(encoding="utf-8")
        cls.post = (root / "src/gpu/post_chain.cpp").read_text(encoding="utf-8")
        cls.parameters = (root / "src/gpu/post_parameters.h").read_text(encoding="utf-8")
        cls.scheduler = (root / "src/gpu/native_post_bridge.cpp").read_text(encoding="utf-8")
        cls.flare = (root / "src/gpu/lens_flare.h").read_text(encoding="utf-8")
        cls.flare_shader = (root / "src/gpu/shaders/hlsl/lens_flare_ps.hlsl").read_text(encoding="utf-8")
        cls.adjust_shader = (root / "src/gpu/shaders/hlsl/post_adjust_ps.hlsl").read_text(encoding="utf-8")
        cls.adjust = (root / "src/gpu/post_adjustments.h").read_text(encoding="utf-8")
        cls.scanline = (root / "src/gpu/post_scanline.h").read_text(encoding="utf-8")
        cls.scanline_shader = (root / "src/gpu/shaders/hlsl/post_scanline_ps.hlsl").read_text(encoding="utf-8")
        cls.grade = (root / "src/gpu/post_grade.h").read_text(encoding="utf-8")
        cls.grade_shader = (root / "src/gpu/shaders/hlsl/post_grade_ps.hlsl").read_text(encoding="utf-8")
        cls.passes = (root / "src/gpu/post_passes.h").read_text(encoding="utf-8")
        cls.heat = (root / "src/gpu/post_heat.h").read_text(encoding="utf-8")
        cls.composite_shader = (root / "src/gpu/shaders/hlsl/post_composite_ps.hlsl").read_text(encoding="utf-8")
        cls.bloom = (root / "src/gpu/post_bloom.h").read_text(encoding="utf-8")
        cls.bloom_shader = (root / "src/gpu/shaders/hlsl/post_bloom_direction_ps.hlsl").read_text(encoding="utf-8")
        cls.images = (root / "src/gpu/host_post_inputs.h").read_text(encoding="utf-8")
        cls.sampled = (root / "src/gpu/sampled_image.h").read_text(encoding="utf-8")
        cls.output = (root / "src/gpu/host_post_output.h").read_text(encoding="utf-8")
        cls.pool = (root / "src/gpu/native_post_images.h").read_text(encoding="utf-8")
        cls.pool_gpu = (root / "src/gpu/native_post_images.cpp").read_text(encoding="utf-8")
        cls.resolve = (root / "src/gpu/resolve.cpp").read_text(encoding="utf-8")
        cls.ring = (root / "src/gpu/frame_ring.cpp").read_text(encoding="utf-8")
        cls.bindless = (root / "src/gpu/bindless.cpp").read_text(encoding="utf-8")

    def test_native_inputs_prepare_their_own_sampling_descriptors(self):
        helper = self.post.split("bool PrepareReadable(", 1)[1].split("GuestTexture *NativeSource(", 1)[0]
        self.assertIn("BindTextureSRVLocked(s, image) == kInvalidDescriptorIndex", helper)
        self.assertLess(helper.index("BindTextureSRVLocked("), helper.index("return Readable(image)"))
        for name in ("s.textures[", "SetTexture(", "ResolveRtToTexture", "copyTexture", "__imp__"):
            self.assertNotIn(name, helper)
        for function, following in (("HostPostPrepareDof", "HostPostProducerSkip"),):
            body = self.post.split(f"bool {function}(", 1)[1].split(f"bool {following}(", 1)[0]
            self.assertEqual(body.count("PrepareReadable(s,"), 2)
            self.assertLess(body.index("lock(s.mutex)"), body.index("PrepareReadable(s,"))
            self.assertLess(body.index("const bool depth_ready"), body.index("Video::OpenCommandListLocked()"))
            self.assertIn("if (!scene_ready || !depth_ready ||", body)
        imported = self.post.split("bool HostPostImportInputs(", 1)[1].split("SampledImage BorrowPostImage(", 1)[0]
        self.assertEqual(imported.count("PrepareReadable(s,"), 2)
        self.assertLess(imported.index("lock(s.mutex)"), imported.index("PrepareReadable(s,"))
        self.assertLess(imported.index("PrepareReadable(s,"), imported.index("Snapshot(Content(scene))"))
        borrow = self.post.split("SampledImage BorrowPostImage(", 1)[1].split("bool HostPostRender(", 1)[0]
        self.assertIn("PrepareReadable(s, image) ? Snapshot(image)", borrow)
        self.assertLess(borrow.index("lock(s.mutex)"), borrow.index("PrepareReadable(s,"))
        for name in ("Content(", "sourceSurface", "s.textures[", "HostTargetAcquire", "ResolveGuestTexture"):
            self.assertNotIn(name, borrow)

    def test_native_sampled_contract_has_no_guest_resource_identity(self):
        for name in ("GuestTexture", "rex/", "selfVa", "sourceSurface", "resolveScale", "D3D", "Xenos"):
            self.assertNotIn(name, self.images + self.sampled)
        self.assertIn("SampledImage scene", self.images)
        self.assertIn("SampledImage depth", self.images)
        self.assertIn("plume::RenderTextureLayout *layout", self.sampled)
        self.assertIn("&t->layout.Get()", self.post)
        render = self.post.split("bool HostPostRender(", 1)[1].split("bool HostPostPrepareDof(", 1)[0]
        self.assertIn("target.CanRender(inputs)", render)
        self.assertLess(render.index("target.CanRender("), render.index("Video::OpenCommandListLocked()"))
        for name in ("source->selfVa", "z->selfVa", "PrepareReadable(s, source)", "PrepareReadable(s, z)"):
            self.assertNotIn(name, render)

    def test_native_output_and_optical_rendering_have_no_resource_headers(self):
        render = self.post.split("bool HostPostRender(", 1)[1].split("bool HostPostPrepareDof(", 1)[0]
        flare = self.post.split("bool RenderLensFlare(", 1)[1].split("} // namespace", 1)[0]
        for body in (self.output, render, flare):
            for name in ("GuestTexture", "selfVa", "sourceSurface", "resolveScale", "descriptorIndex",
                         "surfaceDrawn", "BindTextureSRVLocked", "HostTargetDropLinks", "nativeGpu"):
                self.assertNotIn(name, body)
        self.assertIn("const HostPostOutput &target", render)
        self.assertIn("const std::array<SampledImage, 4> &flare_images", render)
        self.assertIn("*output->layout", render)
        self.assertEqual(render.count("target.CanSampleMono(image)"), 2)
        self.assertLess(render.index("prepare_noise(heat_image"), render.index("Video::OpenCommandListLocked()"))
        self.assertIn("source.texture != image.texture", self.output)
        self.assertIn("R16G16B16A16_FLOAT", self.output)
        for dimension in ("Width", "Height"):
            self.assertIn(f"framebuffer->get{dimension}() == image.{dimension.lower()}", self.output)

    def test_native_output_does_not_mutate_resource_headers_per_root(self):
        body = self.scheduler.split("bool RenderPostPlan(", 1)[1].split("void PublishPostOutput(", 1)[0]
        self.assertLess(body.index("lock(s.mutex)"), body.index("HostPostRender("))
        self.assertLess(body.index("HostPostRender("), body.index("if (rendered)"))
        for name in ("HostTargetDropLinks", "surfaceDrawn", "target.adapter", "PublishNativePostOutput"):
            self.assertNotIn(name, body)
        self.assertNotIn("BorrowPostImage(", body)
        render = self.post.split("bool HostPostRender(", 1)[1].split("bool HostPostPrepareDof(", 1)[0]
        self.assertNotIn("lock(s.mutex)", render)  # caller owns the complete boundary transaction
        effects = self.scheduler.split("struct PostEffects", 1)[1].split("struct PostPlan", 1)[0]
        self.assertNotIn("GuestTexture", effects)

    def test_native_output_pool_has_no_header_allocator_or_guest_target_roles(self):
        for body in (self.pool, self.pool_gpu):
            for name in ("GuestTexture", "HostResourceHeap", "HostTargetAcquire", "D3D", "selfVa", "0x1A2201BF"):
                if name != "D3D":  # only the existing backend compilation guard is permitted
                    self.assertNotIn(name, body)
        self.assertIn("entry.handle.use_count() == 1", self.pool)
        self.assertIn("result->layout = plume::RenderTextureLayout::UNKNOWN", self.pool)
        self.assertIn("pixels > budget_ / stride", self.pool)
        self.assertIn("256ull << 20", self.pool)
        self.assertIn("64", self.pool)
        self.assertIn("TEXTURE_2D_ARRAY", self.pool_gpu)
        self.assertIn("recipe.layers == 2 ? 3u : 0u", self.pool_gpu)
        self.assertLess(self.pool_gpu.index("AllocateSlot(s)"), self.pool_gpu.index("WriteTextureDescriptor(s, result->descriptor"))
        self.assertLess(self.ring.index("framebuffer_graveyard[slot].clear()"), self.ring.index("DrainNativePostImagesLocked(s, slot)"))
        self.assertLess(self.ring.index("DrainNativePostImagesLocked(s, slot)"), self.ring.index("MarkUnusedNativePostImagesLocked(s, slot)"))

    def test_native_output_publication_borrows_without_copy_or_new_resource(self):
        body = self.resolve.split("bool Video::PublishNativePostOutput(", 1)[1].split("bool Video::PublishSceneOutput(", 1)[0]
        for name in ("copyTexture", "CopySurfaceToTexture", "HostResourceHeap", "CreateHostTexture", "AllocateSlot", "sourceSurface ="):
            self.assertNotIn(name, body)
        self.assertIn("PublishNativeImage({source, source->Output().image}, dst, true)", body)
        self.assertIn("dst->nativeImage = lease", body)
        self.assertIn("dst->descriptorIndex = image.descriptor_index", body)
        self.assertIn("dst->format = image.format", body)
        self.assertIn("DetachSourceSurfaceLocked(s, dst)", body)
        self.assertIn("framebuffer_graveyard[slot].push_back", body)
        self.assertLess(body.index("ReleaseTextureSRVLocked(s, dst)"), body.index("dst->nativeImage = lease"))
        self.assertIn("tex->nativeGpu || tex->nativeImage.owner", self.bindless)
        self.assertIn("tex->texture != lease.image.texture", self.bindless)
        framebuffer = (self.root / "src/gpu/draw_framebuffer.cpp").read_text(encoding="utf-8")
        self.assertIn("root->hostOwned || root->nativeImage.owner", framebuffer)

    def test_native_boundary_uses_one_live_layout_record_and_releases_it_before_owner(self):
        body = self.resolve.split("bool Video::PublishNativeImage(", 1)[1].split("bool Video::PublishSceneOutput(", 1)[0]
        self.assertLess(body.index("dst->layout.Unbind()"), body.index("dst->nativeImage = lease"))
        self.assertLess(body.index("dst->nativeImage = lease"), body.index("dst->layout.Bind(*image.layout)"))
        self.assertIn("if (image.texture != dst->texture)", body)
        self.assertIn("if (publish_post_chain)", body)
        self.assertEqual(body.count("NoteTileContentLocked("), 1)
        condition = self.resolve.split("bool NativeImageDestination(", 1)[1].split("} // namespace", 1)[0]
        for check in ("source.CanPublishExtent(dst->width, dst->height, dst->layers, extent)",
                      "IsDepthFormat(source.image.format) != IsDepthFormat(dst->format)",
                      "dst->nativeImage.owner == source.owner",
                      "dst->nativeImage.image.layout == source.image.layout"):
            self.assertIn(check, condition)
        for axis in ("width", "height", "layers"):
            self.assertIn(f"dst->{axis} = image.{axis}", body)
            self.assertLess(body.index("ReleaseTextureSRVLocked(s, dst)"), body.index(f"dst->{axis} = image.{axis}"))
        graveyard = (self.root / "src/gpu/graveyard.cpp").read_text(encoding="utf-8")
        self.assertLess(graveyard.index("tex->layout.Unbind()"), graveyard.index("tex->nativeImage = {}"))

    def test_directional_bloom_imports_intent_without_original_mask_production(self):
        body = self.scheduler.split("bool ReadPlan(", 1)[1].split("void VerifyAdjustmentPublication", 1)[0]
        for offset in (12612, 12624, 12636):
            self.assertIn(f"owner + {offset} + bank * 4", body)
        self.assertNotIn("if (mode == 1 && composed)", body)
        self.assertIn("bloom.directional.enabled", body)
        for name in ("__imp__", "sub_8221E700(", "sub_8221E758(", "REX_STORE", "psFloatConstants"):
            self.assertNotIn(name, body)
        for name in ("PPCContext", "bd::mem::", "plume::", "REX_"):
            self.assertNotIn(name, self.bloom)

    def test_directional_bloom_precedes_heat_and_uses_private_non_aliasing_atlases(self):
        body = self.post.split("BloomMaskView BuildDirectionalBloom(", 1)[1].split("bool RenderLensFlare(", 1)[0]
        self.assertIn("MakeBloomAtlasStep(iteration, direction)", body)
        self.assertIn("atlases[step.input]->slot, step.source_half", body)
        self.assertIn("sizeof(kernel) == 32", body)
        self.assertIn("bright, parameters, {}, {}, 0, {}, true", body)
        self.assertIn("if (directional.iterations == 0) return {atlases[0], false}", body)
        for name in ("s.render_target", "s.textures[", "GuestPixelConstant", "ResolveRtToTexture", "copyTexture"):
            self.assertNotIn(name, body)
        render = self.post.split("bool HostPostRender(", 1)[1].split("bool HostPostPrepareDof(", 1)[0]
        self.assertIn("output->format, 5 + i, output->layers", render)
        self.assertLess(render.index("BuildDirectionalBloom("), render.index("HostComposite("))
        self.assertIn("DirectionalBloom(texCoord)", self.composite_shader)

    def test_directional_bloom_shader_clamps_each_half_and_selects_its_eye(self):
        shader = self.bloom_shader
        self.assertIn("SV_ViewID", shader)
        self.assertIn("int(position.x) % half_width + origin", shader)
        self.assertEqual(shader.count("viewId, 0)"), 3)
        self.assertIn("clamp(p + direction * int(i), lo, hi)", shader)
        self.assertIn("clamp(p - direction * int(i), lo, hi)", shader)
        for name in ("g_PSC", "g_VSC", "g_vSampleWeights", "g_vSampleOffsets"):
            self.assertNotIn(name, shader)
        self.assertIn("float2(0.5, 1) - inset", self.composite_shader)

    def test_heat_has_native_parameters_animation_and_no_guest_submission(self):
        body = self.scheduler.split("bool ReadPlan(", 1)[1].split("void VerifyAdjustmentPublication", 1)[0]
        for offset in (684, 696, 708, 720):
            self.assertIn(f"2712 + {offset} + bank * 4", body)
        self.assertIn("tail.heat.enabled = flag(16)", body)
        self.assertIn("HeatShimmerFramePhase(FrameStatFrameCount())", body)
        for token in ("__imp__", "sub_82216AE8(", "sub_82216D08(", "REX_STORE", "psFloatConstants"):
            self.assertNotIn(token, body)
        for token in ("PPCContext", "bd::mem::", "plume::", "REX_", "rand("):
            self.assertNotIn(token, self.heat)

    def test_heat_fuses_scene_coordinates_without_warping_bloom(self):
        shader = self.composite_shader
        for token in ("g_PSC", "g_VSC", "g_vSampleOffsets", "g_vSampleWeights"):
            self.assertNotIn(token, shader)
        self.assertIn("const float2 scene_uv = HeatSceneUV(texCoord)", shader)
        self.assertIn("HeatShimmerAcceptDepth(original_depth, Tap(g_Indices0.x, displaced_uv).x)", shader)
        bloom = shader.split("// Bloom:", 1)[1].split("// Param0:", 1)[0]
        self.assertIn("TapLevel(2, level[2], texCoord)", bloom)
        self.assertNotIn("scene_uv", bloom)
        self.assertIn("float3(noise_uv.u, noise_uv.v, 0)", shader)
        self.assertIn("float3(uv, float(g_ViewId))", shader)
        self.assertIn("sizeof(CompositeConstants) == 240", self.post)

    def test_grade_uses_authored_inputs_and_not_packed_draw_state(self):
        body = self.scheduler.split("bool ReadPlan(", 1)[1].split("void VerifyAdjustmentPublication", 1)[0]
        for name in ("7132 + 652", "7792 + 652", "rgb(644 + bank * 12)",
                     "rgb(672 + bank * 12)", "rgb(700 + bank * 16)", "AnimateGradeGrain("):
            self.assertIn(name, body)
        for name in ("__imp__", "sub_82219960(", "sub_82219758(", "D3DDevice_", "psFloatConstants"):
            self.assertNotIn(name, body)
        for name in ("PPCContext", "bd::mem::", "REX_", "plume::"):
            self.assertNotIn(name, self.grade)

    def test_grade_has_native_layered_source_and_explicit_constants(self):
        self.assertIn('#include "src/gpu/post_grade.h"', self.grade_shader)
        self.assertIn("float3(sample_uv, view_id)", self.grade_shader)
        self.assertIn("float3((uv + phase) * 2.2, 0)", self.grade_shader)
        self.assertIn("color.a)", self.grade_shader)
        for name in ("g_PSC", "BOOL_BIT", "g_vCcParams", "ms_tex"):
            self.assertNotIn(name, self.grade_shader)
        self.assertIn("PostEffect { Adjust, Scanline, Grade }", self.passes)
        self.assertIn("plan.count > 1 ? 2 : 1", self.passes)

    def test_scanline_has_native_animation_and_no_compatibility_tail(self):
        for name in ("RunTail", "sub_8221E700", "bdSetRenderState", "kState", "ctx.r1"):
            self.assertNotIn(name, self.scheduler)
        code = "\n".join(line.split("//", 1)[0] for line in self.scanline.splitlines())
        for name in ("PPCContext", "bd::mem::", "plume::", "REX_", "rand("):
            self.assertNotIn(name, code)
        self.assertIn("ScanlineFramePhase(", self.scheduler)
        self.assertIn("REXCVAR_GET(bd_ntsc_filter)", self.scheduler)
        self.assertIn("10172 + 620 + bank * 4", self.scheduler)
        self.assertIn("10172 + 632 + bank * 4", self.scheduler)

    def test_scanline_is_layered_four_tap_native_output_after_adjustments(self):
        shader = self.scanline_shader
        self.assertIn('#include "src/gpu/post_scanline.h"', shader)
        self.assertIn("source.GetDimensions(width, height, layers)", shader)
        self.assertEqual(shader.count("source.SampleLevel("), 4)
        self.assertEqual(shader.count("view_id), 0)"), 4)
        for name in ("235.0", "159.0", "33.0", "87.0"):
            self.assertIn(name, shader)
        body = self.post.split("bool HostPostRender(", 1)[1].split("bool HostPostPrepareDof(", 1)[0]
        self.assertIn("MakePostPasses(adjustments.Active(), scanline.enabled, grade.Active())", body)
        self.assertLess(body.index("Shader::Adjust"), body.index("Shader::Scanline"))
        self.assertIn("push.param0 = scanline.strength", body)
        self.assertIn("push.param1 = scanline.phase", body)

    def test_adjustments_use_native_input_and_shared_aspect_math(self):
        self.assertIn('#include "src/gpu/post_adjustments.h"', self.adjust_shader)
        self.assertIn("float(height) / float(width)", self.adjust_shader)
        self.assertIn("float3(sample_uv, view_id)", self.adjust_shader)
        self.assertIn("FisheyeOffsetScale(", self.adjust_shader)
        self.assertIn("ReverseColor(", self.adjust_shader)
        for name in ("PPCContext", "bd::mem::", "plume::", "REX_", "psFloatConstants"):
            self.assertNotIn(name, self.adjust)
        self.assertNotIn("RunTail", self.scheduler)
        self.assertNotIn("sub_8221E758", self.scheduler)

    def test_adjustment_input_is_private_native_scratch_not_a_seed_copy(self):
        body = self.post.split("bool HostPostRender(", 1)[1].split("bool HostPostPrepareDof(", 1)[0]
        self.assertIn("adjustments.Active()", body)
        self.assertIn("attachment(plan.composite_output)", body)
        self.assertIn("std::array<Scratch *, 2> scratch", body)
        self.assertLess(body.index("HostComposite("), body.index("RenderLensFlare("))
        self.assertLess(body.index("RenderLensFlare("), body.index("Shader::Adjust"))
        self.assertNotIn("copyTexture", body)
        self.assertNotIn("HostTargetAcquire", body)

    def test_flare_shader_folds_the_quarter_image_with_tested_mapping(self):
        self.assertIn('#include "src/gpu/lens_flare_uv.h"', self.flare_shader)
        self.assertIn("float2(LensFlareU(uv.x), LensFlareV(uv.y))", self.flare_shader)
        self.assertIn("float3(optical_uv,0)", self.flare_shader)
        self.assertIn("NonUniformResourceIndex(image)", self.flare_shader)

    def test_native_flare_is_one_instanced_draw_into_explicit_output(self):
        body = self.post.split("bool RenderLensFlare(", 1)[1].split("} // namespace", 1)[0]
        self.assertIn("drawInstanced(6, parameters.count, 0, 0)", body)
        self.assertIn("output.framebuffer", body)
        self.assertIn("NativeSource(s, image)", body)
        self.assertNotIn("Content(", body)
        for name in ("s.render_target", "GuestPixelConstant", "D3DDevice_", "s.textures[", "device_guest"):
            self.assertNotIn(name, body)
        self.assertNotIn("filter(8660", self.scheduler)
        self.assertNotIn("RunTail", self.scheduler)

    def test_flare_recipe_has_no_engine_or_register_dependency(self):
        for name in ("PPCContext", "bd::mem::", "plume::", "REX_", "psFloatConstants"):
            self.assertNotIn(name, self.flare)
        body = self.scheduler.split("bool ReadLensFlare(", 1)[1].split("bool ReadPlan(", 1)[0]
        self.assertIn("GetNativeRenderTransforms()", body)
        self.assertIn("MakeLensFlareParameters(", body)
        for name in ("sub_82183DE8(", "sub_82218140(", "__imp__", "psFloatConstants"):
            self.assertNotIn(name, body)

    def test_direct_frame_has_no_old_draw_trigger_or_target_inference(self):
        body = self.post.split("bool HostPostRender(", 1)[1].split("bool HostPostPrepareDof(", 1)[0]
        for name in ("GuestPixelConstant", "device_guest", "s.textures[", "s.render_target",
                     "HostPostIntercept", "ResolveRtToTexture", "TrackResolveSource"):
            self.assertNotIn(name, body)
        self.assertLess(body.index("DrawQueueFlush("), body.index("BuildDofAtlas("))
        self.assertIn("HostComposite(s, c, source, nullptr, composed, bloom, heat, heat_image, heat_sampler, directional)", body)
        self.assertIn("s.draw_framebuffer_bound = false", body)

    def test_composite_consumes_typed_native_parameters(self):
        body = self.post.split("bool HostComposite(", 1)[1].split("} // namespace", 1)[0]
        self.assertNotIn("GuestPixelConstant", body)
        self.assertNotIn("s.render_target", body)
        self.assertIn("parameters.scene_weight[i]", body)
        self.assertIn("parameters.threshold", body)

    def test_native_scheduler_has_explicit_completed_output(self):
        body = self.scheduler.split("bool RenderPostPlan(", 1)[1].split("void PublishPostOutput(", 1)[0]
        self.assertIn("HostPostRender(s, inputs, target.native,", body)
        self.assertNotIn("PublishSceneOutput", body)
        for name in ("__imp__", "sub_8221E758", "sub_8221CB38", "sub_822166E8"):
            self.assertNotIn(name, body)
        publication = self.scheduler.split("void PublishPostOutput(", 1)[1].split("bool AcquirePostTargets(", 1)[0]
        self.assertIn("Video::PublishNativePostOutput(completed.image, scene_output)", publication)

    def test_native_sequence_uses_explicit_depth_without_global_publication(self):
        body = self.scheduler.split("bool RunEffectSequence(", 1)[1].split("void VerifyAdjustmentPublication", 1)[0]
        for name in ("GuestTexture *scene, GuestTexture *depth",
                     "targets.images[sequence->Output(i)]", "RenderPostPlan(plan, inputs,"):
            self.assertIn(name, body)
        for name in ("Texture(kDepth)", "sub_82184790(", "sub_8221CC90(",
                     "REX_CALL", "__imp__", "bd::mem::store", "ctx.r1", "SetTexture("):
            self.assertNotIn(name, body)
        self.assertNotIn("Texture(", body)
        self.assertIn("REX_HOOK_RAW(bdEffectSlotArrayApply)", self.scheduler)

    def test_native_stage_and_atlas_do_not_follow_resolve_links(self):
        render = self.post.split("bool HostPostRender(", 1)[1].split("bool HostPostPrepareDof(", 1)[0]
        atlas = self.post.split("bool BuildDofAtlas(", 1)[1].split("bool BuildDofPyramid(", 1)[0]
        for body in (render, atlas):
            for name in ("Content(", "SourceScale(", "sourceSurface", "resolveScale",
                         "DetachSourceSurfaceLocked(", "s.textures[", "BuildDofPyramid("):
                self.assertNotIn(name, body)
        self.assertIn("auto *source = &inputs.scene", render)
        self.assertIn("auto *z = &inputs.depth", render)
        self.assertIn("c.dof.scene_scale = inputs.exposure", atlas)
        self.assertIn("c.dof.depth = inputs.depth", atlas)
        self.assertIn("NativeSource(s, c.dof.depth)", self.post)
        self.assertIn("NativeSource(s, inputs.scene)", atlas)

    def test_native_sequence_uses_explicit_inputs_and_publishes_only_completed_output(self):
        body = self.scheduler.split("bool RunEffectSequence(", 1)[1].split("bool RunImportedEffectSequence(", 1)[0]
        self.assertIn("HostPostInputs inputs", body)
        self.assertNotIn("HostPostImportInputs(", body)
        self.assertEqual(body.count("PublishPostOutput("), 1)
        execution = body.split("const float exposure = inputs.exposure;", 1)[1]
        loop, publication = execution.split("\n  PublishPostOutput(", 1)
        self.assertNotIn("Publish", loop)
        self.assertNotIn("HostPostImportInputs", loop)
        self.assertIn("inputs.scene = targets.images[sequence->Output(i - 1)].native.image", loop)
        self.assertNotIn("BorrowPostImage(", loop)
        self.assertIn("inputs.exposure = sequence->Exposure(i, exposure)", loop)
        self.assertIn("RenderPostPlan(plan, inputs,", loop)
        self.assertTrue(publication.startswith("targets.images[sequence->Output(sequence->count - 1)], scene)"))
        self.assertLess(body.index("callback !="), body.index("AcquirePostTargets("))
        imported = self.scheduler.split("bool RunImportedEffectSequence(", 1)[1].split("void VerifyAdjustmentPublication", 1)[0]
        self.assertEqual(imported.count("HostPostImportInputs("), 1)
        self.assertIn("RunEffectSequence(list, inputs, scene)", imported)

    def test_scene_import_preserves_alias_exposure_without_producing_gpu_work(self):
        body = self.post.split("bool HostPostImportInputs(", 1)[1].split("bool HostPostRender(", 1)[0]
        self.assertIn("Snapshot(Content(scene)), Snapshot(Content(depth)), SourceScale(scene)", body)
        self.assertIn("std::lock_guard lock(s.mutex)", body)
        self.assertIn("!std::isfinite(imported.exposure) || imported.exposure <= 0", body)
        self.assertLess(body.index("return false"), body.index("inputs = imported"))
        for name in ("ResolveRtToTexture", "PublishSceneOutput", "copyTexture", "Pass(", "OpenCommandList"):
            self.assertNotIn(name, body)

    def test_scene_handoff_borrows_explicit_outputs_without_temporary_containers(self):
        helper = self.scheduler.split("GuestTexture *SceneOutput(", 1)[1].split("struct PostEffects", 1)[0]
        self.assertIn("Words(container, 8) ? Texture(container + 4) : nullptr", helper)
        body = self.scheduler.split("bool bdNativeScenePostHook(", 1)[1]
        self.assertIn("Words(view.u32, 8)", body)
        self.assertIn("SceneOutput(bd::mem::load<uint32_t>(view.u32))", body)
        self.assertIn("SceneOutput(bd::mem::load<uint32_t>(view.u32 + 4))", body)
        self.assertIn("handled = RunImportedEffectSequence(kEffectList, scene, depth)", body)
        native = body.split("} else if (Words(", 1)[0]
        self.assertIn("scene::TakeCompletedSceneImages(view.u32)", native)
        self.assertIn("RunEffectSequence(kEffectList, completed->inputs, completed->output, &published)", native)
        for name in ("SceneOutput(", "Texture(", "bd::mem::", "HostPostImportInputs("):
            self.assertNotIn(name, native)
        self.assertIn("return handled", body)
        for name in ("__imp__", "REX_CALL", "REX_STORE", "bd::mem::store", "ctx.",
                     "sub_8221C9A0(", "ReleaseResourceAdapter(", "AddRef(", "view.u32 ="):
            self.assertNotIn(name, body)

    def test_scene_handoff_hook_skips_constructors_and_complete_cleanup(self):
        hooks = [h for h in self.hooks.split("[[midasm_hook]]")
                 if 'name = "bdNativeScenePostHook"' in h]
        self.assertEqual(len(hooks), 1)
        lines = [line.split("#", 1)[0].strip() for line in hooks[0].splitlines()]
        self.assertEqual([line for line in lines if line], ["address = 0x821865B8",
            'name = "bdNativeScenePostHook"', 'registers = ["r18"]',
            "after_instruction = false", "jump_address_on_true = 0x821867E8"])
        device = (self.root / "src/gpu/hooks/device.cpp").read_text(encoding="utf-8")
        self.assertIn("BD_NOOP(D3DDevice_SetShaderGPRAllocation)", device)

    def test_initial_scene_colour_is_recovered_unless_post_actually_published(self):
        sequence = self.scheduler.split("bool RunEffectSequence(", 1)[1].split("bool RunImportedEffectSequence(", 1)[0]
        self.assertLess(sequence.index("if (published) *published = false"), sequence.index("return false"))
        self.assertEqual(sequence.count("*published = true"), 1)
        self.assertLess(sequence.index("PublishPostOutput("), sequence.index("*published = true"))
        hook = self.scheduler.split("bool bdNativeScenePostHook(", 1)[1]
        self.assertIn("if (published) completed->pending_scene_color = false", hook)
        self.assertIn("else completed->PublishPendingColor()", hook)
        self.assertLess(hook.index("PublishPendingColor()"), hook.index("return handled"))

    def test_owned_generated_scene_handoff_instruction_contract(self):
        # Optional owned-game evidence. Generated code stays generated; this
        # test checks the original PPC comments, not a hand-maintained fixture.
        path = self.root / "generated/reblue_recomp.16.cpp"
        if not path.exists():
            self.skipTest("Owned game codegen is unavailable")
        body = path.read_text(encoding="utf-8").split("DEFINE_REX_FUNC(bdRenderViewSubmit) {", 1)[1]
        body = body.split("\nDEFINE_REX_FUNC(", 1)[0]
        address, instructions = 0x82184E90, {}
        for line in body.splitlines():
            label = re.fullmatch(r"loc_([0-9A-F]+):", line)
            if label:
                self.assertEqual(address, int(label[1], 16))
            if line.startswith("\t// "):
                instructions[address] = line[4:].strip()
                address += 4
        self.assertEqual(address, 0x8218683C)
        self.assertEqual(instructions[0x821865B4], "stfs f29,8(r11)")
        self.assertEqual(instructions[0x821865B8], "lwz r11,4(r18)")
        skipped = [v for k, v in instructions.items() if 0x821865B8 <= k < 0x821867E8]
        self.assertEqual(skipped.count("bl 0x8221c9a0"), 2)
        self.assertEqual(skipped.count("bl 0x82184898"), 1)
        self.assertEqual(skipped.count("bl 0x82481108"), 2)
        self.assertEqual([instructions[a] for a in range(0x821867E8, 0x821867FC, 4)],
            ["lwz r31,288(r1)", "li r5,1", "lbz r4,154(r1)", "mr r3,r31", "bl 0x82173df8"])
        tail = [v for k, v in instructions.items() if k >= 0x821867E8]
        self.assertFalse(any(re.search(r",(?:22[4-9]|2[3-7][0-9]|28[0-7])\(r1\)", v) for v in tail))

    def test_native_sequence_preflights_unknown_callbacks_and_preserves_ordered_focus(self):
        body = self.scheduler.split("bool RunEffectSequence(", 1)[1].split("void VerifyAdjustmentPublication", 1)[0]
        self.assertIn('callback != 0x8221B1D8u', body)
        self.assertIn('MakePostSequence(uint32_t(std::max(0, signed_count)))', body)
        self.assertIn('bd::mem::load<uint32_t>(list + 16)', body)
        self.assertLess(body.index('callback !='), body.index('AcquirePostTargets('))
        self.assertLess(body.index('ReadPlan('), body.index('RenderPostPlan('))
        self.assertLess(body.index('ReadDofProducerParameters('), body.index('RenderPostPlan('))
        self.assertIn('if (i) throw std::runtime_error', body)
        self.assertIn('phase != 3', body)
        acquisition = self.scheduler.split('bool AcquirePostTargets(', 1)[1].split('bool RunEffectSequence(', 1)[0]
        self.assertIn('AcquireNativePostImage(scene->width, scene->height, scene->layers)', acquisition)
        self.assertLess(acquisition.index('CanPublishNativePostOutput('), acquisition.index('AcquireNativePostImage('))
        for name in ('HostTargetClass::PostColor', 'ReleaseResourceAdapter(', 'HostTargetAcquire(', 'BorrowPostOutput('):
            self.assertNotIn(name, self.scheduler)
        self.assertIn('NativePostImageHandle image', self.scheduler)

    def test_native_prepare_has_no_console_parameter_or_resource_producer(self):
        body = self.post.split("bool HostPostPrepareDof(", 1)[1].split("bool HostPostProducerSkip(", 1)[0]
        for name in ("GuestPixelConstant", "device_guest", "s.textures[", "D3DDevice_", "ResolveRtToTexture"):
            self.assertNotIn(name, body)
        self.assertIn("BuildDofPyramid(s, c, scene, depth, parameters)", body)
        self.assertLess(body.index("DrawQueueFlush("), body.index("BuildDofPyramid("))
        self.assertIn("s.draw_framebuffer_bound = false", body)

    def test_parameters_are_not_shader_register_readback(self):
        body = self.bridge.split("bool ReadParameters(", 1)[1].split("void Verify(", 1)[0]
        self.assertIn("GetNativeRenderTransforms()", body)
        self.assertIn("MakeDofParameters(", body)
        for name in ("psFloatConstants", "__imp__", "kEngine"):
            self.assertNotIn(name, body)
        for name in ("PPCContext", "bd::mem::", "plume::", "REX_"):
            self.assertNotIn(name, self.parameters)

    def test_native_consume_has_no_quad_or_resolve(self):
        body = self.bridge.split("REX_HOOK_RAW(bdShadowStencilDrawIndexed)", 1)[1].split("} else {", 1)[0]
        self.assertIn("preparation.Consume(ctx.r3.u32, ctx.r4.u32, FrameStatFrameCount())", body)
        self.assertIn("ctx.r3.u64 = 1", body)
        for name in ("__imp__", "D3DDevice_", "sub_8221CD08", "sub_8221CE78"):
            self.assertNotIn(name, body)


if __name__ == "__main__":
    unittest.main()
