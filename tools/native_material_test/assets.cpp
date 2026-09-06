#include "gpu/scene/native_material_library.h"

#include <barrier>
#include <chrono>
#include <cmath>
#include <fstream>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <thread>

using namespace bd::gpu::scene;
namespace {
// Tiny private fixtures may run on CI volumes with less than the application's
// 20 GiB reserve. Explicitly override only that reserve, never the default caps.
constexpr NativeMaterialDiskBudget kTestDisk{1ull << 20, 4096, 0};
void Require(bool ok, const char *what) {
  if (!ok)
    throw std::runtime_error(what);
}
struct Scratch {
  std::filesystem::path path;
  Scratch() {
    const auto parent = std::filesystem::temp_directory_path();
    const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
    for (unsigned i = 0; i < 100; ++i) {
      auto candidate = parent / ("reblue_native_material_test_" + std::to_string(stamp) +
                                 "_" + std::to_string(i));
      if (std::filesystem::create_directory(candidate)) {
        path = std::move(candidate);
        return;
      }
    }
    throw std::runtime_error("cannot create private material test directory");
  }
  ~Scratch() {
    // Only the directory this test successfully created, never a shared root.
    if (!path.empty()) {
      std::error_code ignored;
      std::filesystem::remove_all(path, ignored);
    }
  }
};
void WriteBytes(const std::filesystem::path &path, std::span<const uint8_t> bytes) {
  std::ofstream file(path, std::ios::binary | std::ios::trunc);
  file.write(reinterpret_cast<const char *>(bytes.data()), bytes.size());
  file.close();
  Require(bool(file), "test file write");
}
void RepairChecksum(std::vector<uint8_t> &file) {
  const uint64_t sum = NativeMaterialContentId(std::span(file).subspan(16));
  for (unsigned i = 0; i < 8; ++i)
    file[8 + i] = uint8_t(sum >> (8 * i));
}
} // namespace

