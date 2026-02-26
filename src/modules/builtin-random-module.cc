// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include <chrono>
#include <cinttypes>
#include <cstdint>

#include "src/execution/exception-utils.h"
#include "src/execution/isolate.h"
#include "src/handles/handles.h"
#include "src/modules/module-manager.h"
#include "src/objects/klass.h"
#include "src/objects/py-dict.h"
#include "src/objects/py-float.h"
#include "src/objects/py-function.h"
#include "src/objects/py-list.h"
#include "src/objects/py-module.h"
#include "src/objects/py-object.h"
#include "src/objects/py-oddballs.h"
#include "src/objects/py-smi.h"
#include "src/objects/py-string.h"
#include "src/objects/py-tuple.h"
#include "src/runtime/runtime-exceptions.h"
#include "src/runtime/string-table.h"
#include "src/utils/maybe.h"
#include "src/utils/random-number-generator.h"

namespace saauso::internal {

namespace {

RandomNumberGenerator g_rng;
bool g_seeded = false;

uint64_t NowSeed() {
  auto now = std::chrono::high_resolution_clock::now().time_since_epoch();
  return static_cast<uint64_t>(
      std::chrono::duration_cast<std::chrono::nanoseconds>(now).count());
}

void EnsureSeeded() {
  if (g_seeded) {
    return;
  }
  g_rng.Seed(NowSeed());
  g_seeded = true;
}

void FailNoKeywordArgs(const char* func_name) {
  Runtime_ThrowErrorf(ExceptionType::kTypeError,
                      "random.%s() takes no keyword arguments", func_name);
}

void FailArgRangeEmpty(const char* func_name) {
  Runtime_ThrowErrorf(ExceptionType::kValueError, "empty range for %s()",
                      func_name);
}

// 成功时返回 true 并写入 *out；类型不符时抛 TypeError 并返回 false。
Maybe<int64_t> ExtractSmi(Handle<PyObject> value, const char* func_name) {
  if (IsPySmi(value)) {
    return Maybe<int64_t>(PySmi::ToInt(Handle<PySmi>::cast(value)));
  }

  Handle<PyString> type_name = PyObject::GetKlass(value)->name();
  Runtime_ThrowErrorf(ExceptionType::kTypeError,
                      "%s() argument must be int, not '%s'", func_name,
                      type_name->buffer());

  return kNullMaybe;
}

void InstallFunc(Handle<PyDict> module_dict,
                 const char* name,
                 NativeFuncPointer func) {
  Handle<PyString> py_name = PyString::NewInstance(name);
  (void)PyDict::Put(module_dict, py_name,
                    PyFunction::NewInstance(func, py_name));
}

MaybeHandle<PyObject> Random_Seed(Handle<PyObject> host,
                                  Handle<PyTuple> args,
                                  Handle<PyDict> kwargs) {
  if (!kwargs.is_null() && kwargs->occupied() != 0) {
    FailNoKeywordArgs("seed");
    return kNullMaybe;
  }
  int64_t argc = args.is_null() ? 0 : args->length();
  if (argc > 1) {
    Runtime_ThrowErrorf(
        ExceptionType::kTypeError,
        "random.seed() takes at most 1 argument (" PRId64 " given)", argc);
    return kNullMaybe;
  }
  uint64_t seed = 0;
  if (argc == 0) {
    seed = NowSeed();
  } else {
    Handle<PyObject> a = args->Get(0);
    if (IsPyNone(a)) {
      seed = NowSeed();
    } else {
      int64_t val = 0;
      ASSIGN_RETURN_ON_EXCEPTION(Isolate::Current(), val,
                                 ExtractSmi(a, "seed"));
      seed = static_cast<uint64_t>(val);
    }
  }
  g_rng.Seed(seed);
  g_seeded = true;
  return handle(Isolate::Current()->py_none_object());
}

MaybeHandle<PyObject> Random_Random(Handle<PyObject> host,
                                    Handle<PyTuple> args,
                                    Handle<PyDict> kwargs) {
  if (!kwargs.is_null() && kwargs->occupied() != 0) {
    FailNoKeywordArgs("random");
    return kNullMaybe;
  }
  int64_t argc = args.is_null() ? 0 : args->length();
  if (argc != 0) {
    Runtime_ThrowErrorf(ExceptionType::kTypeError,
                        "random.random() takes 0 positional arguments but "
                        "%" PRId64 " were given",
                        argc);
    return kNullMaybe;
  }
  EnsureSeeded();
  return PyFloat::NewInstance(g_rng.NextDouble01());
}

MaybeHandle<PyObject> Random_RandInt(Handle<PyObject> host,
                                     Handle<PyTuple> args,
                                     Handle<PyDict> kwargs) {
  if (!kwargs.is_null() && kwargs->occupied() != 0) {
    FailNoKeywordArgs("randint");
    return kNullMaybe;
  }
  int64_t argc = args.is_null() ? 0 : args->length();
  if (argc != 2) {
    Runtime_ThrowErrorf(
        ExceptionType::kTypeError,
        "random.randint() takes exactly 2 arguments (%" PRId64 " given)", argc);
    return kNullMaybe;
  }

  int64_t a = 0;
  ASSIGN_RETURN_ON_EXCEPTION(Isolate::Current(), a,
                             ExtractSmi(args->Get(0), "randint"));

  int64_t b = 0;
  ASSIGN_RETURN_ON_EXCEPTION(Isolate::Current(), b,
                             ExtractSmi(args->Get(1), "randint"));

  if (a > b) {
    FailArgRangeEmpty("randint");
    return kNullMaybe;
  }
  EnsureSeeded();
  const uint64_t bound = static_cast<uint64_t>(b - a) + 1;
  uint64_t offset = g_rng.NextU64Bounded(bound);
  int64_t result = a + static_cast<int64_t>(offset);
  return handle(PySmi::FromInt(result));
}

MaybeHandle<PyObject> Random_RandRange(Handle<PyObject> host,
                                       Handle<PyTuple> args,
                                       Handle<PyDict> kwargs) {
  if (!kwargs.is_null() && kwargs->occupied() != 0) {
    FailNoKeywordArgs("randrange");
    return kNullMaybe;
  }
  int64_t argc = args.is_null() ? 0 : args->length();
  if (argc < 1 || argc > 3) {
    Runtime_ThrowErrorf(ExceptionType::kTypeError,
                        "random.randrange() takes from 1 to 3 positional "
                        "arguments but %" PRId64 " were given",
                        argc);
    return kNullMaybe;
  }
  int64_t start = 0;
  int64_t stop = 0;
  int64_t step = 1;

  if (argc == 1) {
    ASSIGN_RETURN_ON_EXCEPTION(Isolate::Current(), stop,
                               ExtractSmi(args->Get(0), "randrange"));
  } else {
    ASSIGN_RETURN_ON_EXCEPTION(Isolate::Current(), start,
                               ExtractSmi(args->Get(0), "randrange"));
    ASSIGN_RETURN_ON_EXCEPTION(Isolate::Current(), stop,
                               ExtractSmi(args->Get(1), "randrange"));
    if (argc == 3) {
      ASSIGN_RETURN_ON_EXCEPTION(Isolate::Current(), step,
                                 ExtractSmi(args->Get(2), "randrange"));
    }
  }

  if (step == 0) {
    Runtime_ThrowError(ExceptionType::kValueError, "zero step for randrange()");
    return kNullMaybe;
  }

  EnsureSeeded();

  if (step > 0) {
    if (start >= stop) {
      FailArgRangeEmpty("randrange");
      return kNullMaybe;
    }
    uint64_t width = static_cast<uint64_t>(stop - start);
    uint64_t step_u = static_cast<uint64_t>(step);
    uint64_t n = (width + step_u - 1) / step_u;
    uint64_t k = g_rng.NextU64Bounded(n);
    int64_t result = start + static_cast<int64_t>(k * step_u);
    return handle(PySmi::FromInt(result));
  }

  if (start <= stop) {
    FailArgRangeEmpty("randrange");
    return kNullMaybe;
  }

  uint64_t step_u = static_cast<uint64_t>(-step);
  uint64_t width = static_cast<uint64_t>(start - stop);
  uint64_t n = (width + step_u - 1) / step_u;
  uint64_t k = g_rng.NextU64Bounded(n);
  int64_t result = start - static_cast<int64_t>(k * step_u);
  return handle(PySmi::FromInt(result));
}

MaybeHandle<PyObject> Random_GetRandBits(Handle<PyObject> host,
                                         Handle<PyTuple> args,
                                         Handle<PyDict> kwargs) {
  if (!kwargs.is_null() && kwargs->occupied() != 0) {
    FailNoKeywordArgs("getrandbits");
    return kNullMaybe;
  }
  int64_t argc = args.is_null() ? 0 : args->length();
  if (argc != 1) {
    Runtime_ThrowErrorf(ExceptionType::kTypeError,
                        "random.getrandbits() takes exactly 1 argument "
                        "(%" PRId64 " given)",
                        argc);
    return kNullMaybe;
  }

  int64_t k = 0;
  ASSIGN_RETURN_ON_EXCEPTION(Isolate::Current(), k,
                             ExtractSmi(args->Get(0), "getrandbits"));

  if (k < 0) {
    Runtime_ThrowError(ExceptionType::kValueError,
                       "number of bits must be non-negative");
    return kNullMaybe;
  }
  if (k == 0) {
    return handle(PySmi::FromInt(0));
  }
  // MVP：当前虚拟机没有 big-int，因此仅支持可表示为 Smi 的 bit 数。
  if (k > 62) {
    Runtime_ThrowError(ExceptionType::kRuntimeError,
                       "getrandbits() k too large for SAAUSO MVP int");
    return kNullMaybe;
  }
  EnsureSeeded();
  uint64_t mask = (uint64_t{1} << static_cast<uint64_t>(k)) - 1;
  uint64_t x = g_rng.NextU64() & mask;
  return handle(PySmi::FromInt(static_cast<int64_t>(x)));
}

MaybeHandle<PyObject> Random_Choice(Handle<PyObject> host,
                                    Handle<PyTuple> args,
                                    Handle<PyDict> kwargs) {
  if (!kwargs.is_null() && kwargs->occupied() != 0) {
    FailNoKeywordArgs("choice");
    return kNullMaybe;
  }
  int64_t argc = args.is_null() ? 0 : args->length();
  if (argc != 1) {
    Runtime_ThrowErrorf(
        ExceptionType::kTypeError,
        "random.choice() takes exactly 1 argument (%" PRId64 " given)", argc);
    return kNullMaybe;
  }
  Handle<PyObject> seq = args->Get(0);
  EnsureSeeded();

  if (IsPyList(seq)) {
    auto list = Handle<PyList>::cast(seq);
    if (list->length() == 0) {
      Runtime_ThrowError(ExceptionType::kIndexError,
                         "Cannot choose from an empty sequence");
      return kNullMaybe;
    }
    uint64_t index =
        g_rng.NextU64Bounded(static_cast<uint64_t>(list->length()));
    return list->Get(static_cast<int64_t>(index));
  }

  if (IsPyTuple(seq)) {
    auto tuple = Handle<PyTuple>::cast(seq);
    if (tuple->length() == 0) {
      Runtime_ThrowError(ExceptionType::kIndexError,
                         "Cannot choose from an empty sequence");
      return kNullMaybe;
    }
    uint64_t index =
        g_rng.NextU64Bounded(static_cast<uint64_t>(tuple->length()));
    return tuple->Get(static_cast<int64_t>(index));
  }

  if (IsPyString(seq)) {
    auto s = Handle<PyString>::cast(seq);
    if (s->length() == 0) {
      Runtime_ThrowError(ExceptionType::kIndexError,
                         "Cannot choose from an empty sequence");
      return kNullMaybe;
    }
    uint64_t index = g_rng.NextU64Bounded(static_cast<uint64_t>(s->length()));
    return PyString::Slice(s, static_cast<int64_t>(index),
                           static_cast<int64_t>(index) + 1);
  }

  Handle<PyString> type_name = PyObject::GetKlass(seq)->name();
  Runtime_ThrowErrorf(ExceptionType::kTypeError,
                      "choice() unsupported sequence type '%s'",
                      type_name->buffer());
  return kNullMaybe;
}

MaybeHandle<PyObject> Random_Shuffle(Handle<PyObject> host,
                                     Handle<PyTuple> args,
                                     Handle<PyDict> kwargs) {
  if (!kwargs.is_null() && kwargs->occupied() != 0) {
    FailNoKeywordArgs("shuffle");
    return kNullMaybe;
  }
  int64_t argc = args.is_null() ? 0 : args->length();
  if (argc != 1) {
    Runtime_ThrowErrorf(
        ExceptionType::kTypeError,
        "random.shuffle() takes exactly 1 argument (%" PRId64 " given)", argc);
    return kNullMaybe;
  }
  Handle<PyObject> x = args->Get(0);
  if (!IsPyList(x)) {
    Handle<PyString> type_name = PyObject::GetKlass(x)->name();
    Runtime_ThrowErrorf(ExceptionType::kTypeError,
                        "shuffle() argument must be list, not '%s'",
                        type_name->buffer());
    return kNullMaybe;
  }
  EnsureSeeded();

  auto list = Handle<PyList>::cast(x);
  for (int64_t i = list->length() - 1; i > 0; i--) {
    uint64_t j = g_rng.NextU64Bounded(static_cast<uint64_t>(i) + 1);
    if (static_cast<int64_t>(j) == i) {
      continue;
    }
    Handle<PyObject> vi = list->Get(i);
    Handle<PyObject> vj = list->Get(static_cast<int64_t>(j));
    list->Set(i, vj);
    list->Set(static_cast<int64_t>(j), vi);
  }

  return handle(Isolate::Current()->py_none_object());
}

}  // namespace

BUILTIN_MODULE_INIT_FUNC("random", InitRandomModule) {
  EscapableHandleScope scope;
  Handle<PyModule> module = PyModule::NewInstance();
  Handle<PyDict> module_dict = PyObject::GetProperties(module);

  (void)PyDict::Put(module_dict, ST(name), PyString::NewInstance("random"));
  (void)PyDict::Put(module_dict, ST(package), PyString::NewInstance(""));

  g_rng.Seed(NowSeed());
  g_seeded = true;

  InstallFunc(module_dict, "seed", &Random_Seed);
  InstallFunc(module_dict, "random", &Random_Random);
  InstallFunc(module_dict, "randint", &Random_RandInt);
  InstallFunc(module_dict, "randrange", &Random_RandRange);
  InstallFunc(module_dict, "getrandbits", &Random_GetRandBits);
  InstallFunc(module_dict, "choice", &Random_Choice);
  InstallFunc(module_dict, "shuffle", &Random_Shuffle);

  return scope.Escape(module);
}

}  // namespace saauso::internal
