// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/heap/heap.h"
#include "src/objects/cell.h"
#include "src/objects/py-object.h"
#include "src/objects/py-string.h"
#include "src/runtime/isolate.h"
#include "test/unittests/test-helpers.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace saauso::internal {

class CellTest : public VmTestBase {};

TEST_F(CellTest, NewInstanceDefaultsToNullValue) {
  HandleScope scope;

  Handle<Cell> cell = Cell::NewInstance();
  EXPECT_TRUE(cell->value().is_null());
}

TEST_F(CellTest, SetValueSurvivesMinorGc) {
  HandleScope scope;

  Handle<Cell> cell = Cell::NewInstance();
  Handle<PyString> payload = PyString::NewInstance("payload");
  cell->set_value(payload);

  Isolate::Current()->heap()->CollectGarbage();

  Handle<PyObject> cell_value = cell->value();
  ASSERT_FALSE(cell_value.is_null());
  EXPECT_EQ((*cell_value).ptr(), (*payload).ptr());
}

}  // namespace saauso::internal
