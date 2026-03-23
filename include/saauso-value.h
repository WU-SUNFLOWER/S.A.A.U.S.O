// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef INCLUDE_SAAUSO_VALUE_H_
#define INCLUDE_SAAUSO_VALUE_H_

#include <cstdint>
#include <string>

#include "saauso-maybe.h"

namespace saauso {

class Value {
 public:
  bool IsString() const;
  bool IsInteger() const;
  bool IsFloat() const;
  bool IsBoolean() const;
  // 若当前值为字符串，返回字符串值；否则返回 Nothing。
  Maybe<std::string> ToString() const;
  // 若当前值为整数，返回整数值；否则返回 Nothing。
  Maybe<int64_t> ToInteger() const;
  // 若当前值为浮点数，返回浮点值；否则返回 Nothing。
  Maybe<double> ToFloat() const;
  // 若当前值为布尔值，返回布尔值；否则返回 Nothing。
  Maybe<bool> ToBoolean() const;

 private:
  Value() = delete;
  ~Value() = delete;
  Value(const Value&) = delete;
  Value& operator=(const Value&) = delete;
};

}  // namespace saauso

#endif  // INCLUDE_SAAUSO_VALUE_H_
