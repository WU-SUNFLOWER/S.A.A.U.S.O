// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include <string_view>
#include <utility>

#include "src/code/compiler.h"
#include "src/code/cpython312-pyc-compiler.h"
#include "src/handles/handles.h"
#include "src/objects/py-code-object.h"
#include "test/unittests/test-helpers.h"
#include "test/unittests/test-utils.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace saauso::internal {

class CompilerTest : public VmTestBase {};

TEST_F(CompilerTest, CompileSourceKeepsFileName) {
  HandleScope scope;

  constexpr std::string_view kFileName = "saauso_compiler_test.py";
  constexpr std::string_view kSource = "x = 1\n";

  Handle<PyCodeObject> code =
      Compiler::CompileSource(isolate_, kSource, kFileName);
  ASSERT_FALSE(code.is_null());

  EXPECT_TRUE(IsPyStringEqual(code->file_name(), kFileName));
}

TEST_F(CompilerTest, CompilePycBytesParsesSuccessfully) {
  HandleScope scope;

  constexpr std::string_view kFileName = "saauso_compiler_bytes_test.py";
  constexpr std::string_view kSource = "x = 1\n";

  std::vector<uint8_t> pyc =
      EmbeddedPython312Compiler::CompileToPycBytes(kSource, kFileName);
  ASSERT_FALSE(pyc.empty());

  Handle<PyCodeObject> code = Compiler::CompilePyc(isolate_, std::move(pyc));
  ASSERT_FALSE(code.is_null());

  EXPECT_TRUE(IsPyStringEqual(code->file_name(), kFileName));
}

}  // namespace saauso::internal
