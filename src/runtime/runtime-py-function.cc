// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/runtime/runtime-py-function.h"

#include <string>

#include "src/objects/py-function.h"
#include "src/objects/py-string.h"

namespace saauso::internal {

MaybeHandle<PyString> Runtime_NewFunctionRepr(Handle<PyFunction> func) {
  EscapableHandleScope scope;
  std::string repr;
  if (IsNativePyFunction(func)) {
    repr = "<built-in function ";
    repr.append(func->func_name()->ToStdString());
    repr.push_back('>');
    return scope.Escape(PyString::FromStdString(repr));
  }

  repr = "<function ";
  repr.append(func->func_name()->ToStdString());
  char addr[32];
  std::snprintf(addr, sizeof(addr), " at 0x%p>",
                reinterpret_cast<void*>((*func).ptr()));
  repr.append(addr);

  return scope.Escape(PyString::FromStdString(repr));
}

MaybeHandle<PyString> Runtime_NewMethodObjectRepr(Handle<MethodObject> method) {
  char buffer[64];
  std::snprintf(buffer, sizeof(buffer), "<method object at %p>",
                reinterpret_cast<void*>((*method).ptr()));
  return PyString::NewInstance(buffer);
}

}  // namespace saauso::internal
