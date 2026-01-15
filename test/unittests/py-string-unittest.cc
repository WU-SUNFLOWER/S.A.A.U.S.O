// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include <cstdint>
#include <cstring>

#include "src/objects/py-object.h"
#include "src/objects/py-oddballs.h"
#include "src/objects/py-smi.h"
#include "src/objects/py-string.h"
#include "src/runtime/isolate.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace saauso::internal {

// 说明：
// - Isolate::Create()/Dispose() 用于初始化/销毁虚拟机运行时。
// - 每个测试用例内部创建 HandleScope，避免 GC 触发时句柄失效。
class PyStringTest : public testing::Test {
 protected:
  static void SetUpTestSuite() {
    isolate_ = Isolate::Create();
    Isolate::SetCurrent(isolate_);
  }
  static void TearDownTestSuite() {
    Isolate::SetCurrent(nullptr);
    Isolate::Dispose(isolate_);
    isolate_ = nullptr;
  }

  static Isolate* isolate_;
};

Isolate* PyStringTest::isolate_ = nullptr;

static void ExpectStringEquals(Handle<PyString> s, const char* expected) {
  const int64_t len = static_cast<int64_t>(std::strlen(expected));
  ASSERT_EQ(s->length(), len);
  ASSERT_EQ(std::memcmp(s->buffer(), expected, len), 0);
}

TEST_F(PyStringTest, NewInstanceFromCString) {
  HandleScope scope;

  auto s = PyString::NewInstance("Hello World");
  EXPECT_FALSE(s.IsNull());
  ExpectStringEquals(s, "Hello World");
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

  EXPECT_EQ(PyObject::Less(a, b).ptr(),
            Isolate::Current()->py_true_object().ptr());
  EXPECT_EQ(PyObject::Greater(b, a).ptr(),
            Isolate::Current()->py_true_object().ptr());
  EXPECT_EQ(PyObject::Equal(a, a2).ptr(),
            Isolate::Current()->py_true_object().ptr());
  EXPECT_EQ(PyObject::NotEqual(a, b).ptr(),
            Isolate::Current()->py_true_object().ptr());
  EXPECT_EQ(PyObject::LessEqual(a, a2).ptr(),
            Isolate::Current()->py_true_object().ptr());
  EXPECT_EQ(PyObject::GreaterEqual(a, a2).ptr(),
            Isolate::Current()->py_true_object().ptr());
}

TEST_F(PyStringTest, SliceWorks) {
  HandleScope scope;

  auto s = PyString::NewInstance("Hello World");
  auto sub = PyString::Slice(s, 0, 4);
  ExpectStringEquals(sub, "Hello");

  auto tail = PyString::Slice(s, 6, 10);
  ExpectStringEquals(tail, "World");
}

TEST_F(PyStringTest, AppendWorksAndDoesNotMutateInputs) {
  HandleScope scope;

  auto left = PyString::NewInstance("Hello");
  auto right = PyString::NewInstance(" World");

  auto appended = PyString::Append(left, right);
  ExpectStringEquals(appended, "Hello World");

  ExpectStringEquals(left, "Hello");
  ExpectStringEquals(right, " World");
}

TEST_F(PyStringTest, PyObjectAddConcatenatesStrings) {
  HandleScope scope;

  Handle<PyObject> left(PyString::NewInstance("Hello"));
  Handle<PyObject> right(PyString::NewInstance(" World"));
  Handle<PyObject> result = PyObject::Add(left, right);

  ASSERT_TRUE(IsPyString(result));
  ExpectStringEquals(Handle<PyString>::cast(result), "Hello World");
}

TEST_F(PyStringTest, PyObjectSubscrReturnsSingleCharString) {
  HandleScope scope;

  Handle<PyObject> s(PyString::NewInstance("abc"));
  Handle<PyObject> index(PySmi::FromInt(1));

  Handle<PyObject> result = PyObject::Subscr(s, index);
  ASSERT_TRUE(IsPyString(result));

  auto char_str = Handle<PyString>::cast(result);
  ASSERT_EQ(char_str->length(), 1);
  EXPECT_EQ(char_str->Get(0), 'b');
}

}  // namespace saauso::internal
