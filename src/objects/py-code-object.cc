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
  HandleScope scope(isolate);

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
  EscapableHandleScope scope(isolate);

  Handle<PyCodeObject> object = isolate->factory()->NewPyCodeObject();

  object->arg_count_ = arg_count;
  object->posonly_arg_count_ = posonly_arg_count;
  object->kwonly_arg_count_ = kwonly_arg_count;
  object->stack_size_ = stack_size;
  object->flags_ = flags;

  object->set_bytecodes(bytecodes);
  object->set_names(names);
  object->set_consts(consts);

  object->set_localsplusnames(localsplusnames);
  object->set_localspluskinds(localspluskinds);
  assert(!localsplusnames.is_null());
  assert(!localspluskinds.is_null());
  assert(localsplusnames->length() == localspluskinds->length());

  object->nlocalsplus_ = static_cast<int>(localsplusnames->length());
  ComputeLocalsplusCounts(object->nlocalsplus_, object->localspluskinds_,
                          object->nlocals_, object->ncellvars_,
                          object->nfreevars_);

  object->set_var_names(Tagged<PyTuple>::cast(GetNamesFromLocalsplusNames(
      isolate, object, LocalsPlusKind::kFastLocal, object->nlocals_)));
  object->set_cell_vars(Tagged<PyTuple>::cast(GetNamesFromLocalsplusNames(
      isolate, object, LocalsPlusKind::kFastCell, object->ncellvars_)));
  object->set_free_vars(Tagged<PyTuple>::cast(GetNamesFromLocalsplusNames(
      isolate, object, LocalsPlusKind::kFastFree, object->nfreevars_)));

  object->set_file_name(file_name);
  object->set_co_name(co_name);
  object->set_qual_name(qual_name);

  object->line_no_ = line_no;
  object->set_line_table(line_table.is_null() ? Tagged<PyString>::null()
                                              : *line_table);
  object->set_exception_table(exception_table.is_null()
                                  ? Tagged<PyString>::null()
                                  : *exception_table);
  object->set_no_table(line_table.is_null() ? Tagged<PyString>::null()
                                            : *line_table);

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

void PyCodeObject::set_bytecodes(Handle<PyString> bytecodes) {
  set_bytecodes(*bytecodes);
}

void PyCodeObject::set_bytecodes(Tagged<PyString> bytecodes) {
  bytecodes_ = bytecodes;
  WRITE_BARRIER(Tagged<PyObject>(this), &bytecodes_, bytecodes);
}

Handle<PyTuple> PyCodeObject::names(Isolate* isolate) const {
  return handle(Tagged<PyTuple>::cast(names_), isolate);
}

void PyCodeObject::set_names(Handle<PyTuple> names) {
  set_names(*names);
}

void PyCodeObject::set_names(Tagged<PyTuple> names) {
  names_ = names;
  WRITE_BARRIER(Tagged<PyObject>(this), &names_, names);
}

Handle<PyTuple> PyCodeObject::var_names(Isolate* isolate) const {
  return handle(Tagged<PyTuple>::cast(var_names_), isolate);
}

void PyCodeObject::set_var_names(Handle<PyTuple> var_names) {
  set_var_names(*var_names);
}

void PyCodeObject::set_var_names(Tagged<PyTuple> var_names) {
  var_names_ = var_names;
  WRITE_BARRIER(Tagged<PyObject>(this), &var_names_, var_names);
}

Handle<PyTuple> PyCodeObject::free_vars(Isolate* isolate) const {
  return handle(Tagged<PyTuple>::cast(free_vars_), isolate);
}

void PyCodeObject::set_free_vars(Handle<PyTuple> free_vars) {
  set_free_vars(*free_vars);
}

void PyCodeObject::set_free_vars(Tagged<PyTuple> free_vars) {
  free_vars_ = free_vars;
  WRITE_BARRIER(Tagged<PyObject>(this), &free_vars_, free_vars);
}

Handle<PyTuple> PyCodeObject::cell_vars(Isolate* isolate) const {
  return handle(Tagged<PyTuple>::cast(cell_vars_), isolate);
}

void PyCodeObject::set_cell_vars(Handle<PyTuple> cell_vars) {
  set_cell_vars(*cell_vars);
}

