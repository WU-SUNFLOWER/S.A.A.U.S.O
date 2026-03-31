// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/runtime/runtime-intrinsics.h"

#include "src/execution/exception-utils.h"
#include "src/execution/isolate.h"
#include "src/heap/factory.h"
#include "src/objects/py-dict.h"
#include "src/objects/py-list.h"
#include "src/objects/py-object.h"
#include "src/objects/py-oddballs.h"
#include "src/objects/py-string.h"
#include "src/objects/py-tuple.h"
#include "src/runtime/runtime-exceptions.h"
#include "src/runtime/string-table.h"

namespace saauso::internal {

namespace {

Maybe<bool> ImportNameImpl(Isolate* isolate,
                           Handle<PyDict> module_dict,
                           Handle<PyDict> locals,
                           Handle<PyObject> name_obj,
                           bool ignore_private_member) {
  if (!IsPyString(name_obj)) [[unlikely]] {
    Runtime_ThrowError(isolate, ExceptionType::kTypeError,
                       "import * name must be a string");
    return kNullMaybe;
  }

  auto name = Handle<PyString>::cast(name_obj);
  if (ignore_private_member && name->length() > 0 && name->Get(0) == '_') {
    return Maybe<bool>(false);
  }

  Handle<PyObject> value;
  bool found = false;
  if (!module_dict->Get(name, value, isolate).To(&found)) {
    return kNullMaybe;
  }
  if (found) {
    assert(!value.is_null());
    RETURN_ON_EXCEPTION(isolate, PyDict::Put(locals, name, value, isolate));
    return Maybe<bool>(true);
  }

  return Maybe<bool>(false);
}

Maybe<bool> ImportModulesByAllImpl(Isolate* isolate,
                                   Handle<PyObject> all,
                                   Handle<PyDict> module_dict,
                                   Handle<PyDict> locals) {
  if (IsPyTuple(all)) {
    auto names = Handle<PyTuple>::cast(all);
    for (int64_t i = 0; i < names->length(); ++i) {
      RETURN_ON_EXCEPTION(isolate, ImportNameImpl(isolate, module_dict, locals,
                                                  names->Get(i), false));
    }
  } else if (IsPyList(all)) {
    auto names = Handle<PyList>::cast(all);
    for (int64_t i = 0; i < names->length(); ++i) {
      RETURN_ON_EXCEPTION(isolate, ImportNameImpl(isolate, module_dict, locals,
                                                  names->Get(i), false));
    }
  } else {
    Runtime_ThrowError(isolate, ExceptionType::kTypeError,
                       "__all__ must be a tuple or list");
    return kNullMaybe;
  }

  return Maybe<bool>(true);
}

}  // namespace

MaybeHandle<PyTuple> Runtime_IntrinsicListToTuple(Isolate* isolate,
                                                  Handle<PyObject> object) {
  EscapableHandleScope scope;

  if (!IsPyList(object)) {
    Runtime_ThrowError(isolate, ExceptionType::kTypeError,
                       "INTRINSIC_LIST_TO_TUPLE expected a list");
    return kNullMaybeHandle;
  }

  auto list = Handle<PyList>::cast(object);
  auto tuple = PyTuple::New(isolate, list->length());
  for (auto i = 0; i < list->length(); ++i) {
    tuple->SetInternal(i, list->Get(i));
  }
  return scope.Escape(tuple);
}

MaybeHandle<PyObject> Runtime_IntrinsicImportStar(Isolate* isolate,
                                                  Handle<PyObject> module,
                                                  Handle<PyDict> locals) {
  if (locals.is_null()) [[unlikely]] {
    Runtime_ThrowError(isolate, ExceptionType::kRuntimeError,
                       "no locals for import *");
    return kNullMaybeHandle;
  }

  Handle<PyDict> module_dict = PyObject::GetProperties(module);
  if (module_dict.is_null()) [[unlikely]] {
    Runtime_ThrowError(isolate, ExceptionType::kTypeError,
                       "module has no __dict__");
    return kNullMaybeHandle;
  }

  Handle<PyObject> all;
  bool found = false;
  ASSIGN_RETURN_ON_EXCEPTION(isolate, found,
                             module_dict->Get(ST(all, isolate), all, isolate));

  if (!found) {
    all = Handle<PyObject>::null();
  }

  // 1. 如果存在__all__，那么据此定向导入子模块
  // 2. 没有显式的__all__，那么无脑遍历dict并批量导入
  if (!all.is_null()) {
    RETURN_ON_EXCEPTION(
        isolate, ImportModulesByAllImpl(isolate, all, module_dict, locals));
  } else {
    Handle<PyTuple> keys = PyDict::GetKeyTuple(module_dict, isolate);
    for (int64_t i = 0; i < keys->length(); ++i) {
      RETURN_ON_EXCEPTION(isolate, ImportNameImpl(isolate, module_dict, locals,
                                                  keys->Get(i), true));
    }
  }

  return isolate->factory()->py_none_object();
}

}  // namespace saauso::internal
