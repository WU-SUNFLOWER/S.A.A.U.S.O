// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include <cinttypes>

#include "src/builtins/builtins-utils.h"
#include "src/execution/exception-utils.h"
#include "src/execution/execution.h"
#include "src/execution/isolate.h"
#include "src/handles/maybe-handles.h"
#include "src/objects/py-code-object.h"
#include "src/objects/py-dict.h"
#include "src/objects/py-oddballs.h"
#include "src/objects/py-string.h"
#include "src/objects/py-tuple.h"
#include "src/objects/py-type-object.h"
#include "src/runtime/runtime-exceptions.h"
#include "src/runtime/runtime-exec.h"

namespace saauso::internal {

namespace {

// 将 obj 解析为 dict。失败时抛出 TypeError 并返回 null。
MaybeHandle<PyDict> CastToDictOrThrowTypeError(Handle<PyObject> obj,
                                               const char* role_name) {
  if (IsPyDict(*obj)) {
    return MaybeHandle<PyDict>(Handle<PyDict>::cast(obj));
  }

  Handle<PyString> type_name = PyObject::GetKlass(obj)->name();
  Runtime_ThrowErrorf(ExceptionType::kTypeError,
                      "exec() %s must be a dict, not %s", role_name,
                      type_name->buffer());
  return kNullMaybeHandle;
}

// 校验 kwargs 中只允许出现 globals/locals 两个关键字。
// 成功时返回 Python None；失败时抛异常并返回空 MaybeHandle。
// 注意：该函数不能创建独立的 HandleScope，否则会把短生命周期的 handle
// 通过引用返回给调用方，导致悬垂句柄。
MaybeHandle<PyObject> ValidateExecKeywordArguments(
    Isolate* isolate,
    Handle<PyDict> kwargs,
    Handle<PyObject> globals_key,
    Handle<PyObject> locals_key) {
  for (auto i = 0; i < kwargs->capacity(); ++i) {
    auto item = kwargs->ItemAtIndex(i);
    if (item.is_null()) {
      continue;
    }

    auto key = item->Get(0);
    if (!IsPyString(key)) {
      Runtime_ThrowError(ExceptionType::kTypeError, "keywords must be strings");
      return kNullMaybeHandle;
    }

    bool eq = false;
    RETURN_ON_EXCEPTION(isolate, PyObject::EqualBool(key, globals_key));
    if (eq) {
      continue;
    }

    RETURN_ON_EXCEPTION(isolate, PyObject::EqualBool(key, locals_key));
    if (eq) {
      continue;
    }

    auto key_str = Handle<PyString>::cast(key);
    Runtime_ThrowErrorf(ExceptionType::kTypeError,
                        "exec() got an unexpected keyword argument '%s'",
                        key_str->buffer());
    return kNullMaybeHandle;
  }

  return handle(isolate->py_none_object());
}

// 将 kwargs 中的 globals/locals 合并到对应的引用参数中。
// 成功时返回 Python None；失败时抛异常并返回空 MaybeHandle。
// 注意：该函数不能创建独立的 HandleScope，否则会把短生命周期的 handle
// 通过引用返回给调用方，导致悬垂句柄。
MaybeHandle<PyObject> ApplyExecKeywordArgumentOverrides(
    Isolate* isolate,
    Handle<PyDict> kwargs,
    Handle<PyObject> globals_key,
    Handle<PyObject> locals_key,
    Handle<PyObject>& globals,
    Handle<PyObject>& locals,
    bool globals_from_positional,
    bool locals_from_positional) {
  Tagged<PyObject> globals_from_kwargs;

  bool found = false;
  ASSIGN_RETURN_ON_EXCEPTION(
      isolate, found, kwargs->GetTagged(globals_key, globals_from_kwargs));

  if (found) {
    if (globals_from_positional) {
      Runtime_ThrowError(ExceptionType::kTypeError,
                         "exec() got multiple values for argument 'globals'");
      return kNullMaybeHandle;
    }
    globals = handle(globals_from_kwargs);
  }

  Handle<PyObject> locals_from_kwargs;
  ASSIGN_RETURN_ON_EXCEPTION(isolate, found,
                             kwargs->Get(locals_key, locals_from_kwargs));

  if (found) {
    if (locals_from_positional) {
      Runtime_ThrowError(ExceptionType::kTypeError,
                         "exec() got multiple values for argument 'locals'");
      return kNullMaybeHandle;
    }
    locals = locals_from_kwargs;
  }

  return handle(isolate->py_none_object());
}

// 这里的作用是：
// 1) 验证 kwargs 中仅包含 globals/locals 两个关键字；
// 2) 将关键字参数合并进对应的引用参数中，并处理“与位置参数重复赋值”的错误。
// 成功时返回 Python None；失败时抛异常并返回空 MaybeHandle。
// 注意：该函数不能创建独立的 HandleScope，否则会把短生命周期的 handle
// 通过引用返回给调用方，导致悬垂句柄。
MaybeHandle<PyObject> NormalizeExecArgs(Isolate* isolate,
                                        Handle<PyDict> kwargs,
                                        Handle<PyObject>& globals,
                                        Handle<PyObject>& locals,
                                        bool globals_from_positional,
                                        bool locals_from_positional) {
  if (kwargs.is_null()) {
    return handle(isolate->py_none_object());
  }

  Handle<PyObject> globals_key = PyString::NewInstance("globals");
  Handle<PyObject> locals_key = PyString::NewInstance("locals");
  RETURN_ON_EXCEPTION_VALUE(
      isolate,
      ValidateExecKeywordArguments(isolate, kwargs, globals_key, locals_key),
      kNullMaybeHandle);
  RETURN_ON_EXCEPTION_VALUE(
      isolate,
      ApplyExecKeywordArgumentOverrides(
          isolate, kwargs, globals_key, locals_key, globals, locals,
          globals_from_positional, locals_from_positional),
      kNullMaybeHandle);
  return handle(isolate->py_none_object());
}

}  // namespace

BUILTIN(Exec) {
  EscapableHandleScope scope;
  Isolate* isolate = Isolate::Current();

  // 位置参数：exec(obj) / exec(obj, globals) / exec(obj, globals, locals)。
  const int64_t argc = args.is_null() ? 0 : args->length();
  if (argc < 1 || argc > 3) [[unlikely]] {
    if (argc < 1) {
      Runtime_ThrowErrorf(
          ExceptionType::kTypeError,
          "exec() takes at least 1 positional argument (%" PRId64 " given)",
          argc);
    } else {
      Runtime_ThrowErrorf(
          ExceptionType::kTypeError,
          "exec() takes at most 3 positional arguments (%" PRId64 " given)",
          argc);
    }
    return kNullMaybeHandle;
  }

  Handle<PyObject> source_or_code = args->Get(0);

  // 先按位置参数提取 globals/locals，再用 kwargs 进行覆盖与校验。
  bool globals_from_positional = false;
  bool locals_from_positional = false;
  Handle<PyObject> globals_obj = Handle<PyObject>::null();
  Handle<PyObject> locals_obj = Handle<PyObject>::null();
  if (argc >= 2) {
    globals_obj = args->Get(1);
    globals_from_positional = true;
  }
  if (argc == 3) {
    locals_obj = args->Get(2);
    locals_from_positional = true;
  }

  RETURN_ON_EXCEPTION_VALUE(
      isolate,
      NormalizeExecArgs(isolate, kwargs, globals_obj, locals_obj,
                        globals_from_positional, locals_from_positional),
      kNullMaybeHandle);
  const bool globals_explicit =
      !globals_obj.is_null() && !IsPyNone(*globals_obj);

  // 解析 globals：省略或传 None 时使用当前帧 globals；否则必须为 dict。
  Handle<PyDict> globals_dict;
  if (globals_obj.is_null() || IsPyNone(*globals_obj)) {
    globals_dict = Execution::CurrentFrameGlobals(isolate);
  } else {
    auto maybe_globals = CastToDictOrThrowTypeError(globals_obj, "globals");
    if (maybe_globals.is_null()) {
      return kNullMaybeHandle;
    }
    globals_dict = maybe_globals.ToHandleChecked();
  }

  // 解析 locals：
  // - locals 省略/None 时：如果 globals 是显式传入的，则 locals=globals（对齐
  // CPython）。
  // - 否则尝试使用当前帧 locals；若当前帧没有 locals 字典，则回退为 globals。
  // - locals 显式传入时必须为 dict。
  Handle<PyDict> locals_dict;
  if (locals_obj.is_null() || IsPyNone(*locals_obj)) {
    if (globals_from_positional || globals_explicit) {
      locals_dict = globals_dict;
    } else {
      locals_dict = Execution::CurrentFrameLocals(isolate);
      if (locals_dict.is_null()) {
        locals_dict = globals_dict;
      }
    }
  } else {
    auto maybe_locals = CastToDictOrThrowTypeError(locals_obj, "locals");
    if (maybe_locals.is_null()) {
      return kNullMaybeHandle;
    }
    locals_dict = maybe_locals.ToHandleChecked();
  }

  // 根据第一个参数类型分发：
  // - str：走源码编译并执行；
  // - code object：直接执行；
  // - 其它类型：报错。
  if (IsPyString(*source_or_code)) {
#if SAAUSO_ENABLE_CPYTHON_COMPILER
    if (Runtime_ExecutePythonSourceCode(Handle<PyString>::cast(source_or_code),
                                        locals_dict, globals_dict)
            .IsEmpty()) {
      return kNullMaybeHandle;
    }
#else
    Runtime_ThrowError(
        ExceptionType::kRuntimeError,
        "exec(str) requires embedded CPython compiler; build with "
        "saauso_enable_cpython_compiler=true");
    return kNullMaybeHandle;
#endif  // SAAUSO_ENABLE_CPYTHON_COMPILER
  } else if (IsPyCodeObject(*source_or_code)) {
    if (Runtime_ExecutePyCodeObject(Handle<PyCodeObject>::cast(source_or_code),
                                    locals_dict, globals_dict)
            .IsEmpty()) {
      return kNullMaybeHandle;
    }
  } else {
    Runtime_ThrowError(ExceptionType::kTypeError,
                       "exec() arg 1 must be a string or code object");
    return kNullMaybeHandle;
  }

  if (isolate->HasPendingException()) {
    return kNullMaybeHandle;
  }

  // 对齐 CPython：exec 总是返回 None。
  return scope.Escape(handle(isolate->py_none_object()));
}

}  // namespace saauso::internal
