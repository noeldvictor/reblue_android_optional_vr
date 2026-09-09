"""Ownership wiring guards; behavioral coverage lives in the C++ fixture."""
from pathlib import Path
import unittest


class NativeInstanceBoundaryTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        root = Path(__file__).resolve().parents[1]
        cls.core = (root / "src/gpu/scene/native_instance.h").read_text(encoding="utf-8")
        cls.bridge = (root / "src/gpu/scene/native_instance_bridge.cpp").read_text(encoding="utf-8")
        cls.walk = (root / "src/gpu/scene/host_walk.cpp").read_text(encoding="utf-8")
        cls.draw = (root / "src/gpu/scene/host_draw.cpp").read_text(encoding="utf-8")
        cls.layout = (root / "src/gpu/scene/guest_scene.h").read_text(encoding="utf-8")
        cls.source = (root / "src/gpu/scene/native_instance_source.h").read_text(encoding="utf-8")
        cls.skeleton = (root / "src/gpu/scene/native_skeleton.h").read_text(encoding="utf-8")
        cls.loader = (root / "src/gpu/scene/native_material.cpp").read_text(encoding="utf-8")
        cls.animation = (root / "src/gpu/scene/native_animation_clip.h").read_text(encoding="utf-8")
        cls.animation_test = (root / "tools/native_material_test/animation.cpp").read_text(encoding="utf-8")
        cls.animation_bridge = (root / "src/gpu/scene/native_animation_bridge.cpp").read_text(encoding="utf-8")
        cls.animation_asset = (root / "src/gpu/scene/native_animation_asset.h").read_text(encoding="utf-8")
        cls.controller = (root / "src/gpu/scene/native_animation_controller.h").read_text(encoding="utf-8")
        cls.controller_source = (root / "src/gpu/scene/native_animation_controller_source.h").read_text(encoding="utf-8")
        cls.selection_source = (root / "src/gpu/scene/native_animation_selection_source.h").read_text(encoding="utf-8")
        cls.effects = (root / "src/gpu/scene/native_effect_animation.h").read_text(encoding="utf-8")
        cls.effect_source = (root / "src/gpu/scene/native_effect_animation_source.h").read_text(encoding="utf-8")

    def test_ready_slot_selection_reuses_owned_clips_and_preserves_pending_original(self):
        selection = self.animation_bridge.split("bool SelectAnimation(", 1)[1].split("std::optional<Placement>", 1)[0]
        hook = self.animation_bridge.split("REX_HOOK_RAW(bdVisualObjectSetAnimation)", 1)[1].split("REX_HOOK_RAW", 1)[0]
        self.assertIn("if (!SelectAnimation(ctx,base)) __imp__bdVisualObjectSetAnimation(ctx,base);", hook)
        self.assertIn("controller_reference", selection)
        self.assertLess(selection.index("PrepareSlotSelection("), selection.index("PrepareClip("))
        self.assertIn("store.assets.Find(plan->selected.source)", selection)
        self.assertLess(selection.index("return refuse();"), selection.index("__imp__bdVisualObjectSetAnimation"))
        self.assertEqual(selection.count("__imp__bdVisualObjectSetAnimation"), 1)
        self.assertLess(selection.index("throw std::runtime_error"), selection.index("bd::mem::store"))
        self.assertIn("w<plan->after.size()", selection)
        self.assertIn("*state == 1", self.selection_source)
        self.assertLess(self.selection_source.index("*state == 1"), self.selection_source.index("const auto source=read"))
        for forbidden in ("PPCContext", "bdAsyncRequestPoll", "__imp__", "unordered_map", "bd::mem::store"):
            self.assertNotIn(forbidden, self.selection_source)
        self.assertIn("TestNativeSlotSelection()", self.animation_test)

    def test_animation_registration_is_load_scoped_and_sampler_never_reads_source_keys(self):
        for name in ("sub_8217BD70", "sub_8217C5E8"):
            hook = self.animation_bridge.split(f"REX_HOOK_RAW({name})", 1)[1].split("REX_HOOK_RAW", 1)[0]
            self.assertLess(hook.index("Retire(owner)"), hook.index(f"__imp__{name}"))
            self.assertLess(hook.index("LoadScope"), hook.index(f"__imp__{name}"))
        hook = self.animation_bridge.split("REX_HOOK_RAW(sub_82288680)", 1)[1].split("REX_HOOK_RAW", 1)[0]
        self.assertLess(hook.index("__imp__sub_82288680"), hook.index("Import(source)"))
        sampler = self.animation_bridge.split("bool Sample(", 1)[1].split("bool Mix(", 1)[0]
        for forbidden in ("ReadKeyedAsset", "ReadKeyedClip", "emplace", "assets.Publish(", "loading_owner"):
            self.assertNotIn(forbidden, sampler)
        self.assertIn("asset=store.assets.Find(ctx.r5.u32)", sampler)
        self.assertIn("layer.Apply(*asset,model->AnimationTargets()", sampler)
        self.assertLess(sampler.index("throw std::runtime_error"), sampler.index("auto *output"))

    def test_controller_owns_plan_and_layers_before_any_source_publication(self):
        controller = self.animation_bridge.split("bool Controller(", 1)[1].split("} // namespace", 1)[0]
        for forbidden in ("PPCContext", "REX_", "bd::mem", "ReadWord", "ChannelRecord", "24576", "0x82DF4660"):
            self.assertNotIn(forbidden, self.controller)
        for forbidden in ("ReadKeyedAsset", "ReadKeyedClip", "24576", "0x82DF4660"):
            self.assertNotIn(forbidden, controller)
        self.assertLess(controller.index("PlanNativeAnimationController(input)"), controller.index("ExecuteController("))
        self.assertIn("ControllerSourceExtentFits(*count,*active,input.sequential)", controller)
        self.assertLess(controller.index("ExecuteController("), controller.index("__imp__bdAnimationUpdate(ctx,base)"))
        self.assertLess(controller.index("throw std::runtime_error"), controller.index("auto *destination="))
        tail = controller.split("if (!verify) {", 1)[1]
        self.assertEqual(tail.count("bdVisualObjectCollisionTestNearby(ctx,base)"), 1)
        self.assertNotIn("bdEffectUpdate(ctx,base)", controller)
        self.assertEqual(tail.count("effects->Publish("), 1)
        self.assertIn("ReferenceScope reference", controller)
        self.assertIn("ReferenceScope() { controller_reference=true; }", self.animation_bridge)
        self.assertIn("TestNativeControllerPlan()", self.animation_test)
        self.assertIn("TestNativeControllerConsumption()", self.animation_test)

    def test_effects_consume_owned_channels_before_skeleton_and_material_publication(self):
        controller = self.animation_bridge.split("bool Controller(", 1)[1].split("} // namespace", 1)[0]
        self.assertIn("result->output.channels,Word)", controller)
        self.assertLess(controller.index("PrepareEffectUpdate("), controller.index("__imp__bdAnimationUpdate"))
        self.assertLess(controller.index("effects->Matches(Word)"), controller.index("auto *destination="))
        self.assertLess(controller.index("PrepareEffectUpdate("), controller.index("completed_controller.Publish("))
        self.assertLess(controller.index("bdVisualObjectCollisionTestNearby(ctx,base)"), controller.index("effects->Publish("))
        self.assertLess(controller.index("effects->Publish("), controller.index("AdvanceNativeAnimationOffsets("))
        for forbidden in ("PPCContext", "ReadWord", "ChannelRecord", "bd::mem", "0x82", "uint32_t visual"):
            self.assertNotIn(forbidden, self.effects)
        for forbidden in ("PPCContext", "__imp__", "ChannelRecord", "unordered_map", "ReadChannels(", "source+", "*source+"):
            self.assertNotIn(forbidden, self.effect_source)
        self.assertIn("*state >= 1 && *state <= 4", self.effect_source)
        self.assertIn("TestNativeEffectConsumption()", self.animation_test)
        self.assertIn("ReadMaterialTextureInputs<int>", self.animation_test)
        self.assertIn("ComposeMaterialTextures<int>", self.animation_test)

    def test_completed_controller_channels_reach_skeleton_with_late_writer_guard(self):
        producer = self.bridge.split("bool EvaluateOwnedBones(", 1)[1].split("void PublishEvaluatedPose", 1)[0]
        self.assertLess(producer.index("TakeNativeAnimationChannels("), producer.index("skeleton_source::ReadChannels"))
        self.assertIn("model->Generation(),ctx.r5.u32", producer)
        self.assertIn("RetireNativeAnimationChannels(visual)", self.bridge)
        self.assertIn("pending->generation != generation", self.controller_source)
        self.assertIn("if (actual != pending->boundary[n][w])", self.controller_source)
        self.assertIn("changed=true;", self.controller_source)
        self.assertIn("pending_.reset(); changed=false", self.controller_source)

    def test_selected_slots_prepare_owned_curves_before_sampling_with_bounded_refusals(self):
        registration = self.animation_bridge.split("void Import(", 1)[1].split("thread_local uint32_t slot_graph", 1)[0]
        self.assertIn("store.assets.Register(loading_owner,source)", registration)
        self.assertNotIn("ReadKeyedAsset", registration)
        slot = self.animation_bridge.split("void PrepareClip(", 1)[1].split("bool Sample(", 1)[0]
        self.assertIn("SelectedSlotSource(visual,slot,Word)", slot)
        self.assertIn("store.assets.Prepare", slot)
        self.assertIn("ReadKeyedAsset(address,budget,", slot)
        self.assertIn("store.prepare_refused <= 4", slot)
        self.assertIn("budget <= entry.failed_budget", self.animation_asset)
        self.assertIn("candidate.resident.use_count() == 1", self.animation_asset)
        self.assertIn("TestSelectedAnimationResidency()", self.animation_test)

    def test_selected_tracks_are_sampled_only_after_node_selection(self):
        layer = self.controller_source.split("bool Apply(", 1)[1].split("static bool Mix(", 1)[0]
        self.assertNotIn(".Clip().Sample(", layer)
        self.assertLess(layer.index("selected->channels[n]"), layer.index("asset.SampleTarget("))
        self.assertIn("if (!std::isfinite(seconds)) return false;", layer)
        self.assertIn("clip_.SampleTrack(size_t(track-clip_.Tracks().data())", self.animation_asset)
        sample = self.animation.split("bool SampleTrack(", 1)[1].split("private:", 1)[0]
        self.assertIn("ordinal >= tracks_.size()", sample)
        self.assertLess(sample.index("if (!std::isfinite(value)) return false;"), sample.index("out=channel"))

    def test_root_motion_reuses_owned_model_and_selected_asset_before_publication(self):
        root = self.animation_bridge.split("bool RootMotion(", 1)[1].split("bool LateLayers(", 1)[0]
        for forbidden in ("ReadKeyedAsset", "ReadKeyedClip", "SourceNode(", "bdSceneGraphFindNodeByName"):
            self.assertNotIn(forbidden, root)
        self.assertIn("FindLoadedNativeModel(ctx.r4.u32)", root)
        self.assertIn("animation_name.View() == name.View()", root)
        self.assertIn("ReferenceScope reference", root)
        self.assertLess(root.index("SampleRootMotion("), root.index("const auto records="))
        self.assertLess(root.index("throw std::runtime_error(\"Native root-motion comparison failed\")"), root.index("auto *output="))
        hook = self.animation_bridge.split("REX_HOOK_RAW(sub_8218FC98)", 1)[1].split("REX_HOOK_RAW", 1)[0]
        self.assertLess(hook.index("PrepareClip(*source)"), hook.index("RootMotion(ctx,base)"))
        self.assertIn("asset.Indexed() ? 0u : joint.pose_index", self.controller_source)

    def test_single_joint_calls_consume_generation_checked_selection_without_source_keys(self):
        lookup = self.animation_bridge.split("REX_HOOK_RAW(sub_8227EF60)", 1)[1].split("REX_HOOK_RAW", 1)[0]
        self.assertLess(lookup.index("selected_joint.Clear()"), lookup.index("SelectJoint(ctx,base)"))
        self.assertIn("if (!SelectJoint(ctx,base)) __imp__sub_8227EF60", lookup)
        selection = self.animation_bridge.split("bool SelectJoint(", 1)[1].split("bool SingleJoint(", 1)[0]
        self.assertIn("FindLoadedNativeJoint(graph,pose)", selection)
        self.assertNotIn("Word(", selection)
        self.assertLess(selection.index("ReferenceScope reference"), selection.index("__imp__sub_8227EF60"))
        self.assertLess(selection.index("throw std::runtime_error"), selection.index("selected_joint.Publish("))
        sample = self.animation_bridge.split("bool SingleJoint(", 1)[1].split("bool LateLayers(", 1)[0]
        for forbidden in ("ReadKeyedAsset", "ReadKeyedClip", "SourceNode(", "bdSceneGraphFindNodeByName"):
            self.assertNotIn(forbidden, sample)
        self.assertIn("selected_joint.Take(ctx.r4.u32,LoadedNativeModelGeneration)", sample)
        self.assertIn("model->Generation() != selection->generation", sample)
        self.assertLess(sample.index("SampleRootMotion("), sample.index("__imp__sub_8228A4E8"))
        self.assertLess(sample.index("throw std::runtime_error"), sample.index("auto *output="))
        self.assertIn("SameSampledChannelWord", sample)
        self.assertIn("TestJointSelectionHandoff()", self.animation_test)

    def test_controller_excluded_joints_use_load_owned_selection_and_names(self):
        self.assertIn("&animation_targets, &joint_sources", self.loader)
        self.assertIn("std::move(joint_sources)", self.loader)
        self.assertNotIn("SourceNode(", self.animation_bridge)
        excluded = self.animation_bridge.split("for (uint32_t pose : *excluded_nodes)", 1)[1].split("const bool write_exclusions", 1)[0]
        self.assertIn("FindLoadedNativeJoint(slot_graph,pose)", excluded)
        self.assertIn("selected->model->Generation() != model->Generation()", excluded)
        self.assertIn("excluded_names.push_back(joint->animation_name)", excluded)
        self.assertNotIn("ReadJointName", excluded)

    def test_weighted_subtrees_share_visual_scope_and_preserve_controller_side_effects(self):
        hook = self.animation_bridge.split("REX_HOOK_RAW(bdAnimationUpdate)", 1)[1].split("REX_HOOK_RAW", 1)[0]
        self.assertLess(hook.index("VisualScope"), hook.index("__imp__bdAnimationUpdate(ctx,base)"))
        sampler = self.animation_bridge.split("bool Sample(", 1)[1].split("} // namespace", 1)[0]
        self.assertIn("layer.Apply(*asset,model->AnimationTargets()", sampler)
        self.assertIn("model->AnimationTargets()[*index] != *hash", sampler)
        self.assertIn("kNativeAnimationWeightEpsilon) return true", sampler)
        self.assertIn("!euler_mode", sampler)
        self.assertIn("TestWeightedLayerConsumption()", self.animation_test)
        self.assertIn("TestOwnedBlendRest()", self.animation_test)

    def test_late_writers_reuse_and_republish_native_channels_before_bones(self):
        late = self.animation_bridge.split("bool LateLayers(", 1)[1].split("bool Controller(", 1)[0]
        for forbidden in ("ReadKeyedAsset", "ReadKeyedClip", "bdVisualObjectAnimSlotUpdate(ctx", "24576"):
            self.assertNotIn(forbidden, late)
        self.assertIn("*selection == 504", late)
        self.assertLess(late.index("TakeCompletedLayer("), late.index("ExecuteController("))
        self.assertLess(late.index("ExecuteController("), late.index("__imp__sub_822D3CB0(ctx,base)"))
        self.assertLess(late.index("throw std::runtime_error"), late.index("auto *destination="))
        self.assertIn("std::move(layer),*compression,exclusions,false", late)
        self.assertIn("records,true}", late)
        self.assertIn("REX_HOOK_RAW(sub_822D3CB0)", self.animation_bridge)
        sampler = self.animation_bridge.split("bool Sample(", 1)[1].split("bool Mix(", 1)[0]
        self.assertLess(sampler.index("TakeCompletedLayer("), sampler.index("layer.Apply("))
        self.assertIn("completed_controller.Publish({slot_visual,graph,destination", sampler)

    def test_layer_mix_checks_complete_output_before_replacing_original_records(self):
        mix = self.animation_bridge.split("bool Mix(", 1)[1].split("} // namespace", 1)[0]
        self.assertIn("MixChannelRecords(a,b,weight,destination == left,destination == right,records)", mix)
        self.assertIn("count != model->Skeleton().size()", mix)
        self.assertLess(mix.index("throw std::runtime_error"), mix.index("auto *output="))
        self.assertIn("REX_HOOK_RAW(sub_82284BE0)", self.animation_bridge)
        self.assertIn("TestLayerMixing()", self.animation_test)

    def test_attachment_owns_placement_before_controller_and_skeleton_consumers(self):
        prepare = self.animation_bridge.split("std::optional<Placement> PreparePlacement", 1)[1].split("struct PlacementScope;", 1)[0]
        for forbidden in ("bdSceneGraphFindNodeByName(", "sub_8227EF60(", "sub_8228A4E8(", "ChannelRecord", ".Encode(", "ctx.r1"):
            self.assertNotIn(forbidden, prepare)
        self.assertIn("SampleRootMotion(*asset", prepare)
        self.assertIn("ComposeNativeAttachmentPlacement(layer.channels[0]", prepare)
        complete = self.animation_bridge.split("void CompletePlacementBeforeUpdate", 1)[1].split("void AttachmentTail", 1)[0]
        self.assertLess(complete.index("throw std::runtime_error"), complete.index("WritePlacementRoot(placement)"))
        hook = self.animation_bridge.split("REX_HOOK_RAW(bdAnimationUpdate)", 1)[1].split("REX_HOOK_RAW", 1)[0]
        self.assertLess(hook.index("CompletePlacementBeforeUpdate"), hook.index("VisualScope"))
        tail = self.animation_bridge.split("void AttachmentTail", 1)[1].split("bool AttachmentUpdate", 1)[0]
        calls = ["bdAnimationUpdate(ctx,base)", "AnimeData_method_1A60(ctx,base)", "sub_822D3CB0(ctx,base)",
                 "bdVisualObjectInitBones(ctx,base)", "AnimeData_helper_928(ctx,base)"]
        self.assertEqual([tail.index(call) for call in calls], sorted(tail.index(call) for call in calls))
        for call in calls:
            self.assertEqual(tail.count(call), 1)
        self.assertIn("REX_HOOK_RAW(AnimeData_method_4638)", self.animation_bridge)
        root = self.animation_bridge.split("std::optional<RenderMatrix> TakeNativeAnimationPlacementRoot", 1)[1].split("REX_HOOK_RAW", 1)[0]
        self.assertIn("placement.model->Generation() == generation", root)
        self.assertIn("SameNativePlacementRoot(placement.root,boundary)", root)
        self.assertIn("scope->consumed=true", root)
        bones = self.bridge.split("bool EvaluateOwnedBones", 1)[1].split("void PublishEvaluatedPose", 1)[0]
        self.assertLess(bones.index("TakeNativeAnimationPlacementRoot"), bones.index("EvaluateNativeSkeleton"))
        self.assertIn("TestAttachmentPlacement()", self.animation_test)

    def test_named_sampling_uses_owned_model_names_and_bounded_source_filter(self):
        sampler = self.animation_bridge.split("bool Sample(", 1)[1].split("bool Mix(", 1)[0]
        self.assertIn("std::array<NativeJointName,30> excluded_names", sampler)
        self.assertIn("ReadJointName(kSamplerState+8,Word)", sampler)
        self.assertIn("filter,!preserve)", sampler)
        self.assertIn("selected->channels[n]", sampler)
        self.assertIn("Word(kSamplerState+24) == *depth", sampler)
        self.assertIn("TestNamedAnimationSelection()", self.animation_test)
        self.assertIn("TestConstantTimesAndScaleTail()", self.animation_test)

    def test_indexed_animation_reuses_assets_layers_and_preserves_dispatch_state(self):
        sampler = self.animation_bridge.split("bool Sample(", 1)[1].split("bool Mix(", 1)[0]
        self.assertIn("const bool indexed=asset->Indexed()", sampler)
        self.assertIn("preserve && !indexed && ctx.r8.u32 != 0", sampler)
        self.assertIn("preserve && !indexed ? 0u : *exclusion", sampler)
        self.assertIn("if (!indexed) {", sampler)
        self.assertIn("type && *type <= 3", self.animation_bridge)
        self.assertIn("TestIndexedAnimationAssets()", self.animation_test)

    def test_animation_assets_retire_with_both_loader_types_and_pinned_bytes_remain_charged(self):
        for name in ("sub_8217BD00", "sub_8217C580"):
            hook = self.animation_bridge.split(f"REX_HOOK_RAW({name})", 1)[1].split("REX_HOOK_RAW", 1)[0]
            self.assertLess(hook.index("Retire(ctx.r3.u32)"), hook.index(f"__imp__{name}"))
        for forbidden in ("PPCContext", "REX_", "bd::mem", "ReadWord", "ofstream"):
            self.assertNotIn(forbidden, self.animation_asset)
        self.assertIn("accounting->bytes.fetch_sub(bytes)", self.animation_asset)
        self.assertIn("asset.RetainedBytes() > AvailableAssetBytes()", self.animation_asset)

    def test_animation_sampler_owns_keys_and_reuses_existing_channel_and_pose_types(self):
        for forbidden in ("PPCContext", "REX_", "bd::mem", "ReadWord", "name_hash", "NodeTag", "ofstream"):
            self.assertNotIn(forbidden, self.animation)
        self.assertIn("std::vector<NativeAnimationTrack> tracks_", self.animation)
        self.assertIn("std::vector<NativeJointChannels> &out", self.animation)
        self.assertIn("RetainedBytes()", self.animation)
        self.assertLess(self.animation_test.index("source.words.clear()"), self.animation_test.index("lease->Sample("))
        self.assertIn("EvaluateNativeSkeleton(skeleton,channels,root,pose)", self.animation_test)
        self.assertIn("registry.Publish(instance,0,pose)", self.animation_test)

    def test_palette_container_matches_both_original_producer_and_release(self):
        self.assertIn("kVisualBoneContainer = 0xA48", self.layout)
        self.assertIn("kPaletteContainer = 2632", self.source)
        self.assertIn("kVisualBoneContainer == instance_source::kPaletteContainer", self.bridge)
        self.assertIn("uint64_t(container) + 8 + lane * 12", self.source)
        self.assertIn("instance_source::ReadPublication(", self.bridge)

    def test_render_handoff_follows_original_copy_and_shares_native_pose(self):
        hook = self.bridge.split("REX_HOOK_RAW(sub_8213F5E8)", 1)[1]
        self.assertLess(hook.index("TransferReady("), hook.index("__imp__sub_8213F5E8"))
        self.assertLess(hook.index("__imp__sub_8213F5E8"), hook.index("Handoff(container)"))
        self.assertIn("if (copied &&", hook)
        self.assertIn("flags && *flags == 3", self.source)
        self.assertIn("instance_source::PublishCompletedTransfer(", self.bridge)
        self.assertIn("registry.Transfer(binding.instance, 0, 1, transfer->count)", self.source)
        self.assertIn("destination = source;", self.core)

    def test_core_has_no_source_or_render_api_dependency(self):
        for forbidden in ("PPCContext", "REX_", "bd::mem", "GuestBuffer", "NodeTag", "ofstream"):
            self.assertNotIn(forbidden, self.core)

    def test_skeleton_is_load_owned_and_evaluated_before_outgoing_palette_writes(self):
        self.assertIn("skeleton_source::ReadSkeleton", self.loader)
        self.assertIn("skeleton ? std::move(*skeleton)", self.loader)
        for forbidden in ("PPCContext", "REX_", "bd::mem", "ReadWord", "source_model"):
            self.assertNotIn(forbidden, self.skeleton)
        producer = self.bridge.split("bool EvaluateOwnedBones(", 1)[1].split("void PublishEvaluatedPose", 1)[0]
        self.assertIn("skeleton_source::ReadChannels", producer)
        self.assertIn("skeleton_source::ReadRoot", producer)
        self.assertIn("publication->lane != 0", producer)
        self.assertLess(producer.index("EvaluateNativeSkeleton("), producer.index("auto *output ="))
        self.assertLess(producer.index("if (REXCVAR_GET(bd_native_materials_verify))"), producer.index("__imp__bdBoneInitSkinned"))
        self.assertLess(producer.index("throw std::runtime_error"), producer.index("auto *output ="))

    def test_evaluated_update_reuses_instance_owner_without_advancing_completed_history(self):
        publication = self.bridge.split("void PublishEvaluatedPose(", 1)[1].split("void Retire(", 1)[0]
        self.assertIn("found->second.model_generation != scope.model->Generation()", publication)
        self.assertIn("store.instances.Publish(found->second.instance,0,scope.pose)", publication)
        for forbidden in ("bd::mem", "ObserveRenderTick", "ReadRender", ".Transfer("):
            self.assertNotIn(forbidden, publication)
        hook = self.bridge.split("REX_HOOK_RAW(bdVisualObjectInitBones)", 1)[1].split("REX_HOOK_RAW(bdBoneInitSkinned)", 1)[0]
        self.assertLess(hook.index("SkeletonEvaluationScope"), hook.index("__imp__bdVisualObjectInitBones"))
        self.assertLess(hook.index("Attach(visual)"), hook.index("PublishEvaluatedPose(evaluation)"))

    def test_render_timing_is_owned_after_handoff_and_shared_by_native_consumers(self):
        handoff = self.bridge.split("void Handoff(", 1)[1].split("} // namespace", 1)[0]
        self.assertLess(handoff.index("PublishCompletedTransfer("), handoff.index("ObserveRenderTick("))
        render = self.bridge.split("ResolveNativeRenderPose(", 1)[1].split("bool CopyNativeInstanceWorld", 1)[0]
        self.assertIn("source.get() != &completed", render)
        self.assertIn("store.render_phase->frame != frame", render)
        self.assertIn("store.instances.ReadRender(source, *store.render_phase)", render)
        for forbidden in ("bd::mem", ".Publish(", ".Create(", "HostHeap", "ReadVS"):
            self.assertNotIn(forbidden, render)
        self.assertIn("NativeSkinCasterBounds(*program,*render_pose,index)", self.walk)
        self.assertIn("(native_render_node ? render_pose : instance_pose)", self.walk)

    def test_model_lease_is_attached_before_pose_and_used_without_source_lookup(self):
        self.assertIn("FindLoadedNativeModel(graph)", self.bridge)
        self.assertIn("store.instances.Create(generation, model)", self.bridge)
        self.assertIn("slot = OwnPose(id, it->second, transforms)", self.core)
        self.assertIn("owner->pose.model = entry.model", self.core)
        lookup = self.core.split("FindNativeInstanceNode(", 1)[1]
        for forbidden in ("bd::mem", "NodeTag", "source_mesh", "LoadedNativeModelGeneration"):
            self.assertNotIn(forbidden, lookup)
        self.assertIn("FindNativeInstanceNode(*instance_pose, index)", self.walk)
        self.assertIn("!bounds || verify_bounds ?", self.walk)
        self.assertIn("if (instance_pose && REXCVAR_GET(bd_native_materials_verify))\n"
                      "              NoteNativeModelNodeCandidate", self.walk)

    def test_identity_attachment_does_not_import_provisional_poses(self):
        hook = self.bridge.split("REX_HOOK_RAW(bdVisualObjectInitBones)", 1)[1]
        self.assertLess(hook.index("__imp__bdVisualObjectInitBones"), hook.index("Attach(visual)"))
        self.assertNotIn("PrecacheEnabled", hook)
        attach = self.bridge.split("void Attach(", 1)[1].split("void Handoff(", 1)[0]
        self.assertNotIn(".Publish(", attach)
        self.assertNotIn("be_f32", attach)

    def test_unload_retires_before_original_release(self):
        hook = self.bridge.split("REX_HOOK_RAW(sub_82140DF8)", 1)[1]
        self.assertLess(hook.index("Retire(ctx.r3.u32)"), hook.index("__imp__sub_82140DF8"))

    def test_consumer_does_not_publish_or_discover_instances(self):
        lookup = self.bridge.split("FindNativeInstancePose(\n", 1)[1].split("bool CopyNativeInstanceWorld", 1)[0]
        for forbidden in (".Create(", ".Publish(", "kVisualBoneContainer", "emplace"):
            self.assertNotIn(forbidden, lookup)
        self.assertIn("instance_source::Find(store.instances", lookup)
        self.assertIn("generation != binding.model_generation", self.source)
        self.assertIn("bd_native_materials_verify", lookup)

    def test_walk_and_replay_consume_native_pose_before_source(self):
        self.assertIn("const auto instance_pose = FindNativeInstancePose(", self.walk)
        self.assertIn("native_pose ? nullptr : bd::mem::try_at", self.walk)
        world = self.draw.split("float world_rows[16];", 1)[1]
        self.assertLess(world.index("CopyNativeInstanceWorld(tag, m)"), world.index("tag.matrix_va"))


if __name__ == "__main__":
    unittest.main()
