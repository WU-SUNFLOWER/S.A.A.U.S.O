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
  EscapableHandleScope scope(isolate);

  assert(!class_name.is_null());

  Handle<PyTypeObject> type_object;
  ASSIGN_RETURN_ON_EXCEPTION(isolate, type_object,
                             isolate->factory()->NewPyTypeObject());

  // 创建新的klass并注册进isolate
  Tagged<Klass> klass = isolate->factory()->NewPythonKlass();
  isolate->klass_list().PushBack(klass);

  // 设置klass字段
  klass->set_klass_properties(class_properties);
  klass->set_name(class_name);

  // 如果没有有效的supers列表，那么显式创建一个列表，并将object作为基类添加进去
  if (supers.is_null() || supers->IsEmpty()) {
    supers = PyList::New(isolate, 1);
    PyList::Append(supers,
                   PyObjectKlass::GetInstance(isolate)->type_object(isolate),
                   isolate);
  }
  klass->set_supers(supers);

  int native_layout_count = 0;
  Tagged<Klass> native_layout_base = Tagged<Klass>::null();
  NativeLayoutKind native_layout_kind = NativeLayoutKind::kPyObject;

  if (!supers.is_null()) {
    for (int64_t i = 0; i < supers->length(); ++i) {
      auto base_type_object =
          Handle<PyTypeObject>::cast(supers->Get(i, isolate));
      Tagged<Klass> base_klass = base_type_object->own_klass();

      // 如果是第一个super，那么先把它作为 native_layout_base 和
      // native_layout_kind 的初值
      if (native_layout_base.is_null()) [[unlikely]] {
        native_layout_kind = base_klass->native_layout_kind();
        native_layout_base = base_klass;
      }

      // 如果当前基类没有特殊内存布局，就不用往下看了
      if (base_klass->native_layout_kind() == NativeLayoutKind::kPyObject) {
        continue;
      }

      // 不允许出现一种以上的特殊内存布局基类
      if (++native_layout_count > 1) {
        Runtime_ThrowError(isolate, ExceptionType::kTypeError,
                           "multiple bases have instance lay-out conflict");
        return kNullMaybeHandle;
      }

      // 如果遇到有特殊内存布局的基类，那么更新 native_layout_base 和
      // native_layout_kind
      native_layout_kind = base_klass->native_layout_kind();
      native_layout_base = base_klass;
    }
  }

  klass->set_native_layout_kind(native_layout_kind);
  klass->set_native_layout_base(native_layout_base.is_null()
                                    ? PyObjectKlass::GetInstance(isolate)
                                    : native_layout_base);
  // Python中一种类型的实例对象，默认有__dict__字典
  klass->set_instance_has_properties_dict(true);

  // 建立双向绑定
  type_object->BindWithKlass(klass, isolate);

  // 为klass计算mro
  RETURN_ON_EXCEPTION(isolate, klass->OrderSupers(isolate));

  // 初始化虚函数表
  RETURN_ON_EXCEPTION(isolate, klass->InitializeVtable(isolate));

  return scope.Escape(type_object);
}

Maybe<bool> Runtime_IsInstanceOfTypeObject(Isolate* isolate,
                                           Handle<PyObject> object,
                                           Handle<PyTypeObject> type_object) {
  auto mro_of_object =
      PyObject::ResolveObjectKlass(object, isolate)->mro(isolate);
  Handle<PyObject> type_or_tuple = type_object;
  for (auto i = 0; i < mro_of_object->length(); ++i) {
    auto curr_type_object = mro_of_object->Get(i, isolate);

    bool is_equal;
    ASSIGN_RETURN_ON_EXCEPTION(
        isolate, is_equal,
        PyObject::EqualBool(isolate, curr_type_object, type_or_tuple));

    if (is_equal) {
      return Maybe<bool>(true);
    }
  }
  return Maybe<bool>(false);
}

Maybe<bool> Runtime_IsSubtype(Isolate* isolate,
                              Handle<PyTypeObject> derive_type_object,
                              Handle<PyTypeObject> super_type_object) {
  return Runtime_IsSubtype(isolate, derive_type_object->own_klass(),
                           super_type_object->own_klass());
}

Maybe<bool> Runtime_IsSubtype(Isolate* isolate,
                              Tagged<Klass> derive_klass,
                              Tagged<Klass> super_klass) {
  auto mro_of_derive = derive_klass->mro(isolate);
  for (auto i = 0; i < mro_of_derive->length(); ++i) {
    auto curr_type_object =
        Handle<PyTypeObject>::cast(mro_of_derive->Get(i, isolate));
    if (curr_type_object->own_klass() == super_klass) {
      return Maybe<bool>(true);
    }
  }
  return Maybe<bool>(false);
}

Maybe<bool> Runtime_LookupPropertyInInstanceTypeMro(
    Isolate* isolate,
    Handle<PyObject> instance,
    Handle<PyObject> prop_name,
    Handle<PyObject>& out_prop_val) {
  return Runtime_LookupPropertyInKlassMro(
      isolate, PyObject::ResolveObjectKlass(instance, isolate), prop_name,
      out_prop_val);
}

