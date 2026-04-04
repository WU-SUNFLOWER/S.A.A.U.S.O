// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include <cstdint>

#include "src/execution/isolate.h"
#include "src/objects/fixed-array.h"
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
  HandleScope scope(isolate_);

  auto list = PyList::New(isolate_);
  EXPECT_EQ(list->length(), 0);
  EXPECT_TRUE(list->IsEmpty());
  EXPECT_GE(list->capacity(), PyList::kMinimumCapacity);
}

TEST_F(PyListTest, AppendGetGetTaggedGetLastTaggedPopTaggedWork) {
  HandleScope scope(isolate_);

  auto list = PyList::New(isolate_, 2);
  auto s1 = Handle<PyObject>(PyString::New(isolate_, "Item 1"));
  auto i1 = Handle<PyObject>(PySmi::FromInt(100), isolate_);

  PyList::Append(list, s1, isolate_);
  PyList::Append(list, i1, isolate_);

  EXPECT_EQ(list->length(), 2);
  EXPECT_EQ((*list->Get(0, isolate_)).ptr(), (*s1).ptr());
  EXPECT_EQ(list->GetTagged(0).ptr(), (*s1).ptr());
  EXPECT_EQ((*list->GetLast(isolate_)).ptr(), (*i1).ptr());
  EXPECT_EQ(list->GetLastTagged().ptr(), (*i1).ptr());

  auto popped_tagged = list->PopTagged();
  EXPECT_EQ(popped_tagged.ptr(), (*i1).ptr());
  EXPECT_EQ(list->length(), 1);
  EXPECT_TRUE(list->array()->Get(1).is_null());
}

TEST_F(PyListTest, SetRemoveClearWork) {
  HandleScope scope(isolate_);

  auto list = PyList::New(isolate_, 2);
  auto a = Handle<PyObject>(PySmi::FromInt(1), isolate_);
  auto b = Handle<PyObject>(PySmi::FromInt(2), isolate_);
  auto c = Handle<PyObject>(PySmi::FromInt(3), isolate_);

  PyList::Append(list, a, isolate_);
  PyList::Append(list, b, isolate_);
  PyList::Append(list, c, isolate_);

  list->Set(1, a);
  Handle<PyObject> eq;
  ASSERT_TRUE(PyObject::Equal(isolate_, list->Get(1, isolate_), a).ToHandle(&eq));
  EXPECT_TRUE(Handle<PyBoolean>::cast(eq)->value());

  list->RemoveByIndex(0);
  EXPECT_EQ(list->length(), 2);
  ASSERT_TRUE(PyObject::Equal(isolate_, list->Get(0, isolate_), a).ToHandle(&eq));
  EXPECT_TRUE(Handle<PyBoolean>::cast(eq)->value());
  ASSERT_TRUE(PyObject::Equal(isolate_, list->Get(1, isolate_), c).ToHandle(&eq));
  EXPECT_TRUE(Handle<PyBoolean>::cast(eq)->value());
  EXPECT_TRUE(list->array()->Get(2).is_null());

  list->Clear();
  EXPECT_EQ(list->length(), 0);
  EXPECT_TRUE(list->IsEmpty());
  EXPECT_TRUE(list->array()->Get(0).is_null());
  EXPECT_TRUE(list->array()->Get(1).is_null());
  EXPECT_TRUE(list->array()->Get(2).is_null());
}

TEST_F(PyListTest, InsertWorksInTheMiddle) {
  HandleScope scope(isolate_);

  auto list = PyList::New(isolate_, 2);
  auto a = Handle<PyObject>(PySmi::FromInt(1), isolate_);
  auto b = Handle<PyObject>(PySmi::FromInt(2), isolate_);
  auto c = Handle<PyObject>(PySmi::FromInt(3), isolate_);

  PyList::Append(list, a, isolate_);
  PyList::Append(list, c, isolate_);

  PyList::Insert(list, 1, b, isolate_);
  EXPECT_EQ(list->length(), 3);
  EXPECT_EQ(PySmi::ToInt(Handle<PySmi>::cast(list->Get(0, isolate_))), 1);
  EXPECT_EQ(PySmi::ToInt(Handle<PySmi>::cast(list->Get(1, isolate_))), 2);
  EXPECT_EQ(PySmi::ToInt(Handle<PySmi>::cast(list->Get(2, isolate_))), 3);
}

TEST_F(PyListTest, AppendTriggersExpand) {
  HandleScope scope(isolate_);

  auto list = PyList::New(isolate_, 2);
  const int64_t initial_capacity = list->capacity();

  PyList::Append(list, Handle<PyObject>(PySmi::FromInt(1), isolate_), isolate_);
  PyList::Append(list, Handle<PyObject>(PySmi::FromInt(2), isolate_), isolate_);
  PyList::Append(list, Handle<PyObject>(PySmi::FromInt(3), isolate_), isolate_);

  EXPECT_EQ(list->length(), 3);
  EXPECT_GT(list->capacity(), initial_capacity);
}

