// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include <chrono>
#include <cinttypes>
#include <cstdint>
#include <thread>

#include "src/execution/exception-utils.h"
#include "src/execution/isolate.h"
#include "src/handles/handles.h"
#include "src/heap/factory.h"
#include "src/modules/builtin-module-utils.h"
#include "src/modules/module-manager.h"
#include "src/objects/klass.h"
#include "src/objects/py-dict.h"
#include "src/objects/py-float.h"
#include "src/objects/py-module.h"
#include "src/objects/py-object.h"
#include "src/objects/py-oddballs.h"
#include "src/objects/py-smi.h"
#include "src/objects/py-string.h"
#include "src/objects/py-tuple.h"
#include "src/runtime/runtime-exceptions.h"
#include "src/runtime/string-table.h"
#include "src/utils/maybe.h"

namespace saauso::internal {

namespace {

#define TIME_MODULE_FUNC_LIST(V)      \
  V("time", Time_Time)                \
  V("perf_counter", Time_PerfCounter) \
  V("monotonic", Time_Monotonic)      \
  V("sleep", Time_Sleep)

// 成功时返回 true 并写入 *out；类型不符时抛 TypeError 并返回 false。
Maybe<double> ExtractSeconds(Handle<PyObject> value, const char* func_name) {
  if (IsPyFloat(value)) {
    return Maybe<double>(Handle<PyFloat>::cast(value)->value());
  }

  if (IsPySmi(value)) {
    return Maybe<double>(PySmi::ToInt(Handle<PySmi>::cast(value)));
  }

  Handle<PyString> type_name = PyObject::GetKlass(value)->name();
  Runtime_ThrowErrorf(ExceptionType::kTypeError,
                      "%s() argument must be int or float, not '%s'", func_name,
                      type_name->buffer());
  return kNullMaybe;
}

double WallTimeSeconds() {
  using clock = std::chrono::system_clock;
  auto now = clock::now().time_since_epoch();
  return std::chrono::duration_cast<std::chrono::duration<double>>(now).count();
}

double MonotonicSeconds() {
  using clock = std::chrono::steady_clock;
  auto now = clock::now().time_since_epoch();
  return std::chrono::duration_cast<std::chrono::duration<double>>(now).count();
}

BUILTIN_MODULE_FUNC(Time_Time) {
  if (!kwargs.is_null() && kwargs->occupied() != 0) {
    ThrowNoKeywordArgsError("time", "time");
    return kNullMaybe;
  }
  int64_t argc = args.is_null() ? 0 : args->length();
  if (argc != 0) {
    Runtime_ThrowErrorf(ExceptionType::kTypeError,
                        "time.time() takes 0 positional arguments but %" PRId64
                        " "
                        "were given",
                        argc);
    return kNullMaybe;
  }
  return PyFloat::NewInstance(WallTimeSeconds());
}

BUILTIN_MODULE_FUNC(Time_PerfCounter) {
  if (!kwargs.is_null() && kwargs->occupied() != 0) {
    ThrowNoKeywordArgsError("time", "perf_counter");
    return kNullMaybe;
  }
  int64_t argc = args.is_null() ? 0 : args->length();
  if (argc != 0) {
    Runtime_ThrowErrorf(ExceptionType::kTypeError,
                        "time.perf_counter() takes 0 positional arguments but "
                        "%" PRId64 " were given",
                        argc);
    return kNullMaybe;
  }
  return PyFloat::NewInstance(MonotonicSeconds());
}

BUILTIN_MODULE_FUNC(Time_Monotonic) {
  return BUILTIN_MODULE_FUNC_NAME(Time_PerfCounter)(isolate, receiver, args,
                                                    kwargs);
}

BUILTIN_MODULE_FUNC(Time_Sleep) {
  if (!kwargs.is_null() && kwargs->occupied() != 0) {
    ThrowNoKeywordArgsError("time", "sleep");
    return kNullMaybe;
  }
  int64_t argc = args.is_null() ? 0 : args->length();
  if (argc != 1) {
    Runtime_ThrowErrorf(
        ExceptionType::kTypeError,
        "time.sleep() takes exactly 1 argument (%" PRId64 " given)", argc);
    return kNullMaybe;
  }

  double seconds = 0;
  ASSIGN_RETURN_ON_EXCEPTION(isolate, seconds,
                             ExtractSeconds(args->Get(0), "sleep"));

  if (seconds < 0) {
    Runtime_ThrowError(ExceptionType::kValueError,
                       "sleep length must be non-negative");
    return kNullMaybe;
  }

  std::this_thread::sleep_for(std::chrono::duration<double>(seconds));
  return handle(isolate->py_none_object());
}

}  // namespace

BUILTIN_MODULE_INIT_FUNC("time", InitTimeModule) {
  EscapableHandleScope scope;

  Handle<PyModule> module;
  ASSIGN_RETURN_ON_EXCEPTION(isolate, module,
                             NewBuiltinModuleWithDefaultMeta(isolate, "time"));

  Handle<PyDict> module_dict = PyObject::GetProperties(module);

  const BuiltinModuleFuncSpec kTimeModuleFuncs[] = {
#define DEFINE_TIME_FUNC_SPEC(name, func) \
  {name, &BUILTIN_MODULE_FUNC_NAME(func)},
      TIME_MODULE_FUNC_LIST(DEFINE_TIME_FUNC_SPEC)
#undef DEFINE_TIME_FUNC_SPEC
  };
  RETURN_ON_EXCEPTION(isolate,
                      InstallBuiltinModuleFuncsFromSpec(
                          isolate, module_dict, kTimeModuleFuncs,
                          BuiltinModuleFuncSpecCount(kTimeModuleFuncs)));

  return scope.Escape(module);
}

}  // namespace saauso::internal
