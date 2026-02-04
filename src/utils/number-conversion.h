// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef SAAUSO_UTILS_NUMBER_CONVERSION_H_
#define SAAUSO_UTILS_NUMBER_CONVERSION_H_

#include <cstdint>
#include <optional>
#include <string_view>

namespace saauso::internal {

// 数值与字符串之间的转换工具。
//
// - 解析接口支持 ASCII 空白裁剪，并按 Python 风格允许数字中使用 '_' 分隔。
// - 格式化接口要求调用方提供可写 buffer，并保证以 '\\0' 结尾。
std::optional<double> StringToDouble(std::string_view s);
std::optional<int64_t> StringToInt(std::string_view s);

// 将 double 格式化为最短的可 round-trip（再次解析得到原值）的十进制字符串。
// buffer 需预留末尾 '\\0'，且大小不足会直接报错退出。
void DoubleToStringView(double n, std::string_view buffer);

// 将 int64 格式化为十进制字符串写入 buffer（以 '\\0' 结尾）。
// buffer 需预留末尾 '\\0'，且大小不足会直接报错退出。
void Int64ToStringView(int64_t n, std::string_view buffer);

}  // namespace saauso::internal

#endif  // SAAUSO_UTILS_NUMBER_CONVERSION_H_
