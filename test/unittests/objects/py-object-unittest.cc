// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/execution/isolate.h"
#include "src/objects/py-dict.h"
#include "src/objects/py-list.h"
#include "src/objects/py-object.h"
#include "src/objects/py-smi.h"
#include "test/unittests/test-helpers.h"
#include "testing/gtest/include/gtest/gtest.h"


namespace saauso::internal {

class PyObjectTest : public VmTestBase {};

TEST_F(PyObjectTest, GetPropertiesReturnsNullByDefaultAndAcceptsNullSet) {
  HandleScope scope;

  auto list = PyList::NewInstance();
  auto props = PyObject::GetProperties(Handle<PyObject>(list));
  EXPECT_TRUE(props.is_null());

  PyObject::SetProperties(*Handle<PyObject>(list), Tagged<PyDict>::null());
  auto props_after = PyObject::GetProperties(Handle<PyObject>(list));
  EXPECT_TRUE(props_after.is_null());
}

TEST_F(PyObjectTest, SetPropertiesStoresDictAndGetPropertiesReturnsIt) {
  HandleScope scope;

  auto list = PyList::NewInstance();
  auto dict = PyDict::NewInstance();

  PyObject::SetProperties(*Handle<PyObject>(list), *dict);
  auto props = PyObject::GetProperties(Handle<PyObject>(list));

  ASSERT_FALSE(props.is_null());
  EXPECT_TRUE(props.is_identical_to(dict));
}

TEST_F(PyObjectTest, IsHeapObjectReturnsFalseForNullAndSmi) {
  HandleScope scope;

  Tagged<PyObject> null = Tagged<PyObject>::null();
  EXPECT_FALSE(IsHeapObject(null));

  Tagged<PyObject> smi = Tagged<PyObject>(PySmi::FromInt(123));
  EXPECT_FALSE(IsHeapObject(smi));

  auto list = PyList::NewInstance();
  EXPECT_TRUE(IsHeapObject(*Handle<PyObject>(list)));
}

}  // namespace saauso::internal
