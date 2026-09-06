"""Runtime storage wiring guards; behavioral and existing-file checks are separate."""
from pathlib import Path
import unittest


class MeshStorageBoundaryTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        root = Path(__file__).resolve().parents[1]
        cls.bridge = (root / "src/gpu/scene/native_mesh.cpp").read_text(encoding="utf-8")
        cls.storage = (root / "src/gpu/scene/native_mesh_storage.cpp").read_text(encoding="utf-8")

    def test_runtime_uses_the_bounded_store(self):
        self.assertIn("static NativeMeshDiskCache cache(CacheDir());", self.bridge)
        self.assertIn("DiskCache().Read(key, cached)", self.bridge)
        self.assertIn("DiskCache().Write(key, data);", self.bridge)
        self.assertNotIn("std::ofstream", self.bridge)
        self.assertNotIn("void WriteFile(", self.bridge)

    def test_failed_persistence_does_not_discard_native_geometry(self):
        publish = self.bridge.split("auto result = Upload(s, data);", 1)[1]
        self.assertIn("DiskCache().Write(key, data);", publish)
        self.assertLess(publish.index("DiskCache().Write(key, data);"),
                        publish.index("s.meshes.emplace(key, result);"))
        self.assertIn("return result;", publish)
        self.assertNotIn("if (DiskCache().Write", publish)

    def test_store_is_source_and_gpu_independent(self):
        for forbidden in ("PPCContext", "REX_", "bd::mem", "GuestBuffer",
                          "Video::", "plume::", "NativeMeshImport"):
            self.assertNotIn(forbidden, self.storage)
        self.assertNotIn("remove_all", self.storage)

    def test_disk_refusals_are_reported_separately(self):
        self.assertIn("[native-mesh-disk]", self.bridge)
        self.assertIn("disk.budget_refusals", self.bridge)
        self.assertIn("disk.inventory_complete", self.bridge)


if __name__ == "__main__":
    unittest.main()
