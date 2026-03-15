// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef SAAUSO_BUILTINS_BUILTINS_PY_TUPLE_ITERATOR_METHODS_H_
#define SAAUSO_BUILTINS_BUILTINS_PY_TUPLE_ITERATOR_METHODS_H_

#include "src/builtins/builtins-utils.h"
#include "src/common/globals.h"

namespace saauso::internal {

#define PY_TUPLE_ITERATOR_BUILTINS(V) V(Next, "next")

class PyTupleIteratorBuiltinMethods : public AllStatic {
 public:
  static Maybe<void> Install(Isolate* isolate,
                             Handle<PyDict> target,
                             Handle<PyTypeObject> owner_type);

 private:
  PY_TUPLE_ITERATOR_BUILTINS(DECL_BUILTIN_METHOD)
};

}  // namespace saauso::internal

#endif  // SAAUSO_BUILTINS_BUILTINS_PY_TUPLE_ITERATOR_METHODS_H_
