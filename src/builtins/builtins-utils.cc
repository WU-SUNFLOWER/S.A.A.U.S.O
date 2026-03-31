// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/builtins/builtins-utils.h"

#include <cassert>

#include "src/heap/factory.h"
#include "src/objects/py-dict.h"
#include "src/objects/py-function.h"
#include "src/objects/py-string.h"
#include "src/objects/templates.h"

namespace saauso::internal {

Maybe<void> InstallBuiltinMethodImpl(Isolate* isolate,
                                     Handle<PyDict> target,
                                     NativeFuncPointer func,
                                     const char* method_name,
                                     NativeFuncAccessFlag access_flag,
                                     Handle<PyTypeObject> owner_type) {
  HandleScope scope(isolate);

  auto prop_name = PyString::New(isolate, method_name);
  auto func_template =
      FunctionTemplateInfo(isolate, func, prop_name, access_flag, owner_type);

  Handle<PyFunction> func_object;
  ASSIGN_RETURN_ON_EXCEPTION(
      isolate, func_object,
      isolate->factory()->NewPyFunctionWithTemplate(func_template));

  RETURN_ON_EXCEPTION(isolate,
                      PyDict::Put(target, prop_name, func_object, isolate));

  return JustVoid();
}

}  // namespace saauso::internal
