// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include <cstdint>

#include "src/objects/py-list.h"
#include "src/objects/py-object.h"
#include "src/objects/py-oddballs.h"
#include "src/objects/py-smi.h"
#include "src/objects/py-string.h"
#include "src/objects/py-tuple-iterator.h"
#include "src/objects/py-tuple.h"
#include "src/runtime/isolate.h"
#include "test/unittests/test-helpers.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace saauso::internal {

class PyTupleTest : public VmTestBase {};

TEST_F(PyTupleTest, NewInstanceFromListCopiesElements) {
  HandleScope scope;

  auto list = PyList::NewInstance(2);
  PyList::Append(list, Handle<PyObject>(PySmi::FromInt(1)));
  PyList::Append(list, Handle<PyObject>(PySmi::FromInt(2)));

  auto tuple = PyTuple::NewInstance(list);
  EXPECT_EQ(tuple->length(), 2);
  EXPECT_EQ(PySmi::ToInt(Handle<PySmi>::cast(tuple->Get(0))), 1);
  EXPECT_EQ(PySmi::ToInt(Handle<PySmi>::cast(tuple->Get(1))), 2);

  PyObject::StoreSubscr(Handle<PyObject>(list),
                        Handle<PyObject>(PySmi::FromInt(0)),
                        Handle<PyObject>(PySmi::FromInt(42)));
  EXPECT_EQ(PySmi::ToInt(Handle<PySmi>::cast(tuple->Get(0))), 1);
}

TEST_F(PyTupleTest, PyObjectLenAndSubscrWork) {
  HandleScope scope;

  auto list = PyList::NewInstance(2);
  PyList::Append(list, Handle<PyObject>(PyString::NewInstance("x")));
  PyList::Append(list, Handle<PyObject>(PySmi::FromInt(7)));

  auto tuple = PyTuple::NewInstance(list);
  Handle<PyObject> obj(tuple);

  auto len = PyObject::Len(obj);
  EXPECT_EQ(PySmi::ToInt(Handle<PySmi>::cast(len)), 2);

  auto v0 = PyObject::Subscr(obj, Handle<PyObject>(PySmi::FromInt(0)));
  EXPECT_TRUE(IsPyString(*v0));

  auto v1 = PyObject::Subscr(obj, Handle<PyObject>(PySmi::FromInt(1)));
  EXPECT_EQ(PySmi::ToInt(Handle<PySmi>::cast(v1)), 7);
}

TEST_F(PyTupleTest, PyObjectContainsAndEqualWork) {
  HandleScope scope;

  auto list1 = PyList::NewInstance(2);
  auto s = Handle<PyObject>(PyString::NewInstance("x"));
  PyList::Append(list1, s);
  PyList::Append(list1, Handle<PyObject>(PySmi::FromInt(1)));
  auto t1 = PyTuple::NewInstance(list1);

  Handle<PyObject> obj(t1);
  EXPECT_EQ(PyObject::Contains(obj, s).ptr(),
            Isolate::Current()->py_true_object().ptr());
  EXPECT_EQ(PyObject::Contains(obj, Handle<PyObject>(PySmi::FromInt(2))).ptr(),
            Isolate::Current()->py_false_object().ptr());

  auto list2 = PyList::NewInstance(2);
  PyList::Append(list2, Handle<PyObject>(PyString::NewInstance("x")));
  PyList::Append(list2, Handle<PyObject>(PySmi::FromInt(1)));
  auto t2 = PyTuple::NewInstance(list2);

  EXPECT_EQ(PyObject::Equal(Handle<PyObject>(t1), Handle<PyObject>(t2)).ptr(),
            Isolate::Current()->py_true_object().ptr());
}

TEST_F(PyTupleTest, PyObjectIterAndNextWork) {
  HandleScope scope;

  auto list = PyList::NewInstance(2);
  PyList::Append(list, Handle<PyObject>(PySmi::FromInt(1)));
  PyList::Append(list, Handle<PyObject>(PySmi::FromInt(2)));
  auto tuple = PyTuple::NewInstance(list);

  auto iterator = PyObject::Iter(Handle<PyObject>(tuple));
  ASSERT_TRUE(IsPyTupleIterator(*iterator));

  auto v0 = PyObject::Next(iterator);
  ASSERT_TRUE(IsPySmi(*v0));
  EXPECT_EQ(PySmi::ToInt(Handle<PySmi>::cast(v0)), 1);

  auto v1 = PyObject::Next(iterator);
  ASSERT_TRUE(IsPySmi(*v1));
  EXPECT_EQ(PySmi::ToInt(Handle<PySmi>::cast(v1)), 2);

  auto end = PyObject::Next(iterator);
  EXPECT_TRUE(end.is_null());
}

}  // namespace saauso::internal
