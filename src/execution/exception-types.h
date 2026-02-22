// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef SAAUSO_EXECUTION_EXCEPTION_TYPES_H_
#define SAAUSO_EXECUTION_EXCEPTION_TYPES_H_

namespace saauso::internal {

// 定义所有支持的异常类型枚举。
#define EXCEPTION_TYPE_LIST(V)                                         \
  V(kBaseException, base_exception, "BaseException")                   \
  V(kException, exception, "Exception")                                \
  V(kTypeError, type_err, "TypeError")                                 \
  V(kRuntimeError, runtime_err, "RuntimeError")                        \
  V(kValueError, value_err, "ValueError")                              \
  V(kIndexError, index_err, "IndexError")                              \
  V(kKeyError, key_err, "KeyError")                                    \
  V(kNameError, name_err, "NameError")                                 \
  V(kAttributeError, attr_err, "AttributeError")                       \
  V(kZeroDivisionError, div_zero, "ZeroDivisionError")                 \
  V(kImportError, import_err, "ImportError")                           \
  V(kModuleNotFoundError, module_not_found_err, "ModuleNotFoundError") \
  V(kStopIteration, stop_iter, "StopIteration")

enum class ExceptionType {
#define DEFINE_EXCEPTION_TYPE(type, ignore1, ignore2) type,
  EXCEPTION_TYPE_LIST(DEFINE_EXCEPTION_TYPE)
#undef DEFINE_EXCEPTION_TYPE
};

}  // namespace saauso::internal

#endif  // SAAUSO_EXECUTION_EXCEPTION_TYPES_H_
