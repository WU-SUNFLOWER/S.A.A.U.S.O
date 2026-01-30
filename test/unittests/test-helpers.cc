// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "test/unittests/test-helpers.h"

#include "include/saauso.h"
#include "src/code/cpython312-pyc-compiler.h"
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
#include "src/runtime/isolate.h"
#include "test/unittests/test-utils.h"

namespace saauso::internal {

Isolate* VmTestBase::isolate_ = nullptr;

void VmTestBase::SetUpTestSuite() {
  saauso::Saauso::Initialize();
  isolate_ = Isolate::New();
  isolate_->Enter();
}

void VmTestBase::TearDownTestSuite() {
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
  saauso::Saauso::Initialize();
  isolate_ = Isolate::New();
}

void IsolateOnlyTestBase::TearDownTestSuite() {
  if (isolate_ != nullptr) {
    Isolate::Dispose(isolate_);
    isolate_ = nullptr;
  }
  saauso::Saauso::Dispose();
}

//////////////////////////////////////////////////////////////

void EmbeddedPython312VmTestBase::TearDownTestSuite() {
  VmTestBase::TearDownTestSuite();
  FinalizeEmbeddedPython312Runtime();
}

//////////////////////////////////////////////////////////////

Global<PyList> BasicInterpreterTest::printv_result_;

void BasicInterpreterTest::SetUpTestSuite() {
  VmTestBase::SetUpTestSuite();

  HandleScope scope;
  Handle<PyString> func_name = PyString::NewInstance("print");
  PyDict::Put(isolate_->interpreter()->builtins(), func_name,
              PyFunction::NewInstance(&Native_PrintV, func_name));
}

void BasicInterpreterTest::TearDownTestSuite() {
  printv_result_.Reset();
  EmbeddedPython312VmTestBase::TearDownTestSuite();
}

void BasicInterpreterTest::SetUp() {
  HandleScope scope;
  printv_result_ = PyList::NewInstance();
}

void BasicInterpreterTest::RunScript(std::string_view source,
                                     std::string_view file_name) {
  isolate_->interpreter()->Run(CompileScript312(isolate_, source, file_name));
}

Handle<PyObject> BasicInterpreterTest::Native_PrintV(Handle<PyObject> host,
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

  for (auto i = 0; i < actual->length(); ++i) {
    EXPECT_TRUE(IsPyObjectEqual(actual->Get(i), expected->Get(i)));
  }
}

Isolate* BasicInterpreterTest::isolate() {
  return isolate_;
}

}  // namespace saauso::internal
