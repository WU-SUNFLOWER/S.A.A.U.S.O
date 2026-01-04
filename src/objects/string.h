// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef SAAUSO_OBJECTS_STRING_H_
#define SAAUSO_OBJECTS_STRING_H_

#include <cstdint>

#include "objects/heap-object.h"

namespace saauso::internal {

class String : public HeapObject {
 public:
  static String* NewInstance(int length);
  static String* NewInstance(const char* source, int length);

  void Set(int index, char value);
  char Get(int index) const;

  uint64_t GetHash();
  bool HasHashCache() const;

  bool IsLessThan(String* other);
  bool IsEqualTo(String* other);
  bool IsLargerThan(String* other);

  String* Slice(int from, int to) const;
  String* Append(String* other) const;

  int length() const { return length_; }
  const char* buffer() const { return buffer_; }

 private:
  String() {};

  char* buffer_{nullptr};
  int length_{0};
  uint64_t hash_{0};
};

}  // namespace saauso::internal

#endif
