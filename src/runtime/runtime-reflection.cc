// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/runtime/runtime-reflection.h"

#include "src/execution/exception-utils.h"
#include "src/execution/execution.h"
#include "src/execution/isolate.h"
#include "src/objects/klass.h"
#include "src/objects/py-dict.h"
#include "src/objects/py-list.h"
#include "src/objects/py-object-klass.h"
#include "src/objects/py-object.h"
#include "src/objects/py-string.h"
#include "src/objects/py-tuple.h"
#include "src/objects/py-type-object.h"
#include "src/runtime/runtime-exceptions.h"

namespace saauso::internal {

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

bool Runtime_IsInstanceOfTypeObject(Handle<PyObject> object,
                                    Handle<PyTypeObject> type_object) {
  auto mro_of_object = PyObject::GetKlass(object)->mro();
  Handle<PyObject> type_or_tuple = type_object;
  for (auto i = 0; i < mro_of_object->length(); ++i) {
    auto curr_type_object = mro_of_object->Get(i);
    if (PyObject::EqualBool(curr_type_object, type_or_tuple)) {
      return true;
    }
  }
  return false;
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

MaybeHandle<PyObject> Runtime_InvokeMagicOperationMethod(
    Handle<PyObject> object,
    Handle<PyTuple> args,
    Handle<PyDict> kwargs,
    Handle<PyObject> func_name) {
  EscapableHandleScope scope;

  Handle<PyObject> method =
      Runtime_FindPropertyInInstanceTypeMro(object, func_name);

  if (!method.is_null()) {
    auto* isolate = Isolate::Current();
    Handle<PyObject> result;

    ASSIGN_RETURN_ON_EXCEPTION(
        isolate, result,
        Execution::Call(isolate, method, object, args, kwargs));

    return scope.Escape(result);
  }

  Runtime_ThrowTypeErrorf("unsupported operation for class %s",
                          PyObject::GetKlass(object)->name()->buffer());
  return kNullMaybeHandle;
}

MaybeHandle<PyObject> Runtime_NewObject(Handle<PyTypeObject> type_object,
                                        Handle<PyObject> args,
                                        Handle<PyObject> kwargs) {
  Handle<PyObject> result =
      type_object->own_klass()->ConstructInstance(args, kwargs);
  if (result.is_null() && Isolate::Current()->HasPendingException()) {
    return kNullMaybeHandle;
  }
  return result;
}

MaybeHandle<PyObject> Runtime_NewType(Handle<PyObject> args,
                                      Handle<PyObject> kwargs) {
  EscapableHandleScope scope;

  Handle<PyTuple> pos_args = Handle<PyTuple>::cast(args);
  int64_t argc = pos_args.is_null() ? 0 : pos_args->length();
  if (!(argc == 1 || argc == 3)) {
    Runtime_ThrowError(ExceptionType::kTypeError,
                       "type() takes 1 or 3 arguments");
    return kNullMaybeHandle;
  }

  if (argc == 1) {
    if (!kwargs.is_null() && Handle<PyDict>::cast(kwargs)->occupied() != 0) {
      Runtime_ThrowError(ExceptionType::kTypeError,
                         "type() takes 1 or 3 arguments");
      return kNullMaybeHandle;
    }

    Handle<PyObject> obj = pos_args->Get(0);
    return scope.Escape(PyObject::GetKlass(obj)->type_object());
  }

  Handle<PyObject> name_obj = pos_args->Get(0);
  Handle<PyObject> bases_obj = pos_args->Get(1);
  Handle<PyObject> dict_obj = pos_args->Get(2);

  if (!IsPyString(name_obj)) {
    Runtime_ThrowTypeErrorf(
        "type() argument 1 must be str, not '%.*s'",
        static_cast<int>(PyObject::GetKlass(name_obj)->name()->length()),
        PyObject::GetKlass(name_obj)->name()->buffer());
    return kNullMaybeHandle;
  }
  if (!IsPyTuple(bases_obj)) {
    Runtime_ThrowTypeErrorf(
        "type() argument 2 must be tuple, not '%.*s'",
        static_cast<int>(PyObject::GetKlass(bases_obj)->name()->length()),
        PyObject::GetKlass(bases_obj)->name()->buffer());
    return kNullMaybeHandle;
  }
  if (!IsPyDict(dict_obj)) {
    Runtime_ThrowTypeErrorf(
        "type() argument 3 must be dict, not '%.*s'",
        static_cast<int>(PyObject::GetKlass(dict_obj)->name()->length()),
        PyObject::GetKlass(dict_obj)->name()->buffer());
    return kNullMaybeHandle;
  }

  Handle<PyString> name = Handle<PyString>::cast(name_obj);
  Handle<PyTuple> bases_tuple = Handle<PyTuple>::cast(bases_obj);
  Handle<PyDict> class_dict = PyDict::Clone(Handle<PyDict>::cast(dict_obj));

  Handle<PyList> supers;
  if (bases_tuple->length() == 0) {
    supers = PyList::NewInstance(1);
    PyList::Append(supers, PyObjectKlass::GetInstance()->type_object());
  } else {
    supers = PyList::NewInstance(bases_tuple->length());
    for (int64_t i = 0; i < bases_tuple->length(); ++i) {
      Handle<PyObject> base = bases_tuple->Get(i);
      if (!IsPyTypeObject(base)) {
        Runtime_ThrowError(ExceptionType::kTypeError,
                           "type() bases must be types");
        return kNullMaybeHandle;
      }
      PyList::Append(supers, base);
    }
  }

  return scope.Escape(Runtime_CreatePythonClass(name, class_dict, supers));
}

}  // namespace saauso::internal