Maybe<bool> Runtime_LookupPropertyInKlassMro(Isolate* isolate,
                                             Tagged<Klass> klass,
                                             Handle<PyObject> prop_name,
                                             Handle<PyObject>& out_prop_val) {
  EscapableHandleScope scope(isolate);

  // 预设默认输出
  out_prop_val = Handle<PyObject>::null();

  // 沿着mro序列进行查找
  Handle<PyList> mro_of_object = klass->mro(isolate);
  for (auto i = 0; i < mro_of_object->length(); ++i) {
    auto type_object =
        Handle<PyTypeObject>::cast(mro_of_object->Get(i, isolate));
    auto own_klass = type_object->own_klass();
    auto klass_properties = own_klass->klass_properties(isolate);

    Handle<PyObject> result;
    bool found = false;
    ASSIGN_RETURN_ON_EXCEPTION(
        isolate, found,
        PyDict::Get(klass_properties, prop_name, result, isolate));
    if (found) {
      assert(!result.is_null());
      out_prop_val = scope.Escape(result);
      return Maybe<bool>(true);
    }
  }

  return Maybe<bool>(false);
}

MaybeHandle<PyObject> Runtime_GetPropertyInKlassMro(
    Isolate* isolate,
    Tagged<Klass> klass,
    Handle<PyObject> prop_name) {
  EscapableHandleScope scope(isolate);

  Handle<PyObject> out_prop_val;
  bool found = false;
  ASSIGN_RETURN_ON_EXCEPTION(isolate, found,
                             Runtime_LookupPropertyInKlassMro(
                                 isolate, klass, prop_name, out_prop_val));

  if (found) {
    assert(!out_prop_val.is_null());
    return scope.Escape(out_prop_val);
  }

  Runtime_ThrowErrorf(isolate, ExceptionType::kAttributeError,
                      "type object '%s' has no attribute '%s'",
                      klass->name(isolate)->buffer(),
                      Handle<PyString>::cast(prop_name)->buffer());
  return kNullMaybeHandle;
}

MaybeHandle<PyObject> Runtime_GetPropertyInInstanceTypeMro(
    Isolate* isolate,
    Handle<PyObject> instance,
    Handle<PyObject> prop_name) {
  return Runtime_GetPropertyInKlassMro(
      isolate, PyObject::ResolveObjectKlass(instance, isolate), prop_name);
}

MaybeHandle<PyObject> Runtime_InvokeMagicOperationMethod(
    Isolate* isolate,
    Handle<PyObject> object,
    Handle<PyTuple> args,
    Handle<PyDict> kwargs,
    Handle<PyObject> func_name) {
  EscapableHandleScope scope(isolate);

  Handle<PyObject> method;
  ASSIGN_RETURN_ON_EXCEPTION(
      isolate, method,
      Runtime_GetPropertyInInstanceTypeMro(isolate, object, func_name));
  assert(!method.is_null());

  Handle<PyTuple> call_args = args.is_null() ? PyTuple::New(isolate, 0) : args;
  Handle<PyObject> result;
  ASSIGN_RETURN_ON_EXCEPTION(
      isolate, result,
      Execution::Call(isolate, method, object, call_args, kwargs));

  return scope.Escape(result);
}

MaybeHandle<PyObject> Runtime_NewObject(Isolate* isolate,
                                        Handle<PyTypeObject> type_object,
                                        Handle<PyObject> args,
                                        Handle<PyObject> kwargs) {
  auto own_klass = type_object->own_klass();

  Handle<PyObject> result;
  // 创建实例对象
  ASSIGN_RETURN_ON_EXCEPTION(
      isolate, result,
      own_klass->NewInstance(isolate, type_object, args, kwargs));

  // 对齐 CPython 的 type.__call__ 语义：
  // 1. 先执行 __new__（这里对应 own_klass->NewInstance）
  // 2. 仅当 __new__ 返回“当前被调用类型（或其子类）的实例”时，才继续执行
  // __init__
  // 3. 若 __new__ 返回了其他类型对象，则直接返回该对象，跳过 __init__
  //
  // 参见：
  // CPython 的核心判定逻辑（type_call 中对 new 返回值与 type 的关系检查）。
  bool is_instance_of_called_type = false;
  ASSIGN_RETURN_ON_EXCEPTION(
      isolate, is_instance_of_called_type,
      Runtime_IsInstanceOfTypeObject(isolate, result, type_object));
  if (!is_instance_of_called_type) {
    return result;
  }

  // 初始化实例对象
  Handle<PyObject> init_result;
  ASSIGN_RETURN_ON_EXCEPTION(
      isolate, init_result,
      own_klass->InitInstance(isolate, result, args, kwargs));

  if (!IsPyNone(init_result, isolate)) [[unlikely]] {
    auto type_name = PyObject::GetTypeName(init_result, isolate);
    Runtime_ThrowErrorf(isolate, ExceptionType::kTypeError,
                        "__init__() should return None, not '%s'\n",
                        type_name->buffer());
    return kNullMaybeHandle;
  }

  return result;
}

}  // namespace saauso::internal
