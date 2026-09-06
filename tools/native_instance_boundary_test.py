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
