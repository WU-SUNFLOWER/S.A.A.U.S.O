// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/runtime/native-functions.h"

#include <cinttypes>
#include <cstdio>
#include <cstdlib>

#include "src/execution/isolate.h"
#include "src/handles/tagged.h"
#include "src/heap/heap.h"
#include "src/interpreter/interpreter.h"
#include "src/objects/py-code-object.h"
#include "src/objects/py-dict.h"
#include "src/objects/py-function.h"
#include "src/objects/py-list.h"
#include "src/objects/py-oddballs.h"
#include "src/objects/py-string.h"
#include "src/objects/py-tuple.h"
#include "src/objects/py-type-object.h"
#include "src/runtime/runtime.h"
#include "src/runtime/string-table.h"

namespace saauso::internal {
namespace {
void NormalizePrintArgs(Handle<PyDict> kwargs,
                        Handle<PyObject>& sep,
                        Handle<PyObject>& end) {
  if (kwargs.is_null()) {
    return;
  }

  Handle<PyObject> eol;

  auto sep_key = ST(sep);
  auto end_key = ST(end);
  auto eol_key = ST(eol);

  for (auto i = 0; i < kwargs->capacity(); ++i) {
    auto item = kwargs->ItemAtIndex(i);
    if (item.is_null()) {
      continue;
    }

    auto key = item->Get(0);
    if (!IsPyString(*key)) {
      std::fprintf(stderr, "TypeError: keywords must be strings\n");
      std::exit(1);
    }

    if (PyObject::EqualBool(key, sep_key) ||
        PyObject::EqualBool(key, end_key) ||
        PyObject::EqualBool(key, eol_key)) {
      continue;
    }

    auto key_str = Handle<PyString>::cast(key);
    std::fprintf(
        stderr,
        "TypeError: 'print' got an unexpected keyword argument '%.*s'\n",
        static_cast<int>(key_str->length()), key_str->buffer());
    std::exit(1);
  }

  sep = kwargs->Get(sep_key);
  if (!sep.is_null() && !IsPyString(*sep)) {
    auto type_name = PyObject::GetKlass(sep)->name();
    std::fprintf(stderr, "TypeError: sep must be str, not %s\n",
                 type_name->buffer());
    std::exit(1);
  }

  end = kwargs->Get(end_key);
  eol = kwargs->Get(eol_key);
  if (!end.is_null() && !eol.is_null()) {
    std::fprintf(stderr,
                 "TypeError: print() got multiple values for keyword argument "
                 "'end'\n");
    std::exit(1);
  }

  if (end.is_null()) {
    end = eol;
  }

  if (!end.is_null() && !IsPyString(*end)) {
    auto type_name = PyObject::GetKlass(end)->name();
    std::fprintf(stderr, "TypeError: end must be str, not %s\n",
                 type_name->buffer());
    std::exit(1);
  }
}

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

Handle<PyObject> Native_Print(Handle<PyObject> host,
                              Handle<PyTuple> args,
                              Handle<PyDict> kwargs) {
  Handle<PyObject> sep;
  Handle<PyObject> end;
  NormalizePrintArgs(kwargs, sep, end);

  for (auto i = 0; i < args->length(); ++i) {
    if (i > 0) {
      if (sep.is_null()) {
        std::printf(" ");
      } else {
        PyObject::Print(sep);
      }
    }
    PyObject::Print(args->Get(i));
  }

  if (end.is_null()) {
    std::printf("\n");
  } else {
    PyObject::Print(end);
  }

  return handle(Isolate::Current()->py_none_object());
}

Handle<PyObject> Native_Len(Handle<PyObject> host,
                            Handle<PyTuple> args,
                            Handle<PyDict> kwargs) {
  return PyObject::Len(args->Get(0));
}

Handle<PyObject> Native_IsInstance(Handle<PyObject> host,
                                   Handle<PyTuple> args,
                                   Handle<PyDict> kwargs) {
  EscapableHandleScope scope;

  auto object = args->Get(0);
  auto mro_of_object = PyObject::GetKlass(object)->mro();
  auto target_type_object = args->Get(1);

  auto result = handle(Isolate::Current()->py_false_object());
  for (auto i = 0; i < mro_of_object->length(); ++i) {
    auto curr_type_object = mro_of_object->Get(i);
    if (PyObject::EqualBool(curr_type_object, target_type_object)) {
      result = handle(Isolate::Current()->py_true_object());
      break;
    }
  }

  return scope.Escape(result);
}

Handle<PyObject> Native_BuildTypeObject(Handle<PyObject> host,
                                        Handle<PyTuple> args,
                                        Handle<PyDict> kwargs) {
  EscapableHandleScope scope;

  const auto args_length = args.is_null() ? 0 : args->length();
  if (args_length < 2) [[unlikely]] {
    std::fprintf(stderr, "__build_class__: not enough arguments");
    std::exit(1);
  }

  auto class_builder_obj = args->Get(0);
  if (!IsPyFunction(class_builder_obj)) [[unlikely]] {
    std::fprintf(stderr, "__build_class__: func is not a function");
    std::exit(1);
  }
  auto class_builder = Handle<PyFunction>::cast(class_builder_obj);

  auto class_name_obj = args->Get(1);
  if (!IsPyString(class_name_obj)) [[unlikely]] {
    std::fprintf(stderr, "__build_class__: name is not a string");
    std::exit(1);
  }
  auto class_name = Handle<PyString>::cast(class_name_obj);

  auto class_supers = PyList::NewInstance(args_length - 2);
  for (auto i = 2; i < args_length; ++i) {
    PyList::Append(class_supers, args->Get(i));
  }

  auto class_properties = PyDict::NewInstance();
  Isolate::Current()->interpreter()->CallPython(
      class_builder, Handle<PyTuple>::null(), Handle<PyTuple>::null(),
      Handle<PyDict>::null(), class_properties);

  Handle<PyTypeObject> type_object =
      Runtime_CreatePythonClass(class_name, class_properties, class_supers);
  return scope.Escape(type_object);
}

Handle<PyObject> Native_Sysgc(Handle<PyObject> host,
                              Handle<PyTuple> args,
                              Handle<PyDict> kwargs) {
  Isolate::Current()->heap()->CollectGarbage();
  return handle(Isolate::Current()->py_none_object());
}

Handle<PyObject> Native_Exec(Handle<PyObject> host,
                             Handle<PyTuple> args,
                             Handle<PyDict> kwargs) {
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
    Runtime_ExecutePythonSourceCode(Handle<PyString>::cast(source_or_code),
                                    locals_dict, globals_dict);
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
