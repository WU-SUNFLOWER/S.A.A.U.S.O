// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include <cstdint>

#include "src/objects/py-list.h"
#include "src/objects/py-object.h"
#include "src/objects/py-oddballs.h"
#include "src/objects/py-smi.h"
#include "src/objects/py-string.h"
#include "src/runtime/isolate.h"
#include "test/unittests/test-helpers.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace saauso::internal {

class PyListTest : public VmTestBase {};

TEST_F(PyListTest, NewInstanceHasZeroLengthAndMinimumCapacity) {
  HandleScope scope;

  auto list = PyList::NewInstance();
  EXPECT_EQ(list->length(), 0);
  EXPECT_TRUE(list->IsEmpty());
  EXPECT_GE(list->capacity(), PyList::kMinimumCapacity);
}

TEST_F(PyListTest, AppendGetGetLastPopWork) {
  HandleScope scope;

  auto list = PyList::NewInstance(2);
  auto s1 = Handle<PyObject>(PyString::NewInstance("Item 1"));
  auto i1 = Handle<PyObject>(PySmi::FromInt(100));

  PyList::Append(list, s1);
  PyList::Append(list, i1);

  EXPECT_EQ(list->length(), 2);
  EXPECT_EQ((*list->Get(0)).ptr(), (*s1).ptr());
  EXPECT_EQ((*list->GetLast()).ptr(), (*i1).ptr());

  auto popped = list->Pop();
  EXPECT_EQ((*popped).ptr(), (*i1).ptr());
  EXPECT_EQ(list->length(), 1);
}

TEST_F(PyListTest, SetRemoveClearWork) {
  HandleScope scope;

  auto list = PyList::NewInstance(2);
  auto a = Handle<PyObject>(PySmi::FromInt(1));
  auto b = Handle<PyObject>(PySmi::FromInt(2));
  auto c = Handle<PyObject>(PySmi::FromInt(3));

  PyList::Append(list, a);
  PyList::Append(list, b);
  PyList::Append(list, c);

  list->Set(1, a);
  EXPECT_EQ(PyObject::Equal(list->Get(1), a).ptr(),
            Isolate::Current()->py_true_object().ptr());

  list->RemoveByIndex(0);
  EXPECT_EQ(list->length(), 2);
  EXPECT_EQ(PyObject::Equal(list->Get(0), a).ptr(),
            Isolate::Current()->py_true_object().ptr());
  EXPECT_EQ(PyObject::Equal(list->Get(1), c).ptr(),
            Isolate::Current()->py_true_object().ptr());

  list->Clear();
  EXPECT_EQ(list->length(), 0);
  EXPECT_TRUE(list->IsEmpty());
}

TEST_F(PyListTest, InsertWorksInTheMiddle) {
  HandleScope scope;

  auto list = PyList::NewInstance(2);
  auto a = Handle<PyObject>(PySmi::FromInt(1));
  auto b = Handle<PyObject>(PySmi::FromInt(2));
  auto c = Handle<PyObject>(PySmi::FromInt(3));

  PyList::Append(list, a);
  PyList::Append(list, c);

  PyList::Insert(list, 1, b);
  EXPECT_EQ(list->length(), 3);
  EXPECT_EQ(PySmi::ToInt(Handle<PySmi>::cast(list->Get(0))), 1);
  EXPECT_EQ(PySmi::ToInt(Handle<PySmi>::cast(list->Get(1))), 2);
  EXPECT_EQ(PySmi::ToInt(Handle<PySmi>::cast(list->Get(2))), 3);
}

TEST_F(PyListTest, AppendTriggersExpand) {
  HandleScope scope;

  auto list = PyList::NewInstance(2);
  const int64_t initial_capacity = list->capacity();

  PyList::Append(list, Handle<PyObject>(PySmi::FromInt(1)));
  PyList::Append(list, Handle<PyObject>(PySmi::FromInt(2)));
  PyList::Append(list, Handle<PyObject>(PySmi::FromInt(3)));

  EXPECT_EQ(list->length(), 3);
  EXPECT_GT(list->capacity(), initial_capacity);
}

TEST_F(PyListTest, PyObjectAddConcatenatesLists) {
  HandleScope scope;

  auto list = PyList::NewInstance(2);
  PyList::Append(list, Handle<PyObject>(PySmi::FromInt(1)));
  PyList::Append(list, Handle<PyObject>(PySmi::FromInt(2)));

  Handle<PyObject> lhs(list);
  Handle<PyObject> rhs(list);
  auto combined = PyObject::Add(lhs, rhs);

  ASSERT_TRUE(IsPyList(combined));
  auto combined_list = Handle<PyList>::cast(combined);
  EXPECT_EQ(combined_list->length(), 4);
  EXPECT_EQ(list->length(), 2);
}

