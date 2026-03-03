// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "test/unittests/test-helpers.h"

#include "include/saauso.h"
#include "src/code/compiler.h"
#include "src/execution/isolate.h"
#include "src/handles/global-handles.h"
#include "src/handles/handles.h"
#include "src/interpreter/interpreter.h"
#include "src/objects/py-dict.h"
#include "src/objects/py-function.h"
#include "src/objects/py-list.h"
#include "src/objects/py-object.h"
#include "src/objects/py-oddballs.h"
#include "src/objects/py-string.h"
#include "src/objects/py-tuple.h"
#include "src/runtime/runtime-exceptions.h"
#include "test/unittests/test-utils.h"

namespace saauso::internal {

Isolate* VmTestBase::isolate_ = nullptr;

void VmTestBase::SetUpTestSuite() {
  // 运行时初始化必须早于 Isolate 创建；Enter 后 Current Isolate 才可用。
  saauso::Saauso::Initialize();
  isolate_ = Isolate::New();
  ASSERT_NE(isolate_, nullptr);
  ASSERT_TRUE(isolate_->initialized());
  isolate_->Enter();
  ASSERT_NE(isolate_->interpreter(), nullptr);
  ASSERT_FALSE(isolate_->HasPendingException());
}

void VmTestBase::TearDownTestSuite() {
  // 与 SetUpTestSuite 严格配对：Exit -> Dispose(Isolate) -> Dispose(Runtime)。
  if (isolate_ != nullptr) {
    isolate_->Exit();
    Isolate::Dispose(isolate_);
    isolate_ = nullptr;
  }
  saauso::Saauso::Dispose();
}

//////////////////////////////////////////////////////////////

Isolate* IsolateOnlyTestBase::isolate_ = nullptr;

void IsolateOnlyTestBase::SetUpTestSuite() {
  // 该基类不 Enter Isolate，适用于仅验证隔离/并发等不依赖 Current 的场景。
  saauso::Saauso::Initialize();
  isolate_ = Isolate::New();
  ASSERT_NE(isolate_, nullptr);
  ASSERT_TRUE(isolate_->initialized());
  ASSERT_NE(isolate_->interpreter(), nullptr);
  ASSERT_FALSE(isolate_->HasPendingException());
}

void IsolateOnlyTestBase::TearDownTestSuite() {
  if (isolate_ != nullptr) {
    Isolate::Dispose(isolate_);
    isolate_ = nullptr;
  }
  saauso::Saauso::Dispose();
}

//////////////////////////////////////////////////////////////

Global<PyList> BasicInterpreterTest::printv_result_;

void BasicInterpreterTest::SetUpTestSuite() {
  VmTestBase::SetUpTestSuite();

  HandleScope scope;
  Handle<PyString> func_name = PyString::NewInstance("print");
  Handle<PyDict> builtins = handle(isolate_->builtins());

  // 将 builtins.print 替换为 Builtin_PrintV，用于捕获解释器侧的打印参数。
  ASSERT_FALSE(PyDict::Put(builtins, func_name,
                           PyFunction::NewInstance(&Builtin_PrintV, func_name))
                   .IsNothing());
}

void BasicInterpreterTest::TearDownTestSuite() {
  printv_result_.Reset();
  VmTestBase::TearDownTestSuite();
}

void BasicInterpreterTest::SetUp() {
  HandleScope scope;
  printv_result_ = PyList::NewInstance();
  isolate_->interpreter()->ClearPendingException();
}

void BasicInterpreterTest::RunScript(std::string_view source,
                                     std::string_view file_name) {
  Handle<PyFunction> boilerplate;
  if (!Compiler::CompileSource(isolate_, source, file_name).To(&boilerplate)) {
    FAIL() << "fail to compile source code";
    return;
  }

  isolate_->interpreter()->Run(boilerplate);

  if (isolate_->HasPendingException()) {
    HandleScope scope;
    Handle<PyString> formatted;
    if (!Runtime_FormatPendingExceptionForStderr().To(&formatted)) {
      ADD_FAILURE() << "Runtime_FormatPendingExceptionForStderr Failed!"
                    << formatted->ToStdString();
    }
    isolate_->exception_state()->Clear();
  }
}

std::string BasicInterpreterTest::ExpectedAndTakePendingExceptionMessage() {
  HandleScope scope;

  if (!isolate_->HasPendingException()) {
    ADD_FAILURE() << "Expected pending exception, but none is present";
    return std::string();
  }

  Handle<PyString> formatted;
  if (!Runtime_FormatPendingExceptionForStderr().To(&formatted)) {
    ADD_FAILURE() << "Runtime_FormatPendingExceptionForStderr Failed!";
    isolate_->exception_state()->Clear();
    return std::string();
  }

  std::string msg = formatted->ToStdString();
  isolate_->exception_state()->Clear();
  return msg;
}

void BasicInterpreterTest::RunScriptExpectExceptionContains(
    std::string_view source,
    std::string_view expected_substr,
    std::string_view file_name) {
  HandleScope scope;

  Handle<PyFunction> boilerplate;
  if (!Compiler::CompileSource(isolate_, source, file_name).To(&boilerplate)) {
    FAIL() << "fail to compile source code";
    return;
  }

  isolate_->interpreter()->Run(boilerplate);
  ASSERT_TRUE(isolate_->HasPendingException());

  std::string msg = ExpectedAndTakePendingExceptionMessage();
  ASSERT_FALSE(msg.empty());
  EXPECT_NE(msg.find(expected_substr), std::string::npos) << "got: " << msg;
}

MaybeHandle<PyObject> BasicInterpreterTest::Builtin_PrintV(
    Handle<PyObject> host,
    Handle<PyTuple> args,
    Handle<PyDict> kwargs) {
  for (auto i = 0; i < args->length(); ++i) {
    HandleScope scope;
    PyList::Append(printv_result_.Get(), args->Get(i));
  }
  return handle(Isolate::Current()->py_none_object());
}

void BasicInterpreterTest::ExpectPrintResult(Handle<PyList> expected) {
  HandleScope scope;
  Handle<PyList> actual = printv_result_.Get();

  ASSERT_FALSE(expected.is_null());
  ASSERT_FALSE(actual.is_null());

  ASSERT_EQ(actual->length(), expected->length());

  // 逐项按 Python 语义相等性比较，便于定位具体哪一项不匹配。
  for (auto i = 0; i < actual->length(); ++i) {
    EXPECT_TRUE(IsPyObjectEqual(actual->Get(i), expected->Get(i)));
  }
}

Isolate* BasicInterpreterTest::isolate() {
  return isolate_;
}

}  // namespace saauso::internal