void TestMaterialAssets() {
  NativeMaterialAsset asset;
  auto &m = asset.properties;
  m.modulate_diffuse = true;
  m.has_diffuse_multiplier = m.has_specular_colour = true;
  m.has_reflection_colour = m.has_shininess = true;
  m.diffuse_multiplier = {1, 0.5f, 0.25f};
  m.specular_colour = {0.125f, 0.25f, 0.5f};
  m.reflection_colour = {1, 0, 0.5f, 0.75f};
  m.shininess = 12;
  std::vector<uint8_t> file;
  Require(EncodeNativeMaterial(asset, file), "encode");
  Require(file.size() == kNativeMaterialFileBytes, "fixed format size");
  Require(file[16] == 0 && file[20] == 31 && file[24] == 12, "native fields");
  Require(file[28] == 0 && file[29] == 0 && file[30] == 128 && file[31] == 63,
          "binary32 little endian, independent of host layout");
  NativeMaterialAsset decoded;
  Require(DecodeNativeMaterial(file, decoded) && decoded == asset, "round trip");
  const auto id = NativeMaterialContentId(file);
  Require(id == 0x901af8371ac3c368ull, "portable v1 identity golden");
  for (size_t n = 0; n < file.size(); ++n) {
    Require(!DecodeNativeMaterial(std::span(file).first(n), decoded), "truncated file");
    Require(decoded == asset, "transactional decode");
  }
  auto corrupt = file;
  corrupt.push_back(0);
  Require(!DecodeNativeMaterial(corrupt, decoded), "trailing byte");
  for (size_t n = 0; n < file.size(); ++n) {
    corrupt = file;
    corrupt[n] ^= 1;
    Require(!DecodeNativeMaterial(corrupt, decoded), "single-byte corruption");
  }
  for (auto [offset, value] : {std::pair<size_t, uint8_t>{16, 2}, {20, 128},
                              {25, 1}, {31, 127}, {67, 255}}) {
    corrupt = file;
    corrupt[offset] = value;
    RepairChecksum(corrupt);
    Require(!DecodeNativeMaterial(corrupt, decoded), "invalid rechecksummed field");
  }
  auto invalid = asset;
  invalid.properties.specular_colour[0] = std::numeric_limits<float>::infinity();
  corrupt = file;
  Require(!EncodeNativeMaterial(invalid, corrupt) && corrupt == file,
          "transactional invalid encode");
  invalid = asset;
  invalid.properties.has_reflection_colour = false;
  invalid.properties.reflection_colour[0] = std::numeric_limits<float>::quiet_NaN();
  Require(EncodeNativeMaterial(invalid, corrupt) && DecodeNativeMaterial(corrupt, decoded),
          "unknown fields canonicalize, never retain unconsumed bytes");
  Require(decoded.properties.reflection_colour == std::array<float, 4>{}, "unknown zero");
  const auto unknown_file = corrupt;
  corrupt[52] = 1;
  RepairChecksum(corrupt);
  Require(!DecodeNativeMaterial(corrupt, decoded), "noncanonical unknown value rejected");
  invalid.properties.reflection_colour = {};
  Require(EncodeNativeMaterial(invalid, corrupt) && corrupt == unknown_file,
          "unknown values do not alter identity");
  invalid = asset;
  invalid.properties.reflection_colour[1] = -0.0f;
  Require(EncodeNativeMaterial(invalid, corrupt) && corrupt == file, "negative zero canonical");
  invalid.lighting_model = NativeLightingModel::Cel;
  Require(EncodeNativeMaterial(invalid, corrupt) && DecodeNativeMaterial(corrupt, decoded),
          "cel lighting slot round trip");
  Require(NativeMaterialContentId(corrupt) != id, "lighting participates in identity");
  std::array<float, 4> output[3];
  Require(ComposeNativeMaterialAsset(decoded, {1, 1, 1, 1}, true, output) == 0,
          "unsupported shader must not silently become OriginalLit");
  Require(ComposeNativeMaterialAsset(asset, {1, 1, 1, 1}, true, output) == 7,
          "native asset composition");

  Scratch scratch;
  NativeMaterialHandle pinned;
  {
    NativeMaterialLibrary library(scratch.path, 2, kTestDisk);
    pinned = library.Resolve(asset);
    Require(pinned && pinned->id == id && pinned->asset == asset, "cook and pin");
    Require(library.Resolve(asset) == pinned, "dedup ownership");
    Require(library.Load(id) == pinned, "source-free resident lookup");
    const auto stats = library.Stats();
    Require(stats.cooked == 1 && stats.loaded == 0 && stats.resident == 1 &&
            stats.memory_hits == 2 && stats.write_failures == 0, "cold library stats");
  }
  Require(pinned->asset == asset, "draw owns asset beyond library destruction");
  {
    NativeMaterialLibrary restarted(scratch.path, 2, kTestDisk);
    auto loaded = restarted.Load(id); // no guest tags, source commands or runtime
    Require(loaded && loaded->asset == pinned->asset, "disk-only load on restart");
    Require(restarted.Stats().loaded == 1 && restarted.Stats().cooked == 0,
            "warm load not mislabeled cook");
    auto second_asset = asset;
    second_asset.properties.shininess = 13;
    auto second = restarted.Resolve(second_asset);
    Require(bool(second), "second pinned material");
    auto third_asset = asset;
    third_asset.properties.shininess = 14;
    Require(!restarted.Resolve(third_asset), "pinned budget refuses growth");
    Require(restarted.Stats().resident == 2 && restarted.Stats().budget_refusals == 1,
            "bounded residency");
    loaded.reset();
    auto third = restarted.Resolve(third_asset);
    Require(third && restarted.Stats().resident == 2, "unreferenced LRU eviction");
    Require(second->asset == second_asset && pinned->asset == asset, "pins survive eviction");
  }
  corrupt = file;
  corrupt.resize(12);
  WriteBytes(scratch.path / NativeMaterialLibrary::FileName(id), corrupt);
  {
    NativeMaterialLibrary damaged(scratch.path, 16384, kTestDisk);
    Require(!damaged.Load(id), "reject interrupted write");
    Require(bool(damaged.Resolve(asset)), "recook invalid derived file");
    Require(damaged.Stats().cooked == 1 && damaged.Stats().invalid == 2,
            "corruption reported, not a cache hit");
  }
  {
    NativeMaterialLibrary repaired(scratch.path, 16384, kTestDisk);
    Require(bool(repaired.Load(id)), "repaired file reloads");
  }
  WriteBytes(scratch.path / NativeMaterialLibrary::FileName(id ^ 1), file);
  {
    NativeMaterialLibrary wrong_name(scratch.path, 16384, kTestDisk);
    Require(!wrong_name.Load(id ^ 1), "valid bytes under wrong identity rejected");
  }
  {
    // A file where the directory should be forces an I/O failure even as admin.
    NativeMaterialLibrary unwritable(scratch.path / NativeMaterialLibrary::FileName(id), 16384, kTestDisk);
    Require(bool(unwritable.Resolve(asset)), "failed persistence retains usable host data");
    Require(unwritable.Stats().write_failures == 1, "write failure is observable");
    NativeMaterialLibrary zero_budget(scratch.path, 0, kTestDisk);
    Require(!zero_budget.Resolve(asset) && zero_budget.Stats().resident == 0,
            "zero capacity must not grow");
  }
  const auto variant = [&](uint8_t power) {
    auto value = asset;
    value.properties.shininess = power;
    return value;
  };
  {
    Scratch limited;
    const NativeMaterialDiskBudget budget{1024, 2, 0};
    NativeMaterialLibrary library(limited.path, 1, budget);
    NativeMaterialId first_id = 0, missing_id = 0;
    for (uint8_t power = 1; power <= 8; ++power) {
      const auto material = library.Resolve(variant(power));
      Require(material && material->asset == variant(power), "disk-full keeps native material usable");
      if (power == 1) first_id = material->id;
      missing_id = material->id;
    }
    const auto stats = library.Stats();
    Require(stats.resident == 1 && stats.budget_refusals == 0 &&
            stats.disk_budget_refusals == 6 && stats.write_failures == 6,
            "RAM eviction does not reset the independent disk file budget");
    Require(stats.disk_files == 2 && stats.disk_bytes == 2 * kNativeMaterialFileBytes &&
            stats.disk_inventory_complete, "complete bounded inventory");
    Require(!std::filesystem::exists(limited.path / NativeMaterialLibrary::FileName(missing_id)),
            "refusal creates no extra material file");
    Require(!std::filesystem::exists(limited.path / ".bdmat-writer"), "lease released after refusal");
    NativeMaterialLibrary restarted(limited.path, 2, budget);
    Require(bool(restarted.Resolve(variant(9))) && restarted.Stats().disk_budget_refusals == 1,
            "restart cannot reset file budget");
    NativeMaterialLibrary read_only(limited.path, 2, {0, 0, UINT64_MAX});
    Require(bool(read_only.Load(first_id)) && !read_only.Load(missing_id),
            "source-free loading remains available with writes disabled, without inventing missing assets");
  }
  {
    Scratch limited;
    NativeMaterialLibrary library(limited.path, 8, {2 * kNativeMaterialFileBytes - 1, 16, 0});
    Require(bool(library.Resolve(asset)), "one material fits byte budget");
    const auto second = library.Resolve(variant(2));
    Require(bool(second) && library.Resolve(variant(2)) == second, "failed persistence stays deduplicated in RAM");
    Require(library.Stats().disk_budget_refusals == 1 && library.Stats().disk_files == 1 &&
            library.Stats().disk_bytes == kNativeMaterialFileBytes, "byte limit independent of file count");
  }
  {
    Scratch limited;
    const auto directory = limited.path / "disabled";
    NativeMaterialLibrary library(directory, 2, {kNativeMaterialFileBytes - 1, 2, 0});
    Require(bool(library.Resolve(asset)) && !std::filesystem::exists(directory),
            "oversized single output refuses before creating a directory");
    NativeMaterialLibrary no_files(limited.path / "zero-files", 2, {1024, 0, 0});
    Require(bool(no_files.Resolve(asset)) && no_files.Stats().disk_budget_refusals == 1 &&
            !std::filesystem::exists(limited.path / "zero-files"), "zero file budget has no output");
    NativeMaterialLibrary reserve(limited.path, 2, {1024, 16, UINT64_MAX});
    Require(bool(reserve.Resolve(asset)) && reserve.Stats().disk_budget_refusals == 1 &&
            std::filesystem::is_empty(limited.path), "unavailable free-space reserve refuses and releases lease");
  }
  {
    Scratch limited;
    std::vector<uint8_t> foreign(80, 0xa5);
    WriteBytes(limited.path / "unrecognized.tmp", foreign);
    NativeMaterialLibrary library(limited.path, 2, {2 * kNativeMaterialFileBytes, 16, 0});
    Require(bool(library.Resolve(asset)) && library.Stats().disk_budget_refusals == 1 &&
            library.Stats().disk_bytes == foreign.size(), "foreign and partial files count toward disk bytes");
    Require(std::filesystem::file_size(limited.path / "unrecognized.tmp") == foreign.size(),
            "never evict foreign files to make room");
  }
  {
    Scratch limited;
    WriteBytes(limited.path / "a", file);
    WriteBytes(limited.path / "b", file);
    WriteBytes(limited.path / "c", file);
    NativeMaterialLibrary library(limited.path, 2, {1024, 1, 0});
    Require(bool(library.Resolve(asset)) && library.Stats().disk_budget_refusals == 1 &&
            library.Stats().disk_files == 2 && !library.Stats().disk_inventory_complete,
            "over-budget enumeration stops at one entry over the limit and labels partial inventory");
  }
  {
    Scratch limited;
    auto partial = file;
    partial.resize(12);
    WriteBytes(limited.path / NativeMaterialLibrary::FileName(id), partial);
    NativeMaterialLibrary library(limited.path, 2, {kNativeMaterialFileBytes, 1, 0});
    Require(bool(library.Resolve(asset)) && library.Stats().write_failures == 0 &&
            library.Stats().disk_bytes == kNativeMaterialFileBytes && library.Stats().disk_files == 1,
            "repair charges replacement size, not an additional file");
    NativeMaterialLibrary restarted(limited.path, 2, {0, 0, 0});
    Require(bool(restarted.Load(id)), "bounded replacement remains source-free on restart");
  }
  {
    Scratch limited;
    const auto lock = limited.path / ".bdmat-writer";
    Require(std::filesystem::create_directory(lock), "create simulated cooperating/stale writer lease");
    NativeMaterialLibrary library(limited.path, 2, kTestDisk);
    Require(bool(library.Resolve(asset)) && library.Stats().write_failures == 1 &&
            std::filesystem::is_directory(lock) && !std::filesystem::exists(limited.path / NativeMaterialLibrary::FileName(id)),
            "never steal another writer's lease or write without it");
    Require(std::filesystem::remove(lock), "release test-owned simulated lease");
    NativeMaterialLibrary restarted(limited.path, 2, kTestDisk);
    Require(bool(restarted.Resolve(asset)) && restarted.Stats().write_failures == 0,
            "a later writer can persist after lease release");
  }
  {
    Scratch limited;
    Require(std::filesystem::create_directory(limited.path / "unknown-subtree"), "create unknown subtree");
    NativeMaterialLibrary library(limited.path, 2, kTestDisk);
    Require(bool(library.Resolve(asset)) && library.Stats().write_failures == 1 &&
            !library.Stats().disk_inventory_complete, "unknown subtree refuses rather than recursing or ignoring bytes");
  }
  for (unsigned trial = 0; trial < 8; ++trial) {
    Scratch limited;
    std::barrier start(3);
    std::array<NativeMaterialHandle, 2> materials;
    std::array<NativeMaterialLibraryStats, 2> stats;
    const auto write = [&](size_t index) {
      NativeMaterialLibrary library(limited.path, 2, {kNativeMaterialFileBytes, 1, 0});
      start.arrive_and_wait();
      materials[index] = library.Resolve(variant(uint8_t(index + 1)));
      stats[index] = library.Stats();
    };
    std::thread first(write, 0), second(write, 1);
    start.arrive_and_wait();
    first.join();
    second.join();
    Require(materials[0] && materials[1], "contending writers retain both native resident assets");
    Require(stats[0].write_failures + stats[1].write_failures == 1,
            "only one competing library persists within a one-file budget");
    size_t files = 0;
    for (const auto &entry : std::filesystem::directory_iterator(limited.path)) {
      Require(entry.is_regular_file() && entry.file_size() == kNativeMaterialFileBytes,
              "concurrent write leaves one complete file and no stale lease");
      ++files;
    }
    Require(files == 1, "concurrent library instances cannot oversubscribe aggregate disk budget");
  }
  {
    Scratch limited;
    Scratch outside;
    const auto target = outside.path / "protected.bin";
    auto partial = file;
    partial.resize(12);
    WriteBytes(target, partial);
    std::filesystem::create_hard_link(target, limited.path / NativeMaterialLibrary::FileName(id));
    NativeMaterialLibrary library(limited.path, 2, kTestDisk);
    Require(bool(library.Resolve(asset)) && library.Stats().write_failures == 1 &&
            std::filesystem::file_size(target) == partial.size(), "invalid hard-linked target is never overwritten");
    std::error_code error;
    std::filesystem::create_symlink(target, limited.path / "foreign-link", error);
    if (!error) {
      NativeMaterialLibrary linked(limited.path, 2, kTestDisk);
      Require(bool(linked.Resolve(variant(2))) && linked.Stats().write_failures == 1 &&
              !linked.Stats().disk_inventory_complete, "symlinks refuse without following them");
    } else {
      std::cout << "symlink fixture unavailable: " << error.message() << '\n';
    }
  }
  std::cout << "native material disk byte/file/reserve budgets, restart, repair and lease safeguards passed\n";
  std::cout << "native material format, identity, persistence and ownership passed\n";
}
