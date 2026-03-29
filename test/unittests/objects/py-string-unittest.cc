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

  auto s = PyString::New(isolate_, "Hello World");
  EXPECT_FALSE(s.is_null());
  EXPECT_TRUE(IsPyStringEqual(s, "Hello World"));
}

TEST_F(PyStringTest, NewInstanceWithLengthAndSetGet) {
  HandleScope scope;

  auto s = PyString::New(isolate_, 5);
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

  auto s = PyString::New(isolate_, "hash-me");
  EXPECT_FALSE(s->HasHashCache());

  const uint64_t h1 = s->GetHash();
  EXPECT_TRUE(s->HasHashCache());

  const uint64_t h2 = s->GetHash();
  EXPECT_EQ(h1, h2);
}

TEST_F(PyStringTest, EqualityAndOrderingPrimitives) {
  HandleScope scope;

  auto a1 = PyString::New(isolate_, "abc");
  auto a2 = PyString::New(isolate_, "abc");
  auto b = PyString::New(isolate_, "abd");
  auto short_a = PyString::New(isolate_, "ab");

  EXPECT_TRUE(a1->IsEqualTo(*a2));
  EXPECT_FALSE(a1->IsEqualTo(*b));

  EXPECT_TRUE(a1->IsLessThan(*b));
  EXPECT_TRUE(b->IsGreaterThan(*a1));

  EXPECT_TRUE(short_a->IsLessThan(*a1));
  EXPECT_TRUE(a1->IsGreaterThan(*short_a));
}

TEST_F(PyStringTest, PyObjectComparisonsWork) {
  HandleScope scope;

  Handle<PyObject> a(PyString::New(isolate_, "abc"));
  Handle<PyObject> b(PyString::New(isolate_, "abd"));
  Handle<PyObject> a2(PyString::New(isolate_, "abc"));

  Handle<PyObject> res;
  ASSERT_TRUE(PyObject::Less(isolate_, a, b).ToHandle(&res));
  EXPECT_TRUE(Handle<PyBoolean>::cast(res)->value());
  ASSERT_TRUE(PyObject::Greater(isolate_, b, a).ToHandle(&res));
  EXPECT_TRUE(Handle<PyBoolean>::cast(res)->value());
  ASSERT_TRUE(PyObject::Equal(isolate_, a, a2).ToHandle(&res));
  EXPECT_TRUE(Handle<PyBoolean>::cast(res)->value());
  ASSERT_TRUE(PyObject::NotEqual(isolate_, a, b).ToHandle(&res));
  EXPECT_TRUE(Handle<PyBoolean>::cast(res)->value());
  ASSERT_TRUE(PyObject::LessEqual(isolate_, a, a2).ToHandle(&res));
  EXPECT_TRUE(Handle<PyBoolean>::cast(res)->value());
  ASSERT_TRUE(PyObject::GreaterEqual(isolate_, a, a2).ToHandle(&res));
  EXPECT_TRUE(Handle<PyBoolean>::cast(res)->value());
}

TEST_F(PyStringTest, SliceWorks) {
  HandleScope scope;

  auto s = PyString::New(isolate_, "Hello World");
  auto sub = PyString::Slice(s, 0, 4, isolate_);
  EXPECT_TRUE(IsPyStringEqual(sub, "Hello"));

  auto tail = PyString::Slice(s, 6, 10, isolate_);
  EXPECT_TRUE(IsPyStringEqual(tail, "World"));
}

TEST_F(PyStringTest, AppendWorksAndDoesNotMutateInputs) {
  HandleScope scope;

  auto left = PyString::New(isolate_, "Hello");
  auto right = PyString::New(isolate_, " World");

  auto appended = PyString::Append(left, right, isolate_);
  EXPECT_TRUE(IsPyStringEqual(appended, "Hello World"));

  EXPECT_TRUE(IsPyStringEqual(left, "Hello"));
  EXPECT_TRUE(IsPyStringEqual(right, " World"));
}

TEST_F(PyStringTest, PyObjectAddConcatenatesStrings) {
  HandleScope scope;

  Handle<PyObject> left(PyString::New(isolate_, "Hello"));
  Handle<PyObject> right(PyString::New(isolate_, " World"));
  Handle<PyObject> result;
  ASSERT_TRUE(PyObject::Add(isolate_, left, right).ToHandle(&result));
  ASSERT_TRUE(IsPyString(result));
  EXPECT_TRUE(IsPyStringEqual(Handle<PyString>::cast(result), "Hello World"));
}

TEST_F(PyStringTest, PyObjectSubscrReturnsSingleCharString) {
  HandleScope scope;

  Handle<PyObject> s(PyString::New(isolate_, "abc"));
  Handle<PyObject> index(PySmi::FromInt(1));

  Handle<PyObject> result;
  ASSERT_TRUE(PyObject::Subscr(isolate_, s, index).ToHandle(&result));
  ASSERT_TRUE(IsPyString(result));

  auto char_str = Handle<PyString>::cast(result);
  ASSERT_EQ(char_str->length(), 1);
  EXPECT_EQ(char_str->Get(0), 'b');
}

