// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <limits>

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
  std::fprintf(stderr, "TypeError: math.%s() takes no keyword arguments\n",
               func_name);
  std::exit(1);
}

void FailArgc(const char* func_name, int64_t expected, int64_t actual) {
  std::fprintf(
      stderr, "TypeError: math.%s() takes exactly %lld argument (%lld given)\n",
      func_name, static_cast<long long>(expected),
      static_cast<long long>(actual));
  std::exit(1);
}

double ExtractDouble(Handle<PyObject> value, const char* func_name) {
  if (IsPyFloat(value)) {
    return Handle<PyFloat>::cast(value)->value();
  }
  if (IsPySmi(value)) {
    return static_cast<double>(PySmi::ToInt(Handle<PySmi>::cast(value)));
  }

  auto type_name = PyObject::GetKlass(value)->name();
  std::fprintf(stderr, "TypeError: must be real number, not '%.*s'\n",
               static_cast<int>(type_name->length()), type_name->buffer());
  std::exit(1);
  return 0;
}

Handle<PyObject> ReturnPyIntFromDouble(double v, const char* func_name) {
  if (!std::isfinite(v)) {
    std::fprintf(stderr, "OverflowError: cannot convert float %g to integer\n",
                 v);
    std::exit(1);
  }

  if (v < static_cast<double>(PySmi::kSmiMinValue) ||
      v > static_cast<double>(PySmi::kSmiMaxValue)) {
    std::fprintf(stderr, "OverflowError: %s result too large\n", func_name);
    std::exit(1);
  }

  return handle(PySmi::FromInt(static_cast<int64_t>(v)));
}

void InstallFunc(Handle<PyDict> module_dict,
                 const char* name,
                 NativeFuncPointer func) {
  Handle<PyString> py_name = PyString::NewInstance(name);
  PyDict::Put(module_dict, py_name, PyFunction::NewInstance(func, py_name));
}

Handle<PyObject> Math_Sqrt(Handle<PyObject> host,
                           Handle<PyTuple> args,
                           Handle<PyDict> kwargs) {
  if (!kwargs.is_null() && kwargs->occupied() != 0) {
    FailNoKeywordArgs("sqrt");
  }

  int64_t argc = args.is_null() ? 0 : args->length();
  if (argc != 1) {
    FailArgc("sqrt", 1, argc);
  }

  double x = ExtractDouble(args->Get(0), "sqrt");
  if (x < 0) {
    std::fprintf(stderr, "ValueError: math domain error\n");
    std::exit(1);
  }
  return PyFloat::NewInstance(std::sqrt(x));
}

Handle<PyObject> Math_Floor(Handle<PyObject> host,
                            Handle<PyTuple> args,
                            Handle<PyDict> kwargs) {
  if (!kwargs.is_null() && kwargs->occupied() != 0) {
    FailNoKeywordArgs("floor");
  }

  int64_t argc = args.is_null() ? 0 : args->length();
  if (argc != 1) {
    FailArgc("floor", 1, argc);
  }

  double x = ExtractDouble(args->Get(0), "floor");
  return ReturnPyIntFromDouble(std::floor(x), "floor");
}

Handle<PyObject> Math_Ceil(Handle<PyObject> host,
                           Handle<PyTuple> args,
                           Handle<PyDict> kwargs) {
  if (!kwargs.is_null() && kwargs->occupied() != 0) {
    FailNoKeywordArgs("ceil");
  }

  int64_t argc = args.is_null() ? 0 : args->length();
  if (argc != 1) {
    FailArgc("ceil", 1, argc);
  }

  double x = ExtractDouble(args->Get(0), "ceil");
  return ReturnPyIntFromDouble(std::ceil(x), "ceil");
}

Handle<PyObject> Math_Fabs(Handle<PyObject> host,
                           Handle<PyTuple> args,
                           Handle<PyDict> kwargs) {
  if (!kwargs.is_null() && kwargs->occupied() != 0) {
    FailNoKeywordArgs("fabs");
  }

  int64_t argc = args.is_null() ? 0 : args->length();
  if (argc != 1) {
    FailArgc("fabs", 1, argc);
  }

  double x = ExtractDouble(args->Get(0), "fabs");
  return PyFloat::NewInstance(std::fabs(x));
}

