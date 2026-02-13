// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/runtime/runtime.h"

#include "src/code/compiler.h"
#include "src/handles/handles.h"
#include "src/interpreter/interpreter.h"
#include "src/objects/klass.h"
#include "src/objects/py-code-object.h"
#include "src/objects/py-dict.h"
#include "src/objects/py-float.h"
#include "src/objects/py-function.h"
#include "src/objects/py-list.h"
#include "src/objects/py-module.h"
#include "src/objects/py-object-klass.h"
#include "src/objects/py-object.h"
#include "src/objects/py-oddballs.h"
#include "src/objects/py-smi.h"
#include "src/objects/py-string.h"
#include "src/objects/py-tuple.h"
#include "src/objects/py-type-object.h"
#include "src/runtime/string-table.h"

namespace saauso::internal {

bool Runtime_PyObjectIsTrue(Handle<PyObject> object) {
  return Runtime_PyObjectIsTrue(*object);
}

bool Runtime_PyObjectIsTrue(Tagged<PyObject> object) {
  if (IsPyFalse(object) || IsPyNone(object)) {
    return false;
  }
  if (IsPyTrue(object)) {
    return true;
  }
  if (IsPySmi(object)) {
    return Tagged<PySmi>::cast(object).value() != 0;
  }
  if (IsPyFloat(object)) {
    return Tagged<PyFloat>::cast(object)->value() != 0.0;
  }
  if (IsPyString(object)) {
    return Tagged<PyString>::cast(object)->length() != 0;
  }
  if (IsPyList(object)) {
    return Tagged<PyList>::cast(object)->length() != 0;
  }
  if (IsPyTuple(object)) {
    return Tagged<PyTuple>::cast(object)->length() != 0;
  }
  if (IsPyDict(object)) {
    return Tagged<PyDict>::cast(object)->occupied() != 0;
  }
  return true;
}

bool Runtime_IsPyObjectCallable(Tagged<PyObject> object) {
  if (object.is_null()) {
    return false;
  }

  if (IsPyFunction(object) || IsMethodObject(object)) {
    return true;
  }

  Tagged<Klass> klass = PyObject::GetKlass(object);
  if (klass->vtable().call != nullptr) {
    return true;
  }

  return false;
}

void Runtime_ExtendListByItratableObject(Handle<PyList> list,
                                         Handle<PyObject> iteratable) {
  HandleScope scope;

  // Fast Path: 直接展开tuple
  if (IsPyTuple(iteratable)) {
    auto tuple = Handle<PyTuple>::cast(iteratable);
    for (int64_t i = 0; i < tuple->length(); ++i) {
      PyList::Append(list, tuple->Get(i));
    }
    return;
  }

  // Fast Path: 直接展开list
  if (IsPyList(iteratable)) {
    auto source = Handle<PyList>::cast(iteratable);
    for (int64_t i = 0; i < source->length(); ++i) {
      PyList::Append(list, source->Get(i));
    }
    return;
  }

  auto iterator = PyObject::Iter(iteratable);
  auto elem = PyObject::Next(iterator);
  while (!elem.is_null()) {
    PyList::Append(list, elem);
    elem = PyObject::Next(iterator);
  }
}

Handle<PyTuple> Runtime_UnpackIterableObjectToTuple(Handle<PyObject> iterable) {
  EscapableHandleScope scope;
  Handle<PyTuple> tuple;
  // Fast Path: List转Tuple
  if (IsPyList(iterable)) {
    auto list = Handle<PyList>::cast(iterable);
    tuple = PyTuple::NewInstance(list->length());
    for (auto i = 0; i < list->length(); ++i) {
      tuple->SetInternal(i, list->Get(i));
    }
    return scope.Escape(tuple);
  }

  // Fallback: 通过迭代器进行转换
  Handle<PyList> tmp = PyList::NewInstance();
  Runtime_ExtendListByItratableObject(tmp, iterable);
  tuple = PyTuple::NewInstance(tmp->length());
  for (auto i = 0; i < tmp->length(); ++i) {
    tuple->SetInternal(i, tmp->Get(i));
  }
  return scope.Escape(tuple);
}

Handle<PyTuple> Runtime_IntrinsicListToTuple(Handle<PyObject> object) {
  EscapableHandleScope scope;

  if (!IsPyList(object)) {
    std::fprintf(stderr,
                 "TypeError: INTRINSIC_LIST_TO_TUPLE expected a list\n");
    std::exit(1);
  }

  auto list = Handle<PyList>::cast(object);
  auto tuple = PyTuple::NewInstance(list->length());
  for (auto i = 0; i < list->length(); ++i) {
    tuple->SetInternal(i, list->Get(i));
  }
  return scope.Escape(tuple);
}

void Runtime_IntrinsicImportStar(Handle<PyObject> module,
                                 Handle<PyDict> locals) {
  if (locals.is_null()) [[unlikely]] {
    std::fprintf(stderr, "RuntimeError: no locals for import *\n");
    std::exit(1);
  }

  Handle<PyDict> module_dict = PyObject::GetProperties(module);
  if (module_dict.is_null()) [[unlikely]] {
    std::fprintf(stderr, "TypeError: module has no __dict__\n");
    std::exit(1);
  }

  auto import_name = [&](Handle<PyObject> name_obj,
                         bool ignore_private_member) {
    if (!IsPyString(name_obj)) [[unlikely]] {
      std::fprintf(stderr, "TypeError: import * name must be a string\n");
      std::exit(1);
    }

    auto name = Handle<PyString>::cast(name_obj);
    if (!ignore_private_member && name->length() > 0 && name->Get(0) == '_') {
      return;
    }

    Handle<PyObject> value = module_dict->Get(name_obj);
    if (!value.is_null()) {
      PyDict::Put(locals, name_obj, value);
    }
  };

  // 优先尝试根据__all__定向导入子模块
  Handle<PyObject> all = module_dict->Get(ST(all));
  if (!all.is_null()) {
    if (IsPyTuple(all)) {
      auto names = Handle<PyTuple>::cast(all);
      for (int64_t i = 0; i < names->length(); ++i) {
        import_name(names->Get(i), false);
      }
    } else if (IsPyList(all)) {
      auto names = Handle<PyList>::cast(all);
      for (int64_t i = 0; i < names->length(); ++i) {
        import_name(names->Get(i), false);
      }
    } else {
      std::fprintf(stderr, "TypeError: __all__ must be a tuple or list\n");
      std::exit(1);
    }
    return;
  }

  // 没有显式的__all__，那么无脑遍历dict并批量导入
  Handle<PyTuple> keys = PyDict::GetKeyTuple(module_dict);
  for (int64_t i = 0; i < keys->length(); ++i) {
    import_name(keys->Get(i), true);
  }
}

int64_t Runtime_DecodeIntLikeOrDie(Tagged<PyObject> value) {
  if (IsPySmi(value)) {
    return PySmi::ToInt(Tagged<PySmi>::cast(value));
  }
  if (IsPyBoolean(value)) {
    return Tagged<PyBoolean>::cast(value)->value() ? 1 : 0;
  }

  auto type_name = PyObject::GetKlass(value)->name();
  std::fprintf(stderr,
               "TypeError: '%.*s' object cannot be interpreted as an integer\n",
               static_cast<int>(type_name->length()), type_name->buffer());
  std::exit(1);
}

Handle<PyTypeObject> Runtime_CreatePythonClass(Handle<PyString> class_name,
                                               Handle<PyDict> class_properties,
                                               Handle<PyList> supers) {
  EscapableHandleScope scope;

  Handle<PyTypeObject> type_object = PyTypeObject::NewInstance();

  // 创建新的klass并注册进isolate
  Tagged<Klass> klass = Klass::CreateRawPythonKlass();
  Isolate::Current()->klass_list().PushBack(klass);

  // 设置klass字段
  klass->set_klass_properties(class_properties);
  klass->set_name(class_name);
  klass->set_supers(supers);

  // 建立双向绑定
  type_object->BindWithKlass(klass);

  // 为klass计算mro
  klass->OrderSupers();

  return scope.Escape(type_object);
}

Handle<PyObject> Runtime_FindPropertyInInstanceTypeMro(
    Handle<PyObject> instance,
    Handle<PyObject> prop_name) {
  return Runtime_FindPropertyInKlassMro(PyObject::GetKlass(instance),
                                        prop_name);
}

Handle<PyObject> Runtime_FindPropertyInKlassMro(Tagged<Klass> klass,
                                                Handle<PyObject> prop_name) {
  EscapableHandleScope scope;

  // 沿着mro序列进行查找
  Handle<PyList> mro_of_object = klass->mro();
  for (auto i = 0; i < mro_of_object->length(); ++i) {
    auto type_object =
        handle(Tagged<PyTypeObject>::cast(*mro_of_object->Get(i)));
    auto own_klass = type_object->own_klass();
    auto klass_properties = own_klass->klass_properties();
    auto result = klass_properties->Get(prop_name);
    if (!result.is_null()) {
      return scope.Escape(result);
    }
  }

  return Handle<PyObject>::null();
}

// 执行一个 code object，并显式指定其运行环境（locals/globals）。
// 该函数的核心用途是为内建 exec 等路径提供“在指定字典中执行代码”的能力。
Handle<PyObject> Runtime_ExecutePyCodeObject(Handle<PyCodeObject> code,
                                             Handle<PyDict> locals,
                                             Handle<PyDict> globals) {
  EscapableHandleScope scope;

  // 当前异常体系尚未完善，统一采用 fail-fast：stderr + exit(1)。
  if (code.is_null()) [[unlikely]] {
    std::fprintf(stderr, "TypeError: code object must not be null\n");
    std::exit(1);
  }
  if (locals.is_null() || globals.is_null()) [[unlikely]] {
    std::fprintf(stderr, "TypeError: locals and globals must not be null\n");
    std::exit(1);
  }

  // 对齐 CPython：若 globals 未提供 __builtins__，则自动注入当前解释器的
  // builtins。 否则源码中的内建符号（如 print/len/...）将无法解析。
  if (globals->Get(ST(builtins)).is_null()) {
    PyDict::Put(globals, ST(builtins),
                Isolate::Current()->interpreter()->builtins());
  }

  // 将 code object 包装为一个可调用的 PyFunction，随后以“绑定 locals 作为
  // frame.locals” 的方式驱动解释器执行。
  Handle<PyFunction> func = PyFunction::NewInstance(code);
  func->set_func_globals(globals);

  Handle<PyObject> result = Isolate::Current()->interpreter()->CallPython(
      func, Handle<PyObject>::null(), Handle<PyTuple>::null(),
      Handle<PyDict>::null(), locals);
  return scope.Escape(result);
}

// PyString 重载仅用于做薄封装，最终走 string_view 版本统一实现。
Handle<PyObject> Runtime_ExecutePythonSourceCode(Handle<PyString> source,
                                                 Handle<PyDict> locals,
                                                 Handle<PyDict> globals) {
  if (source.is_null()) {
    return Handle<PyObject>::null();
  }
  return Runtime_ExecutePythonSourceCode(
      std::string_view(source->buffer(), static_cast<size_t>(source->length())),
      locals, globals);
}

// 编译并执行一段 Python 源码，并显式指定其运行环境（locals/globals）。
Handle<PyObject> Runtime_ExecutePythonSourceCode(std::string_view source,
                                                 Handle<PyDict> locals,
                                                 Handle<PyDict> globals) {
  EscapableHandleScope scope;

  // 当前异常体系尚未完善，统一采用 fail-fast：stderr + exit(1)。
  if (locals.is_null() || globals.is_null()) [[unlikely]] {
    std::fprintf(stderr, "TypeError: locals and globals must not be null\n");
    std::exit(1);
  }

  // 对齐 CPython：若 globals 未提供 __builtins__，则自动注入当前解释器的
  // builtins。
  if (globals->Get(ST(builtins)).is_null()) {
    PyDict::Put(globals, ST(builtins),
                Isolate::Current()->interpreter()->builtins());
  }

  // 将源码编译为模块级 boilerplate function，然后在指定字典环境中执行它。
  Handle<PyFunction> func =
      Compiler::CompileSource(Isolate::Current(), source, "<string>");
  func->set_func_globals(globals);

  Handle<PyObject> result = Isolate::Current()->interpreter()->CallPython(
      func, Handle<PyObject>::null(), Handle<PyTuple>::null(),
      Handle<PyDict>::null(), locals);
  return scope.Escape(result);
}

}  // namespace saauso::internal
