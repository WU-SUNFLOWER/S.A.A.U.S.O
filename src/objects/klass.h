// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef SAAUSO_OBJECTS_KLASS_H_
#define SAAUSO_OBJECTS_KLASS_H_

#include <cstddef>
#include <cstdint>

#include "src/handles/handles.h"
#include "src/handles/maybe-handles.h"
#include "src/objects/klass-vtable.h"
#include "src/objects/objects.h"
#include "src/utils/maybe.h"
#include "src/utils/vector.h"

namespace saauso::internal {

class Klass;
class PyObject;
class PyString;
class PyTypeObject;
class PyDict;
class PyList;
class PyTuple;
class PyBoolean;
class ObjectVisitor;

enum class NativeLayoutKind : uint8_t {
  kPyObject = 0,
  kList = 1,
  kDict = 2,
  kTuple = 3,
  kString = 4,
};

class Klass : public Object {
 public:
  Klass() = delete;

  Maybe<void> InitializeVtable(Isolate* isolate);

  Handle<PyString> name();
  void set_name(Handle<PyString> name);

  Handle<PyTypeObject> type_object();
  void set_type_object(Handle<PyTypeObject>);

  Handle<PyDict> klass_properties();
  void set_klass_properties(Handle<PyDict>);

  Handle<PyList> supers();
  void set_supers(Handle<PyList>);

  Handle<PyList> mro();
  void set_mro(Handle<PyList> mro);

  NativeLayoutKind native_layout_kind() const { return native_layout_kind_; }
  void set_native_layout_kind(NativeLayoutKind kind) {
    native_layout_kind_ = kind;
  }

  Tagged<Klass> native_layout_base() const { return native_layout_base_; }
  void set_native_layout_base(Tagged<Klass> base) {
    native_layout_base_ = base;
  }

  const KlassVtable& vtable() const { return vtable_; }
  void set_vtable(const KlassVtable& vtable) { vtable_ = vtable; }

  // Klass被视为一种GC ROOT，这是暴露给GC的接口
  void Iterate(ObjectVisitor* v);

  // 添加父类
  void AddSuper(Tagged<Klass> super);
  // 执行C3算法，执行结果保存在mro_
  Maybe<void> OrderSupers(Isolate* isolate);

  // 创建一个Klass类型在Python世界对应的PyTypeObject，并将双方进行绑定
  MaybeHandle<PyTypeObject> CreateAndBindToPyTypeObject(Isolate* isolate);

  // 创建一个对象实例。失败时返回空 MaybeHandle 并已设置 pending exception。
  // 对齐原版 CPython 中 __new__ 操作的语义。
  MaybeHandle<PyObject> NewInstance(Isolate* isolate,
                                    Handle<PyTypeObject> receiver_type,
                                    Handle<PyObject> args,
                                    Handle<PyObject> kwargs);
  // 对创建好的对象实例进行初始化（一般可以理解为填充数据）。
  // 对齐原版 CPython 中 __init__ 操作的语义。
  MaybeHandle<PyObject> InitInstance(Isolate* isolate,
                                     Handle<PyObject> instance,
                                     Handle<PyObject> args,
                                     Handle<PyObject> kwargs);

 protected:
  // Python对象虚函数表
  KlassVtable vtable_;

  // PyDict* klass_properties_
  Tagged<PyObject> klass_properties_{kNullAddress};

 private:
  // PyString* name_ 类型名，如"str"、"list"、"dict"等
  Tagged<PyObject> name_{kNullAddress};

  // PyTypeObject* klass类型在python世界对应的type对象
  Tagged<PyObject> type_object_{kNullAddress};

  // PyList* supers_
  // 当前klass对应的python类型继承自哪几种类型
  Tagged<PyObject> supers_{kNullAddress};
  // C3算法的运行结果
  Tagged<PyObject> mro_{kNullAddress};

  // 当选Python类型的主基类
  // 该字段的语义对应CPython中类的tp_base/__base__
  Tagged<Klass> native_layout_base_{kNullAddress};
  // 当前Python类型在虚拟机内部采用的是哪种基础内存布局
  // 该字段的语义类似于V8当中Map的InstanceType字段
  NativeLayoutKind native_layout_kind_{NativeLayoutKind::kPyObject};
};

}  // namespace saauso::internal

#endif  // SAAUSO_OBJECTS_KLASS_H_
