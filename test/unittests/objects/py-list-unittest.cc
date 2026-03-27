// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include <cstdint>

#include "src/execution/isolate.h"
#include "src/objects/py-list.h"
#include "src/objects/py-object.h"
#include "src/objects/py-oddballs.h"
#include "src/objects/py-smi.h"
#include "src/objects/py-string.h"
#include "test/unittests/test-helpers.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace saauso::internal {

class PyListTest : public VmTestBase {};

TEST_F(PyListTest, NewInstanceHasZeroLengthAndMinimumCapacity) {
  HandleScope scope;

  auto list = PyList::New(isolate_);
  EXPECT_EQ(list->length(), 0);
  EXPECT_TRUE(list->IsEmpty());
  EXPECT_GE(list->capacity(), PyList::kMinimumCapacity);
}

TEST_F(PyListTest, AppendGetGetLastPopWork) {
  HandleScope scope;

  auto list = PyList::New(isolate_, 2);
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

  auto list = PyList::New(isolate_, 2);
  auto a = Handle<PyObject>(PySmi::FromInt(1));
  auto b = Handle<PyObject>(PySmi::FromInt(2));
  auto c = Handle<PyObject>(PySmi::FromInt(3));

  PyList::Append(list, a);
  PyList::Append(list, b);
  PyList::Append(list, c);

  list->Set(1, a);
  Handle<PyObject> eq;
  ASSERT_TRUE(PyObject::Equal(isolate_, list->Get(1), a).ToHandle(&eq));
  EXPECT_TRUE(Handle<PyBoolean>::cast(eq)->value());

  list->RemoveByIndex(0);
  EXPECT_EQ(list->length(), 2);
  ASSERT_TRUE(PyObject::Equal(isolate_, list->Get(0), a).ToHandle(&eq));
  EXPECT_TRUE(Handle<PyBoolean>::cast(eq)->value());
  ASSERT_TRUE(PyObject::Equal(isolate_, list->Get(1), c).ToHandle(&eq));
  EXPECT_TRUE(Handle<PyBoolean>::cast(eq)->value());

  list->Clear();
  EXPECT_EQ(list->length(), 0);
  EXPECT_TRUE(list->IsEmpty());
}

TEST_F(PyListTest, InsertWorksInTheMiddle) {
  HandleScope scope;

  auto list = PyList::New(isolate_, 2);
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

  auto list = PyList::New(isolate_, 2);
  const int64_t initial_capacity = list->capacity();

  PyList::Append(list, Handle<PyObject>(PySmi::FromInt(1)));
  PyList::Append(list, Handle<PyObject>(PySmi::FromInt(2)));
  PyList::Append(list, Handle<PyObject>(PySmi::FromInt(3)));

  EXPECT_EQ(list->length(), 3);
  EXPECT_GT(list->capacity(), initial_capacity);
}

TEST_F(PyListTest, PyObjectAddConcatenatesLists) {
  HandleScope scope;

  auto list = PyList::New(isolate_, 2);
  PyList::Append(list, Handle<PyObject>(PySmi::FromInt(1)));
  PyList::Append(list, Handle<PyObject>(PySmi::FromInt(2)));

  Handle<PyObject> lhs(list);
  Handle<PyObject> rhs(list);
  Handle<PyObject> combined;
  ASSERT_TRUE(PyObject::Add(isolate_, lhs, rhs).ToHandle(&combined));
  ASSERT_TRUE(IsPyList(combined));
  auto combined_list = Handle<PyList>::cast(combined);
  EXPECT_EQ(combined_list->length(), 4);
  EXPECT_EQ(list->length(), 2);
}

