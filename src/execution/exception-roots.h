// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef SAAUSO_EXECUTION_EXCEPTION_ROOTS_H_
#define SAAUSO_EXECUTION_EXCEPTION_ROOTS_H_

#include "include/saauso-maybe.h"
#include "src/execution/exception-types.h"
#include "src/handles/handles.h"
#include "src/handles/tagged.h"

namespace saauso::internal {

class FixedArray;
class PyTypeObject;

class Isolate;
class ObjectVisitor;

class ExceptionRoots final {
 public:
  explicit ExceptionRoots(Isolate* isolate);

  Maybe<void> Setup();

  Handle<PyTypeObject> Get(ExceptionType type) const;

  void Iterate(ObjectVisitor* v);

 private:
  static constexpr int kRootSlotCount = static_cast<int>(ExceptionType::kCount);

  Maybe<void> InitializeBuiltinExceptionTypes();
  void Set(ExceptionType type, Handle<PyTypeObject> value);

  int64_t SlotIndex(ExceptionType type) const;
  Handle<FixedArray> roots() const;

  Isolate* isolate_;
  Tagged<FixedArray> roots_{kNullAddress};
};

}  // namespace saauso::internal

#endif  // SAAUSO_EXECUTION_EXCEPTION_ROOTS_H_
