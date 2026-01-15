// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/objects/py-list.h"
#include "src/objects/py-object.h"
#include "src/objects/py-smi.h"
#include "src/objects/py-string.h"
#include "src/objects/py-function.h"
#include "src/runtime/universe.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace saauso::internal {

class AttributeTest : public testing::Test {
 protected:
  static void SetUpTestSuite() { Universe::Genesis(); }
  static void TearDownTestSuite() { Universe::Destroy(); }
};

TEST_F(AttributeTest, DefaultSetAttrCreatesPropertiesDict) {
  HandleScope scope;

  auto func_name = PyString::NewInstance("foo");
  auto func = PyFunction::NewInstance(nullptr, func_name);

  auto key = PyString::NewInstance("x");
  auto value = handle(PySmi::FromInt(42));

  PyObject::SetAttr(func, key, value);
  auto result = PyObject::GetAttr(func, key);
  
  EXPECT_EQ((*result).ptr(), (*value).ptr());
}

TEST_F(AttributeTest, GetAttrReturnsBoundMethodWithoutCallingIt) {
  HandleScope scope;

  auto list = PyList::NewInstance();
  EXPECT_EQ(list->length(), 0);

  auto append =
      PyObject::GetAttr(Handle<PyObject>(list),
                        Handle<PyObject>(PyString::NewInstance("append")));

  ASSERT_FALSE(append.IsNull());
  EXPECT_TRUE(IsMethodObject(append));
  EXPECT_EQ(list->length(), 0);
}

}  // namespace saauso::internal
