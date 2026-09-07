"""Source guards complement the production texture-table behavioral fixture."""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[1]


class NativeTextureTableBoundary(unittest.TestCase):
    def setUp(self):
        self.bridge = (ROOT / "src/gpu/scene/native_texture_table_bridge.cpp").read_text()

    def test_complete_load_and_release_boundaries(self):
        load = self.bridge.split("REX_HOOK_RAW(hcgLoadTextureArray)")[1].split("REX_HOOK_RAW(hcgTextureListRelease)")[0]
        self.assertLess(load.index("__imp__hcgLoadTextureArray"), load.index("::Publish("))
        release = self.bridge.split("REX_HOOK_RAW(hcgTextureListRelease)")[1].split("REX_HOOK_RAW(bdLookupCurrentTableTexture)")[0]
        self.assertLess(release.index("::Retire("), release.index("__imp__hcgTextureListRelease"))

    def test_lookup_does_not_discover_or_import_tables(self):
        lookup = self.bridge.split("std::optional<Selection> Lookup(")[1].split("} // namespace")[0]
        self.assertNotIn("ReadTextureTableSources", lookup)
        self.assertNotIn("ResolveGuestTexture", lookup)
        self.assertNotIn("CaptureNativeTexture", lookup)
        self.assertIn("store.tables.find", lookup)
        self.assertIn("TextureTableSourceIndex", lookup)

    def test_async_completion_and_distinct_teardown(self):
        load = self.bridge.split("REX_HOOK_RAW(sub_8217B3C0)")[1].split("REX_HOOK_RAW(sub_8217B518)")[0]
        self.assertLess(load.index("__imp__sub_8217B3C0"), load.index("::PublishCompleted"))
        release = self.bridge.split("REX_HOOK_RAW(sub_8217B518)")[1].split("REX_HOOK_RAW(bdLookupCurrentTableTexture)")[0]
        self.assertLess(release.index("::Retire("), release.index("__imp__sub_8217B518"))

    def test_native_owner_has_no_source_address_layout(self):
        core = (ROOT / "src/gpu/scene/native_texture_table.h").read_text()
        for token in ("GuestTexture", "be_u32", "source_image", "source_table", "REX_LOAD", "NodeTag"):
            self.assertNotIn(token, core)
        self.assertIn("slots.capacity()", core)
        self.assertIn("accounting_->live.load()", core)
        self.assertIn("accounting->bytes.fetch_sub(bytes)", core)

    def test_image_events_do_not_reverse_resource_lock_order(self):
        update = self.bridge.split("void NativeTextureTableImageChanged(")[1].split("} // namespace bd::gpu::scene")[0]
        self.assertNotIn("ResolveGuestTexture", update)
        self.assertNotIn("Video::", update)
        self.assertIn("RebindTextureTable", update)
        self.assertIn("store.tables.erase(it)", update)

    def test_snapshot_and_publication_share_the_mirror_lock(self):
        publish = self.bridge.split("void Publish(")[1].split("void PublishCompleted")[0]
        self.assertIn("WithNativeTextureTableSnapshot", publish)
        self.assertNotIn("epoch", publish)
        self.assertNotIn("ResolveGuestTexture", publish)
        mirror = (ROOT / "src/gpu/native_texture_mirror.cpp").read_text()
        snapshot = mirror.split("void WithNativeTextureTableSnapshot(")[1].split("void EvictNativeTexture")[0]
        self.assertIn("PublishTextureTableSnapshot(g_mirror_mutex", snapshot)
        self.assertNotIn("ResolveGuestTexture", snapshot)

    def test_original_comparison_is_independently_opt_in(self):
        self.assertIn("REXCVAR_DEFINE_BOOL(bd_native_texture_tables_verify, false", self.bridge)
        self.assertIn("const bool verify = REXCVAR_GET(bd_native_texture_tables_verify)", self.bridge)
        self.assertNotIn("bd_native_materials_verify", self.bridge)

    def test_replacement_and_eviction_publish_events(self):
        mirror = (ROOT / "src/gpu/native_texture_mirror.cpp").read_text()
        self.assertEqual(mirror.count("NativeTextureTableImageChanged(guest_va,"), 3)
        self.assertIn("scene::CaptureNativeTexture(raw)", mirror)

    def test_reflection_uses_native_handle_before_resource_adapter(self):
        draw = (ROOT / "src/gpu/scene/host_draw.cpp").read_text()
        resolve = draw.split("std::optional<ReflectionBinding> ResolveReflectionBinding(")[1].split("struct PendingReflectionCheck")[0]
        self.assertLess(resolve.index("FindLoadedNativeTableTexture"), resolve.index("ResolveReflectionAddress"))
        self.assertIn("if (!binding->primary)", resolve)


if __name__ == "__main__":
    unittest.main()
