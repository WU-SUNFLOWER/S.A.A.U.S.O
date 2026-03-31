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

  Handle<PyDict> dict = PyDict::New(isolate_);
  Handle<PyObject> key = PyString::New(isolate_, "k");
  Handle<PyObject> value(PySmi::FromInt(1), isolate_);
  ASSERT_FALSE(PyDict::Put(dict, key, value, isolate_).IsNothing());

  Handle<PyObject> miss_key = PyString::New(isolate_, "missing");
  Tagged<PyObject> out_tagged;
  bool found = true;
  ASSERT_TRUE(dict->GetTagged(miss_key, out_tagged, isolate_).To(&found));
  EXPECT_FALSE(found);
  EXPECT_TRUE(out_tagged.is_null());
  EXPECT_FALSE(isolate->HasPendingException());

  ASSERT_TRUE(dict->GetTagged(key, out_tagged, isolate_).To(&found));
  EXPECT_TRUE(found);
  EXPECT_FALSE(out_tagged.is_null());
  bool eq = false;
  ASSERT_TRUE(
      PyObject::EqualBool(isolate_, handle(out_tagged, isolate_), value).To(&eq));
  EXPECT_TRUE(eq);

  Handle<PyObject> out_handle;
  ASSERT_TRUE(dict->Get(key, out_handle, isolate_).To(&found));
  EXPECT_TRUE(found);
  EXPECT_FALSE(out_handle.is_null());

  Handle<PyObject> bad_key = PyDict::New(isolate_);
  out_tagged = Tagged<PyObject>::null();
  EXPECT_TRUE(dict->GetTagged(bad_key, out_tagged, isolate_).IsNothing());
  EXPECT_TRUE(isolate->HasPendingException());
  EXPECT_TRUE(out_tagged.is_null());
  isolate->exception_state()->Clear();
}

TEST_F(PyDictTest, BasicOperations) {
  HandleScope scope;

  Handle<PyDict> dict = PyDict::New(isolate_);
  EXPECT_EQ(dict->occupied(), 0);

  Handle<PyObject> key1 = PyString::New(isolate_, "key1");
  Handle<PyObject> val1(PySmi::FromInt(100), isolate_);

  // Put
  ASSERT_FALSE(PyDict::Put(dict, key1, val1, isolate_).IsNothing());
  EXPECT_EQ(dict->occupied(), 1);

  // Get
  Tagged<PyObject> res1_tagged;
  bool found = false;
  ASSERT_TRUE(dict->GetTagged(key1, res1_tagged, isolate_).To(&found));
  ASSERT_TRUE(found);
  Handle<PyObject> res1 = handle(res1_tagged, isolate_);
  EXPECT_FALSE(res1.is_null());
  Handle<PyObject> eq_res;
  ASSERT_TRUE(PyObject::Equal(isolate_, res1, val1).ToHandle(&eq_res));
  EXPECT_TRUE(Handle<PyBoolean>::cast(eq_res)->value());

  // Contains
  bool contains = false;
  ASSERT_TRUE(dict->ContainsKey(key1, isolate_).To(&contains));
  EXPECT_TRUE(contains);
  Handle<PyObject> key2 = PyString::New(isolate_, "key2");
  ASSERT_TRUE(dict->ContainsKey(key2, isolate_).To(&contains));
  EXPECT_TRUE(!contains);

  // Update
  Handle<PyObject> val2(PySmi::FromInt(200), isolate_);
  ASSERT_FALSE(PyDict::Put(dict, key1, val2, isolate_).IsNothing());
  EXPECT_EQ(dict->occupied(), 1);  // Size shouldn't change
  ASSERT_TRUE(dict->GetTagged(key1, res1_tagged, isolate_).To(&found));
  ASSERT_TRUE(found);
  res1 = handle(res1_tagged, isolate_);
  bool eq = false;
  ASSERT_TRUE(PyObject::EqualBool(isolate_, res1, val2).To(&eq));
  EXPECT_TRUE(eq);

  // Remove
  bool removed = false;
  ASSERT_TRUE(dict->Remove(key1, isolate_).To(&removed));
  ASSERT_TRUE(removed);
  EXPECT_EQ(dict->occupied(), 0);
  ASSERT_TRUE(dict->ContainsKey(key1, isolate_).To(&contains));
  EXPECT_TRUE(!contains);
  ASSERT_TRUE(dict->GetTagged(key1, res1_tagged, isolate_).To(&found));
  EXPECT_FALSE(found);
  EXPECT_TRUE(res1_tagged.is_null());
}

