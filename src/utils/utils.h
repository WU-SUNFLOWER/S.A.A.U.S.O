// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include <cmath>

namespace saauso::internal {

template <typename T1, typename T2>
auto PythonMod(T1 a, T2 b) {
  // 如果两个都是整数类型
  if constexpr (std::is_integral_v<T1> && std::is_integral_v<T2>) {
    auto res = a % b;
    if (((res ^ b) < 0) && res != 0) {
      res += b;
    }
    return res;  // 返回整数类型
  } else {
    // 只要有一个是浮点数
    return a - std::floor(static_cast<double>(a) / b) * b;  // 返回 double
  }
}

template <typename T1, typename T2>
auto PythonFloorDivide(T1 a, T2 b) {
  if constexpr (std::is_integral_v<T1> && std::is_integral_v<T2>) {
    auto r = PythonMod(a, b);
    return (a - r) / b;
  } else {
    return std::floor(static_cast<double>(a) / static_cast<double>(b));
  }
}

template <typename T>
bool InRangeWithRightOpen(T v, T l, T r) {
  return l <= v && v < r;
}

template <typename T>
bool InRangeWithRightClose(T v, T l, T r) {
  return l <= v && v <= r;
}

}  // namespace saauso::internal
