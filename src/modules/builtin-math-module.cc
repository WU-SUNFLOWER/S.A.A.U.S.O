// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include <cinttypes>
#include <cmath>
#include <cstdint>
#include <limits>
#include <numbers>

#include "include/saauso-maybe.h"
#include "src/execution/exception-types.h"
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

constexpr const char* kModuleName = "math";

constexpr double kPI = 3.1415926535897932;
constexpr double kE = std::numbers::e;
constexpr double kTau = 2 * kPI;

#define MATH_MODULE_FUNC_LIST(V) \
  V("sqrt", Math_Sqrt)           \
  V("floor", Math_Floor)         \
  V("ceil", Math_Ceil)           \
  V("fabs", Math_Fabs)           \
  V("sin", Math_Sin)             \
  V("cos", Math_Cos)             \
  V("tan", Math_Tan)             \
  V("exp", Math_Exp)             \
  V("log", Math_Log)             \
  V("log2", Math_Log2)           \
  V("log10", Math_Log10)         \
  V("pow", Math_Pow)             \
  V("isfinite", Math_IsFinite)   \
  V("isnan", Math_IsNaN)         \
  V("isinf", Math_IsInf)

// 参数个数不符时抛 TypeError。
void FailArgc(Isolate* isolate,
              const char* func_name,
              int64_t expected,
              int64_t actual) {
  Runtime_ThrowErrorf(isolate, ExceptionType::kTypeError,
                      "%s.%s() takes exactly %" PRId64 " argument (%" PRId64
                      " given)",
                      kModuleName, func_name, expected, actual);
}

// 成功时返回有效 double；类型不符时抛 TypeError 并返回空。
Maybe<double> ExtractDouble(Isolate* isolate, Handle<PyObject> value) {
  if (IsPyFloat(value)) {
    return Maybe<double>(Handle<PyFloat>::cast(value)->value());
  }
  if (IsPySmi(value)) {
    return Maybe<double>(PySmi::ToInt(Handle<PySmi>::cast(value)));
  }
  Handle<PyString> type_name = PyObject::GetKlass(value)->name();
  Runtime_ThrowErrorf(isolate, ExceptionType::kTypeError,
                      "must be real number, not '%s'", type_name->buffer());
  return kNullMaybe;
}

// 将 double 转为 PyInt(Smi)；溢出时抛 RuntimeError 并返回空。
MaybeHandle<PyObject> ReturnPyIntFromDouble(Isolate* isolate,
                                            double v,
                                            const char* func_name) {
  if (!std::isfinite(v)) {
    Runtime_ThrowErrorf(isolate, ExceptionType::kRuntimeError,
                        "cannot convert float %g to integer", v);
    return kNullMaybe;
  }
  if (v < static_cast<double>(PySmi::kSmiMinValue) ||
      v > static_cast<double>(PySmi::kSmiMaxValue)) {
    Runtime_ThrowErrorf(isolate, ExceptionType::kRuntimeError,
                        "%s result too large", func_name);
    return kNullMaybe;
  }
  return handle(PySmi::FromInt(static_cast<int64_t>(v)));
}

}  // namespace

/////////////////////////////////////////////////////////////////////////////////

