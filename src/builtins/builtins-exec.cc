// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include <cinttypes>

#include "src/builtins/builtins-utils.h"
#include "src/execution/isolate.h"
#include "src/interpreter/interpreter.h"
#include "src/objects/py-code-object.h"
#include "src/objects/py-dict.h"
#include "src/objects/py-oddballs.h"
#include "src/objects/py-string.h"
#include "src/objects/py-tuple.h"
#include "src/objects/py-type-object.h"
#include "src/runtime/runtime.h"

namespace saauso::internal {

namespace {
void NormalizeExecArgs(Handle<PyDict> kwargs,
                       Handle<PyObject>& globals,
                       Handle<PyObject>& locals,
                       bool globals_from_positional,
                       bool locals_from_positional) {
  if (kwargs.is_null()) {
    return;
  }

  // 这里的作用是：
  // 1) 验证 kwargs 中仅包含 globals/locals 两个关键字；
  // 2) 将关键字参数合并进对应的引用参数中，并处理“与位置参数重复赋值”的错误。
  // 注意：该函数不能创建独立的 HandleScope，否则会把短生命周期的 handle
  // 通过引用返回给调用方，导致悬垂句柄。
  HandleScope scope;

  Handle<PyObject> globals_key = PyString::NewInstance("globals");
  Handle<PyObject> locals_key = PyString::NewInstance("locals");
  for (auto i = 0; i < kwargs->capacity(); ++i) {
    auto item = kwargs->ItemAtIndex(i);
    if (item.is_null()) {
      continue;
    }

    auto key = item->Get(0);
    if (!IsPyString(key)) {
      std::fprintf(stderr, "TypeError: keywords must be strings\n");
      std::exit(1);
    }

    if (PyObject::EqualBool(key, globals_key) ||
        PyObject::EqualBool(key, locals_key)) {
      continue;
    }

    auto key_str = Handle<PyString>::cast(key);
    std::fprintf(
        stderr, "TypeError: exec() got an unexpected keyword argument '%.*s'\n",
        static_cast<int>(key_str->length()), key_str->buffer());
    std::exit(1);
  }

  Handle<PyObject> globals_from_kwargs = kwargs->Get(globals_key);
  if (!globals_from_kwargs.is_null()) {
    if (globals_from_positional) {
      std::fprintf(stderr,
                   "TypeError: exec() got multiple values for argument "
                   "'globals'\n");
      std::exit(1);
    }
    globals = globals_from_kwargs;
  }

  Handle<PyObject> locals_from_kwargs = kwargs->Get(locals_key);
  if (!locals_from_kwargs.is_null()) {
    if (locals_from_positional) {
      std::fprintf(stderr,
                   "TypeError: exec() got multiple values for argument "
                   "'locals'\n");
      std::exit(1);
    }
    locals = locals_from_kwargs;
  }
}
}  // namespace

BUILTIN(Exec) {
  EscapableHandleScope scope;

  // 位置参数：exec(obj) / exec(obj, globals) / exec(obj, globals, locals)。
  const int64_t argc = args.is_null() ? 0 : args->length();
  if (argc < 1 || argc > 3) [[unlikely]] {
    std::fprintf(
        stderr,
        "TypeError: exec() takes at most 3 positional arguments (%" PRId64
        " given)\n",
        argc);
    std::exit(1);
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

  NormalizeExecArgs(kwargs, globals_obj, locals_obj, globals_from_positional,
                    locals_from_positional);

  Interpreter* interpreter = Isolate::Current()->interpreter();
  const bool globals_explicit =
      !globals_obj.is_null() && !IsPyNone(*globals_obj);

  // 解析 globals：省略或传 None 时使用当前帧 globals；否则必须为 dict。
  Handle<PyDict> globals_dict;
  if (globals_obj.is_null() || IsPyNone(*globals_obj)) {
    globals_dict = interpreter->CurrentFrameGlobals();
  } else {
    if (!IsPyDict(*globals_obj)) {
      auto type_name = PyObject::GetKlass(globals_obj)->name();
      std::fprintf(stderr, "TypeError: exec() globals must be a dict, not %s\n",
                   type_name->buffer());
      std::exit(1);
    }
    globals_dict = Handle<PyDict>::cast(globals_obj);
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
      locals_dict = interpreter->CurrentFrameLocals();
      if (locals_dict.is_null()) {
        locals_dict = globals_dict;
      }
    }
  } else {
    if (!IsPyDict(*locals_obj)) {
      auto type_name = PyObject::GetKlass(locals_obj)->name();
      std::fprintf(stderr, "TypeError: exec() locals must be a dict, not %s\n",
                   type_name->buffer());
      std::exit(1);
    }
    locals_dict = Handle<PyDict>::cast(locals_obj);
  }

  // 根据第一个参数类型分发：
  // - str：走源码编译并执行；
  // - code object：直接执行；
  // - 其它类型：报错。
  if (IsPyString(*source_or_code)) {
#if SAAUSO_ENABLE_CPYTHON_COMPILER
    Runtime_ExecutePythonSourceCode(Handle<PyString>::cast(source_or_code),
                                    locals_dict, globals_dict);
#else
    std::fprintf(stderr,
                 "RuntimeError: exec(str) requires embedded CPython compiler; "
                 "build with saauso_enable_cpython_compiler=true\n");
    std::exit(1);
#endif  // SAAUSO_ENABLE_CPYTHON_COMPILER
  } else if (IsPyCodeObject(*source_or_code)) {
    Runtime_ExecutePyCodeObject(Handle<PyCodeObject>::cast(source_or_code),
                                locals_dict, globals_dict);
  } else {
    std::fprintf(stderr,
                 "TypeError: exec() arg 1 must be a string or code object\n");
    std::exit(1);
  }

  // 对齐 CPython：exec 总是返回 None。
  return scope.Escape(handle(Isolate::Current()->py_none_object()));
}

}  // namespace saauso::internal
