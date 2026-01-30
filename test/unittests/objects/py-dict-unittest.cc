// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include <set>

#include "src/objects/py-dict.h"
#include "src/objects/py-object.h"
#include "src/objects/py-oddballs.h"
#include "src/objects/py-smi.h"
#include "src/objects/py-string.h"
#include "src/objects/py-tuple.h"
#include "src/runtime/isolate.h"
#include "test/unittests/test-helpers.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace saauso::internal {

class PyDictTest : public VmTestBase {};

TEST_F(PyDictTest, BasicOperations) {
  HandleScope scope;

  Handle<PyDict> dict = PyDict::NewInstance();
  EXPECT_EQ(dict->occupied(), 0);

  Handle<PyObject> key1 = PyString::NewInstance("key1");
  Handle<PyObject> val1(PySmi::FromInt(100));

  // Put
  PyDict::Put(dict, key1, val1);
  EXPECT_EQ(dict->occupied(), 1);

  // Get
  Handle<PyObject> res1 = dict->Get(key1);
  EXPECT_FALSE(res1.is_null());
  EXPECT_TRUE(PyObject::Equal(res1, val1)->value());

  // Contains
  EXPECT_TRUE(dict->Contains(key1));
  Handle<PyObject> key2 = PyString::NewInstance("key2");
  EXPECT_TRUE(!dict->Contains(key2));

  // Update
  Handle<PyObject> val2(PySmi::FromInt(200));
  PyDict::Put(dict, key1, val2);
  EXPECT_EQ(dict->occupied(), 1);  // Size shouldn't change
  res1 = dict->Get(key1);
  EXPECT_TRUE(PyObject::EqualBool(res1, val2));

  // Remove
  dict->Remove(key1);
  EXPECT_EQ(dict->occupied(), 0);
  EXPECT_TRUE(!dict->Contains(key1));
  EXPECT_TRUE(dict->Get(key1).is_null());
}

TEST_F(PyDictTest, CollisionAndShift) {
  HandleScope scope;

  // Create dict with default capacity (min 2, usually expands)
  Handle<PyDict> dict = PyDict::NewInstance();

  // Insert multiple items
  int count = 10;
  for (int i = 0; i < count; ++i) {
    Handle<PyObject> key(PySmi::FromInt(i));
    Handle<PyObject> val(PySmi::FromInt(i * 10));
    PyDict::Put(dict, key, val);
  }

  EXPECT_EQ(dict->occupied(), count);

  // Remove every second item
  for (int i = 0; i < count; i += 2) {
    Handle<PyObject> key(PySmi::FromInt(i));
    dict->Remove(key);
  }

  EXPECT_EQ(dict->occupied(), count / 2);

  // Verify remaining items
  for (int i = 1; i < count; i += 2) {
    Handle<PyObject> key(PySmi::FromInt(i));
    EXPECT_TRUE(dict->Contains(key));
    Handle<PyObject> val = dict->Get(key);
    EXPECT_EQ(PySmi::ToInt(Handle<PySmi>::cast(val)), i * 10);
  }

  // Verify removed items are gone
  for (int i = 0; i < count; i += 2) {
    Handle<PyObject> key(PySmi::FromInt(i));
    EXPECT_TRUE(!dict->Contains(key));
  }
}

TEST_F(PyDictTest, Equality) {
  HandleScope scope;

  Handle<PyDict> d1 = PyDict::NewInstance();
  Handle<PyDict> d2 = PyDict::NewInstance();

  Handle<PyObject> k1 = PyString::NewInstance("a");
  Handle<PyObject> v1(PySmi::FromInt(1));
  Handle<PyObject> k2 = PyString::NewInstance("b");
  Handle<PyObject> v2(PySmi::FromInt(2));

  // d1 = {"a": 1, "b": 2}
  PyDict::Put(d1, k1, v1);
  PyDict::Put(d1, k2, v2);

  // d2 = {"b": 2, "a": 1} (Different insertion order, should be equal)
  PyDict::Put(d2, k2, v2);
  PyDict::Put(d2, k1, v1);

  EXPECT_TRUE(PyObject::EqualBool(d1, d2));

  // d2 = {"b": 2, "a": 3} (Different value)
  Handle<PyObject> v3(PySmi::FromInt(3));
  PyDict::Put(d2, k1, v3);
  EXPECT_TRUE(!PyObject::EqualBool(d1, d2));

  // d2 = {"b": 2} (Different size)
  d2->Remove(k1);
  EXPECT_TRUE(!PyObject::EqualBool(d1, d2));
}

TEST_F(PyDictTest, GetKeyTuple) {
  HandleScope scope;

  auto TupleContains = [](Handle<PyTuple> tuple, Handle<PyObject> key) -> bool {
    for (auto i = 0; i < tuple->length(); ++i) {
      if (PyObject::Equal(tuple->Get(i), key)->value()) {
        return true;
      }
    }
    return false;
  };

  Handle<PyDict> dict = PyDict::NewInstance();

  int count = 20;
  for (int i = 0; i < count; ++i) {
    Handle<PyObject> key(PySmi::FromInt(i));
    Handle<PyObject> val(PySmi::FromInt(i * 10));
    PyDict::Put(dict, key, val);
  }

  Handle<PyTuple> keys = PyDict::GetKeyTuple(dict);
  EXPECT_EQ(keys->length(), dict->occupied());
  EXPECT_EQ(keys->length(), count);

  for (int i = 0; i < count; ++i) {
    Handle<PyObject> key(PySmi::FromInt(i));
    EXPECT_TRUE(TupleContains(keys, key));
  }

  for (auto i = 0; i < keys->length(); ++i) {
    EXPECT_FALSE(keys->Get(i).is_null());
  }

  for (auto i = 0; i < keys->length(); ++i) {
    for (auto j = i + 1; j < keys->length(); ++j) {
      EXPECT_FALSE(PyObject::Equal(keys->Get(i), keys->Get(j))->value());
    }
  }

  for (int i = 0; i < count; i += 2) {
    Handle<PyObject> key(PySmi::FromInt(i));
    dict->Remove(key);
  }

  Handle<PyTuple> keys_after_remove = PyDict::GetKeyTuple(dict);
  EXPECT_EQ(keys_after_remove->length(), dict->occupied());
  EXPECT_EQ(keys_after_remove->length(), count / 2);

  for (int i = 1; i < count; i += 2) {
    Handle<PyObject> key(PySmi::FromInt(i));
    EXPECT_TRUE(TupleContains(keys_after_remove, key));
  }
}

TEST_F(PyDictTest, IteratorIteratesKeys) {
  HandleScope scope;

  Handle<PyDict> dict = PyDict::NewInstance();
  int count = 50;
  for (int i = 0; i < count; ++i) {
    Handle<PyObject> key(PySmi::FromInt(i));
    Handle<PyObject> val(PySmi::FromInt(i * 10));
    PyDict::Put(dict, key, val);
  }

  Handle<PyObject> iterator = PyObject::Iter(dict);
  ASSERT_TRUE(IsPyDictKeyIterator(iterator));

  std::set<int> seen;
  for (;;) {
    Handle<PyObject> key = PyObject::Next(iterator);
    if (key.is_null()) {
      break;
    }
    int v = PySmi::ToInt(Handle<PySmi>::cast(key));
    seen.insert(v);
  }

  EXPECT_EQ(static_cast<int>(seen.size()), count);
  for (int i = 0; i < count; ++i) {
    EXPECT_NE(seen.count(i), 0);
  }
}

}  // namespace saauso::internal
