// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "gtest/gtest.h"
#include "src/builtins/builtins-definitions.h"
#include "src/handles/handles.h"
#include "src/interpreter/interpreter.h"
#include "src/objects/py-dict.h"
#include "src/objects/py-list.h"
#include "src/objects/py-string.h"
#include "src/objects/py-tuple.h"
#include "src/runtime/string-table.h"
#include "test/unittests/test-helpers.h"

namespace saauso::internal {

class NativePrintTest : public VmTestBase {};

TEST_F(NativePrintTest, DefaultSepAndEnd) {
  HandleScope scope;

  auto args = PyTuple::New(isolate_, 2);
  args->SetInternal(0, PyString::NewInstance("a"));
  args->SetInternal(1, PyString::NewInstance("b"));
  auto kwargs = PyDict::New(isolate_);

  testing::internal::CaptureStdout();
  auto maybe_result = BUILTIN_FUNC_NAME(Print)(
      isolate_, Handle<PyObject>::null(), args, kwargs);
  ASSERT_FALSE(maybe_result.IsEmpty());
  std::string out = testing::internal::GetCapturedStdout();
  EXPECT_EQ(out, "a b\n");
}

TEST_F(NativePrintTest, EndParameter) {
  HandleScope scope;

  auto args = PyTuple::New(isolate_, 2);
  args->SetInternal(0, PyString::NewInstance("a"));
  args->SetInternal(1, PyString::NewInstance("b"));
  auto kwargs = PyDict::New(isolate_);
  ASSERT_FALSE(
      PyDict::Put(kwargs, ST(end), PyString::NewInstance("!"), isolate_)
          .IsNothing());

  testing::internal::CaptureStdout();
  auto maybe_result = BUILTIN_FUNC_NAME(Print)(
      isolate_, Handle<PyObject>::null(), args, kwargs);
  ASSERT_FALSE(maybe_result.IsEmpty());
  std::string out = testing::internal::GetCapturedStdout();
  EXPECT_EQ(out, "a b!");
}

TEST_F(NativePrintTest, EolKeywordRejected) {
  HandleScope scope;

  auto args = PyTuple::New(isolate_, 2);
  args->SetInternal(0, PyString::NewInstance("a"));
  args->SetInternal(1, PyString::NewInstance("b"));
  auto kwargs = PyDict::New(isolate_);
  ASSERT_FALSE(
      PyDict::Put(kwargs, ST(eol), PyString::NewInstance("??"), isolate_)
          .IsNothing());
  auto maybe_result = BUILTIN_FUNC_NAME(Print)(
      isolate_, Handle<PyObject>::null(), args, kwargs);
  ASSERT_TRUE(maybe_result.IsEmpty());
  ASSERT_TRUE(isolate_->HasPendingException());
  isolate_->interpreter()->ClearPendingException();
}

TEST_F(NativePrintTest, SepParameter) {
  HandleScope scope;

  auto args = PyTuple::New(isolate_, 3);
  args->SetInternal(0, PyString::NewInstance("a"));
  args->SetInternal(1, PyString::NewInstance("b"));
  args->SetInternal(2, PyString::NewInstance("c"));
  auto kwargs = PyDict::New(isolate_);
  ASSERT_FALSE(
      PyDict::Put(kwargs, ST(sep), PyString::NewInstance(","), isolate_)
          .IsNothing());

  testing::internal::CaptureStdout();
  auto maybe_result = BUILTIN_FUNC_NAME(Print)(
      isolate_, Handle<PyObject>::null(), args, kwargs);
  ASSERT_FALSE(maybe_result.IsEmpty());
  std::string out = testing::internal::GetCapturedStdout();
  EXPECT_EQ(out, "a,b,c\n");
}

TEST_F(NativePrintTest, NoArgsUsesEnd) {
  HandleScope scope;

  auto args = PyTuple::New(isolate_, 0);
  auto kwargs = PyDict::New(isolate_);
  ASSERT_FALSE(
      PyDict::Put(kwargs, ST(end), PyString::NewInstance("X"), isolate_)
          .IsNothing());

  testing::internal::CaptureStdout();
  auto maybe_result = BUILTIN_FUNC_NAME(Print)(
      isolate_, Handle<PyObject>::null(), args, kwargs);
  ASSERT_FALSE(maybe_result.IsEmpty());
  std::string out = testing::internal::GetCapturedStdout();
  EXPECT_EQ(out, "X");
}

TEST_F(NativePrintTest, ReprBuiltinWorksForStringAndList) {
  HandleScope scope;

  auto repr_args = PyTuple::New(isolate_, 1);
  repr_args->SetInternal(0, PyString::NewInstance("abc"));
  auto repr_result = BUILTIN_FUNC_NAME(Repr)(isolate_, Handle<PyObject>::null(),
                                             repr_args, PyDict::New(isolate_));
  ASSERT_FALSE(repr_result.IsEmpty());
  EXPECT_TRUE(IsPyString(*repr_result.ToHandleChecked()));
  auto repr_text = Handle<PyString>::cast(repr_result.ToHandleChecked());
  EXPECT_EQ(std::string(repr_text->buffer(),
                        static_cast<size_t>(repr_text->length())),
            "'abc'");

  auto list = PyList::New(isolate_, 1);
  list->SetAndExtendLength(0, PyString::NewInstance("x"));
  repr_args->SetInternal(0, list);
  repr_result = BUILTIN_FUNC_NAME(Repr)(isolate_, Handle<PyObject>::null(),
                                        repr_args, PyDict::New(isolate_));
  ASSERT_FALSE(repr_result.IsEmpty());
  repr_text = Handle<PyString>::cast(repr_result.ToHandleChecked());
  EXPECT_EQ(std::string(repr_text->buffer(),
                        static_cast<size_t>(repr_text->length())),
            "['x']");
}

}  // namespace saauso::internal