TEST_F(PyListTest, PyObjectMulRepeatsList) {
  HandleScope scope;

  auto list = PyList::NewInstance(2);
  PyList::Append(list, Handle<PyObject>(PySmi::FromInt(1)));
  PyList::Append(list, Handle<PyObject>(PySmi::FromInt(2)));

  Handle<PyObject> lhs(list);
  Handle<PyObject> coeff(PySmi::FromInt(3));
  auto repeated = PyObject::Mul(lhs, coeff);

  ASSERT_TRUE(IsPyList(repeated));
  auto repeated_list = Handle<PyList>::cast(repeated);
  ASSERT_EQ(repeated_list->length(), 6);

  EXPECT_EQ(PySmi::ToInt(Handle<PySmi>::cast(repeated_list->Get(0))), 1);
  EXPECT_EQ(PySmi::ToInt(Handle<PySmi>::cast(repeated_list->Get(1))), 2);
  EXPECT_EQ(PySmi::ToInt(Handle<PySmi>::cast(repeated_list->Get(2))), 1);
  EXPECT_EQ(PySmi::ToInt(Handle<PySmi>::cast(repeated_list->Get(3))), 2);
}

TEST_F(PyListTest, PyObjectSubscrAndStoreAndDeleteWork) {
  HandleScope scope;

  auto list = PyList::NewInstance(2);
  PyList::Append(list, Handle<PyObject>(PySmi::FromInt(1)));
  PyList::Append(list, Handle<PyObject>(PySmi::FromInt(2)));

  Handle<PyObject> obj(list);
  Handle<PyObject> index0(PySmi::FromInt(0));
  Handle<PyObject> index1(PySmi::FromInt(1));

  auto v0 = PyObject::Subscr(obj, index0);
  EXPECT_EQ(PySmi::ToInt(Handle<PySmi>::cast(v0)), 1);

  PyObject::StoreSubscr(obj, index1, Handle<PyObject>(PySmi::FromInt(42)));
  auto v1 = PyObject::Subscr(obj, index1);
  EXPECT_EQ(PySmi::ToInt(Handle<PySmi>::cast(v1)), 42);

  PyObject::DeletSubscr(obj, index0);
  EXPECT_EQ(Handle<PyList>::cast(obj)->length(), 1);
  EXPECT_EQ(
      PySmi::ToInt(Handle<PySmi>::cast(Handle<PyList>::cast(obj)->Get(0))), 42);
}

TEST_F(PyListTest, PyObjectContainsAndEqualWork) {
  HandleScope scope;

  auto list = PyList::NewInstance(2);
  auto s = Handle<PyObject>(PyString::NewInstance("x"));
  PyList::Append(list, s);
  PyList::Append(list, Handle<PyObject>(PySmi::FromInt(1)));

  Handle<PyObject> obj(list);
  EXPECT_EQ(PyObject::Contains(obj, s).ptr(),
            Isolate::Current()->py_true_object().ptr());
  EXPECT_EQ(PyObject::Contains(obj, Handle<PyObject>(PySmi::FromInt(2))).ptr(),
            Isolate::Current()->py_false_object().ptr());

  auto list2 = PyList::NewInstance(2);
  PyList::Append(list2, Handle<PyObject>(PyString::NewInstance("x")));
  PyList::Append(list2, Handle<PyObject>(PySmi::FromInt(1)));

  EXPECT_EQ(
      PyObject::Equal(Handle<PyObject>(list), Handle<PyObject>(list2)).ptr(),
      Isolate::Current()->py_true_object().ptr());
}

TEST_F(PyListTest, PyObjectLessIsLexicographic) {
  HandleScope scope;

  auto a = PyList::NewInstance(2);
  PyList::Append(a, Handle<PyObject>(PySmi::FromInt(1)));
  PyList::Append(a, Handle<PyObject>(PySmi::FromInt(2)));

  auto b = PyList::NewInstance(2);
  PyList::Append(b, Handle<PyObject>(PySmi::FromInt(1)));
  PyList::Append(b, Handle<PyObject>(PySmi::FromInt(3)));

  EXPECT_EQ(PyObject::Less(Handle<PyObject>(a), Handle<PyObject>(b)).ptr(),
            Isolate::Current()->py_true_object().ptr());
}

}  // namespace saauso::internal