void PyCodeObject::set_cell_vars(Tagged<PyTuple> cell_vars) {
  cell_vars_ = cell_vars;
  WRITE_BARRIER(Tagged<PyObject>(this), &cell_vars_, cell_vars);
}

Handle<PyTuple> PyCodeObject::consts(Isolate* isolate) const {
  return handle(Tagged<PyTuple>::cast(consts_), isolate);
}

void PyCodeObject::set_consts(Handle<PyTuple> consts) {
  set_consts(*consts);
}

void PyCodeObject::set_consts(Tagged<PyTuple> consts) {
  consts_ = consts;
  WRITE_BARRIER(Tagged<PyObject>(this), &consts_, consts);
}

Handle<PyString> PyCodeObject::co_name(Isolate* isolate) const {
  return handle(Tagged<PyString>::cast(co_name_), isolate);
}

void PyCodeObject::set_co_name(Handle<PyString> co_name) {
  set_co_name(*co_name);
}

void PyCodeObject::set_co_name(Tagged<PyString> co_name) {
  co_name_ = co_name;
  WRITE_BARRIER(Tagged<PyObject>(this), &co_name_, co_name);
}

Handle<PyString> PyCodeObject::file_name(Isolate* isolate) const {
  return handle(Tagged<PyString>::cast(file_name_), isolate);
}

void PyCodeObject::set_file_name(Handle<PyString> file_name) {
  set_file_name(*file_name);
}

void PyCodeObject::set_file_name(Tagged<PyString> file_name) {
  file_name_ = file_name;
  WRITE_BARRIER(Tagged<PyObject>(this), &file_name_, file_name);
}

Handle<PyTuple> PyCodeObject::localsplusnames(Isolate* isolate) const {
  return handle(Tagged<PyTuple>::cast(localsplusnames_), isolate);
}

void PyCodeObject::set_localsplusnames(Handle<PyTuple> localsplusnames) {
  set_localsplusnames(*localsplusnames);
}

void PyCodeObject::set_localsplusnames(Tagged<PyTuple> localsplusnames) {
  localsplusnames_ = localsplusnames;
  WRITE_BARRIER(Tagged<PyObject>(this), &localsplusnames_, localsplusnames);
}

Handle<PyString> PyCodeObject::localspluskinds(Isolate* isolate) const {
  return handle(Tagged<PyString>::cast(localspluskinds_), isolate);
}

void PyCodeObject::set_localspluskinds(Handle<PyString> localspluskinds) {
  set_localspluskinds(*localspluskinds);
}

void PyCodeObject::set_localspluskinds(Tagged<PyString> localspluskinds) {
  localspluskinds_ = localspluskinds;
  WRITE_BARRIER(Tagged<PyObject>(this), &localspluskinds_, localspluskinds);
}

Handle<PyString> PyCodeObject::exception_table(Isolate* isolate) const {
  return handle(Tagged<PyString>::cast(exception_table_), isolate);
}

void PyCodeObject::set_line_table(Handle<PyString> line_table) {
  set_line_table(*line_table);
}

void PyCodeObject::set_line_table(Tagged<PyString> line_table) {
  line_table_ = line_table;
  WRITE_BARRIER(Tagged<PyObject>(this), &line_table_, line_table);
}

void PyCodeObject::set_exception_table(Handle<PyString> exception_table) {
  set_exception_table(*exception_table);
}

void PyCodeObject::set_exception_table(Tagged<PyString> exception_table) {
  exception_table_ = exception_table;
  WRITE_BARRIER(Tagged<PyObject>(this), &exception_table_, exception_table);
}

void PyCodeObject::set_qual_name(Handle<PyString> qual_name) {
  set_qual_name(*qual_name);
}

void PyCodeObject::set_qual_name(Tagged<PyString> qual_name) {
  qual_name_ = qual_name;
  WRITE_BARRIER(Tagged<PyObject>(this), &qual_name_, qual_name);
}

void PyCodeObject::set_no_table(Handle<PyString> no_table) {
  set_no_table(*no_table);
}

void PyCodeObject::set_no_table(Tagged<PyString> no_table) {
  no_table_ = no_table;
  WRITE_BARRIER(Tagged<PyObject>(this), &no_table_, no_table);
}

};  // namespace saauso::internal
