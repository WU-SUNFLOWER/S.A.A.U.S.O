// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/runtime/runtime-reflection.h"

#include "src/execution/exception-utils.h"
#include "src/execution/execution.h"
#include "src/execution/isolate.h"
#include "src/heap/factory.h"
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

MaybeHandle<PyTypeObject> Runtime_CreatePythonClass(
    Isolate* isolate,
    Handle<PyString> class_name,
    Handle<PyDict> class_properties,
    Handle<PyList> supers) {
  EscapableHandleScope scope;

  Handle<PyTypeObject> type_object;
  ASSIGN_RETURN_ON_EXCEPTION(isolate, type_object,
                             isolate->factory()->NewPyTypeObject());

  // 创建新的klass并注册进isolate
  Tagged<Klass> klass = isolate->factory()->NewPythonKlass();
  Isolate::Current()->klass_list().PushBack(klass);

  // 设置klass字段
  klass->set_klass_properties(class_properties);
  klass->set_name(class_name);
  klass->set_supers(supers);

  int native_base_count = 0;
  Tagged<Klass> native_layout_base = Tagged<Klass>::null();
  NativeLayoutKind native_layout_kind = NativeLayoutKind::kPyObject;
  if (!supers.is_null()) {
    for (int64_t i = 0; i < supers->length(); ++i) {
      auto base_type_object = Handle<PyTypeObject>::cast(supers->Get(i));
      Tagged<Klass> base_klass = base_type_object->own_klass();
      if (base_klass->native_layout_kind() == NativeLayoutKind::kPyObject) {
        continue;
      }
      ++native_base_count;
      if (native_base_count > 1) {
        Runtime_ThrowError(ExceptionType::kTypeError,
                           "multiple native base classes are not supported");
        return kNullMaybeHandle;
      }
      native_layout_kind = base_klass->native_layout_kind();
      native_layout_base = base_klass->native_layout_base().is_null()
                               ? base_klass
                               : base_klass->native_layout_base();
    }
  }
  klass->set_native_layout_kind(native_layout_kind);
  klass->set_native_layout_base(native_layout_base.is_null()
                                    ? PyObjectKlass::GetInstance()
                                    : native_layout_base);
  if (native_layout_kind != NativeLayoutKind::kPyObject) {
    klass->CopyVTableFrom(klass->native_layout_base());
  }

  // 建立双向绑定
  type_object->BindWithKlass(klass);

  // 为klass计算mro
  RETURN_ON_EXCEPTION(isolate, klass->OrderSupers(isolate));

  return scope.Escape(type_object);
}

Maybe<bool> Runtime_IsInstanceOfTypeObject(Handle<PyObject> object,
                                           Handle<PyTypeObject> type_object) {
  auto* isolate [[maybe_unused]] = Isolate::Current();
  auto mro_of_object = PyObject::GetKlass(object)->mro();
  Handle<PyObject> type_or_tuple = type_object;
  for (auto i = 0; i < mro_of_object->length(); ++i) {
    auto curr_type_object = mro_of_object->Get(i);

    bool is_equal;
    ASSIGN_RETURN_ON_EXCEPTION(
        isolate, is_equal,
        PyObject::EqualBool(curr_type_object, type_or_tuple));

    if (is_equal) {
      return Maybe<bool>(true);
    }
  }
  return Maybe<bool>(false);
}

Maybe<bool> Runtime_FindPropertyInInstanceTypeMro(
    Isolate* isolate,
    Handle<PyObject> instance,
    Handle<PyObject> prop_name,
    Handle<PyObject>& out_prop_val) {
  return Runtime_FindPropertyInKlassMro(isolate, PyObject::GetKlass(instance),
                                        prop_name, out_prop_val);
}

