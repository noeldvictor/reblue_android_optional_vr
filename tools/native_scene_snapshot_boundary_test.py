"""Source checks complement the command fixture and desktop image verification."""
from pathlib import Path
import unittest


class SceneSnapshotBoundaryTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        root = Path(__file__).resolve().parents[1]
        cls.core = (root / "src/gpu/scene/native_scene_snapshot.h").read_text(encoding="utf-8")
        cls.bridge = (root / "src/gpu/scene/native_scene_snapshot_bridge.cpp").read_text(encoding="utf-8")

    def test_native_copy_has_no_console_or_resource_header_inputs(self):
        for token in ("PPC", "REX", "GuestTexture", "bd::mem", "resolveScale", "sourceSurface"):
            self.assertNotIn(token, self.core)
        self.assertIn("commands.copyTexture(output.texture, source.texture)", self.core)

    def test_original_only_replays_a_preflight_refusal(self):
        self.assertIn("REX_HOOK_RAW(sub_8221D2C8)", self.bridge)
        self.assertEqual(self.bridge.count("__imp__sub_8221D2C8(ctx, base)"), 1)
        self.assertNotIn("catch", self.bridge)
        recorded = self.bridge.split("BeginCommandList(s)", 1)[1].split("REX_HOOK_RAW", 1)[0]
        self.assertNotIn("return false", recorded)

    def test_native_scope_and_lease_replace_console_resolve(self):
        for call in ("D3DDevice_Resolve(", "D3DDevice_SetTexture(", "TrackResolveSource(", "ResolveRtToTexture("):
            self.assertNotIn(call, self.bridge)
        for call in ("ActiveNativeSceneCommands(", "AcquireNativePostImage(", "CopySceneSnapshot(",
                     "Video::PublishNativeImage(lease, destination, false, extent)"):
            self.assertIn(call, self.bridge)

    def test_snapshot_extent_comes_from_owned_scene_not_fixed_getter(self):
        self.assertIn("NativeImageExtentPolicy::AdoptSource", self.bridge)
        self.assertIn("AcquireNativePostImage(source.width, source.height, source.layers)", self.bridge)
        self.assertNotIn("source.width != destination->width", self.bridge)
        self.assertIn("CanPublishNativeImage(lease, destination, extent)", self.bridge)

    def test_flush_precedes_read_and_publication(self):
        self.assertLess(self.bridge.index("DrawQueueFlushAt("), self.bridge.index("Check(CopySceneSnapshot("))
        self.assertLess(self.bridge.index("Check(CopySceneSnapshot("), self.bridge.index("Video::PublishNativeImage("))
        self.assertLess(self.core.index("commands.setFramebuffer(nullptr)"), self.core.index("commands.copyTexture("))


if __name__ == "__main__":
    unittest.main()
