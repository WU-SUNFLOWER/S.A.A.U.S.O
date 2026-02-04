// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef SAAUSO_UTILS_NUMBER_CONVERSION_H_
#define SAAUSO_UTILS_NUMBER_CONVERSION_H_

#include <cstdint>
#include <optional>
#include <string_view>

namespace saauso::internal {

std::optional<double> StringToDouble(std::string_view s);
std::optional<int64_t> StringToInt(std::string_view s);

void DoubleToStringView(double n, std::string_view buffer);
void Int64ToStringView(int64_t n, std::string_view buffer);

}  // namespace saauso::internal

#endif  // SAAUSO_UTILS_NUMBER_CONVERSION_H_