Maybe<bool> Runtime_FindPropertyInKlassMro(Isolate* isolate,
                                           Tagged<Klass> klass,
                                           Handle<PyObject> prop_name,
                                           Handle<PyObject>& out_prop_val) {
  EscapableHandleScope scope;

  // 预设默认输出
  out_prop_val = Handle<PyObject>::null();

  // 沿着mro序列进行查找
  Handle<PyList> mro_of_object = klass->mro();
  for (auto i = 0; i < mro_of_object->length(); ++i) {
    auto type_object = Handle<PyTypeObject>::cast(mro_of_object->Get(i));
    auto own_klass = type_object->own_klass();
    auto klass_properties = own_klass->klass_properties();

    Handle<PyObject> result;
    bool found = false;
    ASSIGN_RETURN_ON_EXCEPTION(isolate, found,
                               klass_properties->Get(prop_name, result));
    if (found) {
      assert(!result.is_null());
      out_prop_val = scope.Escape(result);
      return Maybe<bool>(true);
    }
  }

  return Maybe<bool>(false);
}

MaybeHandle<PyObject> Runtime_InvokeMagicOperationMethod(
    Handle<PyObject> object,
    Handle<PyTuple> args,
    Handle<PyDict> kwargs,
    Handle<PyObject> func_name) {
  EscapableHandleScope scope;
  auto* isolate = Isolate::Current();

  Handle<PyObject> method;
  RETURN_ON_EXCEPTION(isolate, Runtime_FindPropertyInInstanceTypeMro(
                                   isolate, object, func_name, method));

  if (!method.is_null()) {
    Handle<PyObject> result;

    ASSIGN_RETURN_ON_EXCEPTION(
        isolate, result,
        Execution::Call(isolate, method, object, args, kwargs));

    return scope.Escape(result);
  }

  Runtime_ThrowErrorf(ExceptionType::kTypeError,
                      "unsupported operation for class %s",
                      PyObject::GetKlass(object)->name()->buffer());
  return kNullMaybeHandle;
}

MaybeHandle<PyObject> Runtime_NewObject(Handle<PyTypeObject> type_object,
                                        Handle<PyObject> args,
                                        Handle<PyObject> kwargs) {
  Handle<PyObject> result;
  ASSIGN_RETURN_ON_EXCEPTION(
      Isolate::Current(), result,
      type_object->own_klass()->ConstructInstance(args, kwargs));

  return result;
}

MaybeHandle<PyObject> Runtime_NewType(Handle<PyObject> args,
                                      Handle<PyObject> kwargs) {
  EscapableHandleScope scope;

  auto* isolate = Isolate::Current();

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

  if (!PyString::IsStringLike(name_obj)) {
    Runtime_ThrowErrorf(ExceptionType::kTypeError,
                        "type() argument 1 must be str, not '%s'",
                        PyObject::GetKlass(name_obj)->name()->buffer());
    return kNullMaybeHandle;
  }
  if (!PyTuple::IsTupleLike(bases_obj)) {
    Runtime_ThrowErrorf(ExceptionType::kTypeError,
                        "type() argument 2 must be tuple, not '%s'",
                        PyObject::GetKlass(bases_obj)->name()->buffer());
    return kNullMaybeHandle;
  }
  if (!PyDict::IsDictLike(dict_obj)) {
    Runtime_ThrowErrorf(ExceptionType::kTypeError,
                        "type() argument 3 must be dict, not '%s'",
                        PyObject::GetKlass(dict_obj)->name()->buffer());
    return kNullMaybeHandle;
  }

  Handle<PyString> name = PyString::CastStringLike(name_obj);
  Handle<PyTuple> bases_tuple = PyTuple::CastTupleLike(bases_obj);
  Handle<PyDict> class_dict =
      isolate->factory()->CopyPyDict(Handle<PyDict>::cast(dict_obj));

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

  Handle<PyObject> result;
  ASSIGN_RETURN_ON_EXCEPTION(
      isolate, result,
      Runtime_CreatePythonClass(isolate, name, class_dict, supers));
  return scope.Escape(result);
}

}  // namespace saauso::internal
