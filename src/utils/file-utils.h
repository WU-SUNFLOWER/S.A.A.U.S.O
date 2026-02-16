// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef SAAUSO_UTILS_FILE_UTILS_H_
#define SAAUSO_UTILS_FILE_UTILS_H_

#include <string>
#include <string_view>

namespace saauso::internal {

// 读取一个文本文件的全部内容到 out 中。
// 返回值：成功返回 true；失败返回 false（不会抛异常）。
bool ReadFileToString(std::string_view filename, std::string* out);

// 判断文件是否存在且为普通文件。
bool FileExists(std::string_view filename);

// 判断目录是否存在且为目录。
bool DirectoryExists(std::string_view dirname);

// 将 a/b 拼接为一个平台相关的路径字符串。
std::string JoinPath(std::string_view a, std::string_view b);

// 将路径进行规范化（不访问文件系统）。
// 例如，将 "a/./b" 规范化为 "a/b"；将 "a/b/../c" 规范化为 "a/c"。
// 支持处理含'/'或'\'的路径，但返回结果中的斜杠与当前操作系统环境对齐。
std::string NormalizePath(std::string_view path);

}  // namespace saauso::internal

#endif  // SAAUSO_UTILS_FILE_UTILS_H_

