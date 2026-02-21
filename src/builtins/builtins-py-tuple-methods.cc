// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/builtins/builtins-py-tuple-methods.h"

#include <algorithm>
#include <cstdio>
#include <cstdlib>

#include "src/execution/exception-utils.h"
#include "src/execution/isolate.h"
#include "src/handles/handles.h"
#include "src/objects/py-dict.h"
#include "src/objects/py-function.h"
#include "src/objects/py-smi.h"
#include "src/objects/py-string.h"
#include "src/objects/py-tuple.h"
#include "src/runtime/runtime-conversions.h"

namespace saauso::internal {

void PyTupleBuiltinMethods::Install(Handle<PyDict> target) {
  PY_TUPLE_BUILTINS(INSTALL_BUILTIN_METHOD);
}

////////////////////////////////////////////////////////////////////////

BUILTIN_METHOD(PyTupleBuiltinMethods, Index) {
  EscapableHandleScope scope;
  auto tuple = Handle<PyTuple>::cast(self);

  if (!kwargs.is_null() && kwargs->occupied() != 0) {
    std::fprintf(stderr,
                 "TypeError: tuple.index() takes no keyword arguments\n");
    std::exit(1);
  }

  int64_t argc = args.is_null() ? 0 : args->length();
  if (argc < 1) {
    std::fprintf(
        stderr,
        "TypeError: tuple.index() takes at least 1 argument (%lld given)\n",
        static_cast<long long>(argc));
    std::exit(1);
  }
  if (argc > 3) {
    std::fprintf(
        stderr,
        "TypeError: tuple.index() takes at most 3 arguments (%lld given)\n",
        static_cast<long long>(argc));
    std::exit(1);
  }

  auto target = args->Get(0);
  int64_t length = tuple->length();
  int64_t begin = 0;
  int64_t end = length;

  auto* isolate = Isolate::Current();

  if (argc >= 2) {
    ASSIGN_RETURN_ON_EXCEPTION(isolate, begin,
                               Runtime_DecodeIntLike(args->GetTagged(1)));
  }
  if (argc >= 3) {
    ASSIGN_RETURN_ON_EXCEPTION(isolate, end,
                               Runtime_DecodeIntLike(args->GetTagged(2)));
  }

  if (begin < 0) {
    begin += length;
  }
  if (end < 0) {
    end += length;
  }

  begin = std::min(std::max(static_cast<int64_t>(0), begin), length);
  end = std::min(std::max(static_cast<int64_t>(0), end), length);

  int64_t result = PyTuple::kNotFound;
  if (begin <= end) {
    result = tuple->IndexOf(target, begin, end);
  }
  if (result == PyTuple::kNotFound) {
    std::fprintf(stderr, "ValueError: tuple.index(x): x not in tuple\n");
    std::exit(1);
  }

  return scope.Escape(handle(PySmi::FromInt(result)));
}

}  // namespace saauso::internal
