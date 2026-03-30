// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/objects/py-code-object.h"

#include <cassert>

#include "src/execution/isolate.h"
#include "src/handles/handles.h"
#include "src/heap/factory.h"
#include "src/objects/py-code-object-klass.h"
#include "src/objects/py-dict.h"
#include "src/objects/py-string.h"
#include "src/objects/py-tuple.h"

namespace saauso::internal {

// 参考CPython312源码 Objects/codeobject.c

namespace {
// 计算 localsplus 中各 kind 的计数
void ComputeLocalsplusCounts(int nlocalsplus,
                             Tagged<PyObject> kinds,
                             int& nlocals,
                             int& ncellvars,
                             int& nfreevars) {
  nlocals = ncellvars = nfreevars = 0;

  auto kind_bytes = Tagged<PyString>::cast(kinds);
  [[maybe_unused]] auto kind_bytes_size =
      kind_bytes.is_null() ? 0 : kind_bytes->length();
  assert(kind_bytes_size == nlocalsplus);

  for (auto i = 0; i < nlocalsplus; ++i) {
    char kind = kind_bytes->Get(i);
    if (kind & PyCodeObject::LocalsPlusKind::kFastLocal) {
      ++nlocals;
      if (kind & PyCodeObject::LocalsPlusKind::kFastCell) {
        ++ncellvars;
      }
    } else if (kind & PyCodeObject::LocalsPlusKind::kFastCell) {
      ++ncellvars;
    } else if (kind & PyCodeObject::LocalsPlusKind::kFastFree) {
      ++nfreevars;
    }
  }
}

Tagged<PyObject> GetNamesFromLocalsplusNames(
    Isolate* isolate,
    Handle<PyCodeObject> code_object,
    PyCodeObject::LocalsPlusKind target_kind,
    int num) {
  HandleScope scope;

  Handle<PyTuple> localsplusnames = code_object->localsplusnames(isolate);
  Handle<PyString> localspluskinds = code_object->localspluskinds(isolate);

  Handle<PyTuple> result_names = PyTuple::New(isolate, num);
  int result_index = 0;

  for (auto i = 0; i < code_object->nlocalsplus(); ++i) {
    char kind = localspluskinds->Get(i);
    if ((kind & target_kind) == 0) {
      continue;
    }

    Tagged<PyObject> name = localsplusnames->GetTagged(i);
    assert(result_index < num);
    result_names->SetInternal(result_index, name);
    ++result_index;
  }

  assert(result_names->length() == num);
  return *result_names;
}
}  // namespace

// static
Handle<PyCodeObject> PyCodeObject::New(Isolate* isolate,
                                       int arg_count,
                                       int posonly_arg_count,
                                       int kwonly_arg_count,
                                       int stack_size,
                                       int flags,
                                       Handle<PyString> bytecodes,
                                       Handle<PyTuple> consts,
                                       Handle<PyTuple> names,
                                       Handle<PyTuple> localsplusnames,
                                       Handle<PyString> localspluskinds,
                                       Handle<PyString> file_name,
                                       Handle<PyString> co_name,
                                       Handle<PyString> qual_name,
                                       int line_no,
                                       Handle<PyString> line_table,
                                       Handle<PyString> exception_table) {
  EscapableHandleScope scope;

  Handle<PyCodeObject> object = isolate->factory()->NewPyCodeObject();

  object->arg_count_ = arg_count;
  object->posonly_arg_count_ = posonly_arg_count;
  object->kwonly_arg_count_ = kwonly_arg_count;
  object->stack_size_ = stack_size;
  object->flags_ = flags;

  object->bytecodes_ = *bytecodes;
  object->names_ = *names;
  object->consts_ = *consts;

  object->localsplusnames_ = *localsplusnames;
  object->localspluskinds_ = *localspluskinds;
  assert(!localsplusnames.is_null());
  assert(!localspluskinds.is_null());
  assert(localsplusnames->length() == localspluskinds->length());

  object->nlocalsplus_ = static_cast<int>(localsplusnames->length());
  ComputeLocalsplusCounts(object->nlocalsplus_, object->localspluskinds_,
                          object->nlocals_, object->ncellvars_,
                          object->nfreevars_);

  object->var_names_ = GetNamesFromLocalsplusNames(
      isolate, object, LocalsPlusKind::kFastLocal, object->nlocals_);
  object->cell_vars_ = GetNamesFromLocalsplusNames(
      isolate, object, LocalsPlusKind::kFastCell, object->ncellvars_);
  object->free_vars_ = GetNamesFromLocalsplusNames(
      isolate, object, LocalsPlusKind::kFastFree, object->nfreevars_);

  object->file_name_ = *file_name;
  object->co_name_ = *co_name;
  object->qual_name_ = *qual_name;

  object->line_no_ = line_no;
  object->line_table_ =
      line_table.is_null() ? Tagged<PyObject>::null() : *line_table;
  object->exception_table_ =
      exception_table.is_null() ? Tagged<PyObject>::null() : *exception_table;
  object->no_table_ = object->line_table_;

  return scope.Escape(object);
}

// static
Tagged<PyCodeObject> PyCodeObject::cast(Tagged<PyObject> object) {
  assert(IsPyCodeObject(object));
  return Tagged<PyCodeObject>::cast(object);
}

Handle<PyString> PyCodeObject::bytecodes(Isolate* isolate) const {
  return handle(Tagged<PyString>::cast(bytecodes_), isolate);
}

Handle<PyTuple> PyCodeObject::names(Isolate* isolate) const {
  return handle(Tagged<PyTuple>::cast(names_), isolate);
}

Handle<PyTuple> PyCodeObject::var_names(Isolate* isolate) const {
  return handle(Tagged<PyTuple>::cast(var_names_), isolate);
}

Handle<PyTuple> PyCodeObject::free_vars(Isolate* isolate) const {
  return handle(Tagged<PyTuple>::cast(free_vars_), isolate);
}

Handle<PyTuple> PyCodeObject::cell_vars(Isolate* isolate) const {
  return handle(Tagged<PyTuple>::cast(cell_vars_), isolate);
}

Handle<PyTuple> PyCodeObject::consts(Isolate* isolate) const {
  return handle(Tagged<PyTuple>::cast(consts_), isolate);
}

Handle<PyString> PyCodeObject::co_name(Isolate* isolate) const {
  return handle(Tagged<PyString>::cast(co_name_), isolate);
}

Handle<PyString> PyCodeObject::file_name(Isolate* isolate) const {
  return handle(Tagged<PyString>::cast(file_name_), isolate);
}

Handle<PyTuple> PyCodeObject::localsplusnames(Isolate* isolate) const {
  return handle(Tagged<PyTuple>::cast(localsplusnames_), isolate);
}

Handle<PyString> PyCodeObject::localspluskinds(Isolate* isolate) const {
  return handle(Tagged<PyString>::cast(localspluskinds_), isolate);
}

Handle<PyString> PyCodeObject::exception_table(Isolate* isolate) const {
  return handle(Tagged<PyString>::cast(exception_table_), isolate);
}

};  // namespace saauso::internal
