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
#include "src/code/cpython312-pyc-compiler.h"
#include "src/code/cpython312-pyc-file-parser.h"
#include "src/objects/py-code-object.h"
#include "src/objects/py-string.h"
#include "src/runtime/isolate.h"
#include "test/unittests/test-helpers.h"
#include "test/unittests/test-utils.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace saauso::internal {

class PycFileParserTest : public EmbeddedPython312VmTestBase {};

TEST_F(PycFileParserTest, ParseNameCanLoadFromCache) {
  HandleScope scope;

  std::vector<uint8_t> bytes;
  PutInt32LE(bytes, 0);
  PutInt32LE(bytes, 0);
  PutInt32LE(bytes, 0);
  PutInt32LE(bytes, 0);

  PutByte(bytes, static_cast<uint8_t>('c'));

  PutInt32LE(bytes, 0);
  PutInt32LE(bytes, 0);
  PutInt32LE(bytes, 0);
  PutInt32LE(bytes, 0);
  PutInt32LE(bytes, 0);

  PutByte(bytes, static_cast<uint8_t>('s'));
  PutInt32LE(bytes, 0);

  PutByte(bytes, static_cast<uint8_t>(')'));
  PutByte(bytes, 0);

  PutByte(bytes, static_cast<uint8_t>(')'));
  PutByte(bytes, 0);

  PutByte(bytes, static_cast<uint8_t>(')'));
  PutByte(bytes, 0);

  PutByte(bytes, static_cast<uint8_t>('0'));

  PutByte(bytes, static_cast<uint8_t>(0x80 | static_cast<uint8_t>('s')));
  PutLongString(bytes, "file.py");

  PutByte(bytes, static_cast<uint8_t>('r'));
  PutInt32LE(bytes, 0);

  PutByte(bytes, static_cast<uint8_t>('r'));
  PutInt32LE(bytes, 0);

  PutInt32LE(bytes, 1);

  PutByte(bytes, static_cast<uint8_t>('s'));
  PutInt32LE(bytes, 0);

  PutByte(bytes, static_cast<uint8_t>('s'));
  PutInt32LE(bytes, 0);

  CPython312PycFileParser parser(
      std::span<const uint8_t>(bytes.data(), bytes.size()), isolate_);
  auto code = parser.Parse();
  ASSERT_FALSE(code.is_null());

  auto file_name = (*code)->file_name();
  auto co_name = (*code)->co_name();
  EXPECT_TRUE(IsPyStringEqual(file_name, "file.py"));
  EXPECT_TRUE(IsPyStringEqual(co_name, "file.py"));

  EXPECT_EQ((*file_name).ptr(), (*co_name).ptr());
}

TEST_F(PycFileParserTest, CompileAndParseUsingCPython312) {
  HandleScope scope;

  constexpr std::string_view kFileName = "saauso_unittest_input.py";
  constexpr std::string_view kSource = "x = 1\n";

  std::vector<uint8_t> pyc =
      CompilePythonSourceToPycBytes312(kSource, kFileName);
  ASSERT_FALSE(pyc.empty());

  CPython312PycFileParser parser(
      std::span<const uint8_t>(pyc.data(), pyc.size()), isolate_);
  auto code = parser.Parse();
  ASSERT_FALSE(code.is_null());

  auto file_name = code->file_name();
  EXPECT_TRUE(IsPyStringEqual(file_name, kFileName));
}

}  // namespace saauso::internal
