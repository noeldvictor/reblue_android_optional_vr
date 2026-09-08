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
