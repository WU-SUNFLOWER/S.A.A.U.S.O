// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef SAAUSO_OBJECTS_HASH_TABLE_H_
#define SAAUSO_OBJECTS_HASH_TABLE_H_

#include <cstdint>

#include "src/objects/objects.h"

namespace saauso::internal {

template <typename Shape, typename Key>
class HashTable : public Object {
 public:
  Object* Get(Key key) const;
  void Put(Key key, Object* value);
  bool Contains(Key key) const;

  int capacity() const { return capacity_; }
  int occupied() const { return occupied_; }

 protected:
  void Init(int init_capacity);
  void Expand();

 private:
  struct Entry {
    Key key;
    Object* value;
  };

  Entry* entries_{nullptr};

  uint64_t GetProbe(uint64_t index, uint64_t mask);

  int capacity_{0};
  int occupied_{0};
};

// class StringHashTableShape {
//   static bool IsMatch(String* key, String* other);
//   static uint64_t Hash(String* key);
// };

// class StringHashTable : public HashTable<StringHashTableShape, String*> {
//  public:
//   static StringHashTable* NewInstance(int init_capacity = 4);
// };

}  // namespace saauso::internal

#endif  // SAAUSO_OBJECTS_HASH_TABLE_H_
