// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include <set>

#include "src/execution/isolate.h"
#include "src/objects/py-dict.h"
#include "src/objects/py-object.h"
#include "src/objects/py-oddballs.h"
#include "src/objects/py-smi.h"
#include "src/objects/py-string.h"
#include "src/objects/py-tuple.h"
#include "test/unittests/test-helpers.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace saauso::internal {

class PyDictTest : public VmTestBase {};

TEST_F(PyDictTest, GetApiTriState) {
  HandleScope scope;
  auto* isolate = Isolate::Current();

  Handle<PyDict> dict = PyDict::NewInstance();
  Handle<PyObject> key = PyString::NewInstance("k");
  Handle<PyObject> value(PySmi::FromInt(1));
  ASSERT_FALSE(PyDict::Put(dict, key, value).IsNothing());

  Handle<PyObject> miss_key = PyString::NewInstance("missing");
  Tagged<PyObject> out_tagged;
  bool found = true;
  ASSERT_TRUE(dict->GetTagged(miss_key, out_tagged).To(&found));
  EXPECT_FALSE(found);
  EXPECT_TRUE(out_tagged.is_null());
  EXPECT_FALSE(isolate->HasPendingException());

  ASSERT_TRUE(dict->GetTagged(key, out_tagged).To(&found));
  EXPECT_TRUE(found);
  EXPECT_FALSE(out_tagged.is_null());
  bool eq = false;
  ASSERT_TRUE(PyObject::EqualBool(handle(out_tagged), value).To(&eq));
  EXPECT_TRUE(eq);

  Handle<PyObject> out_handle;
  ASSERT_TRUE(dict->Get(key, out_handle).To(&found));
  EXPECT_TRUE(found);
  EXPECT_FALSE(out_handle.is_null());

  Handle<PyObject> bad_key = PyDict::NewInstance();
  out_tagged = Tagged<PyObject>::null();
  EXPECT_TRUE(dict->GetTagged(bad_key, out_tagged).IsNothing());
  EXPECT_TRUE(isolate->HasPendingException());
  EXPECT_TRUE(out_tagged.is_null());
  isolate->exception_state()->Clear();
}

