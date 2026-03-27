// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/execution/isolate.h"
#include "src/heap/factory.h"
#include "src/heap/heap.h"
#include "src/objects/cell.h"
#include "src/objects/py-dict.h"
#include "src/objects/py-function.h"
#include "src/objects/py-object.h"
#include "src/objects/py-string.h"
#include "src/objects/py-tuple.h"
#include "src/objects/templates.h"
#include "test/unittests/test-helpers.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace saauso::internal {

class CellTest : public VmTestBase {};

namespace {
MaybeHandle<PyObject> DummyNative(Isolate*,
                                  Handle<PyObject>,
                                  Handle<PyTuple>,
                                  Handle<PyDict>) {
  return Handle<PyObject>::null();
}
}  // namespace

TEST_F(CellTest, NewInstanceDefaultsToNullValue) {
  HandleScope scope;

  Handle<Cell> cell = isolate_->factory()->NewCell();
  EXPECT_TRUE(cell->value().is_null());
}

TEST_F(CellTest, SetValueSurvivesMinorGc) {
  HandleScope scope;

  Handle<Cell> cell = isolate_->factory()->NewCell();
  Handle<PyString> payload = PyString::NewInstance("payload");
  cell->set_value(payload);

  Isolate::Current()->heap()->CollectGarbage();

  Handle<PyObject> cell_value = cell->value();
  ASSERT_FALSE(cell_value.is_null());
  EXPECT_EQ((*cell_value).ptr(), (*payload).ptr());
}

TEST_F(CellTest, FunctionClosuresSurviveMinorGc) {
  HandleScope scope;

  Handle<Cell> cell = isolate_->factory()->NewCell();
  Handle<PyString> payload = PyString::NewInstance("payload");
  cell->set_value(payload);

  Handle<PyTuple> closures = PyTuple::New(isolate_, 1);
  closures->SetInternal(0, cell);

  Handle<PyString> func_name = PyString::NewInstance("dummy");
  FunctionTemplateInfo func_template(&DummyNative, func_name);

  Handle<PyFunction> func;
  if (!isolate_->factory()
           ->NewPyFunctionWithTemplate(func_template)
           .To(&func)) {
    FAIL() << "fail to create function object";
  }
  func->set_closures(closures);

  isolate_->heap()->CollectGarbage();

  Handle<PyTuple> closures_after = func->closures();
  Handle<Cell> cell_after = Handle<Cell>::cast(closures_after->Get(0));
  Handle<PyObject> cell_value = cell_after->value();
  ASSERT_FALSE(cell_value.is_null());
  EXPECT_EQ((*cell_value).ptr(), (*payload).ptr());
}

}  // namespace saauso::internal
