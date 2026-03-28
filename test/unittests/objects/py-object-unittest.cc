// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/execution/isolate.h"
#include "src/objects/py-dict-klass.h"
#include "src/objects/py-dict.h"
#include "src/objects/py-list-klass.h"
#include "src/objects/py-list.h"
#include "src/objects/py-object.h"
#include "src/objects/py-smi.h"
#include "src/objects/py-string-klass.h"
#include "src/objects/py-string.h"
#include "src/objects/py-tuple-klass.h"
#include "src/objects/py-type-object.h"
#include "src/runtime/runtime-reflection.h"
#include "test/unittests/test-helpers.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace saauso::internal {

class PyObjectTest : public VmTestBase {};

TEST_F(PyObjectTest, GetPropertiesReturnsNullByDefaultAndAcceptsNullSet) {
  HandleScope scope;

  auto list = PyList::New(isolate_);
  auto props = PyObject::GetProperties(Handle<PyObject>(list));
  EXPECT_TRUE(props.is_null());

  PyObject::SetProperties(*Handle<PyObject>(list), Tagged<PyDict>::null());
  auto props_after = PyObject::GetProperties(Handle<PyObject>(list));
  EXPECT_TRUE(props_after.is_null());
}

TEST_F(PyObjectTest, SetPropertiesStoresDictAndGetPropertiesReturnsIt) {
  HandleScope scope;

  auto list = PyList::New(isolate_);
  auto dict = PyDict::New(isolate_);

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

  auto list = PyList::New(isolate_);
  EXPECT_TRUE(IsHeapObject(*Handle<PyObject>(list)));
}

TEST_F(PyObjectTest, IsPyContainerAndStringSupportLikeAndExactSemantics) {
  HandleScope scope;
  Handle<PyString> list_like_name = PyString::NewInstance("ListLikeCase");
  Handle<PyDict> list_like_props = PyDict::New(isolate_);
  Handle<PyList> list_like_supers = PyList::New(isolate_, 1);
  list_like_supers->SetAndExtendLength(
      0, PyListKlass::GetInstance(isolate_)->type_object());
  Handle<PyTypeObject> list_like_type;
  ASSERT_TRUE(Runtime_CreatePythonClass(isolate_, list_like_name,
                                        list_like_props, list_like_supers)
                  .ToHandle(&list_like_type));
  Handle<PyObject> list_like;
  ASSERT_TRUE(Runtime_NewObject(isolate_, list_like_type,
                                Handle<PyObject>::null(),
                                Handle<PyObject>::null())
                  .ToHandle(&list_like));
  EXPECT_TRUE(IsPyList(list_like));
  EXPECT_FALSE(IsPyListExact(list_like));
  EXPECT_FALSE(PyObject::GetProperties(list_like).is_null());

  Handle<PyString> dict_like_name = PyString::NewInstance("DictLikeCase");
  Handle<PyDict> dict_like_props = PyDict::New(isolate_);
  Handle<PyList> dict_like_supers = PyList::New(isolate_, 1);
  dict_like_supers->SetAndExtendLength(
      0, PyDictKlass::GetInstance(isolate_)->type_object());
  Handle<PyTypeObject> dict_like_type;
  ASSERT_TRUE(Runtime_CreatePythonClass(isolate_, dict_like_name,
                                        dict_like_props, dict_like_supers)
                  .ToHandle(&dict_like_type));
  Handle<PyObject> dict_like;
  ASSERT_TRUE(Runtime_NewObject(isolate_, dict_like_type,
                                Handle<PyObject>::null(),
                                Handle<PyObject>::null())
                  .ToHandle(&dict_like));
  EXPECT_TRUE(IsPyDict(dict_like));
  EXPECT_FALSE(IsPyDictExact(dict_like));
  EXPECT_FALSE(PyObject::GetProperties(dict_like).is_null());

  Handle<PyString> tuple_like_name = PyString::NewInstance("TupleLikeCase");
  Handle<PyDict> tuple_like_props = PyDict::New(isolate_);
  Handle<PyList> tuple_like_supers = PyList::New(isolate_, 1);
  tuple_like_supers->SetAndExtendLength(
      0, PyTupleKlass::GetInstance(isolate_)->type_object());
  Handle<PyTypeObject> tuple_like_type;
  ASSERT_TRUE(Runtime_CreatePythonClass(isolate_, tuple_like_name,
                                        tuple_like_props, tuple_like_supers)
                  .ToHandle(&tuple_like_type));
  Handle<PyObject> tuple_like;
  ASSERT_TRUE(Runtime_NewObject(isolate_, tuple_like_type,
                                Handle<PyObject>::null(),
                                Handle<PyObject>::null())
                  .ToHandle(&tuple_like));
  EXPECT_TRUE(IsPyTuple(tuple_like));
  EXPECT_FALSE(IsPyTupleExact(tuple_like));
  EXPECT_FALSE(PyObject::GetProperties(tuple_like).is_null());

  Handle<PyString> string_like_name = PyString::NewInstance("StringLikeCase");
  Handle<PyDict> string_like_props = PyDict::New(isolate_);
  Handle<PyList> string_like_supers = PyList::New(isolate_, 1);
  string_like_supers->SetAndExtendLength(
      0, PyStringKlass::GetInstance(isolate_)->type_object());
  Handle<PyTypeObject> string_like_type;
  ASSERT_TRUE(Runtime_CreatePythonClass(isolate_, string_like_name,
                                        string_like_props, string_like_supers)
                  .ToHandle(&string_like_type));
  Handle<PyObject> string_like;
  ASSERT_TRUE(Runtime_NewObject(isolate_, string_like_type,
                                Handle<PyObject>::null(),
                                Handle<PyObject>::null())
                  .ToHandle(&string_like));
  EXPECT_TRUE(IsPyString(string_like));
  EXPECT_FALSE(IsPyStringExact(string_like));
  EXPECT_FALSE(PyObject::GetProperties(string_like).is_null());
}

}  // namespace saauso::internal