namespace module_impl {

BUILTIN_MODULE_FUNC(Math_Sqrt) {
  BUILTIN_MODULE_EXPECT_NO_KWARGS_OR_RETURN(isolate, kwargs, kModuleName,
                                            "sqrt");
  int64_t argc = BUILTIN_MODULE_ARGC(args);
  BUILTIN_MODULE_EXPECT_ARGC_EQ_OR_RETURN(argc, 1,
                                          FailArgc(isolate, "sqrt", 1, argc));

  double x = 0;
  ASSIGN_RETURN_ON_EXCEPTION(isolate, x, ExtractDouble(isolate, args->Get(0)));

  if (x < 0) {
    Runtime_ThrowError(isolate, ExceptionType::kValueError,
                       "math domain error");
    return kNullMaybe;
  }
  return PyFloat::NewInstance(std::sqrt(x));
}

BUILTIN_MODULE_FUNC(Math_Floor) {
  BUILTIN_MODULE_EXPECT_NO_KWARGS_OR_RETURN(isolate, kwargs, kModuleName,
                                            "floor");
  int64_t argc = BUILTIN_MODULE_ARGC(args);
  BUILTIN_MODULE_EXPECT_ARGC_EQ_OR_RETURN(argc, 1,
                                          FailArgc(isolate, "floor", 1, argc));

  double x = 0;
  ASSIGN_RETURN_ON_EXCEPTION(isolate, x, ExtractDouble(isolate, args->Get(0)));

  return ReturnPyIntFromDouble(isolate, std::floor(x), "floor");
}

BUILTIN_MODULE_FUNC(Math_Ceil) {
  BUILTIN_MODULE_EXPECT_NO_KWARGS_OR_RETURN(isolate, kwargs, kModuleName,
                                            "ceil");
  int64_t argc = BUILTIN_MODULE_ARGC(args);
  BUILTIN_MODULE_EXPECT_ARGC_EQ_OR_RETURN(argc, 1,
                                          FailArgc(isolate, "ceil", 1, argc));

  double x = 0;
  ASSIGN_RETURN_ON_EXCEPTION(isolate, x, ExtractDouble(isolate, args->Get(0)));

  return ReturnPyIntFromDouble(isolate, std::ceil(x), "ceil");
}

BUILTIN_MODULE_FUNC(Math_Fabs) {
  BUILTIN_MODULE_EXPECT_NO_KWARGS_OR_RETURN(isolate, kwargs, kModuleName,
                                            "fabs");
  int64_t argc = BUILTIN_MODULE_ARGC(args);
  BUILTIN_MODULE_EXPECT_ARGC_EQ_OR_RETURN(argc, 1,
                                          FailArgc(isolate, "fabs", 1, argc));

  double x = 0;
  ASSIGN_RETURN_ON_EXCEPTION(isolate, x, ExtractDouble(isolate, args->Get(0)));

  return PyFloat::NewInstance(std::fabs(x));
}

BUILTIN_MODULE_FUNC(Math_Sin) {
  BUILTIN_MODULE_EXPECT_NO_KWARGS_OR_RETURN(isolate, kwargs, kModuleName,
                                            "sin");
  int64_t argc = BUILTIN_MODULE_ARGC(args);
  BUILTIN_MODULE_EXPECT_ARGC_EQ_OR_RETURN(argc, 1,
                                          FailArgc(isolate, "sin", 1, argc));

  double x = 0;
  ASSIGN_RETURN_ON_EXCEPTION(isolate, x, ExtractDouble(isolate, args->Get(0)));

  return PyFloat::NewInstance(std::sin(x));
}

BUILTIN_MODULE_FUNC(Math_Cos) {
  BUILTIN_MODULE_EXPECT_NO_KWARGS_OR_RETURN(isolate, kwargs, kModuleName,
                                            "cos");
  int64_t argc = BUILTIN_MODULE_ARGC(args);
  BUILTIN_MODULE_EXPECT_ARGC_EQ_OR_RETURN(argc, 1,
                                          FailArgc(isolate, "cos", 1, argc));

  double x = 0;
  ASSIGN_RETURN_ON_EXCEPTION(isolate, x, ExtractDouble(isolate, args->Get(0)));

  return PyFloat::NewInstance(std::cos(x));
}

BUILTIN_MODULE_FUNC(Math_Tan) {
  BUILTIN_MODULE_EXPECT_NO_KWARGS_OR_RETURN(isolate, kwargs, kModuleName,
                                            "tan");
  int64_t argc = BUILTIN_MODULE_ARGC(args);
  BUILTIN_MODULE_EXPECT_ARGC_EQ_OR_RETURN(argc, 1,
                                          FailArgc(isolate, "tan", 1, argc));

  double x = 0;
  ASSIGN_RETURN_ON_EXCEPTION(isolate, x, ExtractDouble(isolate, args->Get(0)));

  return PyFloat::NewInstance(std::tan(x));
}

BUILTIN_MODULE_FUNC(Math_Exp) {
  BUILTIN_MODULE_EXPECT_NO_KWARGS_OR_RETURN(isolate, kwargs, kModuleName,
                                            "exp");
  int64_t argc = BUILTIN_MODULE_ARGC(args);
  BUILTIN_MODULE_EXPECT_ARGC_EQ_OR_RETURN(argc, 1,
                                          FailArgc(isolate, "exp", 1, argc));

  double x = 0;
  ASSIGN_RETURN_ON_EXCEPTION(isolate, x, ExtractDouble(isolate, args->Get(0)));

  return PyFloat::NewInstance(std::exp(x));
}

BUILTIN_MODULE_FUNC(Math_Log) {
  BUILTIN_MODULE_EXPECT_NO_KWARGS_OR_RETURN(isolate, kwargs, kModuleName,
                                            "log");
  int64_t argc = BUILTIN_MODULE_ARGC(args);
  BUILTIN_MODULE_EXPECT_ARGC_EQ_OR_RETURN(argc, 1,
                                          FailArgc(isolate, "log", 1, argc));

  double x = 0;
  ASSIGN_RETURN_ON_EXCEPTION(isolate, x, ExtractDouble(isolate, args->Get(0)));

  if (x <= 0) {
    Runtime_ThrowError(isolate, ExceptionType::kValueError,
                       "math domain error");
    return kNullMaybe;
  }
  return PyFloat::NewInstance(std::log(x));
}

BUILTIN_MODULE_FUNC(Math_Log2) {
  BUILTIN_MODULE_EXPECT_NO_KWARGS_OR_RETURN(isolate, kwargs, kModuleName,
                                            "log2");
  int64_t argc = BUILTIN_MODULE_ARGC(args);
  BUILTIN_MODULE_EXPECT_ARGC_EQ_OR_RETURN(argc, 1,
                                          FailArgc(isolate, "log2", 1, argc));

  double x = 0;
  ASSIGN_RETURN_ON_EXCEPTION(isolate, x, ExtractDouble(isolate, args->Get(0)));

  if (x <= 0) {
    Runtime_ThrowError(isolate, ExceptionType::kValueError,
                       "math domain error");
    return kNullMaybe;
  }
  return PyFloat::NewInstance(std::log2(x));
}

BUILTIN_MODULE_FUNC(Math_Log10) {
  BUILTIN_MODULE_EXPECT_NO_KWARGS_OR_RETURN(isolate, kwargs, kModuleName,
                                            "log10");
  int64_t argc = BUILTIN_MODULE_ARGC(args);
  BUILTIN_MODULE_EXPECT_ARGC_EQ_OR_RETURN(argc, 1,
                                          FailArgc(isolate, "log10", 1, argc));

  double x = 0;
  ASSIGN_RETURN_ON_EXCEPTION(isolate, x, ExtractDouble(isolate, args->Get(0)));

  if (x <= 0) {
    Runtime_ThrowError(isolate, ExceptionType::kValueError,
                       "math domain error");
    return kNullMaybe;
  }
  return PyFloat::NewInstance(std::log10(x));
}

BUILTIN_MODULE_FUNC(Math_Pow) {
  BUILTIN_MODULE_EXPECT_NO_KWARGS_OR_RETURN(isolate, kwargs, kModuleName,
                                            "pow");
  int64_t argc = BUILTIN_MODULE_ARGC(args);
  BUILTIN_MODULE_EXPECT_ARGC_EQ_OR_RETURN(
      argc, 2,
      Runtime_ThrowErrorf(isolate, ExceptionType::kTypeError,
                          "%s.pow() takes exactly 2 arguments (%" PRId64
                          " given)",
                          kModuleName, argc));

  double x = 0;
  ASSIGN_RETURN_ON_EXCEPTION(isolate, x, ExtractDouble(isolate, args->Get(0)));

  double y = 0;
  ASSIGN_RETURN_ON_EXCEPTION(isolate, y, ExtractDouble(isolate, args->Get(1)));

  return PyFloat::NewInstance(std::pow(x, y));
}

BUILTIN_MODULE_FUNC(Math_IsFinite) {
  BUILTIN_MODULE_EXPECT_NO_KWARGS_OR_RETURN(isolate, kwargs, kModuleName,
                                            "isfinite");
  int64_t argc = BUILTIN_MODULE_ARGC(args);
  BUILTIN_MODULE_EXPECT_ARGC_EQ_OR_RETURN(
      argc, 1, FailArgc(isolate, "isfinite", 1, argc));

  double x = 0;
  ASSIGN_RETURN_ON_EXCEPTION(isolate, x, ExtractDouble(isolate, args->Get(0)));

  return handle(Isolate::ToPyBoolean(std::isfinite(x)));
}

BUILTIN_MODULE_FUNC(Math_IsNaN) {
  BUILTIN_MODULE_EXPECT_NO_KWARGS_OR_RETURN(isolate, kwargs, kModuleName,
                                            "isnan");
  int64_t argc = BUILTIN_MODULE_ARGC(args);
  BUILTIN_MODULE_EXPECT_ARGC_EQ_OR_RETURN(argc, 1,
                                          FailArgc(isolate, "isnan", 1, argc));

  double x = 0;
  ASSIGN_RETURN_ON_EXCEPTION(isolate, x, ExtractDouble(isolate, args->Get(0)));

  return handle(Isolate::ToPyBoolean(std::isnan(x)));
}

BUILTIN_MODULE_FUNC(Math_IsInf) {
  BUILTIN_MODULE_EXPECT_NO_KWARGS_OR_RETURN(isolate, kwargs, kModuleName,
                                            "isinf");
  int64_t argc = BUILTIN_MODULE_ARGC(args);
  BUILTIN_MODULE_EXPECT_ARGC_EQ_OR_RETURN(argc, 1,
                                          FailArgc(isolate, "isinf", 1, argc));

  double x = 0;
  ASSIGN_RETURN_ON_EXCEPTION(isolate, x, ExtractDouble(isolate, args->Get(0)));

  return handle(Isolate::ToPyBoolean(std::isinf(x)));
}

}  // namespace module_impl