Handle<PyObject> Math_Sin(Handle<PyObject> host,
                          Handle<PyTuple> args,
                          Handle<PyDict> kwargs) {
  if (!kwargs.is_null() && kwargs->occupied() != 0) {
    FailNoKeywordArgs("sin");
  }

  int64_t argc = args.is_null() ? 0 : args->length();
  if (argc != 1) {
    FailArgc("sin", 1, argc);
  }

  double x = ExtractDouble(args->Get(0), "sin");
  return PyFloat::NewInstance(std::sin(x));
}

Handle<PyObject> Math_Cos(Handle<PyObject> host,
                          Handle<PyTuple> args,
                          Handle<PyDict> kwargs) {
  if (!kwargs.is_null() && kwargs->occupied() != 0) {
    FailNoKeywordArgs("cos");
  }

  int64_t argc = args.is_null() ? 0 : args->length();
  if (argc != 1) {
    FailArgc("cos", 1, argc);
  }

  double x = ExtractDouble(args->Get(0), "cos");
  return PyFloat::NewInstance(std::cos(x));
}

Handle<PyObject> Math_Tan(Handle<PyObject> host,
                          Handle<PyTuple> args,
                          Handle<PyDict> kwargs) {
  if (!kwargs.is_null() && kwargs->occupied() != 0) {
    FailNoKeywordArgs("tan");
  }

  int64_t argc = args.is_null() ? 0 : args->length();
  if (argc != 1) {
    FailArgc("tan", 1, argc);
  }

  double x = ExtractDouble(args->Get(0), "tan");
  return PyFloat::NewInstance(std::tan(x));
}

Handle<PyObject> Math_Exp(Handle<PyObject> host,
                          Handle<PyTuple> args,
                          Handle<PyDict> kwargs) {
  if (!kwargs.is_null() && kwargs->occupied() != 0) {
    FailNoKeywordArgs("exp");
  }

  int64_t argc = args.is_null() ? 0 : args->length();
  if (argc != 1) {
    FailArgc("exp", 1, argc);
  }

  double x = ExtractDouble(args->Get(0), "exp");
  return PyFloat::NewInstance(std::exp(x));
}

Handle<PyObject> Math_Log(Handle<PyObject> host,
                          Handle<PyTuple> args,
                          Handle<PyDict> kwargs) {
  if (!kwargs.is_null() && kwargs->occupied() != 0) {
    FailNoKeywordArgs("log");
  }

  int64_t argc = args.is_null() ? 0 : args->length();
  if (argc != 1) {
    FailArgc("log", 1, argc);
  }

  double x = ExtractDouble(args->Get(0), "log");
  if (x <= 0) {
    std::fprintf(stderr, "ValueError: math domain error\n");
    std::exit(1);
  }
  return PyFloat::NewInstance(std::log(x));
}

Handle<PyObject> Math_Log2(Handle<PyObject> host,
                           Handle<PyTuple> args,
                           Handle<PyDict> kwargs) {
  if (!kwargs.is_null() && kwargs->occupied() != 0) {
    FailNoKeywordArgs("log2");
  }

  int64_t argc = args.is_null() ? 0 : args->length();
  if (argc != 1) {
    FailArgc("log2", 1, argc);
  }

  double x = ExtractDouble(args->Get(0), "log2");
  if (x <= 0) {
    std::fprintf(stderr, "ValueError: math domain error\n");
    std::exit(1);
  }
  return PyFloat::NewInstance(std::log2(x));
}

Handle<PyObject> Math_Log10(Handle<PyObject> host,
                            Handle<PyTuple> args,
                            Handle<PyDict> kwargs) {
  if (!kwargs.is_null() && kwargs->occupied() != 0) {
    FailNoKeywordArgs("log10");
  }

  int64_t argc = args.is_null() ? 0 : args->length();
  if (argc != 1) {
    FailArgc("log10", 1, argc);
  }

  double x = ExtractDouble(args->Get(0), "log10");
  if (x <= 0) {
    std::fprintf(stderr, "ValueError: math domain error\n");
    std::exit(1);
  }
  return PyFloat::NewInstance(std::log10(x));
}

Handle<PyObject> Math_Pow(Handle<PyObject> host,
                          Handle<PyTuple> args,
                          Handle<PyDict> kwargs) {
  if (!kwargs.is_null() && kwargs->occupied() != 0) {
    FailNoKeywordArgs("pow");
  }

  int64_t argc = args.is_null() ? 0 : args->length();
  if (argc != 2) {
    std::fprintf(stderr,
                 "TypeError: math.pow() takes exactly 2 arguments (%lld "
                 "given)\n",
                 static_cast<long long>(argc));
    std::exit(1);
  }

  double x = ExtractDouble(args->Get(0), "pow");
  double y = ExtractDouble(args->Get(1), "pow");
  return PyFloat::NewInstance(std::pow(x, y));
}

