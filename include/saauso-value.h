// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef INCLUDE_SAAUSO_VALUE_H_
#define INCLUDE_SAAUSO_VALUE_H_

#include <cstdint>
#include <string>

namespace saauso {

class Value {
 public:
  bool IsString() const;
  bool IsInteger() const;
  bool IsFloat() const;
  bool IsBoolean() const;
  bool ToString(std::string* out) const;
  bool ToInteger(int64_t* out) const;
  bool ToFloat(double* out) const;
  bool ToBoolean(bool* out) const;

 private:
  Value() = delete;
  ~Value() = delete;
  Value(const Value&) = delete;
  Value& operator=(const Value&) = delete;
};

}  // namespace saauso

#endif  // INCLUDE_SAAUSO_VALUE_H_
