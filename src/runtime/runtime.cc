// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/runtime/runtime.h"

#include "src/handles/handles.h"
#include "src/interpreter/interpreter.h"
#include "src/objects/klass.h"
#include "src/objects/py-dict.h"
#include "src/objects/py-float.h"
#include "src/objects/py-list.h"
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
  if (klass->vtable_.call != nullptr) {
    return true;
  }

  return false;
}

void Runtime_ExtendListByItratableObject(Handle<PyList> list,
                                         Handle<PyObject> iteratable) {
  HandleScope scope;

  if (IsPyTuple(iteratable)) {
    auto tuple = Handle<PyTuple>::cast(iteratable);
    for (int64_t i = 0; i < tuple->length(); ++i) {
      PyList::Append(list, tuple->Get(i));
    }
    return;
  }

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
  HandleScope scope;
  Handle<PyTuple> tuple;
  // Fast Path: List转Tuple
  if (IsPyList(iterable)) {
    auto list = Handle<PyList>::cast(iterable);
    tuple = PyTuple::NewInstance(list->length());
    for (auto i = 0; i < list->length(); ++i) {
      tuple->SetInternal(i, list->Get(i));
    }
    return tuple;
  }

  // Fallback: 通过迭代器进行转换
  Handle<PyList> tmp = PyList::NewInstance();
  Runtime_ExtendListByItratableObject(tmp, iterable);
  tuple = PyTuple::NewInstance(tmp->length());
  for (auto i = 0; i < tmp->length(); ++i) {
    tuple->SetInternal(i, tmp->Get(i));
  }
  return tuple.EscapeFrom(&scope);
}

Handle<PyTuple> Runtime_IntrinsicListToTuple(Handle<PyObject> object) {
  HandleScope scope;

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
  return tuple.EscapeFrom(&scope);
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
  HandleScope scope;

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

  return type_object.EscapeFrom(&scope);
}

Handle<PyObject> Runtime_FindPropertyInMro(Handle<PyObject> instance,
                                           Handle<PyObject> prop_name) {
  HandleScope scope;

  // 沿着mro序列进行查找
  Handle<PyList> mro_of_object = PyObject::GetKlass(instance)->mro();
  for (auto i = 0; i < mro_of_object->length(); ++i) {
    auto type_object =
        handle(Tagged<PyTypeObject>::cast(*mro_of_object->Get(i)));
    auto own_klass = type_object->own_klass();
    auto klass_properties = own_klass->klass_properties();
    auto result = klass_properties->Get(prop_name);
    if (!result.is_null()) {
      return result.EscapeFrom(&scope);
    }
  }

  return Handle<PyObject>::null();
}

}  // namespace saauso::internal
