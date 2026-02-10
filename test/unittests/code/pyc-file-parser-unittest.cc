// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <span>
#include <string>
#include <vector>

#include "src/build/build_config.h"
#include "src/build/buildflag.h"
#include "src/code/compiler.h"
#include "src/execution/isolate.h"
#include "src/objects/py-code-object.h"
#include "src/objects/py-object.h"
#include "src/objects/py-smi.h"
#include "src/objects/py-string.h"
#include "src/objects/py-tuple.h"
#include "test/unittests/test-helpers.h"
#include "test/unittests/test-utils.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace saauso::internal {

class PycFileParserTest : public VmTestBase {};

TEST_F(PycFileParserTest, CompileAndParseUsingCPython312) {
  HandleScope scope;

  constexpr std::string_view kFileName = "saauso_unittest_input.py";
  constexpr std::string_view kSource = "x = 1\n";

  auto code = Compiler::CompileSource(isolate_, kSource, kFileName);
  ASSERT_FALSE(code.is_null());

  auto file_name = code->file_name();
  EXPECT_TRUE(IsPyStringEqual(file_name, kFileName));
}

}  // namespace saauso::internal