TEST_F(PyListTest, PyObjectMulRepeatsList) {
  HandleScope scope;

  auto list = PyList::New(isolate_, 2);
  PyList::Append(list, Handle<PyObject>(PySmi::FromInt(1)));
  PyList::Append(list, Handle<PyObject>(PySmi::FromInt(2)));

  Handle<PyObject> lhs(list);
  Handle<PyObject> coeff(PySmi::FromInt(3));
  Handle<PyObject> repeated;
  ASSERT_TRUE(PyObject::Mul(isolate_, lhs, coeff).ToHandle(&repeated));
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

  auto list = PyList::New(isolate_, 2);
  PyList::Append(list, Handle<PyObject>(PySmi::FromInt(1)));
  PyList::Append(list, Handle<PyObject>(PySmi::FromInt(2)));

  Handle<PyObject> obj(list);
  Handle<PyObject> index0(PySmi::FromInt(0));
  Handle<PyObject> index1(PySmi::FromInt(1));

  Handle<PyObject> v0;
  ASSERT_TRUE(PyObject::Subscr(isolate_, obj, index0).ToHandle(&v0));
  EXPECT_EQ(PySmi::ToInt(Handle<PySmi>::cast(v0)), 1);

  ASSERT_FALSE(PyObject::StoreSubscr(isolate_, obj, index1,
                                     Handle<PyObject>(PySmi::FromInt(42)))
                   .IsEmpty());
  Handle<PyObject> v1;
  ASSERT_TRUE(PyObject::Subscr(isolate_, obj, index1).ToHandle(&v1));
  EXPECT_EQ(PySmi::ToInt(Handle<PySmi>::cast(v1)), 42);

  ASSERT_FALSE(PyObject::DeletSubscr(isolate_, obj, index0).IsEmpty());
  EXPECT_EQ(Handle<PyList>::cast(obj)->length(), 1);
  EXPECT_EQ(
      PySmi::ToInt(Handle<PySmi>::cast(Handle<PyList>::cast(obj)->Get(0))), 42);
}

TEST_F(PyListTest, PyObjectContainsAndEqualWork) {
  HandleScope scope;

  auto list = PyList::New(isolate_, 2);
  auto s = Handle<PyObject>(PyString::NewInstance("x"));
  PyList::Append(list, s);
  PyList::Append(list, Handle<PyObject>(PySmi::FromInt(1)));

  Handle<PyObject> obj(list);
  Handle<PyObject> contains_res;
  ASSERT_TRUE(PyObject::Contains(isolate_, obj, s).ToHandle(&contains_res));
  EXPECT_TRUE(Handle<PyBoolean>::cast(contains_res)->value());
  ASSERT_TRUE(
      PyObject::Contains(isolate_, obj, Handle<PyObject>(PySmi::FromInt(2)))
          .ToHandle(&contains_res));
  EXPECT_FALSE(Handle<PyBoolean>::cast(contains_res)->value());

  auto list2 = PyList::New(isolate_, 2);
  PyList::Append(list2, Handle<PyObject>(PyString::NewInstance("x")));
  PyList::Append(list2, Handle<PyObject>(PySmi::FromInt(1)));

  Handle<PyObject> equal_res;
  ASSERT_TRUE(
      PyObject::Equal(isolate_, Handle<PyObject>(list), Handle<PyObject>(list2))
          .ToHandle(&equal_res));
  EXPECT_TRUE(Handle<PyBoolean>::cast(equal_res)->value());
}

TEST_F(PyListTest, PyObjectLessIsLexicographic) {
  HandleScope scope;

  auto a = PyList::New(isolate_, 2);
  PyList::Append(a, Handle<PyObject>(PySmi::FromInt(1)));
  PyList::Append(a, Handle<PyObject>(PySmi::FromInt(2)));

  auto b = PyList::New(isolate_, 2);
  PyList::Append(b, Handle<PyObject>(PySmi::FromInt(1)));
  PyList::Append(b, Handle<PyObject>(PySmi::FromInt(3)));

  Handle<PyObject> less_res;
  ASSERT_TRUE(PyObject::Less(isolate_, Handle<PyObject>(a), Handle<PyObject>(b))
                  .ToHandle(&less_res));
  EXPECT_TRUE(Handle<PyBoolean>::cast(less_res)->value());
}

}  // namespace saauso::internal
