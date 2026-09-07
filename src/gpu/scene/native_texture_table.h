/**
 * @brief Bounded immutable native texture tables, independent of engine storage.
 * @copyright Copyright (c) 2026 reblue contributors
 * @license BSD 3-Clause License
 */
#pragma once
#include "gpu/scene/native_texture_binding.h"
#include <atomic>
#include <limits>

namespace bd::gpu::scene {
struct NativeTextureTableSlot {
  NativeTextureBinding image;
  // A known null selection is different from an unconverted/dynamic resource.
  bool available = false;
  bool operator==(const NativeTextureTableSlot &) const = default;
};
struct NativeTextureTable {
  uint64_t id = 0; // monotonic runtime identity, never an engine address
  std::vector<NativeTextureTableSlot> slots;
};
using NativeTextureTableHandle = std::shared_ptr<const NativeTextureTable>;

class NativeTextureTableLibrary {
public:
  static constexpr size_t kMaxSlots = 4096;
  // Inline texture lists are per-request objects, not one list per GPU image.
  // A field load already creates >4096 distinct one-slot lists. Keep the byte
  // budget authoritative while bounding the temporary source index as well.
  static constexpr size_t kMaxTables = 16384;
  static constexpr size_t kMaxBytes = 16u << 20;
  explicit NativeTextureTableLibrary(size_t max_bytes = kMaxBytes,
                                    size_t max_tables = kMaxTables)
      : max_bytes_(max_bytes), max_tables_(max_tables) {}
  // The temporary source index is charged here too. GPU residency has its own
  // existing fence-aware owner; these leases pin it without copying image data.
  NativeTextureTableHandle Create(std::vector<NativeTextureTableSlot> slots,
                                  size_t adapter_bytes = 0) {
    std::lock_guard lock(mutex_);
    constexpr size_t overhead = sizeof(Owner) + 256;
    if (slots.size() > kMaxSlots || adapter_bytes > max_bytes_ ||
        overhead > max_bytes_ - adapter_bytes ||
        slots.capacity() > (max_bytes_ - adapter_bytes - overhead) / sizeof(NativeTextureTableSlot))
      return {};
    const size_t bytes = overhead + adapter_bytes + slots.capacity() * sizeof(NativeTextureTableSlot);
    if (next_ == UINT64_MAX || accounting_->live.load() >= max_tables_ ||
        accounting_->bytes.load() > max_bytes_ - bytes)
      return {};
    auto owner = std::make_shared<Owner>();
    owner->table.id = ++next_;
    owner->table.slots = std::move(slots);
    owner->bytes = bytes;
    accounting_->bytes.fetch_add(bytes);
    accounting_->live.fetch_add(1);
    owner->accounting = accounting_;
    return {owner, &owner->table};
  }
  size_t Bytes() const { return accounting_->bytes.load(); }
  size_t Live() const { return accounting_->live.load(); }
private:
  struct Accounting { std::atomic<size_t> bytes{0}, live{0}; };
  struct Owner {
    NativeTextureTable table;
    std::shared_ptr<Accounting> accounting;
    size_t bytes = 0;
    ~Owner() {
      if (accounting) { accounting->bytes.fetch_sub(bytes); accounting->live.fetch_sub(1); }
    }
  };
  const size_t max_bytes_, max_tables_;
  std::shared_ptr<Accounting> accounting_ = std::make_shared<Accounting>();
  std::mutex mutex_;
  uint64_t next_ = 0;
};
} // namespace bd::gpu::scene
