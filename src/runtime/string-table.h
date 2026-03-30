// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef SAAUSO_RUNTIME_STRING_TABLE_H_
#define SAAUSO_RUNTIME_STRING_TABLE_H_

#include "src/build/build_config.h"
#include "src/build/buildflag.h"
#include "src/execution/exception-types.h"
#include "src/execution/isolate.h"
#include "src/handles/handles.h"
#include "src/objects/klass-vtable.h"
#include "src/objects/py-string.h"

namespace saauso::internal {

class ObjectVisitor;

#define ST_TAGGED(x, isolate) (isolate->string_table()->x##_str_)
#define ST(x, isolate) (handle(ST_TAGGED(x, isolate), isolate))

#define PY_ODDBALLS(V)     \
  V(true_symbol, "True")   \
  V(false_symbol, "False") \
  V(none_symbol, "None")

#define PY_OBJECT_MAGIC_ATTR_LIST(V) \
  V(dict, "__dict__")                \
  V(all, "__all__")                  \
  V(mro, "__mro__")                  \
  V(name, "__name__")                \
  V(package, "__package__")          \
  V(path, "__path__")                \
  V(classcell, "__classcell__")      \
  V(class, "__class__")              \
  V(traceback, "__traceback__")      \
  V(context, "__context__")          \
  V(main, "__main_")                 \
  V(dot, ".")

#define PY_BUILTIN_FUNC_LIST(V)          \
  V(func_build_class, "__build_class__") \
  V(func_print, "print")                 \
  V(func_repr, "repr")                   \
  V(func_len, "len")                     \
  V(func_isinstance, "isinstance")       \
  V(func_sysgc, "sysgc")                 \
  V(func_exec, "exec")

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
  V(file, "__file__")           \
  V(args, "args")               \
  V(message, "message")         \
  V(end, "end")                 \
  V(eol, "eol")                 \
  V(print_file, "file")         \
  V(flush, "flush")             \
  V(sep, "sep")

class StringTable {
 public:
  explicit StringTable(Isolate* isolate);
  void Iterate(ObjectVisitor* v);

#define DECLARE_STR_FIELD(name) Tagged<PyString> name##_str_{kNullAddress};
#define DECLARE_STR_FIELD_NORMAL(name, _) DECLARE_STR_FIELD(name)
#define DECLARE_STR_FIELD_FOR_EXCEPTION(ignore1, name, ignore2) \
  DECLARE_STR_FIELD(name)
#define DECLARE_STR_FIELD_FOR_MAGIC_METHOD(ignore1, name, ignore2, ignore3) \
  DECLARE_STR_FIELD(name)

  KLASS_VTABLE_SLOT_EXPOSED(DECLARE_STR_FIELD_FOR_MAGIC_METHOD)
  PY_ODDBALLS(DECLARE_STR_FIELD_NORMAL)
  PY_OBJECT_MAGIC_ATTR_LIST(DECLARE_STR_FIELD_NORMAL)
  PY_BUILTIN_FUNC_LIST(DECLARE_STR_FIELD_NORMAL)
  STRING_IN_TABLE_LIST(DECLARE_STR_FIELD_NORMAL)

  EXCEPTION_TYPE_LIST(DECLARE_STR_FIELD_FOR_EXCEPTION);

#undef DECLARE_STR_FIELD_FOR_MAGIC_METHOD
#undef DECLARE_STR_FIELD_FOR_EXCEPTION
#undef DECLARE_STR_FIELD_NORMAL
#undef DECLARE_STR_FIELD
};

}  // namespace saauso::internal

#endif  // SAAUSO_RUNTIME_STRING_TABLE_H_
