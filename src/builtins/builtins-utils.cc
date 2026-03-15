// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/builtins/builtins-utils.h"

#include <cassert>
#include <cstring>

#include "src/heap/factory.h"
#include "src/objects/klass.h"
#include "src/objects/py-dict.h"
#include "src/objects/py-function.h"
#include "src/objects/py-string.h"
#include "src/objects/templates.h"

namespace saauso::internal {

Maybe<void> InstallBuiltinMethodImpl(Isolate* isolate,
                                     Handle<PyDict> target,
                                     NativeFuncPointer func,
                                     const char* method_name,
                                     Handle<PyTypeObject> owner_type) {
  auto prop_name = PyString::NewInstance(method_name);
  bool should_mark_method_descriptor =
      !owner_type.is_null() && std::strcmp(method_name, "__new__") != 0;
  auto native_call_kind = should_mark_method_descriptor
                              ? NativeFunctionCallKind::kMethodDescriptor
                              : NativeFunctionCallKind::kPlainFunction;
  auto func_template =
      FunctionTemplateInfo(func, prop_name, native_call_kind, owner_type);
  Handle<PyFunction> func_object;
  ASSIGN_RETURN_ON_EXCEPTION(
      isolate, func_object,
      isolate->factory()->NewPyFunctionWithTemplate(func_template));
  func_object->set_native_call_kind(native_call_kind);
  func_object->set_descriptor_owner_type(should_mark_method_descriptor
                                             ? owner_type
                                             : Handle<PyTypeObject>::null());
  if (should_mark_method_descriptor) {
    assert(!func_object->descriptor_owner_type().is_null());
  }

  RETURN_ON_EXCEPTION(isolate, PyDict::Put(target, prop_name, func_object));

  return JustVoid();
}

}  // namespace saauso::internal
