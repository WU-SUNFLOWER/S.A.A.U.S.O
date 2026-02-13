// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef SRC_BUILTINS_BUILTINS_PY_LIST_ITERATOR_METHODS_H_
#define SRC_BUILTINS_BUILTINS_PY_LIST_ITERATOR_METHODS_H_

#include "src/builtins/builtins-utils.h"
#include "src/common/globals.h"

namespace saauso::internal {

#define PY_LIST_ITERATOR_BUILTINS(V) V(Next, "next")

class PyListIteratorBuiltinMethods : public AllStatic {
 public:
  static void Install(Handle<PyDict> target);

 private:
  PY_LIST_ITERATOR_BUILTINS(DECL_BUILTIN_METHOD)
};

}  // namespace saauso::internal

#endif  // SRC_BUILTINS_BUILTINS_PY_LIST_ITERATOR_METHODS_H_