/////////////////////////////////////////////////////////////////////////////////

BUILTIN_MODULE_INIT_FUNC("math", InitMathModule) {
  EscapableHandleScope scope;

  Handle<PyModule> module;
  ASSIGN_RETURN_ON_EXCEPTION(
      isolate, module,
      BuiltinModuleUtils::NewBuiltinModule(isolate, kModuleName));

  Handle<PyDict> module_dict = PyObject::GetProperties(module);
  RETURN_ON_EXCEPTION(isolate,
                      PyDict::Put(module_dict, PyString::NewInstance("pi"),
                                  PyFloat::NewInstance(kPI)));
  RETURN_ON_EXCEPTION(isolate,
                      PyDict::Put(module_dict, PyString::NewInstance("e"),
                                  PyFloat::NewInstance(kE)));
  RETURN_ON_EXCEPTION(isolate,
                      PyDict::Put(module_dict, PyString::NewInstance("tau"),
                                  PyFloat::NewInstance(kTau)));
  RETURN_ON_EXCEPTION(
      isolate, PyDict::Put(module_dict, PyString::NewInstance("inf"),
                           PyFloat::NewInstance(
                               std::numeric_limits<double>::infinity())));
  RETURN_ON_EXCEPTION(
      isolate, PyDict::Put(module_dict, PyString::NewInstance("nan"),
                           PyFloat::NewInstance(
                               std::numeric_limits<double>::quiet_NaN())));

  const BuiltinModuleFuncSpec kMathModuleFuncs[] = {
#define DEFINE_MATH_FUNC_SPEC(name, func) \
  {name, &module_impl::BUILTIN_MODULE_FUNC_NAME(func)},
      MATH_MODULE_FUNC_LIST(DEFINE_MATH_FUNC_SPEC)
#undef DEFINE_MATH_FUNC_SPEC
  };
  RETURN_ON_EXCEPTION(
      isolate,
      BuiltinModuleUtils::InstallBuiltinModuleFuncsFromSpec(
          isolate, module, kMathModuleFuncs,
          BuiltinModuleUtils::BuiltinModuleFuncSpecCount(kMathModuleFuncs)));

  return scope.Escape(module);
}

}  // namespace saauso::internal
