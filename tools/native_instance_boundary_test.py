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

    def test_animation_registration_is_load_scoped_and_sampler_never_reads_source_keys(self):
        for name in ("sub_8217BD70", "sub_8217C5E8"):
            hook = self.animation_bridge.split(f"REX_HOOK_RAW({name})", 1)[1].split("REX_HOOK_RAW", 1)[0]
            self.assertLess(hook.index("Retire(owner)"), hook.index(f"__imp__{name}"))
            self.assertLess(hook.index("LoadScope"), hook.index(f"__imp__{name}"))
        hook = self.animation_bridge.split("REX_HOOK_RAW(sub_82288680)", 1)[1].split("REX_HOOK_RAW", 1)[0]
        self.assertLess(hook.index("__imp__sub_82288680"), hook.index("Import(source)"))
        sampler = self.animation_bridge.split("bool Sample(", 1)[1].split("} // namespace", 1)[0]
        for forbidden in ("ReadKeyedAsset", "ReadKeyedClip", "emplace", "Publish(", "loading_owner"):
            self.assertNotIn(forbidden, sampler)
        self.assertIn("asset=store.assets.Find(ctx.r5.u32)", sampler)
        self.assertIn("ApplyKeyedAsset(*asset,model->AnimationTargets()", sampler)
        self.assertLess(sampler.index("throw std::runtime_error"), sampler.index("auto *output"))

    def test_selected_slots_prepare_owned_curves_before_sampling_with_bounded_refusals(self):
        registration = self.animation_bridge.split("void Import(", 1)[1].split("thread_local uint32_t slot_graph", 1)[0]
        self.assertIn("store.assets.Register(loading_owner,source)", registration)
        self.assertNotIn("ReadKeyedAsset", registration)
        slot = self.animation_bridge.split("struct SlotScope", 1)[1].split("bool Sample(", 1)[0]
        self.assertIn("SelectedSlotSource(visual,slot,Word)", slot)
        self.assertIn("store.assets.Prepare", slot)
        self.assertIn("ReadKeyedAsset(address,budget,Word)", slot)
        self.assertIn("budget <= entry.failed_budget", self.animation_asset)
        self.assertIn("candidate.resident.use_count() == 1", self.animation_asset)
        self.assertIn("TestSelectedAnimationResidency()", self.animation_test)

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