TEST_F(PyDictTest, BasicOperations) {
  HandleScope scope;

  Handle<PyDict> dict = PyDict::NewInstance();
  EXPECT_EQ(dict->occupied(), 0);

  Handle<PyObject> key1 = PyString::NewInstance("key1");
  Handle<PyObject> val1(PySmi::FromInt(100));

  // Put
  ASSERT_FALSE(PyDict::Put(dict, key1, val1).IsNothing());
  EXPECT_EQ(dict->occupied(), 1);

  // Get
  Tagged<PyObject> res1_tagged;
  bool found = false;
  ASSERT_TRUE(dict->GetTagged(key1, res1_tagged).To(&found));
  ASSERT_TRUE(found);
  Handle<PyObject> res1 = handle(res1_tagged);
  EXPECT_FALSE(res1.is_null());
  Handle<PyObject> eq_res;
  ASSERT_TRUE(PyObject::Equal(res1, val1).ToHandle(&eq_res));
  EXPECT_TRUE(Handle<PyBoolean>::cast(eq_res)->value());

  // Contains
  bool contains = false;
  ASSERT_TRUE(dict->ContainsKey(key1).To(&contains));
  EXPECT_TRUE(contains);
  Handle<PyObject> key2 = PyString::NewInstance("key2");
  ASSERT_TRUE(dict->ContainsKey(key2).To(&contains));
  EXPECT_TRUE(!contains);

  // Update
  Handle<PyObject> val2(PySmi::FromInt(200));
  ASSERT_FALSE(PyDict::Put(dict, key1, val2).IsNothing());
  EXPECT_EQ(dict->occupied(), 1);  // Size shouldn't change
  ASSERT_TRUE(dict->GetTagged(key1, res1_tagged).To(&found));
  ASSERT_TRUE(found);
  res1 = handle(res1_tagged);
  bool eq = false;
  ASSERT_TRUE(PyObject::EqualBool(res1, val2).To(&eq));
  EXPECT_TRUE(eq);

  // Remove
  bool removed = false;
  ASSERT_TRUE(dict->Remove(key1).To(&removed));
  ASSERT_TRUE(removed);
  EXPECT_EQ(dict->occupied(), 0);
  ASSERT_TRUE(dict->ContainsKey(key1).To(&contains));
  EXPECT_TRUE(!contains);
  ASSERT_TRUE(dict->GetTagged(key1, res1_tagged).To(&found));
  EXPECT_FALSE(found);
  EXPECT_TRUE(res1_tagged.is_null());
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
    ASSERT_FALSE(PyDict::Put(dict, key, val).IsNothing());
  }

  EXPECT_EQ(dict->occupied(), count);

  // Remove every second item
  for (int i = 0; i < count; i += 2) {
    Handle<PyObject> key(PySmi::FromInt(i));
    bool removed = false;
    ASSERT_TRUE(dict->Remove(key).To(&removed));
  }

  EXPECT_EQ(dict->occupied(), count / 2);

  // Verify remaining items
  for (int i = 1; i < count; i += 2) {
    Handle<PyObject> key(PySmi::FromInt(i));
    bool exists = false;
    ASSERT_TRUE(dict->ContainsKey(key).To(&exists));
    EXPECT_TRUE(exists);
    Tagged<PyObject> val_tagged;
    bool found = false;
    ASSERT_TRUE(dict->GetTagged(key, val_tagged).To(&found));
    ASSERT_TRUE(found);
    Handle<PyObject> val = handle(val_tagged);
    EXPECT_EQ(PySmi::ToInt(Handle<PySmi>::cast(val)), i * 10);
  }

  // Verify removed items are gone
  for (int i = 0; i < count; i += 2) {
    Handle<PyObject> key(PySmi::FromInt(i));
    bool exists = false;
    ASSERT_TRUE(dict->ContainsKey(key).To(&exists));
    EXPECT_TRUE(!exists);
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
  ASSERT_FALSE(PyDict::Put(d1, k1, v1).IsNothing());
  ASSERT_FALSE(PyDict::Put(d1, k2, v2).IsNothing());

  // d2 = {"b": 2, "a": 1} (Different insertion order, should be equal)
  ASSERT_FALSE(PyDict::Put(d2, k2, v2).IsNothing());
  ASSERT_FALSE(PyDict::Put(d2, k1, v1).IsNothing());

  bool eq = false;
  ASSERT_TRUE(PyObject::EqualBool(d1, d2).To(&eq));
  EXPECT_TRUE(eq);

  // d2 = {"b": 2, "a": 3} (Different value)
  Handle<PyObject> v3(PySmi::FromInt(3));
  ASSERT_FALSE(PyDict::Put(d2, k1, v3).IsNothing());
  ASSERT_TRUE(PyObject::EqualBool(d1, d2).To(&eq));
  EXPECT_FALSE(eq);

  // d2 = {"b": 2} (Different size)
  bool removed = false;
  ASSERT_TRUE(d2->Remove(k1).To(&removed));
  ASSERT_TRUE(PyObject::EqualBool(d1, d2).To(&eq));
  EXPECT_FALSE(eq);
}

TEST_F(PyDictTest, GetKeyTuple) {
  HandleScope scope;

  auto TupleContains = [](Handle<PyTuple> tuple, Handle<PyObject> key) -> bool {
    for (auto i = 0; i < tuple->length(); ++i) {
      Handle<PyObject> eq_res;
      if (PyObject::Equal(tuple->Get(i), key).ToHandle(&eq_res) &&
          Handle<PyBoolean>::cast(eq_res)->value()) {
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
    ASSERT_FALSE(PyDict::Put(dict, key, val).IsNothing());
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
      Handle<PyObject> eq_res;
      ASSERT_TRUE(
          PyObject::Equal(keys->Get(i), keys->Get(j)).ToHandle(&eq_res));
      EXPECT_FALSE(Handle<PyBoolean>::cast(eq_res)->value());
    }
  }

  for (int i = 0; i < count; i += 2) {
    Handle<PyObject> key(PySmi::FromInt(i));
    bool removed = false;
    ASSERT_TRUE(dict->Remove(key).To(&removed));
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
    ASSERT_FALSE(PyDict::Put(dict, key, val).IsNothing());
  }

  Handle<PyObject> iterator;
  ASSERT_TRUE(PyObject::Iter(dict).ToHandle(&iterator));
  ASSERT_TRUE(IsPyDictKeyIterator(iterator));

  std::set<int> seen;
  Handle<PyObject> key;
  while (PyObject::Next(iterator).ToHandle(&key)) {
    int v = PySmi::ToInt(Handle<PySmi>::cast(key));
    seen.insert(v);
  }

  EXPECT_EQ(static_cast<int>(seen.size()), count);
  for (int i = 0; i < count; ++i) {
    EXPECT_NE(seen.count(i), static_cast<size_t>(0));
  }
}

}  // namespace saauso::internal
