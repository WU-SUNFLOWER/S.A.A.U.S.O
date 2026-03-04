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
#include "src/objects/templates.h"
#include "src/runtime/runtime-exceptions.h"
#include "src/runtime/string-table.h"
#include "src/utils/maybe.h"

namespace saauso::internal {

namespace {

// 无关键字参数时抛 TypeError，调用方随后应 return kNullMaybe。
void FailNoKeywordArgs(const char* func_name) {
  Runtime_ThrowErrorf(ExceptionType::kTypeError,
                      "math.%s() takes no keyword arguments", func_name);
}

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

Maybe<void> InstallFunc(Isolate* isolate,
                        Handle<PyDict> module_dict,
                        const char* name,
                        NativeFuncPointer func) {
  Handle<PyString> py_name = PyString::NewInstance(name);

  FunctionTemplateInfo function_template(func, py_name);

  Handle<PyFunction> function_object;
  ASSIGN_RETURN_ON_EXCEPTION(
      isolate, function_object,
      isolate->factory()->NewPyFunctionWithTemplate(function_template));

  RETURN_ON_EXCEPTION(isolate,
                      PyDict::Put(module_dict, py_name, function_object));

  return JustVoid();
}

MaybeHandle<PyObject> Math_Sqrt(Handle<PyObject> host,
                                Handle<PyTuple> args,
                                Handle<PyDict> kwargs) {
  if (!kwargs.is_null() && kwargs->occupied() != 0) {
    FailNoKeywordArgs("sqrt");
    return kNullMaybe;
  }
  int64_t argc = args.is_null() ? 0 : args->length();
  if (argc != 1) {
    FailArgc("sqrt", 1, argc);
    return kNullMaybe;
  }

  double x = 0;
  ASSIGN_RETURN_ON_EXCEPTION(Isolate::Current(), x,
                             ExtractDouble(args->Get(0)));

  if (x < 0) {
    Runtime_ThrowError(ExceptionType::kValueError, "math domain error");
    return kNullMaybe;
  }
  return PyFloat::NewInstance(std::sqrt(x));
}

MaybeHandle<PyObject> Math_Floor(Handle<PyObject> host,
                                 Handle<PyTuple> args,
                                 Handle<PyDict> kwargs) {
  if (!kwargs.is_null() && kwargs->occupied() != 0) {
    FailNoKeywordArgs("floor");
    return kNullMaybe;
  }
  int64_t argc = args.is_null() ? 0 : args->length();
  if (argc != 1) {
    FailArgc("floor", 1, argc);
    return kNullMaybe;
  }

  double x = 0;
  ASSIGN_RETURN_ON_EXCEPTION(Isolate::Current(), x,
                             ExtractDouble(args->Get(0)));

  return ReturnPyIntFromDouble(std::floor(x), "floor");
}

MaybeHandle<PyObject> Math_Ceil(Handle<PyObject> host,
                                Handle<PyTuple> args,
                                Handle<PyDict> kwargs) {
  if (!kwargs.is_null() && kwargs->occupied() != 0) {
    FailNoKeywordArgs("ceil");
    return kNullMaybe;
  }
  int64_t argc = args.is_null() ? 0 : args->length();
  if (argc != 1) {
    FailArgc("ceil", 1, argc);
    return kNullMaybe;
  }

  double x = 0;
  ASSIGN_RETURN_ON_EXCEPTION(Isolate::Current(), x,
                             ExtractDouble(args->Get(0)));

  return ReturnPyIntFromDouble(std::ceil(x), "ceil");
}

MaybeHandle<PyObject> Math_Fabs(Handle<PyObject> host,
                                Handle<PyTuple> args,
                                Handle<PyDict> kwargs) {
  if (!kwargs.is_null() && kwargs->occupied() != 0) {
    FailNoKeywordArgs("fabs");
    return kNullMaybe;
  }
  int64_t argc = args.is_null() ? 0 : args->length();
  if (argc != 1) {
    FailArgc("fabs", 1, argc);
    return kNullMaybe;
  }

  double x = 0;
  ASSIGN_RETURN_ON_EXCEPTION(Isolate::Current(), x,
                             ExtractDouble(args->Get(0)));

  return PyFloat::NewInstance(std::fabs(x));
}

MaybeHandle<PyObject> Math_Sin(Handle<PyObject> host,
                               Handle<PyTuple> args,
                               Handle<PyDict> kwargs) {
  if (!kwargs.is_null() && kwargs->occupied() != 0) {
    FailNoKeywordArgs("sin");
    return kNullMaybe;
  }
  int64_t argc = args.is_null() ? 0 : args->length();
  if (argc != 1) {
    FailArgc("sin", 1, argc);
    return kNullMaybe;
  }

  double x = 0;
  ASSIGN_RETURN_ON_EXCEPTION(Isolate::Current(), x,
                             ExtractDouble(args->Get(0)));

  return PyFloat::NewInstance(std::sin(x));
}

MaybeHandle<PyObject> Math_Cos(Handle<PyObject> host,
                               Handle<PyTuple> args,
                               Handle<PyDict> kwargs) {
  if (!kwargs.is_null() && kwargs->occupied() != 0) {
    FailNoKeywordArgs("cos");
    return kNullMaybe;
  }
  int64_t argc = args.is_null() ? 0 : args->length();
  if (argc != 1) {
    FailArgc("cos", 1, argc);
    return kNullMaybe;
  }

  double x = 0;
  ASSIGN_RETURN_ON_EXCEPTION(Isolate::Current(), x,
                             ExtractDouble(args->Get(0)));

  return PyFloat::NewInstance(std::cos(x));
}

MaybeHandle<PyObject> Math_Tan(Handle<PyObject> host,
                               Handle<PyTuple> args,
                               Handle<PyDict> kwargs) {
  if (!kwargs.is_null() && kwargs->occupied() != 0) {
    FailNoKeywordArgs("tan");
    return kNullMaybe;
  }
  int64_t argc = args.is_null() ? 0 : args->length();
  if (argc != 1) {
    FailArgc("tan", 1, argc);
    return kNullMaybe;
  }

  double x = 0;
  ASSIGN_RETURN_ON_EXCEPTION(Isolate::Current(), x,
                             ExtractDouble(args->Get(0)));

  return PyFloat::NewInstance(std::tan(x));
}

MaybeHandle<PyObject> Math_Exp(Handle<PyObject> host,
                               Handle<PyTuple> args,
                               Handle<PyDict> kwargs) {
  if (!kwargs.is_null() && kwargs->occupied() != 0) {
    FailNoKeywordArgs("exp");
    return kNullMaybe;
  }
  int64_t argc = args.is_null() ? 0 : args->length();
  if (argc != 1) {
    FailArgc("exp", 1, argc);
    return kNullMaybe;
  }

  double x = 0;
  ASSIGN_RETURN_ON_EXCEPTION(Isolate::Current(), x,
                             ExtractDouble(args->Get(0)));

  return PyFloat::NewInstance(std::exp(x));
}

MaybeHandle<PyObject> Math_Log(Handle<PyObject> host,
                               Handle<PyTuple> args,
                               Handle<PyDict> kwargs) {
  if (!kwargs.is_null() && kwargs->occupied() != 0) {
    FailNoKeywordArgs("log");
    return kNullMaybe;
  }
  int64_t argc = args.is_null() ? 0 : args->length();
  if (argc != 1) {
    FailArgc("log", 1, argc);
    return kNullMaybe;
  }

  double x = 0;
  ASSIGN_RETURN_ON_EXCEPTION(Isolate::Current(), x,
                             ExtractDouble(args->Get(0)));

  if (x <= 0) {
    Runtime_ThrowError(ExceptionType::kValueError, "math domain error");
    return kNullMaybe;
  }
  return PyFloat::NewInstance(std::log(x));
}

MaybeHandle<PyObject> Math_Log2(Handle<PyObject> host,
                                Handle<PyTuple> args,
                                Handle<PyDict> kwargs) {
  if (!kwargs.is_null() && kwargs->occupied() != 0) {
    FailNoKeywordArgs("log2");
    return kNullMaybe;
  }
  int64_t argc = args.is_null() ? 0 : args->length();
  if (argc != 1) {
    FailArgc("log2", 1, argc);
    return kNullMaybe;
  }

  double x = 0;
  ASSIGN_RETURN_ON_EXCEPTION(Isolate::Current(), x,
                             ExtractDouble(args->Get(0)));

  if (x <= 0) {
    Runtime_ThrowError(ExceptionType::kValueError, "math domain error");
    return kNullMaybe;
  }
  return PyFloat::NewInstance(std::log2(x));
}

MaybeHandle<PyObject> Math_Log10(Handle<PyObject> host,
                                 Handle<PyTuple> args,
                                 Handle<PyDict> kwargs) {
  if (!kwargs.is_null() && kwargs->occupied() != 0) {
    FailNoKeywordArgs("log10");
    return kNullMaybe;
  }
  int64_t argc = args.is_null() ? 0 : args->length();
  if (argc != 1) {
    FailArgc("log10", 1, argc);
    return kNullMaybe;
  }

  double x = 0;
  ASSIGN_RETURN_ON_EXCEPTION(Isolate::Current(), x,
                             ExtractDouble(args->Get(0)));

  if (x <= 0) {
    Runtime_ThrowError(ExceptionType::kValueError, "math domain error");
    return kNullMaybe;
  }
  return PyFloat::NewInstance(std::log10(x));
}

MaybeHandle<PyObject> Math_Pow(Handle<PyObject> host,
                               Handle<PyTuple> args,
                               Handle<PyDict> kwargs) {
  if (!kwargs.is_null() && kwargs->occupied() != 0) {
    FailNoKeywordArgs("pow");
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
  ASSIGN_RETURN_ON_EXCEPTION(Isolate::Current(), x,
                             ExtractDouble(args->Get(0)));

  double y = 0;
  ASSIGN_RETURN_ON_EXCEPTION(Isolate::Current(), x,
                             ExtractDouble(args->Get(1)));

  return PyFloat::NewInstance(std::pow(x, y));
}

MaybeHandle<PyObject> Math_IsFinite(Handle<PyObject> host,
                                    Handle<PyTuple> args,
                                    Handle<PyDict> kwargs) {
  if (!kwargs.is_null() && kwargs->occupied() != 0) {
    FailNoKeywordArgs("isfinite");
    return kNullMaybe;
  }
  int64_t argc = args.is_null() ? 0 : args->length();
  if (argc != 1) {
    FailArgc("isfinite", 1, argc);
    return kNullMaybe;
  }

  double x = 0;
  ASSIGN_RETURN_ON_EXCEPTION(Isolate::Current(), x,
                             ExtractDouble(args->Get(0)));

  return handle(Isolate::ToPyBoolean(std::isfinite(x)));
}

MaybeHandle<PyObject> Math_IsNaN(Handle<PyObject> host,
                                 Handle<PyTuple> args,
                                 Handle<PyDict> kwargs) {
  if (!kwargs.is_null() && kwargs->occupied() != 0) {
    FailNoKeywordArgs("isnan");
    return kNullMaybe;
  }
  int64_t argc = args.is_null() ? 0 : args->length();
  if (argc != 1) {
    FailArgc("isnan", 1, argc);
    return kNullMaybe;
  }

  double x = 0;
  ASSIGN_RETURN_ON_EXCEPTION(Isolate::Current(), x,
                             ExtractDouble(args->Get(0)));

  return handle(Isolate::ToPyBoolean(std::isnan(x)));
}

MaybeHandle<PyObject> Math_IsInf(Handle<PyObject> host,
                                 Handle<PyTuple> args,
                                 Handle<PyDict> kwargs) {
  if (!kwargs.is_null() && kwargs->occupied() != 0) {
    FailNoKeywordArgs("isinf");
    return kNullMaybe;
  }
  int64_t argc = args.is_null() ? 0 : args->length();
  if (argc != 1) {
    FailArgc("isinf", 1, argc);
    return kNullMaybe;
  }

  double x = 0;
  ASSIGN_RETURN_ON_EXCEPTION(Isolate::Current(), x,
                             ExtractDouble(args->Get(0)));

  return handle(Isolate::ToPyBoolean(std::isinf(x)));
}

}  // namespace

////////////////////////////////////////////////////////////////////////////

BUILTIN_MODULE_INIT_FUNC("math", InitMathModule) {
  EscapableHandleScope scope;

  Handle<PyModule> module;
  ASSIGN_RETURN_ON_EXCEPTION(isolate, module,
                             isolate->factory()->NewPyModule());

  Handle<PyDict> module_dict = PyObject::GetProperties(module);

  RETURN_ON_EXCEPTION(isolate, PyDict::Put(module_dict, ST(name),
                                           PyString::NewInstance("math")));
  RETURN_ON_EXCEPTION(isolate, PyDict::Put(module_dict, ST(package),
                                           PyString::NewInstance("")));
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

  RETURN_ON_EXCEPTION(isolate,
                      InstallFunc(isolate, module_dict, "sqrt", &Math_Sqrt));
  RETURN_ON_EXCEPTION(isolate,
                      InstallFunc(isolate, module_dict, "floor", &Math_Floor));
  RETURN_ON_EXCEPTION(isolate,
                      InstallFunc(isolate, module_dict, "ceil", &Math_Ceil));
  RETURN_ON_EXCEPTION(isolate,
                      InstallFunc(isolate, module_dict, "fabs", &Math_Fabs));
  RETURN_ON_EXCEPTION(isolate,
                      InstallFunc(isolate, module_dict, "sin", &Math_Sin));
  RETURN_ON_EXCEPTION(isolate,
                      InstallFunc(isolate, module_dict, "cos", &Math_Cos));
  RETURN_ON_EXCEPTION(isolate,
                      InstallFunc(isolate, module_dict, "tan", &Math_Tan));
  RETURN_ON_EXCEPTION(isolate,
                      InstallFunc(isolate, module_dict, "exp", &Math_Exp));
  RETURN_ON_EXCEPTION(isolate,
                      InstallFunc(isolate, module_dict, "log", &Math_Log));
  RETURN_ON_EXCEPTION(isolate,
                      InstallFunc(isolate, module_dict, "log2", &Math_Log2));
  RETURN_ON_EXCEPTION(isolate,
                      InstallFunc(isolate, module_dict, "log10", &Math_Log10));
  RETURN_ON_EXCEPTION(isolate,
                      InstallFunc(isolate, module_dict, "pow", &Math_Pow));
  RETURN_ON_EXCEPTION(
      isolate, InstallFunc(isolate, module_dict, "isfinite", &Math_IsFinite));
  RETURN_ON_EXCEPTION(isolate,
                      InstallFunc(isolate, module_dict, "isnan", &Math_IsNaN));
  RETURN_ON_EXCEPTION(isolate,
                      InstallFunc(isolate, module_dict, "isinf", &Math_IsInf));

  return scope.Escape(module);
}

}  // namespace saauso::internal
