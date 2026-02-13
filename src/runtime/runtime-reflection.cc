// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/runtime/runtime.h"

#include "src/execution/isolate.h"
#include "src/objects/klass.h"
#include "src/objects/py-dict.h"
#include "src/objects/py-list.h"
#include "src/objects/py-object.h"
#include "src/objects/py-string.h"
#include "src/objects/py-type-object.h"

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

}  // namespace saauso::internal

