// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#include "testing/gtest/include/gtest/gtest.h"

namespace saauso::internal {

namespace {

std::filesystem::path FindRepoRoot(std::filesystem::path start) {
  namespace fs = std::filesystem;
  for (fs::path cur = start; !cur.empty(); cur = cur.parent_path()) {
    if (fs::exists(cur / "BUILD.gn") && fs::exists(cur / "src" / "objects")) {
      return cur;
    }
    if (cur == cur.parent_path()) {
      break;
    }
  }
  return {};
}

bool ContainsDisallowedDirectHeapAllocate(const std::filesystem::path& file) {
  std::ifstream in(file, std::ios::in | std::ios::binary);
  if (!in.is_open()) {
    return false;
  }
  std::string content((std::istreambuf_iterator<char>(in)),
                      std::istreambuf_iterator<char>());
  return content.find("->heap()->Allocate") != std::string::npos ||
         content.find("->heap()->AllocateRaw") != std::string::npos;
}

}

TEST(FactoryBoundaryTest, ObjectsMustNotDirectlyAllocateFromHeap) {
  namespace fs = std::filesystem;
  fs::path root = FindRepoRoot(fs::current_path());
  ASSERT_FALSE(root.empty());

  fs::path objects_dir = root / "src" / "objects";
  std::vector<std::string> offenders;
  for (const auto& entry : fs::recursive_directory_iterator(objects_dir)) {
    if (!entry.is_regular_file()) {
      continue;
    }
    fs::path path = entry.path();
    auto ext = path.extension().string();
    if (ext != ".cc" && ext != ".h") {
      continue;
    }
    auto filename = path.filename().string();
    if (filename.find("-klass.") != std::string::npos) {
      continue;
    }
    if (ContainsDisallowedDirectHeapAllocate(path)) {
      offenders.push_back(path.string());
    }
  }

  EXPECT_TRUE(offenders.empty());
}

}
