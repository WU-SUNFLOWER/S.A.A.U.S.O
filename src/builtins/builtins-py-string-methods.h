// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef SAAUSO_BUILTINS_BUILTINS_PY_STRING_METHODS_H_
#define SAAUSO_BUILTINS_BUILTINS_PY_STRING_METHODS_H_

#include "src/builtins/builtins-utils.h"
#include "src/common/globals.h"

namespace saauso::internal {

#define PY_STRING_BUILTINS(V) \
  V(Repr, "__repr__")         \
  V(Str, "__str__")           \
  V(Upper, "upper")           \
  V(Index, "index")           \
  V(Find, "find")             \
  V(Rfind, "rfind")           \
  V(Split, "split")           \
  V(Join, "join")

class PyStringBuiltinMethods : public AllStatic {
 public:
  static Maybe<void> Install(Isolate* isolate, Handle<PyDict> target);

 private:
  PY_STRING_BUILTINS(DECL_BUILTIN_METHOD)
};

}  // namespace saauso::internal

#endif  // SAAUSO_BUILTINS_BUILTINS_PY_STRING_METHODS_H_
