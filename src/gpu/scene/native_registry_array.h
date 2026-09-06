/**
 * @brief   Address-independent ordered registry array mutations.
 * @copyright Copyright (c) 2026 reblue contributors
 * @license BSD 3-Clause, see LICENSE
 */
#pragma once
#include <cstdint>
#include <stdexcept>
namespace bd::gpu::scene {
// Storage supplies allocation and element access. No callbacks are invoked by
// the mutation itself; registration queries metadata before entering it.
template <class Storage, class Value>
void InsertRegistryEntry(Storage &storage, uint32_t index, Value value) {
  const auto count = storage.Count();
  if (count >= storage.MaxCapacity()) throw std::length_error("Registry capacity exhausted");
  if (index > count) index = count;
  if (count + 1 > storage.Capacity()) {
    const uint64_t capacity = uint64_t(count + 1) * 2;
    if (capacity > storage.MaxCapacity()) throw std::length_error("Registry growth exceeds capacity");
    storage.Reserve(uint32_t(capacity), false);
  }
  for (auto i = count; i > index; --i) storage.Write(i, storage.Read(i - 1));
  storage.Write(index, value);
  storage.SetCount(count + 1);
}
template <class Storage, class Value>
void AppendRegistryEntry(Storage &storage, Value value) {
  const auto count = storage.Count();
  if (count >= storage.MaxCapacity()) throw std::length_error("Registry capacity exhausted");
  if (count + 1 > storage.Capacity()) {
    const uint64_t capacity = uint64_t(count + 1) * (storage.HasStorage() ? 2 : 1);
    if (capacity > storage.MaxCapacity()) throw std::length_error("Registry growth exceeds capacity");
    storage.Reserve(uint32_t(capacity), true);
  }
  storage.SetCount(count + 1);
  storage.Write(count, value);
}
template <class Storage>
void EraseRegistryEntry(Storage &storage, uint32_t index) {
  const auto count = storage.Count();
  if (index >= count) throw std::out_of_range("Registry removal index");
  for (auto i = index; i + 1 < count; ++i) storage.Write(i, storage.Read(i + 1));
  storage.SetCount(count - 1); // retain capacity and the unused tail value
}
} // namespace bd::gpu::scene
