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
#include "src/modules/module-manager.h"
#include "src/objects/klass.h"
#include "src/objects/py-dict.h"
#include "src/objects/py-float.h"
#include "src/objects/py-function.h"
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

void FailNoKeywordArgs(const char* func_name) {
  Runtime_ThrowErrorf(ExceptionType::kTypeError,
                      "time.%s() takes no keyword arguments", func_name);
}

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

void InstallFunc(Handle<PyDict> module_dict,
                 const char* name,
                 NativeFuncPointer func) {
  Handle<PyString> py_name = PyString::NewInstance(name);
  PyDict::Put(module_dict, py_name, PyFunction::NewInstance(func, py_name));
}

MaybeHandle<PyObject> Time_Time(Handle<PyObject> host,
                                Handle<PyTuple> args,
                                Handle<PyDict> kwargs) {
  if (!kwargs.is_null() && kwargs->occupied() != 0) {
    FailNoKeywordArgs("time");
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

MaybeHandle<PyObject> Time_PerfCounter(Handle<PyObject> host,
                                       Handle<PyTuple> args,
                                       Handle<PyDict> kwargs) {
  if (!kwargs.is_null() && kwargs->occupied() != 0) {
    FailNoKeywordArgs("perf_counter");
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

MaybeHandle<PyObject> Time_Monotonic(Handle<PyObject> host,
                                     Handle<PyTuple> args,
                                     Handle<PyDict> kwargs) {
  return Time_PerfCounter(host, args, kwargs);
}

MaybeHandle<PyObject> Time_Sleep(Handle<PyObject> host,
                                 Handle<PyTuple> args,
                                 Handle<PyDict> kwargs) {
  if (!kwargs.is_null() && kwargs->occupied() != 0) {
    FailNoKeywordArgs("sleep");
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
  ASSIGN_RETURN_ON_EXCEPTION(Isolate::Current(), seconds,
                             ExtractSeconds(args->Get(0), "sleep"));

  if (seconds < 0) {
    Runtime_ThrowError(ExceptionType::kValueError,
                       "sleep length must be non-negative");
    return kNullMaybe;
  }

  std::this_thread::sleep_for(std::chrono::duration<double>(seconds));
  return handle(Isolate::Current()->py_none_object());
}

}  // namespace

BUILTIN_MODULE_INIT_FUNC("time", InitTimeModule) {
  EscapableHandleScope scope;
  Handle<PyModule> module = PyModule::NewInstance();
  Handle<PyDict> module_dict = PyObject::GetProperties(module);

  PyDict::Put(module_dict, ST(name), PyString::NewInstance("time"));
  PyDict::Put(module_dict, ST(package), PyString::NewInstance(""));

  InstallFunc(module_dict, "time", &Time_Time);
  InstallFunc(module_dict, "perf_counter", &Time_PerfCounter);
  InstallFunc(module_dict, "monotonic", &Time_Monotonic);
  InstallFunc(module_dict, "sleep", &Time_Sleep);

  return scope.Escape(module);
}

}  // namespace saauso::internal
