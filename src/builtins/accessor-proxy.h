// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef SAAUSO_BUILTINS_ACCESSOR_PROXY_H_
#define SAAUSO_BUILTINS_ACCESSOR_PROXY_H_

#include "include/saauso-maybe.h"
#include "src/common/globals.h"
#include "src/handles/handles.h"

namespace saauso::internal {

class Isolate;
struct AccessorDescriptor;

class AccessorProxy : public AllStatic {
 public:
  static Maybe<bool> TryGet(Isolate* isolate,
                            Handle<PyObject> receiver,
                            Handle<PyObject> name,
                            Handle<PyObject>& out_value);
  static Maybe<bool> TrySet(Isolate* isolate,
                            Handle<PyObject> receiver,
                            Handle<PyObject> name,
                            Handle<PyObject> value);

 private:
  static const AccessorDescriptor* LookupAccessor(Isolate* isolate,
                                                  Handle<PyObject> name);
};

}  // namespace saauso::internal

#endif  // SAAUSO_BUILTINS_ACCESSOR_PROXY_H_
