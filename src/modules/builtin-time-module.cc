// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <thread>

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
#include "src/runtime/string-table.h"

namespace saauso::internal {

namespace {

void FailNoKeywordArgs(const char* func_name) {
  std::fprintf(stderr, "TypeError: time.%s() takes no keyword arguments\n",
               func_name);
  std::exit(1);
}

double ExtractSeconds(Handle<PyObject> value, const char* func_name) {
  if (IsPyFloat(value)) {
    return Handle<PyFloat>::cast(value)->value();
  }
  if (IsPySmi(value)) {
    return static_cast<double>(PySmi::ToInt(Handle<PySmi>::cast(value)));
  }

  auto type_name = PyObject::GetKlass(value)->name();
  std::fprintf(
      stderr, "TypeError: %s() argument must be int or float, not '%.*s'\n",
      func_name, static_cast<int>(type_name->length()), type_name->buffer());
  std::exit(1);
  return 0;
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
  (void)host;
  if (!kwargs.is_null() && kwargs->occupied() != 0) {
    FailNoKeywordArgs("time");
  }

  int64_t argc = args.is_null() ? 0 : args->length();
  if (argc != 0) {
    std::fprintf(stderr,
                 "TypeError: time.time() takes 0 positional arguments but %lld "
                 "were given\n",
                 static_cast<long long>(argc));
    std::exit(1);
  }

  return PyFloat::NewInstance(WallTimeSeconds());
}

MaybeHandle<PyObject> Time_PerfCounter(Handle<PyObject> host,
                                       Handle<PyTuple> args,
                                       Handle<PyDict> kwargs) {
  (void)host;
  if (!kwargs.is_null() && kwargs->occupied() != 0) {
    FailNoKeywordArgs("perf_counter");
  }

  int64_t argc = args.is_null() ? 0 : args->length();
  if (argc != 0) {
    std::fprintf(
        stderr,
        "TypeError: time.perf_counter() takes 0 positional arguments but %lld "
        "were given\n",
        static_cast<long long>(argc));
    std::exit(1);
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
  (void)host;
  if (!kwargs.is_null() && kwargs->occupied() != 0) {
    FailNoKeywordArgs("sleep");
  }

  int64_t argc = args.is_null() ? 0 : args->length();
  if (argc != 1) {
    std::fprintf(stderr,
                 "TypeError: time.sleep() takes exactly 1 argument (%lld "
                 "given)\n",
                 static_cast<long long>(argc));
    std::exit(1);
  }

  double seconds = ExtractSeconds(args->Get(0), "sleep");
  if (seconds < 0) {
    std::fprintf(stderr, "ValueError: sleep length must be non-negative\n");
    std::exit(1);
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
