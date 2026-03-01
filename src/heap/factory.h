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
class MethodObject;
class PyBoolean;
class PyDict;
class PyFloat;
class PyFunction;
class PyModule;
class PyNone;
class PyObject;
class PyString;
class PyTypeObject;

// 基于句柄的堆内存分配接口，全部收拢到 Factory 单例
class Factory {
 public:
  explicit Factory(Isolate* isolate) : isolate_(isolate) {}

  Factory(const Factory&) = delete;
  Factory& operator=(const Factory&) = delete;

  Address AllocateRaw(size_t size_in_bytes, Heap::AllocationSpace space);

  template <typename T>
  Tagged<T> Allocate(Heap::AllocationSpace space) {
    return isolate_->heap()->Allocate<T>(space);
  }

  Handle<PyDict> NewPyDict(int64_t init_capacity);
  Handle<PyDict> AllocateDictLike(Tagged<Klass> klass_self,
                                  int64_t init_capacity,
                                  bool allocate_properties_dict);
  Handle<PyDict> NewPyDictWithoutAllocateData();

  Handle<FixedArray> NewFixedArray(int64_t capacity);
  Handle<PyFloat> NewPyFloat(double value);

  Handle<PyFunction> NewPyFunction();
  Handle<MethodObject> NewMethodObject(Handle<PyObject> func,
                                       Handle<PyObject> owner);

  MaybeHandle<PyModule> NewPyModule();
  Handle<PyTypeObject> NewPyTypeObject();

  Handle<PyObject> AllocateRawPythonObject();

  Tagged<PyBoolean> NewPyBoolean(bool value);
  Tagged<PyNone> NewPyNone();

 private:
  Isolate* isolate_{nullptr};
};

}  // namespace saauso::internal

#endif  // SAAUSO_HEAP_FACTORY_H_
