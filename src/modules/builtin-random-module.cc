// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>

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
#include "src/runtime/string-table.h"
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
  std::fprintf(stderr, "TypeError: random.%s() takes no keyword arguments\n",
               func_name);
  std::exit(1);
}

void FailArgRangeEmpty(const char* func_name) {
  std::fprintf(stderr, "ValueError: empty range for %s()\n", func_name);
  std::exit(1);
}

int64_t ExtractSmi(Handle<PyObject> value, const char* func_name) {
  if (IsPySmi(value)) {
    return PySmi::ToInt(Handle<PySmi>::cast(value));
  }

  auto type_name = PyObject::GetKlass(value)->name();
  std::fprintf(stderr, "TypeError: %s() argument must be int, not '%.*s'\n",
               func_name, static_cast<int>(type_name->length()),
               type_name->buffer());
  std::exit(1);
  return 0;
}

void InstallFunc(Handle<PyDict> module_dict,
                 const char* name,
                 NativeFuncPointer func) {
  Handle<PyString> py_name = PyString::NewInstance(name);
  PyDict::Put(module_dict, py_name, PyFunction::NewInstance(func, py_name));
}

MaybeHandle<PyObject> Random_Seed(Handle<PyObject> host,
                                  Handle<PyTuple> args,
                                  Handle<PyDict> kwargs) {
  (void)host;
  if (!kwargs.is_null() && kwargs->occupied() != 0) {
    FailNoKeywordArgs("seed");
  }

  int64_t argc = args.is_null() ? 0 : args->length();
  if (argc > 1) {
    std::fprintf(stderr,
                 "TypeError: random.seed() takes at most 1 argument (%lld "
                 "given)\n",
                 static_cast<long long>(argc));
    std::exit(1);
  }

  uint64_t seed = 0;
  if (argc == 0) {
    seed = NowSeed();
  } else {
    Handle<PyObject> a = args->Get(0);
    if (IsPyNone(a)) {
      seed = NowSeed();
    } else if (IsPySmi(a)) {
      seed = static_cast<uint64_t>(PySmi::ToInt(Handle<PySmi>::cast(a)));
    } else {
      auto type_name = PyObject::GetKlass(a)->name();
      std::fprintf(stderr,
                   "TypeError: seed() argument must be int, not '%.*s'\n",
                   static_cast<int>(type_name->length()), type_name->buffer());
      std::exit(1);
    }
  }

  g_rng.Seed(seed);
  g_seeded = true;
  return handle(Isolate::Current()->py_none_object());
}

MaybeHandle<PyObject> Random_Random(Handle<PyObject> host,
                                    Handle<PyTuple> args,
                                    Handle<PyDict> kwargs) {
  (void)host;
  if (!kwargs.is_null() && kwargs->occupied() != 0) {
    FailNoKeywordArgs("random");
  }

  int64_t argc = args.is_null() ? 0 : args->length();
  if (argc != 0) {
    std::fprintf(stderr,
                 "TypeError: random.random() takes 0 positional arguments but "
                 "%lld were given\n",
                 static_cast<long long>(argc));
    std::exit(1);
  }

  EnsureSeeded();
  return PyFloat::NewInstance(g_rng.NextDouble01());
}

