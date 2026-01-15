// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef SAAUSO_RUNTIME_STRING_TABLE_H_
#define SAAUSO_RUNTIME_STRING_TABLE_H_

#include "src/build/build_config.h"
#include "src/build/buildflag.h"
#include "src/handles/handles.h"
#include "src/objects/py-string.h"
#include "src/runtime/universe.h"

namespace saauso::internal {

class ObjectVisitor;

#define ST(x) (handle(Universe::string_table_->x##_str_))

#define PY_OBJECT_MAGIC_ATTR_LIST(V) \
  V(add, "__add__")                  \
  V(build_class, "__build_class__")  \
  V(call, "__call__")                \
  V(delitem, "__delitem__")          \
  V(getitem, "__getitem__")          \
  V(getattr, "__getattr__")          \
  V(init, "__init__")                \
  V(len, "__len__")                  \
  V(mro, "__mro__")                  \
  V(name, "__name__")                \
  V(_new, "__new__")                 \
  V(classcell, "__classcell__")      \
  V(class, "__class__")              \
  V(next, "__next__")                \
  V(repr, "__repr__")                \
  V(setitem, "__setitem__")          \
  V(setattr, "__setattr__")          \
  V(str, "__str__")                  \
  V(traceback, "__traceback__")      \
  V(context, "__context__")

#if BUILDFLAG(IS_WIN)
#define LIB_EXT ".dll"
#endif  // BUILDFLAG(IS_WIN)

#if BUILDFLAG(IS_LINUX)
#define LIB_EXT ".so"
#endif  // BUILDFLAG(IS_LINUX)

#define STRING_IN_TABLE_LIST(V)    \
  V(div_zero, "ZeroDivisionError") \
  V(stop_iter, "StopIteration")    \
  V(index_err, "IndexError")       \
  V(exc, "Exception")              \
  V(lib, "lib/")                   \
  V(pyc, ".pyc")                   \
  V(so, LIB_EXT)

class StringTable {
 public:
  StringTable();
  void Iterate(ObjectVisitor* v);

#define DECLARE_STR_FIELD(name, _) Tagged<PyString> name##_str_{kNullAddress};
  PY_OBJECT_MAGIC_ATTR_LIST(DECLARE_STR_FIELD)
  STRING_IN_TABLE_LIST(DECLARE_STR_FIELD)
#undef DECLARE_STR_FIELD
};

}  // namespace saauso::internal

#endif  // SAAUSO_RUNTIME_STRING_TABLE_H_
