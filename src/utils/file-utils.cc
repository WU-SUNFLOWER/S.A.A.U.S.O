// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/utils/file-utils.h"

#include <filesystem>
#include <fstream>

namespace saauso::internal {

bool ReadFileToString(std::string_view filename, std::string* out) {
  if (out == nullptr) {
    return false;
  }
  out->clear();

  std::ifstream file(std::filesystem::path(filename),
                     std::ios::in | std::ios::binary);
  if (!file.is_open()) {
    return false;
  }

  file.seekg(0, std::ios::end);
  std::streamoff size = file.tellg();
  if (size < 0) {
    return false;
  }
  out->resize(static_cast<size_t>(size));
  file.seekg(0, std::ios::beg);
  file.read(out->data(), static_cast<std::streamsize>(out->size()));
  return file.good() || file.eof();
}

bool FileExists(std::string_view filename) {
  std::error_code ec;
  const auto path = std::filesystem::path(filename);
  return std::filesystem::exists(path, ec) &&
         std::filesystem::is_regular_file(path, ec);
}

bool DirectoryExists(std::string_view dirname) {
  std::error_code ec;
  const auto path = std::filesystem::path(dirname);
  return std::filesystem::exists(path, ec) &&
         std::filesystem::is_directory(path, ec);
}

std::string JoinPath(std::string_view a, std::string_view b) {
  std::filesystem::path p(a);
  p /= std::filesystem::path(b);
  return p.string();
}

std::string NormalizePath(std::string_view path) {
  std::filesystem::path p(path);
  return p.lexically_normal().string();
}

}  // namespace saauso::internal

