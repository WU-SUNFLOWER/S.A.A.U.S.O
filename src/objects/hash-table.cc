// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "objects/hash-table.h"

#include <cassert>
#include <cstring>

#include "heap/heap.h"
#include "runtime/universe.h"

namespace saauso::internal {

namespace {
constexpr double kMaxLoadFactor = 0.75;
}  // namespace

///////////////////////////////////////////////////////////////////////////////////////
// HashTable基座实现

template <typename Shape, typename Key>
Object* HashTable<Shape, Key>::Get(Key key) const {
  if (capacity_ == 0) {
    return nullptr;
  }

  uint64_t hash = Shape::Hash(key);
  uint64_t mask = capacity_ - 1;
  uint64_t index = hash & mask;

  while (entries_[index].value != nullptr) {
    if (Shape::IsMatch(key, entries_[index].key)) {
      return entries_[index].value;
    }
    index = GetProbe(index, mask);
  }

  return nullptr;
}

template <typename Shape, typename Key>
void HashTable<Shape, Key>::Put(Key key, Object* value) {
  assert(value != nullptr);

  if (occupied_ + 1 > capacity_ * kMaxLoadFactor) {
    Expand();
  }

  uint64_t hash = Shape::Hash(key);
  uint64_t mask = capacity_ - 1;
  uint64_t index = hash & mask;

  while (entries_[index].value != nullptr) {
    if (Shape::IsMatch(key, entries_[index].key)) {
      entries_[index].value = value;
      return;
    }
    index = GetProbe(index, mask);
  }

  entries_[index].key = key;
  entries_[index].value = value;
  ++occupied_;
}

template <typename Shape, typename Key>
bool HashTable<Shape, Key>::Contains(Key key) const {
  return Get(key) != nullptr;
}

template <typename Shape, typename Key>
void HashTable<Shape, Key>::Init(int init_capacity) {
  // 因为探测公式取`index = (capacity_ - 1) & mask`，
  // 为了在按位与之后充分利用哈希表中的各个槽位，
  // 我们要求init_capacity必须取2的幂次方
  assert((init_capacity & (init_capacity - 1)) == 0);

  size_t entries_size = sizeof(Entry) * init_capacity;
  entries_ = static_cast<Entry*>(Universe::heap_->AllocateRaw(entries_size));
  std::memset(entries_, 0x00, entries_size);

  capacity_ = init_capacity;
}

template <typename Shape, typename Key>
void HashTable<Shape, Key>::Expand() {
  int new_capacity = capacity_ << 1;

  size_t new_entries_size = sizeof(Entry) * new_capacity;
  Entry* new_entries = Universe::heap_->AllocateRaw(new_entries_size);
  std::memset(new_entries, 0x00, new_entries_size);

  uint64_t mask = new_capacity - 1;

  // Rehash所有旧数据
  for (int i = 0; i < capacity_; ++i) {
    if (entries_[i].value != nullptr) {
      Key key = entries_[i].key;
      uint64_t hash = Shape::Hash(key);
      uint64_t index = hash & mask;

      while (new_entries[index].value != nullptr) {
        index = GetProbe(index, mask);
      }

      new_entries[index] = entries_[i];
    }
  }

  capacity_ = new_capacity;
  entries_ = new_entries;
}

template <typename Shape, typename Key>
uint64_t HashTable<Shape, Key>::GetProbe(uint64_t index, uint64_t mask) {
  return (index + 1) & mask;
}

///////////////////////////////////////////////////////////////////////////////////////
// 字符串哈希表实现

// // static
// bool StringHashTableShape::IsMatch(String* key, String* other) {
//   return key->IsEqualTo(other);
// }

// // static
// uint64_t StringHashTableShape::Hash(String* key) {
//   return key->GetHash();
// }

// // static
// StringHashTable* StringHashTable::NewInstance(int init_capacity) {
//   HandleScope scope;

//   Handle<StringHashTable>
//   object(Universe::heap_->Allocate<StringHashTable>());
//   object->Init(init_capacity);

//   return object.get();
// }

}  // namespace saauso::internal
