// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "include/saauso.h"
#include "src/objects/py-dict.h"
#include "src/objects/py-list.h"
#include "src/objects/py-object.h"
#include "src/objects/py-smi.h"
#include "src/runtime/isolate.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace saauso::internal {

class PyObjectTest : public testing::Test {
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

Isolate* PyObjectTest::isolate_ = nullptr;

TEST_F(PyObjectTest, GetPropertiesReturnsNullByDefaultAndAcceptsNullSet) {
  HandleScope scope;

  auto list = PyList::NewInstance();
  auto props = PyObject::GetProperties(Handle<PyObject>(list));
  EXPECT_TRUE(props.IsNull());

  PyObject::SetProperties(*Handle<PyObject>(list), Tagged<PyDict>::Null());
  auto props_after = PyObject::GetProperties(Handle<PyObject>(list));
  EXPECT_TRUE(props_after.IsNull());
}

TEST_F(PyObjectTest, SetPropertiesStoresDictAndGetPropertiesReturnsIt) {
  HandleScope scope;

  auto list = PyList::NewInstance();
  auto dict = PyDict::NewInstance();

  PyObject::SetProperties(*Handle<PyObject>(list), *dict);
  auto props = PyObject::GetProperties(Handle<PyObject>(list));

  ASSERT_FALSE(props.IsNull());
  EXPECT_TRUE(props.is_identical_to(dict));
}

TEST_F(PyObjectTest, IsHeapObjectReturnsFalseForNullAndSmi) {
  HandleScope scope;

  Tagged<PyObject> null = Tagged<PyObject>::Null();
  EXPECT_FALSE(IsHeapObject(null));

  Tagged<PyObject> smi = Tagged<PyObject>(PySmi::FromInt(123));
  EXPECT_FALSE(IsHeapObject(smi));

  auto list = PyList::NewInstance();
  EXPECT_TRUE(IsHeapObject(*Handle<PyObject>(list)));
}

}  // namespace saauso::internal
