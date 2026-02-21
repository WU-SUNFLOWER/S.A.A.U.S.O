// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef SRC_EXECUTION_EXCEPTION_UTILS_H_
#define SRC_EXECUTION_EXCEPTION_UTILS_H_

#include <cassert>

#include "src/execution/isolate.h"
#include "src/handles/maybe-handles.h"

#define ASSIGN_RETURN_ON_EXCEPTION_VALUE(isolate, dst, call, value) \
  do {                                                             \
    auto _maybe_handle_ = (call);                                  \
    if (!_maybe_handle_.ToHandle(&dst)) {                          \
      assert((isolate)->exception_state()->HasPendingException()); \
      return value;                                                \
    }                                                              \
  } while (false)

#define ASSIGN_RETURN_ON_EXCEPTION(isolate, dst, call) \
  ASSIGN_RETURN_ON_EXCEPTION_VALUE(isolate, dst, call, kNullMaybe)

#define RETURN_ON_EXCEPTION_VALUE(isolate, call, value)             \
  do {                                                              \
    auto _maybe_handle_ = (call);                                   \
    if (_maybe_handle_.IsEmpty()) {                                 \
      assert((isolate)->exception_state()->HasPendingException());  \
      return value;                                                 \
    }                                                               \
  } while (false)

#define RETURN_ON_EXCEPTION(isolate, call) \
  RETURN_ON_EXCEPTION_VALUE(isolate, call, kNullMaybe)

#endif  // SRC_EXECUTION_EXCEPTION_UTILS_H_

