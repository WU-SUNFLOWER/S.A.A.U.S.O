// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/runtime/runtime-intrinsics.h"

#include "src/execution/isolate.h"
#include "src/objects/py-dict.h"
#include "src/objects/py-list.h"
#include "src/objects/py-object.h"
#include "src/objects/py-string.h"
#include "src/objects/py-tuple.h"
#include "src/runtime/runtime-exceptions.h"
#include "src/runtime/string-table.h"

namespace saauso::internal {

MaybeHandle<PyTuple> Runtime_IntrinsicListToTuple(Handle<PyObject> object) {
  EscapableHandleScope scope;

  if (!IsPyList(object)) {
    Runtime_ThrowTypeError("INTRINSIC_LIST_TO_TUPLE expected a list");
    return kNullMaybeHandle;
  }

  auto list = Handle<PyList>::cast(object);
  auto tuple = PyTuple::NewInstance(list->length());
  for (auto i = 0; i < list->length(); ++i) {
    tuple->SetInternal(i, list->Get(i));
  }
  return scope.Escape(tuple);
}

void Runtime_IntrinsicImportStar(Handle<PyObject> module,
                                 Handle<PyDict> locals) {
  if (locals.is_null()) [[unlikely]] {
    Runtime_ThrowRuntimeError("no locals for import *");
    return;
  }

  Handle<PyDict> module_dict = PyObject::GetProperties(module);
  if (module_dict.is_null()) [[unlikely]] {
    Runtime_ThrowTypeError("module has no __dict__");
    return;
  }

  auto import_name = [&](Handle<PyObject> name_obj,
                         bool ignore_private_member) {
    if (!IsPyString(name_obj)) [[unlikely]] {
      Runtime_ThrowTypeError("import * name must be a string");
      return;
    }

    auto name = Handle<PyString>::cast(name_obj);
    if (!ignore_private_member && name->length() > 0 && name->Get(0) == '_') {
      return;
    }

    Handle<PyObject> value = module_dict->Get(name_obj);
    if (!value.is_null()) {
      PyDict::Put(locals, name_obj, value);
    }
  };

  // 优先尝试根据__all__定向导入子模块
  Handle<PyObject> all = module_dict->Get(ST(all));
  if (!all.is_null()) {
    if (IsPyTuple(all)) {
      auto names = Handle<PyTuple>::cast(all);
      for (int64_t i = 0; i < names->length(); ++i) {
        import_name(names->Get(i), false);
        if (Isolate::Current()->exception_state()->HasPendingException()) {
          return;
        }
      }
    } else if (IsPyList(all)) {
      auto names = Handle<PyList>::cast(all);
      for (int64_t i = 0; i < names->length(); ++i) {
        import_name(names->Get(i), false);
        if (Isolate::Current()->exception_state()->HasPendingException()) {
          return;
        }
      }
    } else {
      Runtime_ThrowTypeError("__all__ must be a tuple or list");
      return;
    }
    return;
  }

  // 没有显式的__all__，那么无脑遍历dict并批量导入
  Handle<PyTuple> keys = PyDict::GetKeyTuple(module_dict);
  for (int64_t i = 0; i < keys->length(); ++i) {
    import_name(keys->Get(i), true);
    if (Isolate::Current()->exception_state()->HasPendingException()) {
      return;
    }
  }
}

}  // namespace saauso::internal
