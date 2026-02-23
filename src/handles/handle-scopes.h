// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef SAAUSO_HANDLES_HANDLE_SCOPES_H_
#define SAAUSO_HANDLES_HANDLE_SCOPES_H_

#include "src/handles/tagged.h"

namespace saauso::internal {

class Isolate;
class HandleScopeImplementer;

class HandleScope {
 public:
  struct State {
    Address* next{nullptr};
    Address* limit{nullptr};
    int extension{-1};  // 进入该scope后额外申请的block数量
  };

  HandleScope();
  HandleScope(const HandleScope&) = delete;
  HandleScope& operator=(const HandleScope&) = delete;

  ~HandleScope();

  static Address* CreateHandle(Address ptr);
  static void AssertValidLocation(Address* location);

 protected:
  Address* CloseAndEscape(Address ptr);

 private:
  static void Extend();

  void Close();

  Isolate* isolate_{nullptr};
  State previous_;
  bool is_closed_{false};
};

template <typename>
class Handle;

class EscapableHandleScope final : public HandleScope {
 public:
  EscapableHandleScope() = default;
  EscapableHandleScope(const EscapableHandleScope&) = delete;
  EscapableHandleScope& operator=(const EscapableHandleScope&) = delete;

  template <typename T>
  Handle<T> Escape(Handle<T> value) {
    return value.is_null() ? Handle<T>::null()
                           : Handle<T>(CloseAndEscape(*value.location()));
  }
};

}  // namespace saauso::internal

#endif  // SAAUSO_HANDLES_HANDLE_SCOPES_H_