TEST_F(PyDictTest, CollisionAndShift) {
  HandleScope scope;

  // Create dict with default capacity (min 2, usually expands)
  Handle<PyDict> dict = PyDict::New(isolate_);

  // Insert multiple items
  int count = 10;
  for (int i = 0; i < count; ++i) {
    Handle<PyObject> key(PySmi::FromInt(i), isolate_);
    Handle<PyObject> val(PySmi::FromInt(i * 10), isolate_);
    ASSERT_FALSE(PyDict::Put(dict, key, val, isolate_).IsNothing());
  }

  EXPECT_EQ(dict->occupied(), count);

  // Remove every second item
  for (int i = 0; i < count; i += 2) {
    Handle<PyObject> key(PySmi::FromInt(i), isolate_);
    bool removed = false;
    ASSERT_TRUE(dict->Remove(key, isolate_).To(&removed));
  }

  EXPECT_EQ(dict->occupied(), count / 2);

  // Verify remaining items
  for (int i = 1; i < count; i += 2) {
    Handle<PyObject> key(PySmi::FromInt(i), isolate_);
    bool exists = false;
    ASSERT_TRUE(dict->ContainsKey(key, isolate_).To(&exists));
    EXPECT_TRUE(exists);
    Tagged<PyObject> val_tagged;
    bool found = false;
    ASSERT_TRUE(dict->GetTagged(key, val_tagged, isolate_).To(&found));
    ASSERT_TRUE(found);
    Handle<PyObject> val = handle(val_tagged, isolate_);
    EXPECT_EQ(PySmi::ToInt(Handle<PySmi>::cast(val)), i * 10);
  }

  // Verify removed items are gone
  for (int i = 0; i < count; i += 2) {
    Handle<PyObject> key(PySmi::FromInt(i), isolate_);
    bool exists = false;
    ASSERT_TRUE(dict->ContainsKey(key, isolate_).To(&exists));
    EXPECT_TRUE(!exists);
  }
}

TEST_F(PyDictTest, Equality) {
  HandleScope scope;

  Handle<PyDict> d1 = PyDict::New(isolate_);
  Handle<PyDict> d2 = PyDict::New(isolate_);

  Handle<PyObject> k1 = PyString::New(isolate_, "a");
  Handle<PyObject> v1(PySmi::FromInt(1), isolate_);
  Handle<PyObject> k2 = PyString::New(isolate_, "b");
  Handle<PyObject> v2(PySmi::FromInt(2), isolate_);

  // d1 = {"a": 1, "b": 2}
  ASSERT_FALSE(PyDict::Put(d1, k1, v1, isolate_).IsNothing());
  ASSERT_FALSE(PyDict::Put(d1, k2, v2, isolate_).IsNothing());

  // d2 = {"b": 2, "a": 1} (Different insertion order, should be equal)
  ASSERT_FALSE(PyDict::Put(d2, k2, v2, isolate_).IsNothing());
  ASSERT_FALSE(PyDict::Put(d2, k1, v1, isolate_).IsNothing());

  bool eq = false;
  ASSERT_TRUE(PyObject::EqualBool(isolate_, d1, d2).To(&eq));
  EXPECT_TRUE(eq);

  // d2 = {"b": 2, "a": 3} (Different value)
  Handle<PyObject> v3(PySmi::FromInt(3), isolate_);
  ASSERT_FALSE(PyDict::Put(d2, k1, v3, isolate_).IsNothing());
  ASSERT_TRUE(PyObject::EqualBool(isolate_, d1, d2).To(&eq));
  EXPECT_FALSE(eq);

  // d2 = {"b": 2} (Different size)
  bool removed = false;
  ASSERT_TRUE(d2->Remove(k1, isolate_).To(&removed));
  ASSERT_TRUE(PyObject::EqualBool(isolate_, d1, d2).To(&eq));
  EXPECT_FALSE(eq);
}

