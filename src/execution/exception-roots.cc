// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/execution/exception-roots.h"

#include "src/execution/exception-utils.h"
#include "src/execution/isolate.h"
#include "src/heap/factory.h"
#include "src/objects/fixed-array.h"
#include "src/objects/py-base-exception-klass.h"
#include "src/objects/py-dict.h"
#include "src/objects/py-list.h"
#include "src/objects/py-string.h"
#include "src/objects/py-type-object.h"
#include "src/objects/visitors.h"
#include "src/runtime/runtime-reflection.h"
#include "src/runtime/string-table.h"

namespace saauso::internal {

namespace {

MaybeHandle<PyTypeObject> NewDerivedExceptionType(
    Isolate* isolate,
    Handle<PyString> type_name,
    Handle<PyTypeObject> type_base) {
  auto supers = PyList::New(isolate, 1);
  supers->SetAndExtendLength(0, Handle<PyObject>::cast(type_base));

  return Runtime_CreatePythonClass(isolate, type_name, PyDict::New(isolate),
                                   supers);
}

}  // namespace

///////////////////////////////////////////////////////////////////////////

ExceptionRoots::ExceptionRoots(Isolate* isolate) : isolate_(isolate) {}

Maybe<void> ExceptionRoots::Setup() {
  roots_ = *isolate_->factory()->NewFixedArray(kRootSlotCount);
  return InitializeBuiltinExceptionTypes();
}

Handle<PyTypeObject> ExceptionRoots::Get(ExceptionType type) const {
  Tagged<PyObject> value = roots()->Get(SlotIndex(type));
  assert(!value.is_null());
  return Handle<PyTypeObject>::cast(handle(value, isolate_));
}

void ExceptionRoots::Set(ExceptionType type, Handle<PyTypeObject> value) {
  roots()->Set(SlotIndex(type), Handle<PyObject>::cast(value));
}

Maybe<void> ExceptionRoots::InitializeBuiltinExceptionTypes() {
  Handle<PyTypeObject> base_exception =
      PyBaseExceptionKlass::GetInstance(isolate_)->type_object(isolate_);
  Set(ExceptionType::kBaseException, base_exception);

  Handle<PyTypeObject> exception;
  ASSIGN_RETURN_ON_EXCEPTION(
      isolate_, exception,
      NewDerivedExceptionType(isolate_, ST(exception, isolate_),
                              base_exception));
  Set(ExceptionType::kException, exception);

#define INITIALIZE_NORMAL_EXCEPTION_TYPE(type_enum, name_in_string_table, _)  \
  do {                                                                        \
    Handle<PyTypeObject> type_object;                                         \
    ASSIGN_RETURN_ON_EXCEPTION(                                               \
        isolate_, type_object,                                                \
        NewDerivedExceptionType(isolate_, ST(name_in_string_table, isolate_), \
                                exception));                                  \
    Set(ExceptionType::type_enum, type_object);                               \
  } while (false);

  NORMAL_EXCEPTION_TYPE(INITIALIZE_NORMAL_EXCEPTION_TYPE);

#undef INITIALIZE_NORMAL_EXCEPTION_TYPE

  return JustVoid();
}

void ExceptionRoots::Iterate(ObjectVisitor* v) {
  v->VisitPointer(reinterpret_cast<Tagged<PyObject>*>(&roots_));
}

int64_t ExceptionRoots::SlotIndex(ExceptionType type) const {
  const int index = static_cast<int>(type);
  assert(0 <= index && index < kRootSlotCount);
  return index;
}

Handle<FixedArray> ExceptionRoots::roots() const {
  assert(!roots_.is_null());
  return handle(roots_, isolate_);
}

}  // namespace saauso::internal
