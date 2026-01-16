// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "include/saauso.h"
#include "src/objects/py-function.h"
#include "src/objects/py-list.h"
#include "src/objects/py-object.h"
#include "src/objects/py-smi.h"
#include "src/objects/py-string.h"
#include "src/runtime/isolate.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace saauso::internal {

class AttributeTest : public testing::Test {
 protected:
  static void SetUpTestSuite() {
    saauso::Saauso::Initialize();
    isolate_ = Isolate::New();
    isolate_->Enter();
  }
  static void TearDownTestSuite() {
    isolate_->Exit();
    Isolate::Dispose(isolate_);
    isolate_ = nullptr;
    saauso::Saauso::Dispose();
  }

  static Isolate* isolate_;
};

Isolate* AttributeTest::isolate_ = nullptr;

TEST_F(AttributeTest, DefaultSetAttrCreatesPropertiesDict) {
  HandleScope scope;

  auto func_name = PyString::NewInstance("foo");
  auto func = PyFunction::NewInstance(nullptr, func_name);

  auto key = PyString::NewInstance("x");
  auto value = handle(PySmi::FromInt(42));

  PyObject::SetAttr(func, key, value);
  auto result = PyObject::GetAttr(func, key);

  EXPECT_TRUE(result.is_identical_to(value));
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
