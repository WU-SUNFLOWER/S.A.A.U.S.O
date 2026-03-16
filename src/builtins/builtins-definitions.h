// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef SAAUSO_BUILTINS_BUILTINS_DEFINITIONS_H_
#define SAAUSO_BUILTINS_BUILTINS_DEFINITIONS_H_

#include "src/builtins/builtins-utils.h"

namespace saauso::internal {

#define BUILTIN_FUNC_LIST(V)           \
  /* builtins-io */                    \
  V(func_print, Print)                 \
  /* builtins-refection */             \
  V(func_repr, Repr)                   \
  V(func_len, Len)                     \
  V(func_isinstance, IsInstance)       \
  V(func_build_class, BuildTypeObject) \
  /* builtins-exec */                  \
  V(func_exec, Exec)                   \
  /* builtins-debug */                 \
  V(func_sysgc, Sysgc)

#define DECL_BUILTIN_FUNC(func_name_in_string_table, cpp_func_name) \
  BUILTIN(cpp_func_name);
BUILTIN_FUNC_LIST(DECL_BUILTIN_FUNC)
#undef DECL_BUILTIN_FUNC

}  // namespace saauso::internal

#endif  // SAAUSO_BUILTINS_BUILTINS_DEFINITIONS_H_
