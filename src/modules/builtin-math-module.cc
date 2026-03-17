// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include <cinttypes>
#include <cmath>
#include <cstdint>
#include <limits>

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
#include "src/utils/maybe.h"

namespace saauso::internal {

namespace {

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
void FailArgc(const char* func_name, int64_t expected, int64_t actual) {
  Runtime_ThrowErrorf(ExceptionType::kTypeError,
                      "math.%s() takes exactly %" PRId64 " argument (%" PRId64
                      " given)",
                      func_name, expected, actual);
}

// 成功时返回有效 double；类型不符时抛 TypeError 并返回空。
Maybe<double> ExtractDouble(Handle<PyObject> value) {
  if (IsPyFloat(value)) {
    return Maybe<double>(Handle<PyFloat>::cast(value)->value());
  }
  if (IsPySmi(value)) {
    return Maybe<double>(PySmi::ToInt(Handle<PySmi>::cast(value)));
  }
  Handle<PyString> type_name = PyObject::GetKlass(value)->name();
  Runtime_ThrowErrorf(ExceptionType::kTypeError,
                      "must be real number, not '%s'", type_name->buffer());
  return kNullMaybe;
}

// 将 double 转为 PyInt(Smi)；溢出时抛 RuntimeError 并返回空。
MaybeHandle<PyObject> ReturnPyIntFromDouble(double v, const char* func_name) {
  if (!std::isfinite(v)) {
    Runtime_ThrowErrorf(ExceptionType::kRuntimeError,
                        "cannot convert float %g to integer", v);
    return kNullMaybe;
  }
  if (v < static_cast<double>(PySmi::kSmiMinValue) ||
      v > static_cast<double>(PySmi::kSmiMaxValue)) {
    Runtime_ThrowErrorf(ExceptionType::kRuntimeError, "%s result too large",
                        func_name);
    return kNullMaybe;
  }
  return handle(PySmi::FromInt(static_cast<int64_t>(v)));
}

BUILTIN_MODULE_FUNC(Math_Sqrt) {
  if (!kwargs.is_null() && kwargs->occupied() != 0) {
    ThrowNoKeywordArgsError("math", "sqrt");
    return kNullMaybe;
  }
  int64_t argc = args.is_null() ? 0 : args->length();
  if (argc != 1) {
    FailArgc("sqrt", 1, argc);
    return kNullMaybe;
  }

  double x = 0;
  ASSIGN_RETURN_ON_EXCEPTION(isolate, x, ExtractDouble(args->Get(0)));

  if (x < 0) {
    Runtime_ThrowError(ExceptionType::kValueError, "math domain error");
    return kNullMaybe;
  }
  return PyFloat::NewInstance(std::sqrt(x));
}

BUILTIN_MODULE_FUNC(Math_Floor) {
  if (!kwargs.is_null() && kwargs->occupied() != 0) {
    ThrowNoKeywordArgsError("math", "floor");
    return kNullMaybe;
  }
  int64_t argc = args.is_null() ? 0 : args->length();
  if (argc != 1) {
    FailArgc("floor", 1, argc);
    return kNullMaybe;
  }

  double x = 0;
  ASSIGN_RETURN_ON_EXCEPTION(isolate, x, ExtractDouble(args->Get(0)));

  return ReturnPyIntFromDouble(std::floor(x), "floor");
}

BUILTIN_MODULE_FUNC(Math_Ceil) {
  if (!kwargs.is_null() && kwargs->occupied() != 0) {
    ThrowNoKeywordArgsError("math", "ceil");
    return kNullMaybe;
  }
  int64_t argc = args.is_null() ? 0 : args->length();
  if (argc != 1) {
    FailArgc("ceil", 1, argc);
    return kNullMaybe;
  }

  double x = 0;
  ASSIGN_RETURN_ON_EXCEPTION(isolate, x, ExtractDouble(args->Get(0)));

  return ReturnPyIntFromDouble(std::ceil(x), "ceil");
}

BUILTIN_MODULE_FUNC(Math_Fabs) {
  if (!kwargs.is_null() && kwargs->occupied() != 0) {
    ThrowNoKeywordArgsError("math", "fabs");
    return kNullMaybe;
  }
  int64_t argc = args.is_null() ? 0 : args->length();
  if (argc != 1) {
    FailArgc("fabs", 1, argc);
    return kNullMaybe;
  }

  double x = 0;
  ASSIGN_RETURN_ON_EXCEPTION(isolate, x, ExtractDouble(args->Get(0)));

  return PyFloat::NewInstance(std::fabs(x));
}

BUILTIN_MODULE_FUNC(Math_Sin) {
  if (!kwargs.is_null() && kwargs->occupied() != 0) {
    ThrowNoKeywordArgsError("math", "sin");
    return kNullMaybe;
  }
  int64_t argc = args.is_null() ? 0 : args->length();
  if (argc != 1) {
    FailArgc("sin", 1, argc);
    return kNullMaybe;
  }

  double x = 0;
  ASSIGN_RETURN_ON_EXCEPTION(isolate, x, ExtractDouble(args->Get(0)));

  return PyFloat::NewInstance(std::sin(x));
}

BUILTIN_MODULE_FUNC(Math_Cos) {
  if (!kwargs.is_null() && kwargs->occupied() != 0) {
    ThrowNoKeywordArgsError("math", "cos");
    return kNullMaybe;
  }
  int64_t argc = args.is_null() ? 0 : args->length();
  if (argc != 1) {
    FailArgc("cos", 1, argc);
    return kNullMaybe;
  }

  double x = 0;
  ASSIGN_RETURN_ON_EXCEPTION(isolate, x, ExtractDouble(args->Get(0)));

  return PyFloat::NewInstance(std::cos(x));
}

BUILTIN_MODULE_FUNC(Math_Tan) {
  if (!kwargs.is_null() && kwargs->occupied() != 0) {
    ThrowNoKeywordArgsError("math", "tan");
    return kNullMaybe;
  }
  int64_t argc = args.is_null() ? 0 : args->length();
  if (argc != 1) {
    FailArgc("tan", 1, argc);
    return kNullMaybe;
  }

  double x = 0;
  ASSIGN_RETURN_ON_EXCEPTION(isolate, x, ExtractDouble(args->Get(0)));

  return PyFloat::NewInstance(std::tan(x));
}

BUILTIN_MODULE_FUNC(Math_Exp) {
  if (!kwargs.is_null() && kwargs->occupied() != 0) {
    ThrowNoKeywordArgsError("math", "exp");
    return kNullMaybe;
  }
  int64_t argc = args.is_null() ? 0 : args->length();
  if (argc != 1) {
    FailArgc("exp", 1, argc);
    return kNullMaybe;
  }

  double x = 0;
  ASSIGN_RETURN_ON_EXCEPTION(isolate, x, ExtractDouble(args->Get(0)));

  return PyFloat::NewInstance(std::exp(x));
}

BUILTIN_MODULE_FUNC(Math_Log) {
  if (!kwargs.is_null() && kwargs->occupied() != 0) {
    ThrowNoKeywordArgsError("math", "log");
    return kNullMaybe;
  }
  int64_t argc = args.is_null() ? 0 : args->length();
  if (argc != 1) {
    FailArgc("log", 1, argc);
    return kNullMaybe;
  }

  double x = 0;
  ASSIGN_RETURN_ON_EXCEPTION(isolate, x, ExtractDouble(args->Get(0)));

  if (x <= 0) {
    Runtime_ThrowError(ExceptionType::kValueError, "math domain error");
    return kNullMaybe;
  }
  return PyFloat::NewInstance(std::log(x));
}

BUILTIN_MODULE_FUNC(Math_Log2) {
  if (!kwargs.is_null() && kwargs->occupied() != 0) {
    ThrowNoKeywordArgsError("math", "log2");
    return kNullMaybe;
  }
  int64_t argc = args.is_null() ? 0 : args->length();
  if (argc != 1) {
    FailArgc("log2", 1, argc);
    return kNullMaybe;
  }

  double x = 0;
  ASSIGN_RETURN_ON_EXCEPTION(isolate, x, ExtractDouble(args->Get(0)));

  if (x <= 0) {
    Runtime_ThrowError(ExceptionType::kValueError, "math domain error");
    return kNullMaybe;
  }
  return PyFloat::NewInstance(std::log2(x));
}

BUILTIN_MODULE_FUNC(Math_Log10) {
  if (!kwargs.is_null() && kwargs->occupied() != 0) {
    ThrowNoKeywordArgsError("math", "log10");
    return kNullMaybe;
  }
  int64_t argc = args.is_null() ? 0 : args->length();
  if (argc != 1) {
    FailArgc("log10", 1, argc);
    return kNullMaybe;
  }

  double x = 0;
  ASSIGN_RETURN_ON_EXCEPTION(isolate, x, ExtractDouble(args->Get(0)));

  if (x <= 0) {
    Runtime_ThrowError(ExceptionType::kValueError, "math domain error");
    return kNullMaybe;
  }
  return PyFloat::NewInstance(std::log10(x));
}

BUILTIN_MODULE_FUNC(Math_Pow) {
  if (!kwargs.is_null() && kwargs->occupied() != 0) {
    ThrowNoKeywordArgsError("math", "pow");
    return kNullMaybe;
  }
  int64_t argc = args.is_null() ? 0 : args->length();
  if (argc != 2) {
    Runtime_ThrowErrorf(
        ExceptionType::kTypeError,
        "math.pow() takes exactly 2 arguments (%" PRId64 " given)", argc);
    return kNullMaybe;
  }

  double x = 0;
  ASSIGN_RETURN_ON_EXCEPTION(isolate, x, ExtractDouble(args->Get(0)));

  double y = 0;
  ASSIGN_RETURN_ON_EXCEPTION(isolate, y, ExtractDouble(args->Get(1)));

  return PyFloat::NewInstance(std::pow(x, y));
}

BUILTIN_MODULE_FUNC(Math_IsFinite) {
  if (!kwargs.is_null() && kwargs->occupied() != 0) {
    ThrowNoKeywordArgsError("math", "isfinite");
    return kNullMaybe;
  }
  int64_t argc = args.is_null() ? 0 : args->length();
  if (argc != 1) {
    FailArgc("isfinite", 1, argc);
    return kNullMaybe;
  }

  double x = 0;
  ASSIGN_RETURN_ON_EXCEPTION(isolate, x, ExtractDouble(args->Get(0)));

  return handle(Isolate::ToPyBoolean(std::isfinite(x)));
}

BUILTIN_MODULE_FUNC(Math_IsNaN) {
  if (!kwargs.is_null() && kwargs->occupied() != 0) {
    ThrowNoKeywordArgsError("math", "isnan");
    return kNullMaybe;
  }
  int64_t argc = args.is_null() ? 0 : args->length();
  if (argc != 1) {
    FailArgc("isnan", 1, argc);
    return kNullMaybe;
  }

  double x = 0;
  ASSIGN_RETURN_ON_EXCEPTION(isolate, x, ExtractDouble(args->Get(0)));

  return handle(Isolate::ToPyBoolean(std::isnan(x)));
}

BUILTIN_MODULE_FUNC(Math_IsInf) {
  if (!kwargs.is_null() && kwargs->occupied() != 0) {
    ThrowNoKeywordArgsError("math", "isinf");
    return kNullMaybe;
  }
  int64_t argc = args.is_null() ? 0 : args->length();
  if (argc != 1) {
    FailArgc("isinf", 1, argc);
    return kNullMaybe;
  }

  double x = 0;
  ASSIGN_RETURN_ON_EXCEPTION(isolate, x, ExtractDouble(args->Get(0)));

  return handle(Isolate::ToPyBoolean(std::isinf(x)));
}

}  // namespace

////////////////////////////////////////////////////////////////////////////

BUILTIN_MODULE_INIT_FUNC("math", InitMathModule) {
  EscapableHandleScope scope;

  Handle<PyModule> module;
  ASSIGN_RETURN_ON_EXCEPTION(isolate, module,
                             NewBuiltinModuleWithDefaultMeta(isolate, "math"));

  Handle<PyDict> module_dict = PyObject::GetProperties(module);
  RETURN_ON_EXCEPTION(isolate,
                      PyDict::Put(module_dict, PyString::NewInstance("pi"),
                                  PyFloat::NewInstance(std::acos(-1.0))));
  RETURN_ON_EXCEPTION(isolate,
                      PyDict::Put(module_dict, PyString::NewInstance("e"),
                                  PyFloat::NewInstance(std::exp(1.0))));
  RETURN_ON_EXCEPTION(isolate,
                      PyDict::Put(module_dict, PyString::NewInstance("tau"),
                                  PyFloat::NewInstance(2.0 * std::acos(-1.0))));
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
  {name, &BUILTIN_MODULE_FUNC_NAME(func)},
      MATH_MODULE_FUNC_LIST(DEFINE_MATH_FUNC_SPEC)
#undef DEFINE_MATH_FUNC_SPEC
  };
  RETURN_ON_EXCEPTION(isolate,
                      InstallBuiltinModuleFuncsFromSpec(
                          isolate, module_dict, kMathModuleFuncs,
                          BuiltinModuleFuncSpecCount(kMathModuleFuncs)));

  return scope.Escape(module);
}

}  // namespace saauso::internal
