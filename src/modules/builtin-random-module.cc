// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include <cinttypes>
#include <cstdint>

#include "include/saauso-maybe.h"
#include "src/execution/exception-utils.h"
#include "src/execution/isolate.h"
#include "src/handles/handles.h"
#include "src/heap/factory.h"
#include "src/modules/builtin-module-utils.h"
#include "src/modules/module-manager.h"
#include "src/objects/klass.h"
#include "src/objects/py-dict.h"
#include "src/objects/py-float.h"
#include "src/objects/py-list.h"
#include "src/objects/py-module.h"
#include "src/objects/py-object.h"
#include "src/objects/py-oddballs.h"
#include "src/objects/py-smi.h"
#include "src/objects/py-string.h"
#include "src/objects/py-tuple.h"
#include "src/runtime/runtime-exceptions.h"
#include "src/runtime/string-table.h"
#include "src/utils/random-number-generator.h"

namespace saauso::internal {

namespace {

constexpr const char* kModuleName = "random";

#define RANDOM_MODULE_FUNC_LIST(V)     \
  V("seed", Random_Seed)               \
  V("random", Random_Random)           \
  V("randint", Random_RandInt)         \
  V("randrange", Random_RandRange)     \
  V("getrandbits", Random_GetRandBits) \
  V("choice", Random_Choice)           \
  V("shuffle", Random_Shuffle)

RandomNumberGenerator* GetRng(Isolate* isolate) {
  return isolate->random_number_generator();
}

void FailArgRangeEmpty(Isolate* isolate, const char* func_name) {
  Runtime_ThrowErrorf(isolate, ExceptionType::kValueError,
                      "empty range for %s()", func_name);
}

// 成功时返回 true 并写入 *out；类型不符时抛 TypeError 并返回 false。
Maybe<int64_t> ExtractSmi(Isolate* isolate,
                          Handle<PyObject> value,
                          const char* func_name) {
  if (IsPySmi(value)) {
    return Maybe<int64_t>(PySmi::ToInt(Handle<PySmi>::cast(value)));
  }

  Handle<PyString> type_name = PyObject::GetTypeName(value, isolate);
  Runtime_ThrowErrorf(isolate, ExceptionType::kTypeError,
                      "%s() argument must be int, not '%s'", func_name,
                      type_name->buffer());

  return kNullMaybe;
}

}  // namespace

/////////////////////////////////////////////////////////////////////////////////

namespace module_impl {

BUILTIN_MODULE_FUNC(Random_Seed) {
  BUILTIN_MODULE_EXPECT_NO_KWARGS_OR_RETURN(isolate, kwargs, kModuleName,
                                            "seed");
  int64_t argc = BUILTIN_MODULE_ARGC(args);
  if (argc > 1) {
    Runtime_ThrowErrorf(isolate, ExceptionType::kTypeError,
                        "%s.seed() takes at most 1 argument (" PRId64 " given)",
                        kModuleName, argc);
    return kNullMaybe;
  }
  uint64_t seed = 0;
  if (argc == 0) {
    seed = RandomNumberGenerator::NowSeed();
  } else {
    Handle<PyObject> a = args->Get(0);
    if (IsPyNone(a)) {
      seed = RandomNumberGenerator::NowSeed();
    } else {
      int64_t val = 0;
      ASSIGN_RETURN_ON_EXCEPTION(isolate, val, ExtractSmi(isolate, a, "seed"));
      seed = static_cast<uint64_t>(val);
    }
  }
  GetRng(isolate)->SetSeed(seed);

  return handle(isolate->py_none_object());
}

BUILTIN_MODULE_FUNC(Random_Random) {
  BUILTIN_MODULE_EXPECT_NO_KWARGS_OR_RETURN(isolate, kwargs, kModuleName,
                                            "random");
  int64_t argc = BUILTIN_MODULE_ARGC(args);
  BUILTIN_MODULE_EXPECT_ARGC_EQ_OR_RETURN(
      argc, 0,
      Runtime_ThrowErrorf(isolate, ExceptionType::kTypeError,
                          "%s.random() takes 0 positional arguments but "
                          "%" PRId64 " were given",
                          kModuleName, argc));

  return PyFloat::New(isolate, GetRng(isolate)->NextDouble01());
}

BUILTIN_MODULE_FUNC(Random_RandInt) {
  BUILTIN_MODULE_EXPECT_NO_KWARGS_OR_RETURN(isolate, kwargs, kModuleName,
                                            "randint");
  int64_t argc = BUILTIN_MODULE_ARGC(args);
  BUILTIN_MODULE_EXPECT_ARGC_EQ_OR_RETURN(
      argc, 2,
      Runtime_ThrowErrorf(isolate, ExceptionType::kTypeError,
                          "%s.randint() takes exactly 2 arguments (%" PRId64
                          " given)",
                          kModuleName, argc));

  int64_t a = 0;
  ASSIGN_RETURN_ON_EXCEPTION(isolate, a,
                             ExtractSmi(isolate, args->Get(0), "randint"));

  int64_t b = 0;
  ASSIGN_RETURN_ON_EXCEPTION(isolate, b,
                             ExtractSmi(isolate, args->Get(1), "randint"));

  if (a > b) {
    FailArgRangeEmpty(isolate, "randint");
    return kNullMaybe;
  }

  const uint64_t bound = static_cast<uint64_t>(b - a) + 1;
  uint64_t offset = GetRng(isolate)->NextU64Bounded(bound);
  int64_t result = a + static_cast<int64_t>(offset);
  return handle(PySmi::FromInt(result));
}

BUILTIN_MODULE_FUNC(Random_RandRange) {
  BUILTIN_MODULE_EXPECT_NO_KWARGS_OR_RETURN(isolate, kwargs, kModuleName,
                                            "randrange");
  int64_t argc = BUILTIN_MODULE_ARGC(args);
  if (argc < 1 || argc > 3) {
    Runtime_ThrowErrorf(isolate, ExceptionType::kTypeError,
                        "%s.randrange() takes from 1 to 3 positional "
                        "arguments but %" PRId64 " were given",
                        kModuleName, argc);
    return kNullMaybe;
  }
  int64_t start = 0;
  int64_t stop = 0;
  int64_t step = 1;

  if (argc == 1) {
    ASSIGN_RETURN_ON_EXCEPTION(isolate, stop,
                               ExtractSmi(isolate, args->Get(0), "randrange"));
  } else {
    ASSIGN_RETURN_ON_EXCEPTION(isolate, start,
                               ExtractSmi(isolate, args->Get(0), "randrange"));
    ASSIGN_RETURN_ON_EXCEPTION(isolate, stop,
                               ExtractSmi(isolate, args->Get(1), "randrange"));
    if (argc == 3) {
      ASSIGN_RETURN_ON_EXCEPTION(
          isolate, step, ExtractSmi(isolate, args->Get(2), "randrange"));
    }
  }

  if (step == 0) {
    Runtime_ThrowError(isolate, ExceptionType::kValueError,
                       "zero step for randrange()");
    return kNullMaybe;
  }

  if (step > 0) {
    if (start >= stop) {
      FailArgRangeEmpty(isolate, "randrange");
      return kNullMaybe;
    }
    uint64_t width = static_cast<uint64_t>(stop - start);
    uint64_t step_u = static_cast<uint64_t>(step);
    uint64_t n = (width + step_u - 1) / step_u;
    uint64_t k = GetRng(isolate)->NextU64Bounded(n);
    int64_t result = start + static_cast<int64_t>(k * step_u);
    return handle(PySmi::FromInt(result));
  }

  if (start <= stop) {
    FailArgRangeEmpty(isolate, "randrange");
    return kNullMaybe;
  }

  // 这里不能直接 `static_cast<uint64_t>(-step)`。
  // 因为如果 step 恰好是 `INT64_MIN`，对它取反就会出现有符号类型的溢出。
  // 在 C++ 中，有符号类型溢出是一个 UB 行为！！！
  // 参见：https://juejin.cn/post/7609924915807043630#heading-7
  uint64_t step_u = static_cast<uint64_t>(0) - static_cast<uint64_t>(step);

  uint64_t width = static_cast<uint64_t>(start - stop);
  uint64_t n = (width + step_u - 1) / step_u;
  uint64_t k = GetRng(isolate)->NextU64Bounded(n);
  int64_t result = start - static_cast<int64_t>(k * step_u);
  return handle(PySmi::FromInt(result));
}

BUILTIN_MODULE_FUNC(Random_GetRandBits) {
  BUILTIN_MODULE_EXPECT_NO_KWARGS_OR_RETURN(isolate, kwargs, kModuleName,
                                            "getrandbits");
  int64_t argc = BUILTIN_MODULE_ARGC(args);
  BUILTIN_MODULE_EXPECT_ARGC_EQ_OR_RETURN(
      argc, 1,
      Runtime_ThrowErrorf(isolate, ExceptionType::kTypeError,
                          "%s.getrandbits() takes exactly 1 argument "
                          "(%" PRId64 " given)",
                          kModuleName, argc));

  int64_t k = 0;
  ASSIGN_RETURN_ON_EXCEPTION(isolate, k,
                             ExtractSmi(isolate, args->Get(0), "getrandbits"));

  if (k < 0) {
    Runtime_ThrowError(isolate, ExceptionType::kValueError,
                       "number of bits must be non-negative");
    return kNullMaybe;
  }
  if (k == 0) {
    return handle(PySmi::FromInt(0));
  }
  // MVP：当前虚拟机没有 big-int，因此仅支持可表示为 Smi 的 bit 数。
  if (k > 62) {
    Runtime_ThrowError(isolate, ExceptionType::kRuntimeError,
                       "getrandbits() k too large for SAAUSO MVP int");
    return kNullMaybe;
  }

  uint64_t mask = (uint64_t{1} << static_cast<uint64_t>(k)) - 1;
  uint64_t x = GetRng(isolate)->NextU64() & mask;
  return handle(PySmi::FromInt(static_cast<int64_t>(x)));
}

BUILTIN_MODULE_FUNC(Random_Choice) {
  BUILTIN_MODULE_EXPECT_NO_KWARGS_OR_RETURN(isolate, kwargs, kModuleName,
                                            "choice");
  int64_t argc = BUILTIN_MODULE_ARGC(args);
  BUILTIN_MODULE_EXPECT_ARGC_EQ_OR_RETURN(
      argc, 1,
      Runtime_ThrowErrorf(isolate, ExceptionType::kTypeError,
                          "%s.choice() takes exactly 1 argument (%" PRId64
                          " given)",
                          kModuleName, argc));
  Handle<PyObject> seq = args->Get(0);

  // TODO:
  // 这种设计在未来引入 bytearray 或用户自定义序列时会违反 DRY 和 OCP 原则。
  // 建议在 VM 运行时层（如 PyObject 或新的 PySequence Helper 中）抽象出
  // 通用的 Runtime_SequenceLength(Isolate*, Handle<PyObject>)
  // 和 Runtime_SequenceGetItem(Isolate*, Handle<PyObject>, int64_t) 接口，
  // 将多态分发下沉，让 Random_Choice 只需面对一套通用 API。
  if (IsPyList(seq)) {
    auto list = Handle<PyList>::cast(seq);
    if (list->length() == 0) {
      Runtime_ThrowError(isolate, ExceptionType::kIndexError,
                         "Cannot choose from an empty sequence");
      return kNullMaybe;
    }
    uint64_t index =
        GetRng(isolate)->NextU64Bounded(static_cast<uint64_t>(list->length()));
    return list->Get(static_cast<int64_t>(index));
  }

  if (IsPyTuple(seq)) {
    auto tuple = Handle<PyTuple>::cast(seq);
    if (tuple->length() == 0) {
      Runtime_ThrowError(isolate, ExceptionType::kIndexError,
                         "Cannot choose from an empty sequence");
      return kNullMaybe;
    }
    uint64_t index =
        GetRng(isolate)->NextU64Bounded(static_cast<uint64_t>(tuple->length()));
    return tuple->Get(static_cast<int64_t>(index));
  }

  if (IsPyString(seq)) {
    auto s = Handle<PyString>::cast(seq);
    if (s->length() == 0) {
      Runtime_ThrowError(isolate, ExceptionType::kIndexError,
                         "Cannot choose from an empty sequence");
      return kNullMaybe;
    }
    uint64_t index =
        GetRng(isolate)->NextU64Bounded(static_cast<uint64_t>(s->length()));
    return PyString::Slice(s, static_cast<int64_t>(index),
                           static_cast<int64_t>(index) + 1, isolate);
  }

  Handle<PyString> type_name = PyObject::GetTypeName(seq, isolate);
  Runtime_ThrowErrorf(isolate, ExceptionType::kTypeError,
                      "%s.choice() unsupported sequence type '%s'", kModuleName,
                      type_name->buffer());
  return kNullMaybe;
}

BUILTIN_MODULE_FUNC(Random_Shuffle) {
  BUILTIN_MODULE_EXPECT_NO_KWARGS_OR_RETURN(isolate, kwargs, kModuleName,
                                            "shuffle");
  int64_t argc = BUILTIN_MODULE_ARGC(args);
  BUILTIN_MODULE_EXPECT_ARGC_EQ_OR_RETURN(
      argc, 1,
      Runtime_ThrowErrorf(isolate, ExceptionType::kTypeError,
                          "%s.shuffle() takes exactly 1 argument (%" PRId64
                          " given)",
                          kModuleName, argc));
  Handle<PyObject> x = args->Get(0);
  if (!IsPyListExact(x, isolate)) {
    Handle<PyString> type_name = PyObject::GetTypeName(x, isolate);
    Runtime_ThrowErrorf(isolate, ExceptionType::kTypeError,
                        "%s.shuffle() argument must be list, not '%s'",
                        kModuleName, type_name->buffer());
    return kNullMaybe;
  }

  auto list = Handle<PyList>::cast(x);
  for (int64_t i = list->length() - 1; i > 0; i--) {
    uint64_t j = GetRng(isolate)->NextU64Bounded(static_cast<uint64_t>(i) + 1);
    if (static_cast<int64_t>(j) == i) {
      continue;
    }
    Handle<PyObject> vi = list->Get(i);
    Handle<PyObject> vj = list->Get(static_cast<int64_t>(j));
    list->Set(i, vj);
    list->Set(static_cast<int64_t>(j), vi);
  }

  return handle(isolate->py_none_object());
}

}  // namespace module_impl

/////////////////////////////////////////////////////////////////////////////////

BUILTIN_MODULE_INIT_FUNC("random", InitRandomModule) {
  EscapableHandleScope scope;

  Handle<PyModule> module;
  ASSIGN_RETURN_ON_EXCEPTION(
      isolate, module,
      BuiltinModuleUtils::NewBuiltinModule(isolate, kModuleName));

  const BuiltinModuleFuncSpec kRandomModuleFuncs[] = {
#define DEFINE_RANDOM_FUNC_SPEC(name, func) \
  {name, &module_impl::BUILTIN_MODULE_FUNC_NAME(func)},
      RANDOM_MODULE_FUNC_LIST(DEFINE_RANDOM_FUNC_SPEC)
#undef DEFINE_RANDOM_FUNC_SPEC
  };
  RETURN_ON_EXCEPTION(
      isolate,
      BuiltinModuleUtils::InstallBuiltinModuleFuncsFromSpec(
          isolate, module, kRandomModuleFuncs,
          BuiltinModuleUtils::BuiltinModuleFuncSpecCount(kRandomModuleFuncs)));

  return scope.Escape(module);
}

}  // namespace saauso::internal
