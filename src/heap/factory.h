// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef SAAUSO_HEAP_FACTORY_H_
#define SAAUSO_HEAP_FACTORY_H_

#include <cstddef>
#include <cstdint>

#include "src/execution/isolate.h"
#include "src/handles/handles.h"
#include "src/handles/maybe-handles.h"
#include "src/handles/tagged.h"
#include "src/heap/heap.h"

namespace saauso::internal {

class Klass;

class FixedArray;
class Cell;
class MethodObject;
class PyBoolean;
class PyCodeObject;
class PyDict;
class PyDictItemIterator;
class PyDictItems;
class PyDictKeyIterator;
class PyDictKeys;
class PyDictValueIterator;
class PyDictValues;
class PyFloat;
class PyFunction;
class PyList;
class PyListIterator;
class PyModule;
class PyNone;
class PyObject;
class PyString;
class PyTuple;
class PyTupleIterator;
class PyTypeObject;

// 基于句柄的堆内存分配接口，全部收拢到 Factory 单例
class Factory {
 public:
  explicit Factory(Isolate* isolate) : isolate_(isolate) {}

  Factory(const Factory&) = delete;
  Factory& operator=(const Factory&) = delete;

  Handle<PyDict> NewPyDict(int64_t init_capacity);
  Handle<PyDict> AllocateDictLike(Tagged<Klass> klass_self,
                                  int64_t init_capacity,
                                  bool allocate_properties_dict);
  Handle<PyDict> NewPyDictWithoutAllocateData();

  Handle<FixedArray> NewFixedArray(int64_t capacity);
  Handle<Cell> NewCell();
  Tagged<Klass> CreateRawPythonKlass();

  Handle<PyFloat> NewPyFloat(double value);
  Handle<PyList> AllocateListLike(Tagged<Klass> klass_self,
                                  int64_t init_capacity,
                                  bool allocate_properties_dict);

  Handle<PyTuple> NewPyTupleLike(Tagged<Klass> klass_self,
                                 int64_t length,
                                 bool allocate_properties_dict);
  Handle<PyTuple> NewPyTuple(int64_t length);
  Handle<PyTuple> NewPyTupleWithElements(Handle<PyList> elements);

  Handle<PyString> NewRawStringLike(Tagged<Klass> klass_self,
                                    int64_t str_length,
                                    bool in_meta_space,
                                    bool allocate_properties_dict);
  Handle<PyString> NewRawString(int64_t str_length, bool in_meta_space);
  Handle<PyString> NewString(const char* source,
                             int64_t str_length,
                             bool in_meta_space);
  Handle<PyString> NewConsString(Handle<PyString> left, Handle<PyString> right);

  Handle<PyCodeObject> NewPyCodeObject();

  Handle<PyFunction> NewPyFunction();
  Handle<MethodObject> NewMethodObject(Handle<PyObject> func,
                                       Handle<PyObject> owner);

  MaybeHandle<PyModule> NewPyModule();
  Handle<PyTypeObject> NewPyTypeObject();

  // 根据已知类型，创建一个Python对象实例
  // 注意：该函数仅保证输出Python对象的类型是确定的，不负责调用__init__方法等后续操作！
  MaybeHandle<PyObject> NewPythonObject(Handle<PyTypeObject> type_object);

  Handle<PyListIterator> NewPyListIterator(Handle<PyObject> owner);
  Handle<PyTupleIterator> NewPyTupleIterator(Handle<PyObject> owner);

  Handle<PyDictKeys> NewPyDictKeys(Handle<PyObject> owner);
  Handle<PyDictValues> NewPyDictValues(Handle<PyObject> owner);
  Handle<PyDictItems> NewPyDictItems(Handle<PyObject> owner);
  Handle<PyDictKeyIterator> NewPyDictKeyIterator(Handle<PyObject> owner);
  Handle<PyDictValueIterator> NewPyDictValueIterator(Handle<PyObject> owner);
  Handle<PyDictItemIterator> NewPyDictItemIterator(Handle<PyObject> owner);

  Tagged<PyBoolean> NewPyBoolean(bool value);
  Tagged<PyNone> NewPyNone();

 private:
  Isolate* isolate_{nullptr};

  Address AllocateRaw(size_t size_in_bytes, Heap::AllocationSpace space);

  template <typename T>
  Tagged<T> Allocate(Heap::AllocationSpace space) {
    return isolate_->heap()->Allocate<T>(space);
  }
};

}  // namespace saauso::internal

#endif  // SAAUSO_HEAP_FACTORY_H_
