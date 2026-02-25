// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/objects/py-module.h"

#include "src/execution/isolate.h"
#include "src/heap/heap.h"
#include "src/objects/py-dict.h"
#include "src/objects/py-module-klass.h"
#include "src/objects/py-string.h"

namespace saauso::internal {

// static
Handle<PyModule> PyModule::NewInstance() {
  EscapableHandleScope scope;

  Handle<PyModule> object(Isolate::Current()->heap()->Allocate<PyModule>(
      Heap::AllocationSpace::kNewSpace));

  // module 必须有 __dict__。我们复用 PyObject::properties_ 作为模块命名空间。
  Handle<PyDict> properties = PyDict::NewInstance();
  (void)PyDict::PutMaybe(properties, PyString::NewInstance("__dict__"),
                         properties);
  PyObject::SetProperties(*object, *properties);

  SetKlass(object, PyModuleKlass::GetInstance());

  return scope.Escape(object);
}

// static
Tagged<PyModule> PyModule::cast(Tagged<PyObject> object) {
  assert(IsPyModule(object));
  return Tagged<PyModule>::cast(object);
}

}  // namespace saauso::internal
