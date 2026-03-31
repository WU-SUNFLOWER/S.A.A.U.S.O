// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/runtime/runtime-iterable.h"

#include "src/execution/exception-utils.h"
#include "src/execution/isolate.h"
#include "src/heap/factory.h"
#include "src/objects/py-list.h"
#include "src/objects/py-object.h"
#include "src/objects/py-oddballs.h"
#include "src/objects/py-tuple.h"
#include "src/runtime/runtime-exceptions.h"

namespace saauso::internal {

MaybeHandle<PyObject> Runtime_ExtendListByItratableObject(
    Isolate* isolate,
    Handle<PyList> list,
    Handle<PyObject> iteratable) {
  HandleScope scope;

  // Fast Path: 直接展开tuple
  if (IsPyTuple(iteratable)) {
    auto tuple = Handle<PyTuple>::cast(iteratable);
    for (int64_t i = 0; i < tuple->length(); ++i) {
      PyList::Append(list, tuple->Get(i, isolate), isolate);
    }
    return isolate->factory()->py_none_object();
  }

  // Fast Path: 直接展开list
  if (IsPyList(iteratable)) {
    auto source = Handle<PyList>::cast(iteratable);
    for (int64_t i = 0; i < source->length(); ++i) {
      PyList::Append(list, source->Get(i), isolate);
    }
    return isolate->factory()->py_none_object();
  }
  // Slow Path: 走一般的迭代器调用流程
  Handle<PyObject> iterator;
  ASSIGN_RETURN_ON_EXCEPTION(isolate, iterator,
                             PyObject::Iter(isolate, iteratable));

  while (true) {
    Handle<PyObject> elem;
    MaybeHandle<PyObject> next_maybe = PyObject::Next(isolate, iterator);
    if (!next_maybe.ToHandle(&elem)) {
      // 迭代结束或出现异常：
      // - 如果是 StopIteration，那么直接就地消费并终止迭代。
      // - 其余异常向上抛出。
      if (Runtime_ConsumePendingStopIterationIfSet(isolate).ToChecked()) {
        break;
      }
      return kNullMaybeHandle;
    }
    PyList::Append(list, elem, isolate);
  }

  return isolate->factory()->py_none_object();
}

MaybeHandle<PyTuple> Runtime_UnpackIterableObjectToTuple(
    Isolate* isolate,
    Handle<PyObject> iterable) {
  EscapableHandleScope scope;
  Handle<PyTuple> tuple;
  // Fast Path: List转Tuple
  if (IsPyList(iterable)) {
    auto list = Handle<PyList>::cast(iterable);
    tuple = PyTuple::New(isolate, list->length());
    for (auto i = 0; i < list->length(); ++i) {
      tuple->SetInternal(i, list->Get(i));
    }
    return scope.Escape(tuple);
  }

  // Fallback: 通过迭代器进行转换
  Handle<PyList> tmp = PyList::New(isolate);
  RETURN_ON_EXCEPTION(
      isolate, Runtime_ExtendListByItratableObject(isolate, tmp, iterable));
  tuple = PyTuple::New(isolate, tmp->length());
  for (auto i = 0; i < tmp->length(); ++i) {
    tuple->SetInternal(i, tmp->Get(i));
  }
  return scope.Escape(tuple);
}

}  // namespace saauso::internal
