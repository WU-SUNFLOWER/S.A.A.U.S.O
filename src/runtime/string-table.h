// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef SAAUSO_RUNTIME_STRING_TABLE_H_
#define SAAUSO_RUNTIME_STRING_TABLE_H_

#include "src/build/build_config.h"
#include "src/build/buildflag.h"
#include "src/execution/isolate.h"
#include "src/handles/handles.h"
#include "src/objects/py-string.h"

namespace saauso::internal {

class ObjectVisitor;

#define ST_TAGGED(x) (Isolate::Current()->string_table()->x##_str_)
#define ST(x) (handle(ST_TAGGED(x)))

#define PY_OBJECT_MAGIC_ATTR_LIST(V) \
  V(add, "__add__")                  \
  V(call, "__call__")                \
  V(all, "__all__")                  \
  V(eq, "__eq__")                    \
  V(delitem, "__delitem__")          \
  V(ge, "__ge__")                    \
  V(getitem, "__getitem__")          \
  V(getattr, "__getattr__")          \
  V(gt, "__gt__")                    \
  V(init, "__init__")                \
  V(le, "__le__")                    \
  V(len, "__len__")                  \
  V(lt, "__lt__")                    \
  V(mro, "__mro__")                  \
  V(name, "__name__")                \
  V(package, "__package__")          \
  V(path, "__path__")                \
  V(ne, "__ne__")                    \
  V(_new, "__new__")                 \
  V(classcell, "__classcell__")      \
  V(class, "__class__")              \
  V(next, "__next__")                \
  V(iter, "__iter__")                \
  V(repr, "__repr__")                \
  V(setitem, "__setitem__")          \
  V(setattr, "__setattr__")          \
  V(str, "__str__")                  \
  V(traceback, "__traceback__")      \
  V(context, "__context__")          \
  V(main, "__main_")                 \
  V(dot, ".")

#define PY_BUILTIN_FUNC_LIST(V)          \
  V(func_build_class, "__build_class__") \
  V(func_print, "print")                 \
  V(func_len, "len")                     \
  V(func_isinstance, "isinstance")       \
  V(func_sysgc, "sysgc")                 \
  V(func_exec, "exec")

#define PY_EXCEPTION_TYPE_LIST(V)    \
  V(base_exception, "BaseException") \
  V(exception, "Exception")          \
  V(type_err, "TypeError")           \
  V(value_err, "ValueError")         \
  V(name_err, "NameError")           \
  V(attr_err, "AttributeError")      \
  V(index_err, "IndexError")         \
  V(key_err, "KeyError")             \
  V(runtime_err, "RuntimeError")     \
  V(div_zero, "ZeroDivisionError")   \
  V(stop_iter, "StopIteration")

#if BUILDFLAG(IS_WIN)
#define LIB_EXT ".dll"
#endif  // BUILDFLAG(IS_WIN)

#if BUILDFLAG(IS_LINUX)
#define LIB_EXT ".so"
#endif  // BUILDFLAG(IS_LINUX)

#define STRING_IN_TABLE_LIST(V) \
  V(lib, "lib/")                \
  V(pyc, ".pyc")                \
  V(so, LIB_EXT)                \
  V(builtins, "__builtins__")   \
  V(end, "end")                 \
  V(eol, "eol")                 \
  V(sep, "sep")

class StringTable {
 public:
  StringTable();
  void Iterate(ObjectVisitor* v);

#define DECLARE_STR_FIELD(name, _) Tagged<PyString> name##_str_{kNullAddress};
  PY_OBJECT_MAGIC_ATTR_LIST(DECLARE_STR_FIELD)
  PY_BUILTIN_FUNC_LIST(DECLARE_STR_FIELD)
  PY_EXCEPTION_TYPE_LIST(DECLARE_STR_FIELD)
  STRING_IN_TABLE_LIST(DECLARE_STR_FIELD)
#undef DECLARE_STR_FIELD
};

}  // namespace saauso::internal

#endif  // SAAUSO_RUNTIME_STRING_TABLE_H_
