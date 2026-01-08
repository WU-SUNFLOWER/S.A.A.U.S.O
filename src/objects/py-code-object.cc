// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/objects/py-code-object.h"

#include "src/handles/handles.h"
#include "src/heap/heap.h"
#include "src/objects/py-code-object-klass.h"
#include "src/objects/py-list.h"
#include "src/objects/py-string.h"
#include "src/runtime/universe.h"

namespace saauso::internal {

// static
Handle<PyCodeObject> PyCodeObject::NewInstance(int arg_count,
                                               int posonly_arg_count,
                                               int kwonly_arg_count,
                                               int nlocals,
                                               int stack_size,
                                               int flags,
                                               Handle<PyString> bytecodes,
                                               Handle<PyList> names,
                                               Handle<PyList> consts,
                                               Handle<PyList> var_names,
                                               Handle<PyList> free_vars,
                                               Handle<PyList> cell_vars,
                                               Handle<PyString> file_name,
                                               Handle<PyString> co_name,
                                               int line_no,
                                               Handle<PyString> no_table) {
  Handle<PyCodeObject> object(Universe::heap_->Allocate<PyCodeObject>(
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
  object->var_names_ = *var_names;

  object->free_vars_ = *free_vars;
  object->cell_vars_ = *cell_vars;

  object->file_name_ = *file_name;
  object->co_name_ = *co_name;

  object->line_no_ = line_no;
  object->no_table_ = *no_table;

  return object;
}

// static
Tagged<PyCodeObject> PyCodeObject::Cast(Tagged<PyObject> object) {
  assert(IsPyCodeObject(object));
  return Tagged<PyCodeObject>::Cast(object);
}

};  // namespace saauso::internal
