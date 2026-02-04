// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/objects/py-list-klass.h"

#include <algorithm>
#include <cstdio>
#include <vector>

#include "src/handles/tagged.h"
#include "src/heap/heap.h"
#include "src/interpreter/interpreter.h"
#include "src/objects/fixed-array.h"
#include "src/objects/py-dict.h"
#include "src/objects/py-function.h"
#include "src/objects/py-list-iterator.h"
#include "src/objects/py-list.h"
#include "src/objects/py-object-klass.h"
#include "src/objects/py-object.h"
#include "src/objects/py-oddballs.h"
#include "src/objects/py-smi.h"
#include "src/objects/py-string.h"
#include "src/objects/py-tuple.h"
#include "src/objects/py-type-object.h"
#include "src/objects/visitors.h"
#include "src/runtime/isolate.h"
#include "src/runtime/runtime.h"
#include "src/runtime/string-table.h"
#include "src/utils/stable-merge-sort.h"
#include "src/utils/utils.h"

namespace saauso::internal {

namespace {

int64_t DecodeIntLikeOrDie(Handle<PyObject> value) {
  if (IsPySmi(value)) {
    return PySmi::ToInt(Handle<PySmi>::cast(value));
  }
  if (IsPyBoolean(value)) {
    return Handle<PyBoolean>::cast(value)->value() ? 1 : 0;
  }

  auto type_name = PyObject::GetKlass(value)->name();
  std::fprintf(stderr,
               "TypeError: '%.*s' object cannot be interpreted as an integer\n",
               static_cast<int>(type_name->length()), type_name->buffer());
  std::exit(1);
}

void PrintObjectForError(FILE* out, Handle<PyObject> value) {
  if (IsPySmi(value)) {
    std::fprintf(
        out, "%lld",
        static_cast<long long>(PySmi::ToInt(Handle<PySmi>::cast(value))));
    return;
  }
  if (IsPyBoolean(value)) {
    std::fprintf(out, "%s",
                 Handle<PyBoolean>::cast(value)->value() ? "True" : "False");
    return;
  }
  if (IsPyNone(value)) {
    std::fprintf(out, "None");
    return;
  }
  if (IsPyString(value)) {
    auto s = Handle<PyString>::cast(value);
    std::fprintf(out, "%.*s", static_cast<int>(s->length()), s->buffer());
    return;
  }

  PyObject::Print(value);
}

Handle<PyObject> NativeMethod_Append(Handle<PyObject> self,
                                     Handle<PyTuple> args,
                                     Handle<PyDict> kwargs) {
  EscapableHandleScope scope;
  auto object = Handle<PyList>::cast(self);
  PyList::Append(object, args->Get(0));
  return scope.Escape(handle(Isolate::Current()->py_none_object()));
}

Handle<PyObject> NativeMethod_Pop(Handle<PyObject> self,
                                  Handle<PyTuple> args,
                                  Handle<PyDict> kwargs) {
  EscapableHandleScope scope;
  auto object = Handle<PyList>::cast(self);
  if (object->IsEmpty()) {
    std::fprintf(stderr, "IndexError: pop from empty list\n");
    std::exit(1);
  }
  return scope.Escape(object->Pop());
}

Handle<PyObject> NativeMethod_Insert(Handle<PyObject> self,
                                     Handle<PyTuple> args,
                                     Handle<PyDict> kwargs) {
  EscapableHandleScope scope;
  auto object = Handle<PyList>::cast(self);
  auto index = Handle<PySmi>::cast(args->Get(0));
  PyList::Insert(object, PySmi::ToInt(index), args->Get(1));
  return scope.Escape(handle(Isolate::Current()->py_none_object()));
}

Handle<PyObject> NativeMethod_Index(Handle<PyObject> self,
                                    Handle<PyTuple> args,
                                    Handle<PyDict> kwargs) {
  EscapableHandleScope scope;
  auto list = Handle<PyList>::cast(self);

  if (!kwargs.is_null() && kwargs->occupied() != 0) {
    std::fprintf(stderr,
                 "TypeError: list.index() takes no keyword arguments\n");
    std::exit(1);
  }

  int64_t argc = args.is_null() ? 0 : args->length();
  if (argc < 1) {
    std::fprintf(
        stderr,
        "TypeError: list.index() takes at least 1 argument (%lld given)\n",
        static_cast<long long>(argc));
    std::exit(1);
  }
  if (argc > 3) {
    std::fprintf(
        stderr,
        "TypeError: list.index() takes at most 3 arguments (%lld given)\n",
        static_cast<long long>(argc));
    std::exit(1);
  }

  auto target = args->Get(0);

  int64_t length = list->length();
  int64_t begin = 0;
  int64_t end = length;

  if (argc >= 2) {
    begin = DecodeIntLikeOrDie(args->Get(1));
  }
  if (argc >= 3) {
    end = DecodeIntLikeOrDie(args->Get(2));
  }

  if (begin < 0) {
    begin += length;
  }
  if (end < 0) {
    end += length;
  }

  begin = std::min(std::max(static_cast<int64_t>(0), begin), length);
  end = std::min(std::max(static_cast<int64_t>(0), end), length);

  int64_t result = PyList::kNotFound;
  if (begin <= end) {
    result = list->IndexOf(target, begin, end);
  }
  if (result == PyList::kNotFound) {
    std::fprintf(stderr, "ValueError: ");
    PrintObjectForError(stderr, target);
    std::fprintf(stderr, " is not in list\n");
    std::exit(1);
  }

  return scope.Escape(handle(PySmi::FromInt(result)));
}

Handle<PyObject> NativeMethod_Reverse(Handle<PyObject> self,
                                      Handle<PyTuple> args,
                                      Handle<PyDict> kwargs) {
  EscapableHandleScope scope;
  auto list = Handle<PyList>::cast(self);

  auto length = list->length();
  for (auto i = 0; i < (length >> 1); ++i) {
    Handle<PyObject> tmp = list->Get(i);
    list->Set(i, list->Get(length - i - 1));
    list->Set(length - i - 1, tmp);
  }

  return scope.Escape(handle(Isolate::Current()->py_none_object()));
}

Handle<PyObject> NativeMethod_Extend(Handle<PyObject> self,
                                     Handle<PyTuple> args,
                                     Handle<PyDict> kwargs) {
  Runtime_ExtendListByItratableObject(Handle<PyList>::cast(self), args->Get(0));
  return handle(Isolate::Current()->py_none_object());
}

Handle<PyObject> NativeMethod_Sort(Handle<PyObject> self,
                                   Handle<PyTuple> args,
                                   Handle<PyDict> kwargs) {
  EscapableHandleScope scope;

  if (!args.is_null() && args->length() != 0) {
    std::fprintf(stderr, "TypeError: sort() takes no positional arguments\n");
    std::exit(1);
  }

  auto list = Handle<PyList>::cast(self);
  int64_t expected_length = list->length();
  if (expected_length <= 1) {
    return scope.Escape(handle(Isolate::Current()->py_none_object()));
  }

  Handle<PyObject> key_func = handle(Isolate::Current()->py_none_object());
  bool reverse = false;

  if (!kwargs.is_null() && kwargs->occupied() != 0) {
    Handle<PyString> key_name = PyString::NewInstance("key");
    Handle<PyString> reverse_name = PyString::NewInstance("reverse");

    for (int64_t i = 0; i < kwargs->capacity(); ++i) {
      Handle<PyObject> k = kwargs->KeyAtIndex(i);
      if (k.is_null()) {
        continue;
      }
      if (!IsPyString(*k)) {
        std::fprintf(stderr, "TypeError: sort() keywords must be strings\n");
        std::exit(1);
      }
      auto key_str = Handle<PyString>::cast(k);
      if (!key_str->IsEqualTo(*key_name) &&
          !key_str->IsEqualTo(*reverse_name)) {
        std::fprintf(stderr, "TypeError: sort() got an unexpected keyword\n");
        std::exit(1);
      }
    }

    Handle<PyObject> value = kwargs->Get(key_name);
    if (!value.is_null()) {
      key_func = value;
    }

    value = kwargs->Get(reverse_name);
    if (!value.is_null()) {
      reverse = Runtime_PyObjectIsTrue(value);
    }
  }

  if (!IsPyNone(*key_func) && !IsNormalPyFunction(key_func) &&
      !IsPyNativeFunction(*key_func) && !IsMethodObject(*key_func)) {
    std::fprintf(stderr, "TypeError: key must be callable\n");
    std::exit(1);
  }

  Handle<FixedArray> keys = FixedArray::NewInstance(expected_length);

  if (IsPyNone(*key_func)) {
    for (int64_t i = 0; i < expected_length; ++i) {
      keys->Set(i, list->Get(i));
    }
  } else {
    Handle<PyTuple> key_args = PyTuple::NewInstance(1);
    Handle<PyDict> empty_kwargs = PyDict::NewInstance();

    for (int64_t i = 0; i < expected_length; ++i) {
      if (list->length() != expected_length) {
        std::fprintf(stderr, "ValueError: list modified during sort (key)\n");
        std::exit(1);
      }
      Handle<PyObject> elem = list->Get(i);
      key_args->SetInternal(0, elem);
      Handle<PyObject> key = Isolate::Current()->interpreter()->CallPython(
          key_func, Handle<PyObject>::null(), key_args, empty_kwargs);
      keys->Set(i, key);
    }
  }

  std::vector<int64_t> indices(static_cast<size_t>(expected_length));
  for (int64_t i = 0; i < expected_length; ++i) {
    indices[static_cast<size_t>(i)] = i;
  }

  struct CompareContext {
    Handle<PyList> list;
    Handle<FixedArray> keys;
    int64_t expected_length;
  };

  CompareContext context{list, keys, expected_length};

  auto less = [](int64_t a, int64_t b, void* ctx) -> bool {
    auto* c = static_cast<CompareContext*>(ctx);
    if (c->list->length() != c->expected_length) {
      std::fprintf(stderr, "ValueError: list modified during sort\n");
      std::exit(1);
    }
    HandleScope scope;
    Handle<PyObject> ka = handle(c->keys->Get(a));
    Handle<PyObject> kb = handle(c->keys->Get(b));
    return PyObject::LessBool(ka, kb);
  };

  StableMergeSort::Sort(indices.data(), expected_length, less, &context);

  Handle<FixedArray> tmp = FixedArray::NewInstance(expected_length);
  for (int64_t i = 0; i < expected_length; ++i) {
    tmp->Set(i, list->Get(indices[static_cast<size_t>(i)]));
  }
  for (int64_t i = 0; i < expected_length; ++i) {
    list->Set(i, handle(tmp->Get(i)));
  }

  if (reverse) {
    for (int64_t i = 0; i < (expected_length >> 1); ++i) {
      Handle<PyObject> t = list->Get(i);
      list->Set(i, list->Get(expected_length - i - 1));
      list->Set(expected_length - i - 1, t);
    }
  }

  return scope.Escape(handle(Isolate::Current()->py_none_object()));
}

}  // namespace

////////////////////////////////////////////////////////////////////

// static
Tagged<PyListKlass> PyListKlass::GetInstance() {
  Isolate* isolate = Isolate::Current();
  Tagged<PyListKlass> instance = isolate->py_list_klass();
  if (instance.is_null()) [[unlikely]] {
    instance = isolate->heap()->Allocate<PyListKlass>(
        Heap::AllocationSpace::kMetaSpace);
    isolate->set_py_list_klass(instance);
  }
  return instance;
}

////////////////////////////////////////////////////////////////////

void PyListKlass::PreInitialize() {
  // 将自己注册到universe
  Isolate::Current()->klass_list().PushBack(Tagged<Klass>(this));

  // 初始化虚函数表
  vtable_.len = &Virtual_Len;
  vtable_.print = &Virtual_Print;
  vtable_.add = &Virtual_Add;
  vtable_.mul = &Virtual_Mul;
  vtable_.subscr = &Virtual_Subscr;
  vtable_.store_subscr = &Virtual_StoreSubscr;
  vtable_.del_subscr = &Virtual_DelSubscr;
  vtable_.less = &Virtual_Less;
  vtable_.iter = &Virtual_Iter;
  vtable_.contains = &Virtual_Contains;
  vtable_.equal = &Virtual_Equal;
  vtable_.instance_size = &Virtual_InstanceSize;
  vtable_.iterate = &Virtual_Iterate;
}

void PyListKlass::Initialize() {
  // 建立与type object的双向绑定
  PyTypeObject::NewInstance()->BindWithKlass(Tagged<Klass>(this));

  // 设置类属性
  auto klass_properties = PyDict::NewInstance();

  auto prop_name = PyString::NewInstance("append");
  PyDict::Put(klass_properties, prop_name,
              PyFunction::NewInstance(&NativeMethod_Append, prop_name));

  prop_name = PyString::NewInstance("pop");
  PyDict::Put(klass_properties, prop_name,
              PyFunction::NewInstance(&NativeMethod_Pop, prop_name));

  prop_name = PyString::NewInstance("insert");
  PyDict::Put(klass_properties, prop_name,
              PyFunction::NewInstance(&NativeMethod_Insert, prop_name));

  prop_name = PyString::NewInstance("index");
  PyDict::Put(klass_properties, prop_name,
              PyFunction::NewInstance(&NativeMethod_Index, prop_name));

  prop_name = PyString::NewInstance("reverse");
  PyDict::Put(klass_properties, prop_name,
              PyFunction::NewInstance(&NativeMethod_Reverse, prop_name));

  prop_name = PyString::NewInstance("extend");
  PyDict::Put(klass_properties, prop_name,
              PyFunction::NewInstance(&NativeMethod_Extend, prop_name));

  prop_name = PyString::NewInstance("sort");
  PyDict::Put(klass_properties, prop_name,
              PyFunction::NewInstance(&NativeMethod_Sort, prop_name));

  set_klass_properties(klass_properties);

  // 设置父类并计算mro序列
  AddSuper(PyObjectKlass::GetInstance());
  OrderSupers();

  // 设置类名
  set_name(PyString::NewInstance("list"));
}

void PyListKlass::Finalize() {
  Isolate::Current()->set_py_list_klass(Tagged<PyListKlass>::null());
}

Handle<PyObject> PyListKlass::Virtual_Len(Handle<PyObject> self) {
  return Handle<PyObject>(PySmi::FromInt(Handle<PyList>::cast(self)->length()));
}

void PyListKlass::Virtual_Print(Handle<PyObject> self) {
  auto list = Handle<PyList>::cast(self);

  std::printf("[");

  if (list->length() > 0) {
    PyObject::Print(list->Get(0));
  }

  for (auto i = 1; i < list->length(); ++i) {
    std::printf(", ");
    PyObject::Print(list->Get(i));
  }

  std::printf("]");
}

Handle<PyObject> PyListKlass::Virtual_Add(Handle<PyObject> self,
                                          Handle<PyObject> other) {
  auto list1 = Handle<PyList>::cast(self);
  auto list2 = Handle<PyList>::cast(other);

  auto new_result = PyList::NewInstance(list1->length() + list2->length());
  for (auto i = 0; i < list1->length(); ++i) {
    PyList::Append(new_result, list1->Get(i));
  }

  for (auto i = 0; i < list2->length(); ++i) {
    PyList::Append(new_result, list2->Get(i));
  }

  return new_result;
}

Handle<PyObject> PyListKlass::Virtual_Mul(Handle<PyObject> self,
                                          Handle<PyObject> coeff) {
  if (!IsPySmi(coeff)) {
    std::fprintf(stderr, "can't multiply sequence by non-int of type '%.*s'",
                 static_cast<int>(PyObject::GetKlass(coeff)->name()->length()),
                 PyObject::GetKlass(coeff)->name()->buffer());
    std::exit(1);
  }

  auto list = Handle<PyList>::cast(self);
  auto decoded_coeff = std::max(static_cast<int64_t>(0),
                                PySmi::ToInt(Handle<PySmi>::cast(coeff)));

  auto result = PyList::NewInstance(list->length() * decoded_coeff);
  while (decoded_coeff-- > 0) {
    for (int i = 0; i < list->length(); ++i) {
      PyList::Append(result, list->Get(i));
    }
  }

  return result;
}

Handle<PyObject> PyListKlass::Virtual_Subscr(Handle<PyObject> self,
                                             Handle<PyObject> subscr) {
  if (!IsPySmi(subscr)) {
    std::fprintf(stderr, "list indices must be integers or slices, not %.*s",
                 static_cast<int>(PyObject::GetKlass(subscr)->name()->length()),
                 PyObject::GetKlass(subscr)->name()->buffer());
    std::exit(1);
  }

  auto decoded_subscr = PySmi::ToInt(Handle<PySmi>::cast(subscr));
  return Handle<PyList>::cast(self)->Get(decoded_subscr);
}

void PyListKlass::Virtual_StoreSubscr(Handle<PyObject> self,
                                      Handle<PyObject> subscr,
                                      Handle<PyObject> value) {
  auto list = Handle<PyList>::cast(self);

  auto decoded_subscr = PySmi::ToInt(Handle<PySmi>::cast(subscr));
  if (!InRangeWithRightOpen(decoded_subscr, static_cast<int64_t>(0),
                            list->length())) {
    std::fprintf(stderr, "IndexError: list assignment index out of range");
    std::exit(1);
  }

  list->Set(decoded_subscr, value);
}

void PyListKlass::Virtual_DelSubscr(Handle<PyObject> self,
                                    Handle<PyObject> subscr) {
  auto list = Handle<PyList>::cast(self);

  auto decoded_subscr = PySmi::ToInt(Handle<PySmi>::cast(subscr));
  if (!InRangeWithRightOpen(decoded_subscr, static_cast<int64_t>(0),
                            list->length())) {
    std::fprintf(stderr, "IndexError: list assignment index out of range");
    std::exit(1);
  }

  list->RemoveByIndex(decoded_subscr);
}

bool PyListKlass::Virtual_Less(Handle<PyObject> self, Handle<PyObject> other) {
  auto list_l = Handle<PyList>::cast(self);
  auto list_r = Handle<PyList>::cast(other);
  auto min_len = std::min(list_l->length(), list_r->length());

  for (auto i = 0; i < min_len; ++i) {
    auto l = list_l->Get(i);
    auto r = list_r->Get(i);

    if (PyObject::LessBool(l, r)) {
      return true;
    }

    if (PyObject::GreaterBool(l, r)) {
      return false;
    }
  }

  return list_l->length() < list_r->length();
}

bool PyListKlass::Virtual_Contains(Handle<PyObject> self,
                                   Handle<PyObject> target) {
  auto list = Handle<PyList>::cast(self);
  for (auto i = 0; i < list->length(); ++i) {
    if (PyObject::EqualBool(list->Get(i), target)) {
      return true;
    }
  }

  return false;
}

bool PyListKlass::Virtual_Equal(Handle<PyObject> self,
                                Handle<PyObject> target) {
  auto list1 = Handle<PyList>::cast(self);
  auto list2 = Handle<PyList>::cast(target);

  if (list1->length() != list2->length()) {
    return false;
  }

  for (auto i = 0; i < list1->length(); ++i) {
    if (!PyObject::EqualBool(list1->Get(i), list2->Get(i))) {
      return false;
    }
  }

  return true;
}

Handle<PyObject> PyListKlass::Virtual_Iter(Handle<PyObject> object) {
  return PyListIterator::NewInstance(Handle<PyList>::cast(object));
}

size_t PyListKlass::Virtual_InstanceSize(Tagged<PyObject> self) {
  return ObjectSizeAlign(sizeof(PyList));
}

void PyListKlass::Virtual_Iterate(Tagged<PyObject> self, ObjectVisitor* v) {
  assert(IsPyList(self));
  v->VisitPointer(&Tagged<PyList>::cast(self)->array_);
}

}  // namespace saauso::internal
