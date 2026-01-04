// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef SAAUSO_OBJECTS_ARRAY_LIST_H_
#define SAAUSO_OBJECTS_ARRAY_LIST_H_

#include "objects/heap-object.h"

namespace saauso::internal {

class ArrayList : public HeapObject {
 public:
  static ArrayList* NewInstance(int init_capacity = 8);

  void PushBack(Object* value);
  void Insert(int index, Object* value);
  Object* PopBack();

  Object* Get(int index) const;
  Object* GetBack() const;

  void Set(int index, Object* value);

  void Remove(int index);
  void Clear();

  int capacity() const { return capacity_; };
  int length() const { return length_; };

  bool IsEmpty() const { return length_ == 0; };
  bool IsFull() const { return length_ == capacity_; }

 private:
  ArrayList() {};

  void Expand();

  int capacity_{0};
  int length_{0};
  Object** array_{0};
};

}  // namespace saauso::internal

#endif  // SAAUSO_OBJECTS_ARRAY_LIST_H_
