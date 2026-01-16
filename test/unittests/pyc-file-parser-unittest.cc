// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#include "include/saauso.h"
#include "src/code/pyc-file-parser.h"
#include "src/objects/py-code-object.h"
#include "src/objects/py-string.h"
#include "src/runtime/isolate.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace saauso::internal {

class PycFileParserTest : public testing::Test {
 protected:
  static void SetUpTestSuite() {
    saauso::Saauso::Initialize();
    isolate_ = Isolate::New();
    isolate_->Enter();
  }
  static void TearDownTestSuite() {
    isolate_->Exit();
    Isolate::Dispose(isolate_);
    isolate_ = nullptr;
    saauso::Saauso::Dispose();
  }

  static Isolate* isolate_;
};

Isolate* PycFileParserTest::isolate_ = nullptr;

static void PutInt32LE(std::vector<uint8_t>& out, int32_t v) {
  out.push_back(static_cast<uint8_t>(v & 0xff));
  out.push_back(static_cast<uint8_t>((v >> 8) & 0xff));
  out.push_back(static_cast<uint8_t>((v >> 16) & 0xff));
  out.push_back(static_cast<uint8_t>((v >> 24) & 0xff));
}

static void PutByte(std::vector<uint8_t>& out, uint8_t v) {
  out.push_back(v);
}

static void PutLongString(std::vector<uint8_t>& out, const std::string& s) {
  PutInt32LE(out, static_cast<int32_t>(s.size()));
  out.insert(out.end(), s.begin(), s.end());
}

static void ExpectStringEquals(Handle<PyString> s, const char* expected) {
  const int64_t len = static_cast<int64_t>(std::strlen(expected));
  ASSERT_EQ(s->length(), len);
  ASSERT_EQ(std::memcmp(s->buffer(), expected, len), 0);
}

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
  PutInt32LE(bytes, 0);

  PutByte(bytes, static_cast<uint8_t>('s'));
  PutInt32LE(bytes, 0);

  for (int i = 0; i < 5; ++i) {
    PutByte(bytes, static_cast<uint8_t>(')'));
    PutByte(bytes, 0);
  }

  PutByte(bytes, static_cast<uint8_t>(0x80 | static_cast<uint8_t>('s')));
  PutLongString(bytes, "file.py");

  PutByte(bytes, static_cast<uint8_t>('r'));
  PutInt32LE(bytes, 0);

  PutInt32LE(bytes, 1);

  PutByte(bytes, static_cast<uint8_t>('s'));
  PutInt32LE(bytes, 0);

  std::filesystem::path p =
      std::filesystem::temp_directory_path() /
      ("saauso_pyc_test_ParseNameCanLoadFromCache.pyc");
  {
    std::ofstream os(p, std::ios::binary | std::ios::trunc);
    ASSERT_TRUE(os.is_open());
    os.write(reinterpret_cast<const char*>(bytes.data()),
             static_cast<std::streamsize>(bytes.size()));
  }

  PycFileParser parser(p.string().c_str());
  auto code = parser.Parse();
  ASSERT_FALSE(code.IsNull());

  auto file_name = handle(Tagged<PyString>::cast((*code)->file_name_));
  auto co_name = handle(Tagged<PyString>::cast((*code)->co_name_));
  ExpectStringEquals(file_name, "file.py");
  ExpectStringEquals(co_name, "file.py");

  EXPECT_EQ((*file_name).ptr(), (*co_name).ptr());

  std::error_code ec;
  std::filesystem::remove(p, ec);
}

}  // namespace saauso::internal
