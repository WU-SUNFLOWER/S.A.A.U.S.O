// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include <cstdint>

#include "src/execution/isolate.h"
#include "src/heap/factory.h"
#include "src/objects/py-list.h"
#include "src/objects/py-object.h"
#include "src/objects/py-oddballs.h"
#include "src/objects/py-smi.h"
#include "src/objects/py-string.h"
#include "src/objects/py-tuple-iterator.h"
#include "src/objects/py-tuple.h"
#include "test/unittests/test-helpers.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace saauso::internal {

class PyTupleTest : public VmTestBase {};

TEST_F(PyTupleTest, NewInstanceFromListCopiesElements) {
  HandleScope scope;

  auto list = PyList::New(isolate_, 2);
  PyList::Append(list, Handle<PyObject>(PySmi::FromInt(1), isolate_), isolate_);
  PyList::Append(list, Handle<PyObject>(PySmi::FromInt(2), isolate_), isolate_);

  auto tuple = isolate_->factory()->NewPyTupleWithElements(list);
  EXPECT_EQ(tuple->length(), 2);
  EXPECT_EQ(PySmi::ToInt(Handle<PySmi>::cast(tuple->Get(0, isolate_))), 1);
  EXPECT_EQ(PySmi::ToInt(Handle<PySmi>::cast(tuple->Get(1, isolate_))), 2);

  ASSERT_FALSE(PyObject::StoreSubscr(isolate_, Handle<PyObject>(list),
                                     Handle<PyObject>(PySmi::FromInt(0), isolate_),
                                     Handle<PyObject>(PySmi::FromInt(42), isolate_))
                   .IsEmpty());
  EXPECT_EQ(PySmi::ToInt(Handle<PySmi>::cast(tuple->Get(0, isolate_))), 1);
}

TEST_F(PyTupleTest, PyObjectLenAndSubscrWork) {
  HandleScope scope;

  auto list = PyList::New(isolate_, 2);
  PyList::Append(list, Handle<PyObject>(PyString::New(isolate_, "x")),
                 isolate_);
  PyList::Append(list, Handle<PyObject>(PySmi::FromInt(7), isolate_), isolate_);

  auto tuple = isolate_->factory()->NewPyTupleWithElements(list);
  Handle<PyObject> obj(tuple);

  Handle<PyObject> len;
  ASSERT_TRUE(PyObject::Len(isolate_, obj).ToHandle(&len));
  EXPECT_EQ(PySmi::ToInt(Handle<PySmi>::cast(len)), 2);

  Handle<PyObject> v0;
  ASSERT_TRUE(
      PyObject::Subscr(isolate_, obj, Handle<PyObject>(PySmi::FromInt(0), isolate_))
          .ToHandle(&v0));
  EXPECT_TRUE(IsPyString(*v0));

  Handle<PyObject> v1;
  ASSERT_TRUE(
      PyObject::Subscr(isolate_, obj, Handle<PyObject>(PySmi::FromInt(1), isolate_))
          .ToHandle(&v1));
  EXPECT_EQ(PySmi::ToInt(Handle<PySmi>::cast(v1)), 7);
}

TEST_F(PyTupleTest, PyObjectContainsAndEqualWork) {
  HandleScope scope;

  auto list1 = PyList::New(isolate_, 2);
  auto s = Handle<PyObject>(PyString::New(isolate_, "x"));
  PyList::Append(list1, s, isolate_);
  PyList::Append(list1, Handle<PyObject>(PySmi::FromInt(1), isolate_), isolate_);
  auto t1 = isolate_->factory()->NewPyTupleWithElements(list1);

  Handle<PyObject> obj(t1);
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
  auto t2 = isolate_->factory()->NewPyTupleWithElements(list2);

  Handle<PyObject> equal_res;
  ASSERT_TRUE(
      PyObject::Equal(isolate_, Handle<PyObject>(t1), Handle<PyObject>(t2))
          .ToHandle(&equal_res));
  EXPECT_TRUE(Handle<PyBoolean>::cast(equal_res)->value());
}

TEST_F(PyTupleTest, PyObjectIterAndNextWork) {
  HandleScope scope;

  auto list = PyList::New(isolate_, 2);
  PyList::Append(list, Handle<PyObject>(PySmi::FromInt(1), isolate_), isolate_);
  PyList::Append(list, Handle<PyObject>(PySmi::FromInt(2), isolate_), isolate_);
  auto tuple = isolate_->factory()->NewPyTupleWithElements(list);

  Handle<PyObject> iterator;
  ASSERT_TRUE(
      PyObject::Iter(isolate_, Handle<PyObject>(tuple)).ToHandle(&iterator));
  ASSERT_TRUE(IsPyTupleIterator(*iterator));

  Handle<PyObject> v0;
  ASSERT_TRUE(PyObject::Next(isolate_, iterator).ToHandle(&v0));
  ASSERT_TRUE(IsPySmi(*v0));
  EXPECT_EQ(PySmi::ToInt(Handle<PySmi>::cast(v0)), 1);

  Handle<PyObject> v1;
  ASSERT_TRUE(PyObject::Next(isolate_, iterator).ToHandle(&v1));
  ASSERT_TRUE(IsPySmi(*v1));
  EXPECT_EQ(PySmi::ToInt(Handle<PySmi>::cast(v1)), 2);

  // 迭代器耗尽时 Next 返回空 MaybeHandle，ToHandle 为 false。
  Handle<PyObject> end;
  EXPECT_FALSE(PyObject::Next(isolate_, iterator).ToHandle(&end));
  EXPECT_TRUE(end.is_null());
}

}  // namespace saauso::internal
