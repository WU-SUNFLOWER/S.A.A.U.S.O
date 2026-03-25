// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include <cstdint>
#include <cstring>
#include <limits>

#include "src/execution/isolate.h"
#include "src/objects/py-dict.h"
#include "src/objects/py-object.h"
#include "src/objects/py-oddballs.h"
#include "src/objects/py-smi.h"
#include "src/objects/py-string-klass.h"
#include "src/objects/py-string.h"
#include "src/objects/py-tuple.h"
#include "test/unittests/test-helpers.h"
#include "test/unittests/test-utils.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace saauso::internal {

class PyStringTest : public VmTestBase {};

TEST_F(PyStringTest, NewInstanceFromCString) {
  HandleScope scope;

  auto s = PyString::NewInstance("Hello World");
  EXPECT_FALSE(s.is_null());
  EXPECT_TRUE(IsPyStringEqual(s, "Hello World"));
}

TEST_F(PyStringTest, NewInstanceWithLengthAndSetGet) {
  HandleScope scope;

  auto s = PyString::NewInstance(5);
  EXPECT_EQ(s->length(), 5);

  const char* payload = "abcde";
  for (int64_t i = 0; i < s->length(); ++i) {
    s->Set(i, payload[i]);
  }
  for (int64_t i = 0; i < s->length(); ++i) {
    EXPECT_EQ(s->Get(i), payload[i]);
  }
}

TEST_F(PyStringTest, HashCacheWorks) {
  HandleScope scope;

  auto s = PyString::NewInstance("hash-me");
  EXPECT_FALSE(s->HasHashCache());

  const uint64_t h1 = s->GetHash();
  EXPECT_TRUE(s->HasHashCache());

  const uint64_t h2 = s->GetHash();
  EXPECT_EQ(h1, h2);
}

TEST_F(PyStringTest, EqualityAndOrderingPrimitives) {
  HandleScope scope;

  auto a1 = PyString::NewInstance("abc");
  auto a2 = PyString::NewInstance("abc");
  auto b = PyString::NewInstance("abd");
  auto short_a = PyString::NewInstance("ab");

  EXPECT_TRUE(a1->IsEqualTo(*a2));
  EXPECT_FALSE(a1->IsEqualTo(*b));

  EXPECT_TRUE(a1->IsLessThan(*b));
  EXPECT_TRUE(b->IsGreaterThan(*a1));

  EXPECT_TRUE(short_a->IsLessThan(*a1));
  EXPECT_TRUE(a1->IsGreaterThan(*short_a));
}

TEST_F(PyStringTest, PyObjectComparisonsWork) {
  HandleScope scope;

  Handle<PyObject> a(PyString::NewInstance("abc"));
  Handle<PyObject> b(PyString::NewInstance("abd"));
  Handle<PyObject> a2(PyString::NewInstance("abc"));

  Handle<PyObject> res;
  ASSERT_TRUE(PyObject::Less(a, b).ToHandle(&res));
  EXPECT_TRUE(Handle<PyBoolean>::cast(res)->value());
  ASSERT_TRUE(PyObject::Greater(b, a).ToHandle(&res));
  EXPECT_TRUE(Handle<PyBoolean>::cast(res)->value());
  ASSERT_TRUE(PyObject::Equal(a, a2).ToHandle(&res));
  EXPECT_TRUE(Handle<PyBoolean>::cast(res)->value());
  ASSERT_TRUE(PyObject::NotEqual(a, b).ToHandle(&res));
  EXPECT_TRUE(Handle<PyBoolean>::cast(res)->value());
  ASSERT_TRUE(PyObject::LessEqual(a, a2).ToHandle(&res));
  EXPECT_TRUE(Handle<PyBoolean>::cast(res)->value());
  ASSERT_TRUE(PyObject::GreaterEqual(a, a2).ToHandle(&res));
  EXPECT_TRUE(Handle<PyBoolean>::cast(res)->value());
}

TEST_F(PyStringTest, SliceWorks) {
  HandleScope scope;

  auto s = PyString::NewInstance("Hello World");
  auto sub = PyString::Slice(s, 0, 4);
  EXPECT_TRUE(IsPyStringEqual(sub, "Hello"));

  auto tail = PyString::Slice(s, 6, 10);
  EXPECT_TRUE(IsPyStringEqual(tail, "World"));
}

TEST_F(PyStringTest, AppendWorksAndDoesNotMutateInputs) {
  HandleScope scope;

  auto left = PyString::NewInstance("Hello");
  auto right = PyString::NewInstance(" World");

  auto appended = PyString::Append(left, right);
  EXPECT_TRUE(IsPyStringEqual(appended, "Hello World"));

  EXPECT_TRUE(IsPyStringEqual(left, "Hello"));
  EXPECT_TRUE(IsPyStringEqual(right, " World"));
}

TEST_F(PyStringTest, PyObjectAddConcatenatesStrings) {
  HandleScope scope;

  Handle<PyObject> left(PyString::NewInstance("Hello"));
  Handle<PyObject> right(PyString::NewInstance(" World"));
  Handle<PyObject> result;
  ASSERT_TRUE(
      PyObject::Add(isolate_, left, right).ToHandle(&result));
  ASSERT_TRUE(IsPyString(result));
  EXPECT_TRUE(IsPyStringEqual(Handle<PyString>::cast(result), "Hello World"));
}

