// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef SAAUSO_EXECUTION_EXCEPTION_UTILS_H_
#define SAAUSO_EXECUTION_EXCEPTION_UTILS_H_

#include <cassert>

#include "src/execution/isolate.h"
#include "src/handles/maybe-handles.h"

namespace saauso::internal {

#define ASSIGN_RETURN_ON_EXCEPTION_VALUE(isolate, dst, call, value) \
  do {                                                              \
    if (!(call).To(&dst)) {                                         \
      assert((isolate)->exception_state()->HasPendingException());  \
      return value;                                                 \
    }                                                               \
  } while (false)

#define ASSIGN_RETURN_ON_EXCEPTION(isolate, dst, call) \
  ASSIGN_RETURN_ON_EXCEPTION_VALUE(isolate, dst, call, kNullMaybe)

//////////////////////////////////////////////////////////////////////////

#define RETURN_ON_EXCEPTION_VALUE(isolate, call, value)            \
  do {                                                             \
    if ((call).IsEmpty()) {                                        \
      assert((isolate)->exception_state()->HasPendingException()); \
      return value;                                                \
    }                                                              \
  } while (false)

#define RETURN_ON_EXCEPTION(isolate, call) \
  RETURN_ON_EXCEPTION_VALUE(isolate, call, kNullMaybe)

//////////////////////////////////////////////////////////////////////////

#define ASSIGN_GOTO_ON_EXCEPTION_TARGET(isolate, dst, call, target) \
  do {                                                              \
    if (!(call).To(&dst)) {                                         \
      assert((isolate)->exception_state()->HasPendingException());  \
      goto target;                                                  \
    }                                                               \
  } while (false)

#define GOTO_ON_EXCEPTION_TARGET(isolate, call, target)            \
  do {                                                             \
    if ((call).IsEmpty()) {                                        \
      assert((isolate)->exception_state()->HasPendingException()); \
      goto target;                                                 \
    }                                                              \
  } while (false)

}  // namespace saauso::internal

#endif  // SAAUSO_EXECUTION_EXCEPTION_UTILS_H_
