// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/objects/py-type-object-klass.h"

#include "src/heap/heap.h"
#include "src/interpreter/interpreter.h"
#include "src/objects/klass.h"
#include "src/objects/py-dict.h"
#include "src/objects/py-function.h"
#include "src/objects/py-list.h"
#include "src/objects/py-object-klass.h"
#include "src/objects/py-object.h"
#include "src/objects/py-oddballs.h"
#include "src/objects/py-string.h"
#include "src/objects/py-tuple.h"
#include "src/objects/py-type-object.h"
#include "src/objects/visitors.h"
#include "src/runtime/isolate.h"
#include "src/runtime/runtime.h"
#include "src/runtime/string-table.h"

namespace saauso::internal {

namespace {
Handle<PyObject> Native_Mro(Handle<PyObject> self,
                            Handle<PyTuple> args,
                            Handle<PyDict> kwargs) {
  return Handle<PyTypeObject>::cast(self)->mro();
}
}  // namespace

// static
Tagged<PyTypeObjectKlass> PyTypeObjectKlass::GetInstance() {
  Isolate* isolate = Isolate::Current();
  Tagged<PyTypeObjectKlass> instance = isolate->py_type_object_klass();
  if (instance.is_null()) [[unlikely]] {
    instance = isolate->heap()->Allocate<PyTypeObjectKlass>(
        Heap::AllocationSpace::kMetaSpace);
    isolate->set_py_type_object_klass(instance);
  }
  return instance;
}

void PyTypeObjectKlass::PreInitialize() {
  // 将自己注册到isolate
  Isolate::Current()->klass_list().PushBack(Tagged<Klass>(this));

  // 初始化虚函数表
  vtable_.print = &Virtual_Print;
  vtable_.getattr = &Virtual_GetAttr;
  vtable_.setattr = &Virtual_SetAttr;
  vtable_.hash = &Virtual_Hash;
  vtable_.equal = &Virtual_Equal;
  vtable_.not_equal = &Virtual_NotEqual;
  vtable_.construct_instance = &Virtual_ConstructInstance;
  vtable_.call = &Virtual_Call;
  vtable_.instance_size = &Virtual_InstanceSize;
  vtable_.iterate = &Virtual_Iterate;
}

void PyTypeObjectKlass::Initialize() {
  // 建立与type object的双向绑定
  PyTypeObject::NewInstance()->BindWithKlass(Tagged<Klass>(this));

  // 初始化类字典
  auto klass_properties = PyDict::NewInstance();

  auto func_name = PyString::NewInstance("mro");
  auto func = PyFunction::NewInstance(&Native_Mro, func_name);
  PyDict::Put(klass_properties, func_name, func);

  set_klass_properties(klass_properties);

  // 设置父类并计算mro序列
  AddSuper(PyObjectKlass::GetInstance());
  OrderSupers();

  // 设置类名
  set_name(PyString::NewInstance("type"));
}

void PyTypeObjectKlass::Finalize() {
  Isolate::Current()->set_py_type_object_klass(
      Tagged<PyTypeObjectKlass>::null());
}

///////////////////////////////////////////////////////////////////////////

// static
void PyTypeObjectKlass::Virtual_Print(Handle<PyObject> self) {
  auto type_object = Handle<PyTypeObject>::cast(self);
  auto type_name = type_object->own_klass()->name();
  std::printf("<class '%.*s'>", static_cast<int>(type_name->length()),
              type_name->buffer());
}

Handle<PyObject> PyTypeObjectKlass::Virtual_GetAttr(Handle<PyObject> self,
                                                    Handle<PyObject> prop_name,
                                                    bool is_try) {
  assert(IsPyString(prop_name));
  auto own_klass = Handle<PyTypeObject>::cast(self)->own_klass();

  // 沿着当前type object的mro序列进行查找
  Handle<PyObject> result =
      Runtime_FindPropertyInKlassMro(own_klass, prop_name);
  if (!result.is_null()) {
    return result;
  }
  if (is_try) {
    return Handle<PyObject>::null();
  }
  std::fprintf(stderr, "AttributeError: '%s' object has no attribute '%s'\n",
               PyObject::GetKlass(self)->name()->buffer(),
               Handle<PyString>::cast(prop_name)->buffer());
  std::exit(1);
  return Handle<PyObject>::null();
}

void PyTypeObjectKlass::Virtual_SetAttr(Handle<PyObject> self,
                                        Handle<PyObject> prop_name,
                                        Handle<PyObject> prop_value) {
  auto own_klass = Handle<PyTypeObject>::cast(self)->own_klass();
  PyDict::Put(own_klass->klass_properties(), prop_name, prop_value);
}

uint64_t PyTypeObjectKlass::Virtual_Hash(Handle<PyObject> self) {
  return static_cast<uint64_t>((*self).ptr());
}

bool PyTypeObjectKlass::Virtual_Equal(Handle<PyObject> self,
                                      Handle<PyObject> other) {
  if (!IsPyTypeObject(other)) {
    return false;
  }
  return self.is_identical_to(other);
}

bool PyTypeObjectKlass::Virtual_NotEqual(Handle<PyObject> self,
                                         Handle<PyObject> other) {
  return !Virtual_Equal(self, other);
}

Handle<PyObject> PyTypeObjectKlass::Virtual_ConstructInstance(
    Tagged<Klass> klass_self,
    Handle<PyObject> args,
    Handle<PyObject> kwargs) {
  assert(klass_self == PyTypeObjectKlass::GetInstance());

  Handle<PyTuple> pos_args = Handle<PyTuple>::cast(args);
  int64_t argc = pos_args.is_null() ? 0 : pos_args->length();
  if (!(argc == 1 || argc == 3)) {
    std::fprintf(stderr, "TypeError: type() takes 1 or 3 arguments\n");
    std::exit(1);
  }

  // 只有一个参数，直接返回传入对象对应类型的type object。
  // 这是Python中type最常见的用法。
  if (argc == 1) {
    if (!kwargs.is_null() && Handle<PyDict>::cast(kwargs)->occupied() != 0) {
      std::fprintf(stderr, "TypeError: type() takes 1 or 3 arguments\n");
      std::exit(1);
    }

    Handle<PyObject> obj = pos_args->Get(0);
    return PyObject::GetKlass(obj)->type_object();
  }

  // 有三个参数，走以下的创建新的Python类型的逻辑
  Handle<PyObject> name_obj = pos_args->Get(0);
  Handle<PyObject> bases_obj = pos_args->Get(1);
  Handle<PyObject> dict_obj = pos_args->Get(2);

  if (!IsPyString(name_obj)) {
    std::fprintf(
        stderr, "TypeError: type() argument 1 must be str, not '%.*s'\n",
        static_cast<int>(PyObject::GetKlass(name_obj)->name()->length()),
        PyObject::GetKlass(name_obj)->name()->buffer());
    std::exit(1);
  }
  if (!IsPyTuple(bases_obj)) {
    std::fprintf(
        stderr, "TypeError: type() argument 2 must be tuple, not '%.*s'\n",
        static_cast<int>(PyObject::GetKlass(bases_obj)->name()->length()),
        PyObject::GetKlass(bases_obj)->name()->buffer());
    std::exit(1);
  }
  if (!IsPyDict(dict_obj)) {
    std::fprintf(
        stderr, "TypeError: type() argument 3 must be dict, not '%.*s'\n",
        static_cast<int>(PyObject::GetKlass(dict_obj)->name()->length()),
        PyObject::GetKlass(dict_obj)->name()->buffer());
    std::exit(1);
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
        std::fprintf(stderr, "TypeError: type() bases must be types\n");
        std::exit(1);
      }
      PyList::Append(supers, base);
    }
  }

  // 关键字参数在 CPython 中会进入 metaclass machinery（例如
  // __init_subclass__）。 当前 MVP 尚未实现该机制，因此先忽略 kwargs
  // 的语义（不影响常见的动态建类用法）。
  return Runtime_CreatePythonClass(name, class_dict, supers);
}

Handle<PyObject> PyTypeObjectKlass::Virtual_Call(Handle<PyObject> self,
                                                 Handle<PyObject> host,
                                                 Handle<PyObject> args,
                                                 Handle<PyObject> kwargs) {
  auto type_object = Handle<PyTypeObject>::cast(self);
  auto own_klass = type_object->own_klass();
  // type object本身并不知道如何创建新对象的逻辑，需要转发给对应的klass
  return own_klass->ConstructInstance(args, kwargs);
}

// static
size_t PyTypeObjectKlass::Virtual_InstanceSize(Tagged<PyObject> self) {
  return sizeof(PyTypeObject);
}

// static
void PyTypeObjectKlass::Virtual_Iterate(Tagged<PyObject> self,
                                        ObjectVisitor* v) {
  // 虽然type object中有一个klass指针，
  // 但是我们约定GC阶段对klass的扫描统一走Heap::IterateRoots。
  // 因此这里不做任何事情！
}

}  // namespace saauso::internal
