/**
 * @file    storage.cpp
 * @brief   Tiny disk-limit fixtures and bounded read-only existing-cache checks.
 * @copyright Copyright (c) 2026 reblue contributors
 * @license BSD 3-Clause, see LICENSE
 */
#include "gpu/scene/native_mesh_storage.h"
#include <barrier>
#include <charconv>
#include <chrono>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <thread>

using namespace bd::gpu::scene;
namespace fs = std::filesystem;
namespace {
void Require(bool value, const char *message) {
  if (!value)
    throw std::runtime_error(message);
}
struct Scratch {
  fs::path path;
  Scratch() {
    const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
    for (unsigned i = 0; i < 100; ++i) {
      const auto candidate = fs::temp_directory_path() /
          ("reblue_mesh_storage_test_" + std::to_string(stamp) + "_" + std::to_string(i));
      if (fs::create_directory(candidate)) {
        path = candidate;
        return;
      }
    }
    throw std::runtime_error("cannot create private mesh fixture directory");
  }
  ~Scratch() {
    // Only the private directory this fixture successfully created. Symlink
    // fixtures are removed explicitly before this cleanup; never a shared root.
    if (!path.empty()) {
      std::error_code ignored;
      fs::remove_all(path, ignored);
    }
  }
};
NativeMeshData Mesh(uint8_t value = 0x3f) {
  NativeMeshData mesh;
  mesh.layout = 123;
  mesh.indices = {0, 1, 2};
  mesh.streams.push_back({0, 4, std::vector<uint8_t>(12, value)});
  return mesh;
}
void Put(const fs::path &path, std::span<const uint8_t> bytes) {
  std::ofstream file(path, std::ios::binary | std::ios::trunc);
  file.write(reinterpret_cast<const char *>(bytes.data()), bytes.size());
  file.close();
  Require(bool(file), "private fixture write");
}
std::vector<uint8_t> Bytes(const NativeMeshData &mesh) {
  std::vector<uint8_t> bytes;
  Require(EncodeNativeMesh(mesh, bytes), "fixture encoding");
  return bytes;
}
} // namespace

