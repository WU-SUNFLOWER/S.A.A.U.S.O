// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/objects/py-code-object.h"

#include "src/handles/handles.h"
#include "src/heap/heap.h"
#include "src/objects/py-code-object-klass.h"
#include "src/objects/py-string.h"
#include "src/objects/py-tuple.h"
#include "src/runtime/isolate.h"

namespace saauso::internal {

// static
Handle<PyCodeObject> PyCodeObject::NewInstance(int arg_count,
                                               int posonly_arg_count,
                                               int kwonly_arg_count,
                                               int nlocals,
                                               int stack_size,
                                               int flags,
                                               Handle<PyString> bytecodes,
                                               Handle<PyTuple> names,
                                               Handle<PyTuple> consts,
                                               Handle<PyTuple> var_names,
                                               Handle<PyTuple> free_vars,
                                               Handle<PyTuple> cell_vars,
                                               Handle<PyString> file_name,
                                               Handle<PyString> co_name,
                                               int line_no,
                                               Handle<PyString> no_table) {
  Handle<PyCodeObject> object(
      Isolate::Current()->heap()->Allocate<PyCodeObject>(
          Heap::AllocationSpace::kNewSpace));
  PyObject::SetKlass(object, PyCodeObjectKlass::GetInstance());

  object->arg_count_ = arg_count;
  object->posonly_arg_count_ = posonly_arg_count;
  object->kwonly_arg_count_ = kwonly_arg_count;
  object->nlocals_ = nlocals;
  object->stack_size_ = stack_size;
  object->flags_ = flags;

  object->bytecodes_ = *bytecodes;
  object->names_ = *names;
  object->consts_ = *consts;
  object->localsplusnames_ = *var_names;
  object->localspluskinds_ = Tagged<PyObject>::Null();
  object->var_names_ = *var_names;

  object->free_vars_ = *free_vars;
  object->cell_vars_ = *cell_vars;

  object->file_name_ = *file_name;
  object->co_name_ = *co_name;
  object->qual_name_ = *co_name;

  object->line_no_ = line_no;
  object->line_table_ = *no_table;
  object->exception_table_ = Tagged<PyObject>::Null();
  object->no_table_ = *no_table;

  PyObject::SetProperties(*object, Tagged<PyDict>::Null());

  return object;
}

// static
Handle<PyCodeObject> PyCodeObject::NewInstance312(
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
    Handle<PyTuple> var_names,
    Handle<PyTuple> free_vars,
    Handle<PyTuple> cell_vars,
    Handle<PyString> file_name,
    Handle<PyString> co_name,
    Handle<PyString> qual_name,
    int line_no,
    Handle<PyString> line_table,
    Handle<PyString> exception_table) {
  Handle<PyCodeObject> object(
      Isolate::Current()->heap()->Allocate<PyCodeObject>(
          Heap::AllocationSpace::kNewSpace));
  PyObject::SetKlass(object, PyCodeObjectKlass::GetInstance());

  object->arg_count_ = arg_count;
  object->posonly_arg_count_ = posonly_arg_count;
  object->kwonly_arg_count_ = kwonly_arg_count;
  object->nlocals_ = static_cast<int>(localsplusnames->length());
  object->stack_size_ = stack_size;
  object->flags_ = flags;

  object->bytecodes_ = *bytecodes;
  object->names_ = *names;
  object->consts_ = *consts;
  object->localsplusnames_ = *localsplusnames;
  object->localspluskinds_ =
      localspluskinds.IsNull() ? Tagged<PyObject>::Null() : *localspluskinds;
  object->var_names_ = *var_names;

  object->free_vars_ = *free_vars;
  object->cell_vars_ = *cell_vars;

  object->file_name_ = *file_name;
  object->co_name_ = *co_name;
  object->qual_name_ = *qual_name;

  object->line_no_ = line_no;
  object->line_table_ =
      line_table.IsNull() ? Tagged<PyObject>::Null() : *line_table;
  object->exception_table_ =
      exception_table.IsNull() ? Tagged<PyObject>::Null() : *exception_table;
  object->no_table_ = object->line_table_;

  PyObject::SetProperties(*object, Tagged<PyDict>::Null());

  return object;
}

// static
Tagged<PyCodeObject> PyCodeObject::cast(Tagged<PyObject> object) {
  assert(IsPyCodeObject(object));
  return Tagged<PyCodeObject>::cast(object);
}

};  // namespace saauso::internal
