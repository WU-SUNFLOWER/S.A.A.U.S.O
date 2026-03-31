// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef SAAUSO_HANDLES_HANDLE_BASE_H_
#define SAAUSO_HANDLES_HANDLE_BASE_H_

#include "src/handles/handle-scopes.h"
#include "src/handles/tagged.h"

namespace saauso::internal {

class HandleBase {
 public:
  constexpr bool is_null() const { return location_ == nullptr; }
  constexpr Address* location() const { return location_; }

  constexpr bool is_identical_to(const HandleBase& that) const {
    if (location_ == that.location_) {
      return true;
    }
    if (location_ == nullptr || that.location_ == nullptr) {
      return false;
    }
    return *location_ == *that.location_;
  }

 protected:
  constexpr HandleBase() = default;
  explicit constexpr HandleBase(Address* location) : location_(location) {}

  constexpr HandleBase(Address object, Isolate* isolate) {
    if (object != kNullAddress) {
      location_ = HandleScope::CreateHandle(isolate, object);
    }
  }

#if defined(_DEBUG) || defined(ASAN_BUILD)
  void CheckDereferenceAllowed() const;
#endif  // defined(_DEBUG) || defined(ASAN_BUILD)

#if SAAUSO_ENABLE_CHECK_VALID_LOC_SLOW
  void CheckValidLocationSlow() const;
#endif  // SAAUSO_ENABLE_CHECK_VALID_LOC_SLOW

 private:
  Address* location_{nullptr};
};

}  // namespace saauso::internal

#endif  // SAAUSO_HANDLES_HANDLE_BASE_H_