TEST_F(PyDictTest, GetKeyTuple) {
  HandleScope scope;

  auto TupleContains = [](Handle<PyTuple> tuple, Handle<PyObject> key) -> bool {
    for (auto i = 0; i < tuple->length(); ++i) {
      Handle<PyObject> eq_res;
      if (PyObject::Equal(isolate_, tuple->Get(i, isolate_), key)
              .ToHandle(&eq_res) &&
          Handle<PyBoolean>::cast(eq_res)->value()) {
        return true;
      }
    }
    return false;
  };

  Handle<PyDict> dict = PyDict::New(isolate_);

  int count = 20;
  for (int i = 0; i < count; ++i) {
    Handle<PyObject> key(PySmi::FromInt(i), isolate_);
    Handle<PyObject> val(PySmi::FromInt(i * 10), isolate_);
    ASSERT_FALSE(PyDict::Put(dict, key, val, isolate_).IsNothing());
  }

  Handle<PyTuple> keys = PyDict::GetKeyTuple(dict, isolate_);
  EXPECT_EQ(keys->length(), dict->occupied());
  EXPECT_EQ(keys->length(), count);

  for (int i = 0; i < count; ++i) {
    Handle<PyObject> key(PySmi::FromInt(i), isolate_);
    EXPECT_TRUE(TupleContains(keys, key));
  }

  for (auto i = 0; i < keys->length(); ++i) {
    EXPECT_FALSE(keys->Get(i, isolate_).is_null());
  }

  for (auto i = 0; i < keys->length(); ++i) {
    for (auto j = i + 1; j < keys->length(); ++j) {
      Handle<PyObject> eq_res;
      ASSERT_TRUE(PyObject::Equal(isolate_, keys->Get(i, isolate_),
                                  keys->Get(j, isolate_))
                      .ToHandle(&eq_res));
      EXPECT_FALSE(Handle<PyBoolean>::cast(eq_res)->value());
    }
  }

  for (int i = 0; i < count; i += 2) {
    Handle<PyObject> key(PySmi::FromInt(i), isolate_);
    bool removed = false;
    ASSERT_TRUE(dict->Remove(key, isolate_).To(&removed));
  }

  Handle<PyTuple> keys_after_remove = PyDict::GetKeyTuple(dict, isolate_);
  EXPECT_EQ(keys_after_remove->length(), dict->occupied());
  EXPECT_EQ(keys_after_remove->length(), count / 2);

  for (int i = 1; i < count; i += 2) {
    Handle<PyObject> key(PySmi::FromInt(i));
    EXPECT_TRUE(TupleContains(keys_after_remove, key));
  }
}

TEST_F(PyDictTest, IteratorIteratesKeys) {
  HandleScope scope;

  Handle<PyDict> dict = PyDict::New(isolate_);
  int count = 50;
  for (int i = 0; i < count; ++i) {
    Handle<PyObject> key(PySmi::FromInt(i), isolate_);
    Handle<PyObject> val(PySmi::FromInt(i * 10), isolate_);
    ASSERT_FALSE(PyDict::Put(dict, key, val, isolate_).IsNothing());
  }

  Handle<PyObject> iterator;
  ASSERT_TRUE(PyObject::Iter(isolate_, dict).ToHandle(&iterator));
  ASSERT_TRUE(IsPyDictKeyIterator(iterator));

  std::set<int> seen;
  Handle<PyObject> key;
  while (PyObject::Next(isolate_, iterator).ToHandle(&key)) {
    int v = PySmi::ToInt(Handle<PySmi>::cast(key));
    seen.insert(v);
  }

  EXPECT_EQ(static_cast<int>(seen.size()), count);
  for (int i = 0; i < count; ++i) {
    EXPECT_NE(seen.count(i), static_cast<size_t>(0));
  }
}

}  // namespace saauso::internal
