// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef SAAUSO_BUILTINS_BUILTINS_PY_DICT_VIEWS_METHODS_H_
#define SAAUSO_BUILTINS_BUILTINS_PY_DICT_VIEWS_METHODS_H_

#include "src/builtins/builtins-utils.h"
#include "src/common/globals.h"

namespace saauso::internal {

#define PY_DICT_ITERATOR_BUILTINS(V) V(Next, "next")

class PyDictKeyIteratorBuiltinMethods : public AllStatic {
 public:
  static void Install(Handle<PyDict> target);

 private:
  PY_DICT_ITERATOR_BUILTINS(DECL_BUILTIN_METHOD)
};

class PyDictItemIteratorBuiltinMethods : public AllStatic {
 public:
  static void Install(Handle<PyDict> target);

 private:
  PY_DICT_ITERATOR_BUILTINS(DECL_BUILTIN_METHOD)
};

class PyDictValueIteratorBuiltinMethods : public AllStatic {
 public:
  static void Install(Handle<PyDict> target);

 private:
  PY_DICT_ITERATOR_BUILTINS(DECL_BUILTIN_METHOD)
};

}  // namespace saauso::internal

#endif  // SAAUSO_BUILTINS_BUILTINS_PY_DICT_VIEWS_METHODS_H_
