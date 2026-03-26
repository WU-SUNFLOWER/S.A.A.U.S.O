// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include <chrono>
#include <cinttypes>
#include <cstdint>
#include <thread>

#include "include/saauso-maybe.h"
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

namespace saauso::internal {

namespace {

constexpr const char* kModuleName = "time";

#define TIME_MODULE_FUNC_LIST(V)      \
  V("time", Time_Time)                \
  V("perf_counter", Time_PerfCounter) \
  V("monotonic", Time_Monotonic)      \
  V("sleep", Time_Sleep)

// 成功时返回有效值；类型不符时抛 TypeError 并返回 kNullMaybe。
Maybe<double> ExtractSeconds(Isolate* isolate,
                             Handle<PyObject> value,
                             const char* func_name) {
  if (IsPyFloat(value)) {
    return Maybe<double>(Handle<PyFloat>::cast(value)->value());
  }

  if (IsPySmi(value)) {
    return Maybe<double>(PySmi::ToInt(Handle<PySmi>::cast(value)));
  }

  Handle<PyString> type_name = PyObject::GetKlass(value)->name();
  Runtime_ThrowErrorf(isolate, ExceptionType::kTypeError,
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

}  // namespace

/////////////////////////////////////////////////////////////////////////////////

namespace module_impl {

BUILTIN_MODULE_FUNC(Time_Time) {
  BUILTIN_MODULE_EXPECT_NO_KWARGS_OR_RETURN(isolate, kwargs, kModuleName,
                                            "time");
  int64_t argc = BUILTIN_MODULE_ARGC(args);
  BUILTIN_MODULE_EXPECT_ARGC_EQ_OR_RETURN(
      argc, 0,
      Runtime_ThrowErrorf(isolate, ExceptionType::kTypeError,
                          "%s.time() takes 0 positional arguments but %" PRId64
                          " were given",
                          kModuleName, argc));
  return PyFloat::NewInstance(WallTimeSeconds());
}

BUILTIN_MODULE_FUNC(Time_PerfCounter) {
  BUILTIN_MODULE_EXPECT_NO_KWARGS_OR_RETURN(isolate, kwargs, kModuleName,
                                            "perf_counter");
  int64_t argc = BUILTIN_MODULE_ARGC(args);
  BUILTIN_MODULE_EXPECT_ARGC_EQ_OR_RETURN(
      argc, 0,
      Runtime_ThrowErrorf(isolate, ExceptionType::kTypeError,
                          "%s.perf_counter() takes 0 positional arguments "
                          "but %" PRId64 " were given",
                          kModuleName, argc));
  return PyFloat::NewInstance(MonotonicSeconds());
}

BUILTIN_MODULE_FUNC(Time_Monotonic) {
  return BUILTIN_MODULE_FUNC_NAME(Time_PerfCounter)(isolate, receiver, args,
                                                    kwargs);
}

BUILTIN_MODULE_FUNC(Time_Sleep) {
  BUILTIN_MODULE_EXPECT_NO_KWARGS_OR_RETURN(isolate, kwargs, kModuleName,
                                            "sleep");
  int64_t argc = BUILTIN_MODULE_ARGC(args);
  BUILTIN_MODULE_EXPECT_ARGC_EQ_OR_RETURN(
      argc, 1,
      Runtime_ThrowErrorf(isolate, ExceptionType::kTypeError,
                          "%s.sleep() takes exactly 1 argument (%" PRId64
                          " given)",
                          kModuleName, argc));

  double seconds = 0;
  ASSIGN_RETURN_ON_EXCEPTION(isolate, seconds,
                             ExtractSeconds(isolate, args->Get(0), "sleep"));

  if (seconds < 0) {
    Runtime_ThrowError(isolate, ExceptionType::kValueError,
                       "sleep length must be non-negative");
    return kNullMaybe;
  }

  std::this_thread::sleep_for(std::chrono::duration<double>(seconds));
  return handle(isolate->py_none_object());
}

}  // namespace module_impl

/////////////////////////////////////////////////////////////////////////////////

BUILTIN_MODULE_INIT_FUNC("time", InitTimeModule) {
  EscapableHandleScope scope;

  Handle<PyModule> module;
  ASSIGN_RETURN_ON_EXCEPTION(
      isolate, module,
      BuiltinModuleUtils::NewBuiltinModule(isolate, kModuleName));

  const BuiltinModuleFuncSpec kTimeModuleFuncs[] = {
#define DEFINE_TIME_FUNC_SPEC(name, func) \
  {name, &module_impl::BUILTIN_MODULE_FUNC_NAME(func)},
      TIME_MODULE_FUNC_LIST(DEFINE_TIME_FUNC_SPEC)
#undef DEFINE_TIME_FUNC_SPEC
  };
  RETURN_ON_EXCEPTION(
      isolate,
      BuiltinModuleUtils::InstallBuiltinModuleFuncsFromSpec(
          isolate, module, kTimeModuleFuncs,
          BuiltinModuleUtils::BuiltinModuleFuncSpecCount(kTimeModuleFuncs)));

  return scope.Escape(module);
}

}  // namespace saauso::internal
