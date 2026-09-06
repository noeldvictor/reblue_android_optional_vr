/**
 * @file    native_material_library.cpp
 * @brief   Bounded, pinned material library and checked derived asset files.
 * @copyright Copyright (c) 2026 reblue contributors
 * @license   BSD 3-Clause License
 */
#include "gpu/scene/native_material_library.h"

#include <algorithm>
#include <charconv>
#include <fstream>
#include <string>

namespace bd::gpu::scene {
namespace {
// Non-waiting interprocess lease shared by runtime and standalone cookers.
// Never steal an existing/stale lock. A crash may leave it behind: read-only
// loading and in-memory cooking still work until an owner reviews that lock.
struct MaterialWriteLease {
  std::filesystem::path path;
  bool owned = false;
  explicit MaterialWriteLease(const std::filesystem::path &directory)
      : path(directory / ".bdmat-writer") {
    std::error_code error;
    owned = std::filesystem::create_directory(path, error);
  }
  ~MaterialWriteLease() {
    if (owned) {
      std::error_code ignored;
      std::filesystem::remove(path, ignored); // our empty lease, never recursive
    }
  }
};
} // namespace
NativeMaterialLibrary::NativeMaterialLibrary(std::filesystem::path directory,
                                           size_t capacity,
                                           NativeMaterialDiskBudget disk_budget)
    : directory_(std::move(directory)), capacity_(capacity), disk_budget_(disk_budget) {}

std::filesystem::path NativeMaterialLibrary::FileName(NativeMaterialId id) {
  char digits[16];
  const auto result = std::to_chars(std::begin(digits), std::end(digits), id, 16);
  std::string name(16 - (result.ptr - digits), '0');
  name.append(digits, result.ptr);
  return name + ".bdmat";
}

NativeMaterialHandle NativeMaterialLibrary::Find(NativeMaterialId id) {
  auto it = entries_.find(id);
  if (it == entries_.end())
    return {};
  recent_.splice(recent_.begin(), recent_, it->second.recent);
  ++stats_.memory_hits;
  return it->second.material;
}

bool NativeMaterialLibrary::MakeRoom() {
  if (entries_.size() < capacity_)
    return true;
  for (auto it = recent_.rbegin(); it != recent_.rend(); ++it) {
    auto entry = entries_.find(*it);
    if (entry->second.material.use_count() != 1)
      continue;
    recent_.erase(entry->second.recent);
    entries_.erase(entry);
    return true;
  }
  ++stats_.budget_refusals;
  return false;
}

NativeMaterialHandle NativeMaterialLibrary::Insert(NativeMaterialId id,
                                                 const NativeMaterialAsset &asset) {
  if (!MakeRoom())
    return {};
  auto material = std::make_shared<const NativeMaterial>(NativeMaterial{id, asset});
  recent_.push_front(id);
  entries_.emplace(id, Entry{material, recent_.begin()});
  return material;
}

bool NativeMaterialLibrary::Read(NativeMaterialId id, NativeMaterialAsset &asset) {
  if (directory_.empty())
    return {};
  std::ifstream file(directory_ / FileName(id), std::ios::binary | std::ios::ate);
  if (!file)
    return {};
  if (file.tellg() != std::streamoff(kNativeMaterialFileBytes)) {
    ++stats_.invalid;
    return {};
  }
  std::array<uint8_t, kNativeMaterialFileBytes> bytes;
  file.seekg(0);
  if (!file.read(reinterpret_cast<char *>(bytes.data()), bytes.size()) ||
      NativeMaterialContentId(bytes) != id || !DecodeNativeMaterial(bytes, asset)) {
    ++stats_.invalid;
    return {};
  }
  return true;
}

bool NativeMaterialLibrary::Write(NativeMaterialId id, std::span<const uint8_t> bytes) {
  if (directory_.empty())
    return false;
  const auto refuse = [&] { ++stats_.disk_budget_refusals; return false; };
  if (!disk_budget_.max_files || bytes.size() > disk_budget_.max_bytes)
    return refuse();
  std::error_code error;
  std::filesystem::create_directories(directory_, error);
  if (error || !std::filesystem::is_directory(std::filesystem::symlink_status(directory_, error)) || error)
    return false;
  MaterialWriteLease lease(directory_);
  if (!lease.owned)
    return false;

  // Rescan under the lease so restarts and other cooperating library instances
  // cannot reset the budget. Do not recurse, follow links, or evict files.
  stats_.disk_files = 0;
  stats_.disk_bytes = 0;
  stats_.disk_inventory_complete = false;
  const auto path = directory_ / FileName(id);
  uint64_t previous_bytes = 0;
  bool exists = false;
  for (std::filesystem::directory_iterator it(directory_, error), end; it != end && !error; it.increment(error)) {
    if (it->path() == lease.path)
      continue;
    const auto status = it->symlink_status(error);
    if (error || !std::filesystem::is_regular_file(status))
      return false; // unknown subtree or link has an unbounded/unowned footprint
    const auto size = it->file_size(error);
    if (error || size > UINT64_MAX - stats_.disk_bytes)
      return false;
    ++stats_.disk_files;
    stats_.disk_bytes += size;
    if (stats_.disk_files > disk_budget_.max_files || stats_.disk_bytes > disk_budget_.max_bytes)
      return refuse();
    if (it->path() == path) {
      // Repairing an invalid derived file must not change another hard link.
      if (it->hard_link_count(error) != 1 || error)
        return false;
      exists = true;
      previous_bytes = size;
    }
  }
  if (error)
    return false;
  stats_.disk_inventory_complete = true;
  const auto retained_bytes = stats_.disk_bytes - previous_bytes;
  if ((!exists && stats_.disk_files == disk_budget_.max_files) ||
      bytes.size() > disk_budget_.max_bytes - retained_bytes)
    return refuse();
  const auto space = std::filesystem::space(directory_, error);
  if (error || space.available == uintmax_t(-1))
    return false;
  // Keep headroom for the file's allocation and lease/directory metadata, not
  // just its 68 logical bytes. The outer job still budgets total peak overlap.
  if (space.available < disk_budget_.min_free_bytes ||
      space.available - disk_budget_.min_free_bytes < (64ull << 10))
    return refuse();

  // Another writer may have completed between Resolve's read and this lease.
  // Preserve a valid file, including the content-hash collision case.
  if (exists && previous_bytes == kNativeMaterialFileBytes) {
    std::ifstream current(path, std::ios::binary);
    std::array<uint8_t, kNativeMaterialFileBytes> existing;
    NativeMaterialAsset asset;
    if (!current.read(reinterpret_cast<char *>(existing.data()), existing.size()))
      return false;
    if (NativeMaterialContentId(existing) == id && DecodeNativeMaterial(existing, asset)) {
      const bool same = std::equal(existing.begin(), existing.end(), bytes.begin(), bytes.end());
      stats_.invalid += !same;
      return same;
    }
  }
  // A derived cache, never an original asset. Length, checksum and identity
  // validation reject interrupted writes; Resolve recooks them from the source.
  // New names are exclusive; do not truncate a file created outside the lease.
  std::ofstream file(path, std::ios::binary | (exists ? std::ios::trunc : std::ios::noreplace));
  if (!file.is_open())
    return false;
  file.write(reinterpret_cast<const char *>(bytes.data()), bytes.size());
  file.close();
  if (file) {
    stats_.disk_files += !exists;
    stats_.disk_bytes = retained_bytes + bytes.size();
  } else {
    stats_.disk_inventory_complete = false;
    if (!exists)
      std::filesystem::remove(path, error); // only the new partial file we opened exclusively
  }
  return bool(file);
}

NativeMaterialHandle NativeMaterialLibrary::Load(NativeMaterialId id) {
  std::lock_guard lock(mutex_);
  if (auto found = Find(id))
    return found;
  NativeMaterialAsset asset;
  if (!Read(id, asset))
    return {};
  auto material = Insert(id, asset);
  if (material)
    ++stats_.loaded;
  return material;
}

NativeMaterialHandle NativeMaterialLibrary::Resolve(const NativeMaterialAsset &asset) {
  std::vector<uint8_t> bytes;
  NativeMaterialAsset canonical;
  if (!EncodeNativeMaterial(asset, bytes) || !DecodeNativeMaterial(bytes, canonical))
    return {};
  const auto id = NativeMaterialContentId(bytes);
  std::lock_guard lock(mutex_);
  auto found = Find(id);
  if (found) {
    if (found->asset == canonical)
      return found;
    ++stats_.invalid; // hash collision: never alias or overwrite another asset
    return {};
  }
  NativeMaterialAsset cached;
  const bool loaded = Read(id, cached);
  if (loaded && cached != canonical) {
    ++stats_.invalid;
    return {}; // a content-hash collision must not overwrite another asset
  }
  auto material = Insert(id, loaded ? cached : canonical);
  if (!material)
    return {};
  if (loaded) {
    ++stats_.loaded;
    return material;
  }
  ++stats_.cooked;
  if (!Write(id, bytes))
    ++stats_.write_failures;
  return material;
}

NativeMaterialLibraryStats NativeMaterialLibrary::Stats() const {
  std::lock_guard lock(mutex_);
  auto result = stats_;
  result.resident = entries_.size();
  return result;
}
} // namespace bd::gpu::scene