TEST_F(PyStringTest, PyObjectSubscrReturnsSingleCharString) {
  HandleScope scope;

  Handle<PyObject> s(PyString::NewInstance("abc"));
  Handle<PyObject> index(PySmi::FromInt(1));

  Handle<PyObject> result;
  ASSERT_TRUE(PyObject::Subscr(s, index).ToHandle(&result));
  ASSERT_TRUE(IsPyString(result));

  auto char_str = Handle<PyString>::cast(result);
  ASSERT_EQ(char_str->length(), 1);
  EXPECT_EQ(char_str->Get(0), 'b');
}

TEST_F(PyStringTest, IndexOfAndContainsWork) {
  HandleScope scope;

  auto s = PyString::NewInstance("abcabcabcd");
  auto pattern1 = PyString::NewInstance("abcabcd");
  auto pattern2 = PyString::NewInstance("bca");
  auto missing = PyString::NewInstance("xyz");
  auto empty = PyString::NewInstance("");

  EXPECT_EQ(s->IndexOf(pattern1), 3);
  EXPECT_EQ(s->IndexOf(pattern2), 1);
  EXPECT_EQ(s->IndexOf(missing), PyString::kNotFound);
  EXPECT_EQ(s->IndexOf(empty), 0);

  EXPECT_TRUE(s->Contains(pattern1));
  EXPECT_TRUE(s->Contains(pattern2));
  EXPECT_FALSE(s->Contains(missing));
  EXPECT_TRUE(s->Contains(empty));
}

TEST_F(PyStringTest, LastIndexOfWorks) {
  HandleScope scope;

  auto s = PyString::NewInstance("abcabcabcd");
  auto pattern = PyString::NewInstance("abc");
  auto missing = PyString::NewInstance("xyz");
  auto empty = PyString::NewInstance("");

  EXPECT_EQ(s->LastIndexOf(pattern), 6);
  EXPECT_EQ(s->LastIndexOf(missing), PyString::kNotFound);
  EXPECT_EQ(s->LastIndexOf(empty), s->length());

  EXPECT_EQ(s->LastIndexOf(pattern, 0, 7), 3);
  EXPECT_EQ(s->LastIndexOf(pattern, 2, 7), 3);
  EXPECT_EQ(s->LastIndexOf(empty, 2, 7), 7);
}

TEST_F(PyStringTest, PyObjectContainsWorksForStrings) {
  HandleScope scope;

  Handle<PyObject> s(PyString::NewInstance("Hello World"));
  Handle<PyObject> pat(PyString::NewInstance("World"));
  Handle<PyObject> missing(PyString::NewInstance("planet"));
  Handle<PyObject> not_a_string(PySmi::FromInt(1));

  Handle<PyObject> contains_res;
  ASSERT_TRUE(PyObject::Contains(s, pat).ToHandle(&contains_res));
  EXPECT_TRUE(IsPyTrue(*contains_res));
  ASSERT_TRUE(PyObject::Contains(s, missing).ToHandle(&contains_res));
  EXPECT_TRUE(IsPyFalse(*contains_res));
  ASSERT_TRUE(PyObject::Contains(s, not_a_string).ToHandle(&contains_res));
  EXPECT_TRUE(IsPyFalse(*contains_res));
}

TEST_F(PyStringTest, FromIntAndFromDoubleWork) {
  HandleScope scope;

  EXPECT_TRUE(IsPyStringEqual(PyString::FromInt(0), "0"));
  EXPECT_TRUE(IsPyStringEqual(PyString::FromInt(-42), "-42"));
  EXPECT_TRUE(IsPyStringEqual(PyString::FromPySmi(PySmi::FromInt(233)), "233"));

  EXPECT_TRUE(IsPyStringEqual(PyString::FromDouble(1.0), "1.0"));
  EXPECT_TRUE(IsPyStringEqual(PyString::FromDouble(1000000.0), "1000000.0"));
  EXPECT_TRUE(IsPyStringEqual(PyString::FromDouble(1e-5), "1e-05"));
  EXPECT_TRUE(IsPyStringEqual(PyString::FromDouble(1e16), "1e+16"));
  EXPECT_TRUE(IsPyStringEqual(
      PyString::FromDouble(std::numeric_limits<double>::infinity()), "inf"));
  EXPECT_TRUE(IsPyStringEqual(
      PyString::FromDouble(-std::numeric_limits<double>::infinity()), "-inf"));
}

TEST_F(PyStringTest, StringUpperMethod) {
  HandleScope scope;

  Handle<PyObject> s(PyString::NewInstance("Hello World"));
  Handle<PyObject> attr(PyString::NewInstance("upper"));

  Handle<PyDict> attrs =
      PyStringKlass::GetInstance(isolate_)->klass_properties();

  bool flag = false;
  Handle<PyTuple> attrs_tuple = PyDict::GetKeyTuple(attrs);
  for (auto i = 0; i < attrs_tuple->length(); ++i) {
    bool eq = false;
    if (PyObject::EqualBool(attr, attrs_tuple->Get(i)).To(&eq) && eq) {
      flag = true;
      break;
    }
  }

  EXPECT_TRUE(flag);

  bool contains = false;
  ASSERT_TRUE(PyStringKlass::GetInstance(isolate_)
                  ->klass_properties()
                  ->ContainsKey(attr)
                  .To(&contains));
  EXPECT_TRUE(contains);

  Handle<PyObject> upper_method;
  ASSERT_TRUE(PyObject::GetAttr(s, attr).ToHandle(&upper_method));
  EXPECT_FALSE(upper_method.is_null());
}

}  // namespace saauso::internal
