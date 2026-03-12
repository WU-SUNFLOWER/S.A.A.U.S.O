// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef SAAUSO_BUILTINS_BUILTINS_PY_DICT_METHODS_H_
#define SAAUSO_BUILTINS_BUILTINS_PY_DICT_METHODS_H_

#include "src/builtins/builtins-utils.h"
#include "src/common/globals.h"

namespace saauso::internal {

#define PY_DICT_BUILTINS(V)   \
  V(Init, "__init__")         \
  V(SetDefault, "setdefault") \
  V(Pop, "pop")               \
  V(Keys, "keys")             \
  V(Values, "values")         \
  V(Items, "items")           \
  V(Get, "get")

class PyDictBuiltinMethods : public AllStatic {
 public:
  static Maybe<void> Install(Isolate* isolate, Handle<PyDict> target);

 private:
  PY_DICT_BUILTINS(DECL_BUILTIN_METHOD)
};

}  // namespace saauso::internal

#endif  // SAAUSO_BUILTINS_BUILTINS_PY_DICT_METHODS_H_
