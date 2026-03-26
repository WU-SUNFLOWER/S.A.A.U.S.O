// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef SAAUSO_BUILTINS_ACCESSORS_H_
#define SAAUSO_BUILTINS_ACCESSORS_H_

#include "include/saauso-maybe.h"
#include "src/common/globals.h"
#include "src/handles/handles.h"
#include "src/handles/maybe-handles.h"

namespace saauso::internal {

class Isolate;

class PyObject;

using AccessorGetter = MaybeHandle<PyObject> (*)(Isolate* isolate,
                                                 Handle<PyObject> receiver);
using AccessorSetter = MaybeHandle<PyObject> (*)(Isolate* isolate,
                                                 Handle<PyObject> receiver,
                                                 Handle<PyObject> value);

struct AccessorDescriptor {
  AccessorGetter getter;
  AccessorSetter setter;
};

class Accessors : public AllStatic {
 public:
  // AccessorDescriptor 定义
  static const AccessorDescriptor kPyObjectClassAccessor;
  static const AccessorDescriptor kPyObjectDictAccessor;

  // Accessor Getter/Setter 函数实现
  static MaybeHandle<PyObject> PyObjectClassGetter(Isolate* isolate,
                                                   Handle<PyObject> receiver);
  static MaybeHandle<PyObject> PyObjectClassSetter(Isolate* isolate,
                                                   Handle<PyObject> receiver,
                                                   Handle<PyObject> value);

  static MaybeHandle<PyObject> PyObjectDictGetter(Isolate* isolate,
                                                  Handle<PyObject> receiver);
  static MaybeHandle<PyObject> PyObjectDictSetter(Isolate* isolate,
                                                  Handle<PyObject> receiver,
                                                  Handle<PyObject> value);
};

}  // namespace saauso::internal

#endif  // SAAUSO_BUILTINS_ACCESSORS_H_
