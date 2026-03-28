// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef SAAUSO_OBJECTS_NATIVE_LAYOUT_KIND_H_
#define SAAUSO_OBJECTS_NATIVE_LAYOUT_KIND_H_

#include <cstdint>

namespace saauso::internal {

enum class NativeLayoutKind : uint8_t {
  kPyObject,
  kList,
  kDict,
  kTuple,
  kString,
  kFloat,
  kTypeObject,
  kCodeObject,
  kFixedArray,
  kCell,
  kMethodObject,
  kListIterator,
  kTupleIterator,
  kDictKeys,
  kDictValues,
  kDictItems,
  kDictKeyIterator,
  kDictItemIterator,
  kDictValueIterator,
};

}  // namespace saauso::internal

#endif  // SAAUSO_OBJECTS_NATIVE_LAYOUT_KIND_H_