MaybeHandle<PyObject> Random_RandInt(Handle<PyObject> host,
                                     Handle<PyTuple> args,
                                     Handle<PyDict> kwargs) {
  (void)host;
  if (!kwargs.is_null() && kwargs->occupied() != 0) {
    FailNoKeywordArgs("randint");
  }

  int64_t argc = args.is_null() ? 0 : args->length();
  if (argc != 2) {
    std::fprintf(stderr,
                 "TypeError: random.randint() takes exactly 2 arguments (%lld "
                 "given)\n",
                 static_cast<long long>(argc));
    std::exit(1);
  }

  int64_t a = ExtractSmi(args->Get(0), "randint");
  int64_t b = ExtractSmi(args->Get(1), "randint");
  if (a > b) {
    FailArgRangeEmpty("randint");
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
  (void)host;
  if (!kwargs.is_null() && kwargs->occupied() != 0) {
    FailNoKeywordArgs("randrange");
  }

  int64_t argc = args.is_null() ? 0 : args->length();
  if (argc < 1 || argc > 3) {
    std::fprintf(
        stderr,
        "TypeError: random.randrange() takes from 1 to 3 positional arguments "
        "but %lld were given\n",
        static_cast<long long>(argc));
    std::exit(1);
  }

  int64_t start = 0;
  int64_t stop = 0;
  int64_t step = 1;

  if (argc == 1) {
    stop = ExtractSmi(args->Get(0), "randrange");
  } else {
    start = ExtractSmi(args->Get(0), "randrange");
    stop = ExtractSmi(args->Get(1), "randrange");
    if (argc == 3) {
      step = ExtractSmi(args->Get(2), "randrange");
    }
  }

  if (step == 0) {
    std::fprintf(stderr, "ValueError: zero step for randrange()\n");
    std::exit(1);
  }

  EnsureSeeded();

  if (step > 0) {
    if (start >= stop) {
      FailArgRangeEmpty("randrange");
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
  (void)host;
  if (!kwargs.is_null() && kwargs->occupied() != 0) {
    FailNoKeywordArgs("getrandbits");
  }

  int64_t argc = args.is_null() ? 0 : args->length();
  if (argc != 1) {
    std::fprintf(stderr,
                 "TypeError: random.getrandbits() takes exactly 1 argument "
                 "(%lld given)\n",
                 static_cast<long long>(argc));
    std::exit(1);
  }

  int64_t k = ExtractSmi(args->Get(0), "getrandbits");
  if (k < 0) {
    std::fprintf(stderr, "ValueError: number of bits must be non-negative\n");
    std::exit(1);
  }
  if (k == 0) {
    return handle(PySmi::FromInt(0));
  }

  // MVP：当前虚拟机没有 big-int，因此仅支持可表示为 Smi 的 bit 数。
  if (k > 62) {
    std::fprintf(stderr,
                 "OverflowError: getrandbits() k too large for SAAUSO MVP "
                 "int\n");
    std::exit(1);
  }

  EnsureSeeded();
  uint64_t mask = (uint64_t{1} << static_cast<uint64_t>(k)) - 1;
  uint64_t x = g_rng.NextU64() & mask;
  return handle(PySmi::FromInt(static_cast<int64_t>(x)));
}

MaybeHandle<PyObject> Random_Choice(Handle<PyObject> host,
                                    Handle<PyTuple> args,
                                    Handle<PyDict> kwargs) {
  (void)host;
  if (!kwargs.is_null() && kwargs->occupied() != 0) {
    FailNoKeywordArgs("choice");
  }

  int64_t argc = args.is_null() ? 0 : args->length();
  if (argc != 1) {
    std::fprintf(stderr,
                 "TypeError: random.choice() takes exactly 1 argument (%lld "
                 "given)\n",
                 static_cast<long long>(argc));
    std::exit(1);
  }

  Handle<PyObject> seq = args->Get(0);
  EnsureSeeded();

  if (IsPyList(seq)) {
    auto list = Handle<PyList>::cast(seq);
    if (list->length() == 0) {
      std::fprintf(stderr,
                   "IndexError: Cannot choose from an empty sequence\n");
      std::exit(1);
    }
    uint64_t index =
        g_rng.NextU64Bounded(static_cast<uint64_t>(list->length()));
    return list->Get(static_cast<int64_t>(index));
  }

  if (IsPyTuple(seq)) {
    auto tuple = Handle<PyTuple>::cast(seq);
    if (tuple->length() == 0) {
      std::fprintf(stderr,
                   "IndexError: Cannot choose from an empty sequence\n");
      std::exit(1);
    }
    uint64_t index =
        g_rng.NextU64Bounded(static_cast<uint64_t>(tuple->length()));
    return tuple->Get(static_cast<int64_t>(index));
  }

  if (IsPyString(seq)) {
    auto s = Handle<PyString>::cast(seq);
    if (s->length() == 0) {
      std::fprintf(stderr,
                   "IndexError: Cannot choose from an empty sequence\n");
      std::exit(1);
    }
    uint64_t index = g_rng.NextU64Bounded(static_cast<uint64_t>(s->length()));
    return PyString::Slice(s, static_cast<int64_t>(index),
                           static_cast<int64_t>(index) + 1);
  }

  auto type_name = PyObject::GetKlass(seq)->name();
  std::fprintf(stderr, "TypeError: choice() unsupported sequence type '%.*s'\n",
               static_cast<int>(type_name->length()), type_name->buffer());
  std::exit(1);
  return Handle<PyObject>::null();
}

MaybeHandle<PyObject> Random_Shuffle(Handle<PyObject> host,
                                     Handle<PyTuple> args,
                                     Handle<PyDict> kwargs) {
  (void)host;
  if (!kwargs.is_null() && kwargs->occupied() != 0) {
    FailNoKeywordArgs("shuffle");
  }

  int64_t argc = args.is_null() ? 0 : args->length();
  if (argc != 1) {
    std::fprintf(stderr,
                 "TypeError: random.shuffle() takes exactly 1 argument (%lld "
                 "given)\n",
                 static_cast<long long>(argc));
    std::exit(1);
  }

  Handle<PyObject> x = args->Get(0);
  if (!IsPyList(x)) {
    auto type_name = PyObject::GetKlass(x)->name();
    std::fprintf(stderr,
                 "TypeError: shuffle() argument must be list, not '%.*s'\n",
                 static_cast<int>(type_name->length()), type_name->buffer());
    std::exit(1);
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

  PyDict::Put(module_dict, ST(name), PyString::NewInstance("random"));
  PyDict::Put(module_dict, ST(package), PyString::NewInstance(""));

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