TEST_F(PyStringTest, IndexOfAndContainsWork) {
  HandleScope scope;

  auto s = PyString::New(isolate_, "abcabcabcd");
  auto pattern1 = PyString::New(isolate_, "abcabcd");
  auto pattern2 = PyString::New(isolate_, "bca");
  auto missing = PyString::New(isolate_, "xyz");
  auto empty = PyString::New(isolate_, "");

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

  auto s = PyString::New(isolate_, "abcabcabcd");
  auto pattern = PyString::New(isolate_, "abc");
  auto missing = PyString::New(isolate_, "xyz");
  auto empty = PyString::New(isolate_, "");

  EXPECT_EQ(s->LastIndexOf(pattern), 6);
  EXPECT_EQ(s->LastIndexOf(missing), PyString::kNotFound);
  EXPECT_EQ(s->LastIndexOf(empty), s->length());

  EXPECT_EQ(s->LastIndexOf(pattern, 0, 7), 3);
  EXPECT_EQ(s->LastIndexOf(pattern, 2, 7), 3);
  EXPECT_EQ(s->LastIndexOf(empty, 2, 7), 7);
}

TEST_F(PyStringTest, PyObjectContainsWorksForStrings) {
  HandleScope scope;

  Handle<PyObject> s(PyString::New(isolate_, "Hello World"));
  Handle<PyObject> pat(PyString::New(isolate_, "World"));
  Handle<PyObject> missing(PyString::New(isolate_, "planet"));
  Handle<PyObject> not_a_string(PySmi::FromInt(1));

  Handle<PyObject> contains_res;
  ASSERT_TRUE(PyObject::Contains(isolate_, s, pat).ToHandle(&contains_res));
  EXPECT_TRUE(IsPyTrue(contains_res, isolate_));
  ASSERT_TRUE(PyObject::Contains(isolate_, s, missing).ToHandle(&contains_res));
  EXPECT_TRUE(IsPyFalse(contains_res, isolate_));
  ASSERT_TRUE(
      PyObject::Contains(isolate_, s, not_a_string).ToHandle(&contains_res));
  EXPECT_TRUE(IsPyFalse(contains_res, isolate_));
}

TEST_F(PyStringTest, FromIntAndFromDoubleWork) {
  HandleScope scope;

  EXPECT_TRUE(IsPyStringEqual(PyString::FromInt(isolate_, 0), "0"));
  EXPECT_TRUE(IsPyStringEqual(PyString::FromInt(isolate_, -42), "-42"));
  EXPECT_TRUE(IsPyStringEqual(
      PyString::FromPySmi(isolate_, PySmi::FromInt(233)), "233"));

  EXPECT_TRUE(IsPyStringEqual(PyString::FromDouble(isolate_, 1.0), "1.0"));
  EXPECT_TRUE(
      IsPyStringEqual(PyString::FromDouble(isolate_, 1000000.0), "1000000.0"));
  EXPECT_TRUE(IsPyStringEqual(PyString::FromDouble(isolate_, 1e-5), "1e-05"));
  EXPECT_TRUE(IsPyStringEqual(PyString::FromDouble(isolate_, 1e16), "1e+16"));
  EXPECT_TRUE(IsPyStringEqual(
      PyString::FromDouble(isolate_, std::numeric_limits<double>::infinity()),
      "inf"));
  EXPECT_TRUE(IsPyStringEqual(
      PyString::FromDouble(isolate_, -std::numeric_limits<double>::infinity()),
      "-inf"));
}

TEST_F(PyStringTest, StringUpperMethod) {
  HandleScope scope;

  Handle<PyObject> s(PyString::New(isolate_, "Hello World"));
  Handle<PyObject> attr(PyString::New(isolate_, "upper"));

  Handle<PyDict> attrs =
      PyStringKlass::GetInstance(isolate_)->klass_properties();

  bool flag = false;
  Handle<PyTuple> attrs_tuple = PyDict::GetKeyTuple(attrs, isolate_);
  for (auto i = 0; i < attrs_tuple->length(); ++i) {
    bool eq = false;
    if (PyObject::EqualBool(isolate_, attr, attrs_tuple->Get(i)).To(&eq) &&
        eq) {
      flag = true;
      break;
    }
  }

  EXPECT_TRUE(flag);

  bool contains = false;
  ASSERT_TRUE(PyStringKlass::GetInstance(isolate_)
                  ->klass_properties()
                  ->ContainsKey(attr, isolate_)
                  .To(&contains));
  EXPECT_TRUE(contains);

  Handle<PyObject> upper_method;
  ASSERT_TRUE(PyObject::GetAttr(isolate_, s, attr).ToHandle(&upper_method));
  EXPECT_FALSE(upper_method.is_null());
}

}  // namespace saauso::internal