TEST_F(PyListTest, PyObjectAddConcatenatesLists) {
  HandleScope scope(isolate_);

  auto list = PyList::New(isolate_, 2);
  PyList::Append(list, Handle<PyObject>(PySmi::FromInt(1), isolate_), isolate_);
  PyList::Append(list, Handle<PyObject>(PySmi::FromInt(2), isolate_), isolate_);

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
  HandleScope scope(isolate_);

  auto list = PyList::New(isolate_, 2);
  PyList::Append(list, Handle<PyObject>(PySmi::FromInt(1), isolate_), isolate_);
  PyList::Append(list, Handle<PyObject>(PySmi::FromInt(2), isolate_), isolate_);

  Handle<PyObject> lhs(list);
  Handle<PyObject> coeff(PySmi::FromInt(3), isolate_);
  Handle<PyObject> repeated;
  ASSERT_TRUE(PyObject::Mul(isolate_, lhs, coeff).ToHandle(&repeated));
  ASSERT_TRUE(IsPyList(repeated));
  auto repeated_list = Handle<PyList>::cast(repeated);
  ASSERT_EQ(repeated_list->length(), 6);

  EXPECT_EQ(PySmi::ToInt(Handle<PySmi>::cast(repeated_list->Get(0, isolate_))), 1);
  EXPECT_EQ(PySmi::ToInt(Handle<PySmi>::cast(repeated_list->Get(1, isolate_))), 2);
  EXPECT_EQ(PySmi::ToInt(Handle<PySmi>::cast(repeated_list->Get(2, isolate_))), 1);
  EXPECT_EQ(PySmi::ToInt(Handle<PySmi>::cast(repeated_list->Get(3, isolate_))), 2);
}

TEST_F(PyListTest, PyObjectSubscrAndStoreAndDeleteWork) {
  HandleScope scope(isolate_);

  auto list = PyList::New(isolate_, 2);
  PyList::Append(list, Handle<PyObject>(PySmi::FromInt(1), isolate_), isolate_);
  PyList::Append(list, Handle<PyObject>(PySmi::FromInt(2), isolate_), isolate_);

  Handle<PyObject> obj(list);
  Handle<PyObject> index0(PySmi::FromInt(0), isolate_);
  Handle<PyObject> index1(PySmi::FromInt(1), isolate_);

  Handle<PyObject> v0;
  ASSERT_TRUE(PyObject::Subscr(isolate_, obj, index0).ToHandle(&v0));
  EXPECT_EQ(PySmi::ToInt(Handle<PySmi>::cast(v0)), 1);

  ASSERT_FALSE(PyObject::StoreSubscr(isolate_, obj, index1,
                                     Handle<PyObject>(PySmi::FromInt(42), isolate_))
                   .IsEmpty());
  Handle<PyObject> v1;
  ASSERT_TRUE(PyObject::Subscr(isolate_, obj, index1).ToHandle(&v1));
  EXPECT_EQ(PySmi::ToInt(Handle<PySmi>::cast(v1)), 42);

  ASSERT_FALSE(PyObject::DeletSubscr(isolate_, obj, index0).IsEmpty());
  EXPECT_EQ(Handle<PyList>::cast(obj)->length(), 1);
  EXPECT_EQ(
      PySmi::ToInt(
          Handle<PySmi>::cast(Handle<PyList>::cast(obj)->Get(0, isolate_))),
      42);
}

TEST_F(PyListTest, PyObjectContainsAndEqualWork) {
  HandleScope scope(isolate_);

  auto list = PyList::New(isolate_, 2);
  auto s = Handle<PyObject>(PyString::New(isolate_, "x"));
  PyList::Append(list, s, isolate_);
  PyList::Append(list, Handle<PyObject>(PySmi::FromInt(1), isolate_), isolate_);

  Handle<PyObject> obj(list);
  Handle<PyObject> contains_res;
  ASSERT_TRUE(PyObject::Contains(isolate_, obj, s).ToHandle(&contains_res));
  EXPECT_TRUE(Handle<PyBoolean>::cast(contains_res)->value());
  ASSERT_TRUE(
      PyObject::Contains(isolate_, obj, Handle<PyObject>(PySmi::FromInt(2), isolate_))
          .ToHandle(&contains_res));
  EXPECT_FALSE(Handle<PyBoolean>::cast(contains_res)->value());

  auto list2 = PyList::New(isolate_, 2);
  PyList::Append(list2, Handle<PyObject>(PyString::New(isolate_, "x")),
                 isolate_);
  PyList::Append(list2, Handle<PyObject>(PySmi::FromInt(1), isolate_), isolate_);

  Handle<PyObject> equal_res;
  ASSERT_TRUE(
      PyObject::Equal(isolate_, Handle<PyObject>(list), Handle<PyObject>(list2))
          .ToHandle(&equal_res));
  EXPECT_TRUE(Handle<PyBoolean>::cast(equal_res)->value());
}

TEST_F(PyListTest, PyObjectLessIsLexicographic) {
  HandleScope scope(isolate_);

  auto a = PyList::New(isolate_, 2);
  PyList::Append(a, Handle<PyObject>(PySmi::FromInt(1), isolate_), isolate_);
  PyList::Append(a, Handle<PyObject>(PySmi::FromInt(2), isolate_), isolate_);

  auto b = PyList::New(isolate_, 2);
  PyList::Append(b, Handle<PyObject>(PySmi::FromInt(1), isolate_), isolate_);
  PyList::Append(b, Handle<PyObject>(PySmi::FromInt(3), isolate_), isolate_);

  Handle<PyObject> less_res;
  ASSERT_TRUE(PyObject::Less(isolate_, Handle<PyObject>(a), Handle<PyObject>(b))
                  .ToHandle(&less_res));
  EXPECT_TRUE(Handle<PyBoolean>::cast(less_res)->value());
}

}  // namespace saauso::internal