void TestMeshStorage() {
  const auto mesh = Mesh();
  const auto bytes = Bytes(mesh);
  constexpr uint64_t metadata = 64ull << 10;
  Require(!NativeMeshWriteFitsReserve(metadata + 99, 100, 0), "whole payload reserve");
  Require(NativeMeshWriteFitsReserve(metadata + 100, 100, 0), "exact reserve");
  Require(!NativeMeshWriteFitsReserve(100, 0, 101), "reserve underflow");
  Require(!NativeMeshWriteFitsReserve(UINT64_MAX, UINT64_MAX, 1), "payload overflow");
  Require(!NativeMeshWriteFitsReserve(metadata, kNativeMeshMaxBytes, 0), "large mesh headroom");
  Require(NativeMeshDiskCache::FileName(1) == "0000000000000001.bdmesh", "v1 filename preserved");
  {
    Scratch scratch;
    NativeMeshDiskCache cache(scratch.path, {bytes.size() * 2, 2, 0});
    Require(cache.Write(1, mesh) && cache.Write(1, mesh), "write and unchanged reuse");
    const auto stamp = fs::last_write_time(scratch.path / cache.FileName(1));
    Require(cache.Write(1, mesh) && fs::last_write_time(scratch.path / cache.FileName(1)) == stamp,
            "reuse never rewrites the file");
    Require(cache.Write(2, Mesh(2)) && !cache.Write(3, Mesh(3)), "file/byte budget");
    Require(cache.Stats().written == 2 && cache.Stats().reused == 2 &&
            cache.Stats().budget_refusals == 1 && cache.Stats().inventory_complete,
            "observable complete inventory and refusals");
    NativeMeshDiskCache restarted(scratch.path, {bytes.size() * 2, 2, 0});
    Require(!restarted.Write(3, Mesh(3)), "restart cannot reset budget");
    NativeMeshDiskCache read_only(scratch.path, {0, 0, UINT64_MAX});
    NativeMeshData loaded;
    Require(read_only.Read(1, loaded) && Bytes(loaded) == bytes, "source-free read with writes disabled");
    Require(!read_only.Read(3, loaded) && Bytes(loaded) == bytes, "failed read is transactional");
    Require(!cache.Write(1, Mesh(9)) && cache.Stats().conflicts == 1, "valid conflicting key preserved");
    Require(cache.Read(1, loaded) && Bytes(loaded) == bytes, "conflict did not overwrite");
    Require(!fs::exists(scratch.path / ".bdmesh-writer"), "lease released");
  }
  {
    Scratch scratch;
    NativeMeshDiskCache cache(scratch.path, {bytes.size() * 2 - 1, 20, 0});
    Require(cache.Write(1, mesh) && !cache.Write(2, mesh), "independent byte cap");
    NativeMeshDiskCache zero(scratch.path / "disabled", {bytes.size() - 1, 1, 0});
    Require(!zero.Write(1, mesh) && !fs::exists(scratch.path / "disabled"), "oversized output creates nothing");
    NativeMeshDiskCache no_files(scratch.path / "no-files", {1024, 0, 0});
    Require(!no_files.Write(1, mesh) && !fs::exists(scratch.path / "no-files"), "zero file cap creates nothing");
    NativeMeshDiskCache reserve(scratch.path, {1024, 20, UINT64_MAX});
    Require(!reserve.Write(2, mesh) && reserve.Stats().budget_refusals == 1, "free-space reserve refusal");
  }
  {
    Scratch scratch;
    Put(scratch.path / "foreign.partial", bytes);
    NativeMeshDiskCache cache(scratch.path, {bytes.size(), 4, 0});
    Require(!cache.Write(1, mesh) && cache.Stats().bytes == bytes.size(), "foreign partials count");
    Require(fs::file_size(scratch.path / "foreign.partial") == bytes.size(), "no eviction");
    Put(scratch.path / "second", bytes);
    Put(scratch.path / "third", bytes);
    NativeMeshDiskCache bounded(scratch.path, {4096, 1, 0});
    Require(!bounded.Write(1, mesh) && bounded.Stats().files == 2 &&
            !bounded.Stats().inventory_complete, "bounded partial inventory");
  }
  {
    Scratch scratch;
    const auto path = scratch.path / NativeMeshDiskCache::FileName(1);
    auto bad = bytes;
    bad[0] ^= 1;
    Put(path, bad);
    NativeMeshDiskCache cache(scratch.path, {bytes.size(), 1, 0});
    NativeMeshData loaded;
    Require(!cache.Read(1, loaded) && cache.Write(1, mesh) && cache.Read(1, loaded), "repair exact-budget corruption");
    Require(Bytes(loaded) == bytes, "repair contents");
    Put(path, std::span(bytes).first(4));
    Require(cache.Write(1, mesh), "repair truncated payload");
    fs::create_directory(scratch.path / ".bdmesh-writer");
    Require(!cache.Write(2, mesh) && fs::exists(scratch.path / ".bdmesh-writer"), "foreign lease never stolen");
    Require(cache.Read(1, loaded), "reads work under held writer lease");
    fs::remove(scratch.path / ".bdmesh-writer");
    fs::create_directory(scratch.path / "unknown-subtree");
    Require(!cache.Write(1, mesh), "unknown subtree refuses inventory");
  }
  {
    Scratch scratch;
    Scratch other;
    const auto target = other.path / "keep";
    Put(target, std::span(bytes).first(4));
    fs::create_hard_link(target, scratch.path / NativeMeshDiskCache::FileName(1));
    NativeMeshDiskCache cache(scratch.path, {1024, 20, 0});
    Require(!cache.Write(1, mesh) && fs::file_size(target) == 4, "hard-linked repair refused");
    std::error_code link_error;
    fs::create_directory_symlink(other.path, scratch.path / "link", link_error);
    if (!link_error) {
      NativeMeshDiskCache through_link(scratch.path / "link" / "nested", {1024, 20, 0});
      Require(!through_link.Write(1, mesh) && !fs::exists(other.path / "nested"), "linked ancestor never followed");
      fs::remove(scratch.path / "link");
      std::cout << "mesh storage symlink ancestor fixture passed\n";
    } else {
      std::cout << "mesh storage symlink ancestor fixture unavailable: " << link_error.message() << '\n';
    }
  }
  for (int trial = 0; trial < 8; ++trial) {
    Scratch scratch;
    NativeMeshDiskCache first(scratch.path, {bytes.size(), 1, 0});
    NativeMeshDiskCache second(scratch.path, {bytes.size(), 1, 0});
    std::barrier start(2);
    bool a = false, b = false;
    std::thread one([&] { start.arrive_and_wait(); a = first.Write(1, mesh); });
    std::thread two([&] { start.arrive_and_wait(); b = second.Write(2, Mesh(2)); });
    one.join();
    two.join();
    Require(a != b, "two writers cannot overrun one-file budget");
    Require(!fs::exists(scratch.path / ".bdmesh-writer"), "contended lease released");
    NativeMeshData loaded;
    Require(first.Read(a ? 1 : 2, loaded), "winning write is complete");
  }
  std::cout << "native mesh disk budgets, restart, reuse, conflicts, repair and writer leases passed\n";
}

int VerifyMeshCache(const char *path) {
  // Explicit read-only mode: no leases, directory creation, repairs or recooking.
  NativeMeshDiskCache cache(path, {0, 0, UINT64_MAX});
  uint64_t bytes = 0;
  size_t files = 0;
  for (const auto &entry : fs::directory_iterator(path)) {
    Require(++files <= 16384, "verification file cap");
    Require(fs::is_regular_file(entry.symlink_status()), "non-regular cache entry");
    const auto size = entry.file_size();
    Require(size <= (256ull << 20) - bytes, "verification byte cap");
    bytes += size;
    const auto stem = entry.path().stem().string();
    Require(stem.size() == 16 && entry.path().extension() == ".bdmesh", "unexpected filename");
    uint64_t key = 0;
    const auto parsed = std::from_chars(stem.data(), stem.data() + stem.size(), key, 16);
    Require(parsed.ec == std::errc{} && parsed.ptr == stem.data() + stem.size(), "invalid key");
    NativeMeshData mesh;
    Require(cache.Read(key, mesh), "existing native mesh rejected");
  }
  Require(files > 0, "empty cache is not coverage");
  std::cout << "read-only native mesh verification: " << files << " files / " << bytes
            << " bytes loaded without source memory or writes\n";
  return 0;
}
