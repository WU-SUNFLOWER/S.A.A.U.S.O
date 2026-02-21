// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/builtins/builtins-py-list-methods.h"

#include <algorithm>
#include <cstdio>
#include <vector>

#include "src/execution/execution.h"
#include "src/execution/isolate.h"
#include "src/handles/handles.h"
#include "src/objects/fixed-array.h"
#include "src/objects/klass.h"
#include "src/objects/py-dict.h"
#include "src/objects/py-function.h"
#include "src/objects/py-list.h"
#include "src/objects/py-object.h"
#include "src/objects/py-oddballs.h"
#include "src/objects/py-smi.h"
#include "src/objects/py-string.h"
#include "src/objects/py-tuple.h"
#include "src/runtime/runtime-iterable.h"
#include "src/runtime/runtime-truthiness.h"
#include "src/utils/stable-merge-sort.h"

namespace saauso::internal {

namespace {

int64_t DecodeIntLikeOrDie(Handle<PyObject> value) {
  if (IsPySmi(value)) {
    return PySmi::ToInt(Handle<PySmi>::cast(value));
  }
  if (IsPyBoolean(value)) {
    return Handle<PyBoolean>::cast(value)->value() ? 1 : 0;
  }

  auto type_name = PyObject::GetKlass(value)->name();
  std::fprintf(stderr,
               "TypeError: '%.*s' object cannot be interpreted as an integer\n",
               static_cast<int>(type_name->length()), type_name->buffer());
  std::exit(1);
}

void PrintObjectForError(FILE* out, Handle<PyObject> value) {
  if (IsPySmi(value)) {
    std::fprintf(
        out, "%lld",
        static_cast<long long>(PySmi::ToInt(Handle<PySmi>::cast(value))));
    return;
  }
  if (IsPyBoolean(value)) {
    std::fprintf(out, "%s",
                 Handle<PyBoolean>::cast(value)->value() ? "True" : "False");
    return;
  }
  if (IsPyNone(value)) {
    std::fprintf(out, "None");
    return;
  }
  if (IsPyString(value)) {
    auto s = Handle<PyString>::cast(value);
    std::fprintf(out, "%.*s", static_cast<int>(s->length()), s->buffer());
    return;
  }

  PyObject::Print(value);
}

}  // namespace

void PyListBuiltinMethods::Install(Handle<PyDict> target) {
  PY_LIST_BUILTINS(INSTALL_BUILTIN_METHOD);
}

////////////////////////////////////////////////////////////////////////

BUILTIN_METHOD(PyListBuiltinMethods, Append) {
  EscapableHandleScope scope;
  auto object = Handle<PyList>::cast(self);
  PyList::Append(object, args->Get(0));
  return scope.Escape(handle(Isolate::Current()->py_none_object()));
}

BUILTIN_METHOD(PyListBuiltinMethods, Pop) {
  EscapableHandleScope scope;
  auto object = Handle<PyList>::cast(self);
  if (object->IsEmpty()) {
    std::fprintf(stderr, "IndexError: pop from empty list\n");
    std::exit(1);
  }
  return scope.Escape(object->Pop());
}

BUILTIN_METHOD(PyListBuiltinMethods, Insert) {
  EscapableHandleScope scope;
  auto object = Handle<PyList>::cast(self);
  auto index = Handle<PySmi>::cast(args->Get(0));
  PyList::Insert(object, PySmi::ToInt(index), args->Get(1));
  return scope.Escape(handle(Isolate::Current()->py_none_object()));
}

BUILTIN_METHOD(PyListBuiltinMethods, Index) {
  EscapableHandleScope scope;
  auto list = Handle<PyList>::cast(self);

  if (!kwargs.is_null() && kwargs->occupied() != 0) {
    std::fprintf(stderr,
                 "TypeError: list.index() takes no keyword arguments\n");
    std::exit(1);
  }

  int64_t argc = args.is_null() ? 0 : args->length();
  if (argc < 1) {
    std::fprintf(
        stderr,
        "TypeError: list.index() takes at least 1 argument (%lld given)\n",
        static_cast<long long>(argc));
    std::exit(1);
  }
  if (argc > 3) {
    std::fprintf(
        stderr,
        "TypeError: list.index() takes at most 3 arguments (%lld given)\n",
        static_cast<long long>(argc));
    std::exit(1);
  }

  auto target = args->Get(0);

  int64_t length = list->length();
  int64_t begin = 0;
  int64_t end = length;

  if (argc >= 2) {
    begin = DecodeIntLikeOrDie(args->Get(1));
  }
  if (argc >= 3) {
    end = DecodeIntLikeOrDie(args->Get(2));
  }

  if (begin < 0) {
    begin += length;
  }
  if (end < 0) {
    end += length;
  }

  begin = std::min(std::max(static_cast<int64_t>(0), begin), length);
  end = std::min(std::max(static_cast<int64_t>(0), end), length);

  int64_t result = PyList::kNotFound;
  if (begin <= end) {
    result = list->IndexOf(target, begin, end);
  }
  if (result == PyList::kNotFound) {
    std::fprintf(stderr, "ValueError: ");
    PrintObjectForError(stderr, target);
    std::fprintf(stderr, " is not in list\n");
    std::exit(1);
  }

  return scope.Escape(handle(PySmi::FromInt(result)));
}

BUILTIN_METHOD(PyListBuiltinMethods, Reverse) {
  EscapableHandleScope scope;
  auto list = Handle<PyList>::cast(self);

  auto length = list->length();
  for (auto i = 0; i < (length >> 1); ++i) {
    Handle<PyObject> tmp = list->Get(i);
    list->Set(i, list->Get(length - i - 1));
    list->Set(length - i - 1, tmp);
  }

  return scope.Escape(handle(Isolate::Current()->py_none_object()));
}

BUILTIN_METHOD(PyListBuiltinMethods, Extend) {
  Runtime_ExtendListByItratableObject(Handle<PyList>::cast(self), args->Get(0));
  return handle(Isolate::Current()->py_none_object());
}

BUILTIN_METHOD(PyListBuiltinMethods, Sort) {
  EscapableHandleScope scope;

  if (!args.is_null() && args->length() != 0) {
    std::fprintf(stderr, "TypeError: sort() takes no positional arguments\n");
    std::exit(1);
  }

  auto list = Handle<PyList>::cast(self);
  int64_t expected_length = list->length();
  if (expected_length <= 1) {
    return scope.Escape(handle(Isolate::Current()->py_none_object()));
  }

  Handle<PyObject> key_func = handle(Isolate::Current()->py_none_object());
  bool reverse = false;

  if (!kwargs.is_null() && kwargs->occupied() != 0) {
    Handle<PyString> key_name = PyString::NewInstance("key");
    Handle<PyString> reverse_name = PyString::NewInstance("reverse");

    for (int64_t i = 0; i < kwargs->capacity(); ++i) {
      Handle<PyObject> k = kwargs->KeyAtIndex(i);
      if (k.is_null()) {
        continue;
      }
      if (!IsPyString(*k)) {
        std::fprintf(stderr, "TypeError: sort() keywords must be strings\n");
        std::exit(1);
      }
      auto key_str = Handle<PyString>::cast(k);
      if (!key_str->IsEqualTo(*key_name) &&
          !key_str->IsEqualTo(*reverse_name)) {
        std::fprintf(stderr, "TypeError: sort() got an unexpected keyword\n");
        std::exit(1);
      }
    }

    Handle<PyObject> value = kwargs->Get(key_name);
    if (!value.is_null()) {
      key_func = value;
    }

    value = kwargs->Get(reverse_name);
    if (!value.is_null()) {
      reverse = Runtime_PyObjectIsTrue(value);
    }
  }

  if (!IsPyNone(*key_func) && !IsNormalPyFunction(key_func) &&
      !IsPyNativeFunction(*key_func) && !IsMethodObject(*key_func)) {
    std::fprintf(stderr, "TypeError: key must be callable\n");
    std::exit(1);
  }

  Handle<FixedArray> keys = FixedArray::NewInstance(expected_length);

  if (IsPyNone(*key_func)) {
    for (int64_t i = 0; i < expected_length; ++i) {
      keys->Set(i, list->Get(i));
    }
  } else {
    Handle<PyTuple> key_args = PyTuple::NewInstance(1);
    Handle<PyDict> empty_kwargs = PyDict::NewInstance();

    for (int64_t i = 0; i < expected_length; ++i) {
      if (list->length() != expected_length) {
        std::fprintf(stderr, "ValueError: list modified during sort (key)\n");
        std::exit(1);
      }
      Handle<PyObject> elem = list->Get(i);
      key_args->SetInternal(0, elem);
      Handle<PyObject> key;
      if (!Execution::Call(Isolate::Current(), key_func,
                           Handle<PyObject>::null(), key_args, empty_kwargs)
               .ToHandle(&key)) {
        return Handle<PyObject>::null();
      }
      keys->Set(i, key);
    }
  }

  std::vector<int64_t> indices(static_cast<size_t>(expected_length));
  for (int64_t i = 0; i < expected_length; ++i) {
    indices[static_cast<size_t>(i)] = i;
  }

  struct CompareContext {
    Handle<PyList> list;
    Handle<FixedArray> keys;
    int64_t expected_length;
  };

  CompareContext context{list, keys, expected_length};

  auto less = [](int64_t a, int64_t b, void* ctx) -> bool {
    auto* c = static_cast<CompareContext*>(ctx);
    if (c->list->length() != c->expected_length) {
      std::fprintf(stderr, "ValueError: list modified during sort\n");
      std::exit(1);
    }
    HandleScope scope;
    Handle<PyObject> ka = handle(c->keys->Get(a));
    Handle<PyObject> kb = handle(c->keys->Get(b));
    return PyObject::LessBool(ka, kb);
  };

  StableMergeSort::Sort(indices.data(), expected_length, less, &context);

  Handle<FixedArray> tmp = FixedArray::NewInstance(expected_length);
  for (int64_t i = 0; i < expected_length; ++i) {
    tmp->Set(i, list->Get(indices[static_cast<size_t>(i)]));
  }
  for (int64_t i = 0; i < expected_length; ++i) {
    list->Set(i, handle(tmp->Get(i)));
  }

  if (reverse) {
    for (int64_t i = 0; i < (expected_length >> 1); ++i) {
      Handle<PyObject> t = list->Get(i);
      list->Set(i, list->Get(expected_length - i - 1));
      list->Set(expected_length - i - 1, t);
    }
  }

  return scope.Escape(handle(Isolate::Current()->py_none_object()));
}

}  // namespace saauso::internal
