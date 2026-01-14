// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef SAAUSO_RUNTIME_INTERPRETER_H_
#define SAAUSO_RUNTIME_INTERPRETER_H_

#include "src/handles/handles.h"

namespace saauso::internal {

class PyObject;
class PyList;
class ObjectVisitor;

class Interpreter {
 public:
  Interpreter();

  Handle<PyObject> builtins() const;

  Handle<PyObject> CallVirtual(Handle<PyObject> func, Handle<PyList> args);

  // GC接口
  void Iterate(ObjectVisitor* v);

 private:
  // PyDict* builtins_;
  Tagged<PyObject> builtins_{kNullAddress};
};

}  // namespace saauso::internal

#endif  // SAAUSO_RUNTIME_INTERPRETER_H_