Handle<PyObject> Math_IsFinite(Handle<PyObject> host,
                               Handle<PyTuple> args,
                               Handle<PyDict> kwargs) {
  if (!kwargs.is_null() && kwargs->occupied() != 0) {
    FailNoKeywordArgs("isfinite");
  }

  int64_t argc = args.is_null() ? 0 : args->length();
  if (argc != 1) {
    FailArgc("isfinite", 1, argc);
  }

  double x = ExtractDouble(args->Get(0), "isfinite");
  return handle(Isolate::ToPyBoolean(std::isfinite(x)));
}

Handle<PyObject> Math_IsNaN(Handle<PyObject> host,
                            Handle<PyTuple> args,
                            Handle<PyDict> kwargs) {
  if (!kwargs.is_null() && kwargs->occupied() != 0) {
    FailNoKeywordArgs("isnan");
  }

  int64_t argc = args.is_null() ? 0 : args->length();
  if (argc != 1) {
    FailArgc("isnan", 1, argc);
  }

  double x = ExtractDouble(args->Get(0), "isnan");
  return handle(Isolate::ToPyBoolean(std::isnan(x)));
}

Handle<PyObject> Math_IsInf(Handle<PyObject> host,
                            Handle<PyTuple> args,
                            Handle<PyDict> kwargs) {
  if (!kwargs.is_null() && kwargs->occupied() != 0) {
    FailNoKeywordArgs("isinf");
  }

  int64_t argc = args.is_null() ? 0 : args->length();
  if (argc != 1) {
    FailArgc("isinf", 1, argc);
  }

  double x = ExtractDouble(args->Get(0), "isinf");
  return handle(Isolate::ToPyBoolean(std::isinf(x)));
}

}  // namespace

Handle<PyModule> InitMathModule(Isolate* isolate, ModuleManager* manager) {
  (void)isolate;
  (void)manager;

  EscapableHandleScope scope;
  Handle<PyModule> module = PyModule::NewInstance();
  Handle<PyDict> module_dict = PyObject::GetProperties(module);

  PyDict::Put(module_dict, ST(name), PyString::NewInstance("math"));
  PyDict::Put(module_dict, ST(package), PyString::NewInstance(""));

  PyDict::Put(module_dict, PyString::NewInstance("pi"),
              PyFloat::NewInstance(std::acos(-1.0)));
  PyDict::Put(module_dict, PyString::NewInstance("e"),
              PyFloat::NewInstance(std::exp(1.0)));
  PyDict::Put(module_dict, PyString::NewInstance("tau"),
              PyFloat::NewInstance(2.0 * std::acos(-1.0)));
  PyDict::Put(module_dict, PyString::NewInstance("inf"),
              PyFloat::NewInstance(std::numeric_limits<double>::infinity()));
  PyDict::Put(module_dict, PyString::NewInstance("nan"),
              PyFloat::NewInstance(std::numeric_limits<double>::quiet_NaN()));

  InstallFunc(module_dict, "sqrt", &Math_Sqrt);
  InstallFunc(module_dict, "floor", &Math_Floor);
  InstallFunc(module_dict, "ceil", &Math_Ceil);
  InstallFunc(module_dict, "fabs", &Math_Fabs);
  InstallFunc(module_dict, "sin", &Math_Sin);
  InstallFunc(module_dict, "cos", &Math_Cos);
  InstallFunc(module_dict, "tan", &Math_Tan);
  InstallFunc(module_dict, "exp", &Math_Exp);
  InstallFunc(module_dict, "log", &Math_Log);
  InstallFunc(module_dict, "log2", &Math_Log2);
  InstallFunc(module_dict, "log10", &Math_Log10);
  InstallFunc(module_dict, "pow", &Math_Pow);
  InstallFunc(module_dict, "isfinite", &Math_IsFinite);
  InstallFunc(module_dict, "isnan", &Math_IsNaN);
  InstallFunc(module_dict, "isinf", &Math_IsInf);

  return scope.Escape(module);
}

}  // namespace saauso::internal
