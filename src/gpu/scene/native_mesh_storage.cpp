/**
 * @file    native_mesh_storage.cpp
 * @brief   Restart-safe mesh disk limits and checked, non-evicting writes.
 * @copyright Copyright (c) 2026 reblue contributors
 * @license BSD 3-Clause, see LICENSE
 */
#include "gpu/scene/native_mesh_storage.h"

#include <charconv>
#include <fstream>
#include <string>
#ifdef _WIN32
#include <Windows.h>
#endif

namespace bd::gpu::scene {
namespace {
namespace fs = std::filesystem;

bool IsReparse(const fs::path &path) {
#ifdef _WIN32
  const auto attributes = GetFileAttributesW(path.c_str());
  return attributes == INVALID_FILE_ATTRIBUTES ||
         (attributes & FILE_ATTRIBUTE_REPARSE_POINT) != 0;
#else
  std::error_code error;
  return fs::is_symlink(fs::symlink_status(path, error)) || bool(error);
#endif
}

bool SafeDirectoryAncestors(const fs::path &directory) {
  if (directory.empty() || directory == directory.root_path())
    return false;
  for (auto path = directory; !path.empty();) {
    std::error_code error;
    const auto status = fs::symlink_status(path, error);
    if (error && error != std::errc::no_such_file_or_directory)
      return false;
    if (fs::exists(status) && (!fs::is_directory(status) || IsReparse(path)))
      return false;
    const auto parent = path.parent_path();
    if (parent == path)
      break;
    path = parent;
  }
  return true;
}

// Same non-waiting lease policy as the native material library. Only remove
// the empty directory this instance created; never clear another writer's lock.
struct WriteLease {
  fs::path path;
  bool owned = false;
  explicit WriteLease(const fs::path &directory) : path(directory / ".bdmesh-writer") {
    std::error_code error;
    owned = fs::create_directory(path, error);
  }
  ~WriteLease() {
    if (owned) {
      std::error_code ignored;
      fs::remove(path, ignored);
    }
  }
};

bool ReadBytes(const fs::path &path, std::vector<uint8_t> &bytes) {
  std::error_code error;
  if (!fs::is_regular_file(fs::symlink_status(path, error)) || error || IsReparse(path))
    return false;
  std::ifstream file(path, std::ios::binary | std::ios::ate);
  if (!file)
    return false;
  const auto size = file.tellg();
  if (size < 36 || uint64_t(size) > kNativeMeshMaxBytes)
    return false;
  bytes.resize(static_cast<size_t>(size));
  file.seekg(0);
  return bool(file.read(reinterpret_cast<char *>(bytes.data()), size));
}
} // namespace

NativeMeshDiskCache::NativeMeshDiskCache(std::filesystem::path directory,
                                       NativeMeshDiskBudget budget)
    : budget_(budget) {
  if (!directory.empty()) {
    std::error_code error;
    directory_ = std::filesystem::absolute(directory, error).lexically_normal();
    if (error)
      directory_.clear();
  }
}

std::filesystem::path NativeMeshDiskCache::FileName(uint64_t key) {
  char digits[16];
  const auto end = std::to_chars(std::begin(digits), std::end(digits), key, 16).ptr;
  return std::string(16 - (end - digits), '0') + std::string(digits, end) + ".bdmesh";
}

bool NativeMeshDiskCache::Read(uint64_t key, NativeMeshData &mesh) {
  std::lock_guard lock(mutex_);
  std::vector<uint8_t> bytes;
  if (!SafeDirectoryAncestors(directory_) ||
      !ReadBytes(directory_ / FileName(key), bytes))
    return false;
  if (!DecodeNativeMesh(bytes, mesh)) {
    ++stats_.invalid;
    return false;
  }
  ++stats_.loaded;
  return true;
}

bool NativeMeshDiskCache::Write(uint64_t key, const NativeMeshData &mesh) {
  std::lock_guard lock(mutex_);
  const auto fail = [&] { ++stats_.write_failures; return false; };
  const auto refuse = [&] { ++stats_.budget_refusals; return fail(); };
  std::vector<uint8_t> bytes;
  if (!EncodeNativeMesh(mesh, bytes))
    return fail();
  if (!budget_.max_files || bytes.size() > budget_.max_bytes)
    return refuse();
  if (!SafeDirectoryAncestors(directory_))
    return fail();
  std::error_code error;
  fs::create_directories(directory_, error);
  if (error || !SafeDirectoryAncestors(directory_))
    return fail();
  WriteLease lease(directory_);
  if (!lease.owned)
    return fail();

  stats_.files = 0;
  stats_.bytes = 0;
  stats_.inventory_complete = false;
  const auto path = directory_ / FileName(key);
  uint64_t previous_bytes = 0;
  bool exists = false;
  // Flat, bounded inventory under the lease, including invalid/foreign files.
  // A restart, another cache instance, or RAM eviction cannot reset disk use.
  for (fs::directory_iterator it(directory_, error), end; it != end && !error; it.increment(error)) {
    if (it->path() == lease.path)
      continue;
    const auto status = it->symlink_status(error);
    if (error || !fs::is_regular_file(status) || IsReparse(it->path()))
      return fail();
    const auto size = it->file_size(error);
    if (error || size > UINT64_MAX - stats_.bytes)
      return fail();
    ++stats_.files;
    stats_.bytes += size;
    if (stats_.files > budget_.max_files || stats_.bytes > budget_.max_bytes)
      return refuse();
    if (it->path() == path) {
      if (it->hard_link_count(error) != 1 || error)
        return fail(); // do not repair a file through another owner's hard link
      exists = true;
      previous_bytes = size;
    }
  }
  if (error)
    return fail();
  stats_.inventory_complete = true;
  const auto retained = stats_.bytes - previous_bytes;
  if ((!exists && stats_.files == budget_.max_files) ||
      bytes.size() > budget_.max_bytes - retained)
    return refuse();

  if (exists && previous_bytes >= 36 && previous_bytes <= kNativeMeshMaxBytes) {
    std::vector<uint8_t> current;
    NativeMeshData decoded;
    if (!ReadBytes(path, current))
      return fail(); // unreadable is not proof of corruption; do not truncate it
    if (DecodeNativeMesh(current, decoded)) {
      if (current == bytes) {
        ++stats_.reused;
        return true;
      }
      ++stats_.conflicts;
      return fail(); // a different valid mesh under this key is never overwritten
    }
  }
  const auto space = fs::space(directory_, error);
  if (error || space.available == uintmax_t(-1))
    return fail();
  // Reserve the entire incoming payload plus allocation/metadata headroom.
  // Do not treat a large mesh like the material library's tiny fixed-size file.
  if (!NativeMeshWriteFitsReserve(space.available, bytes.size(), budget_.min_free_bytes))
    return refuse();

  // Derived cache only: invalid single-link files may be repaired. Exclusive
  // creation protects new names from non-cooperating creators. Length/checksum
  // validation rejects interrupted writes; no temporary disk copy is required.
  std::ofstream file(path, std::ios::binary | (exists ? std::ios::trunc : std::ios::noreplace));
  if (!file.is_open())
    return fail();
  file.write(reinterpret_cast<const char *>(bytes.data()), bytes.size());
  file.close();
  if (!file) {
    stats_.inventory_complete = false;
    if (!exists)
      fs::remove(path, error); // only our exclusively-created partial file
    return fail();
  }
  ++stats_.written;
  stats_.files += !exists;
  stats_.bytes = retained + bytes.size();
  return true;
}

NativeMeshDiskStats NativeMeshDiskCache::Stats() const {
  std::lock_guard lock(mutex_);
  return stats_;
}
} // namespace bd::gpu::scene
