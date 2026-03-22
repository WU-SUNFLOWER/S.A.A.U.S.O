// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef INCLUDE_SAAUSO_PRIMITIVE_H_
#define INCLUDE_SAAUSO_PRIMITIVE_H_

#include "saauso-local-handle.h"
#include "saauso-value.h"

namespace saauso {

class Isolate;

class String final : public Value {
 public:
  static Local<String> New(Isolate* isolate, std::string_view value);
  std::string Value() const;
};

class Integer final : public Value {
 public:
  static Local<Integer> New(Isolate* isolate, int64_t value);
  int64_t Value() const;
};

class Float final : public Value {
 public:
  static Local<Float> New(Isolate* isolate, double value);
  double Value() const;
};

class Boolean final : public Value {
 public:
  static Local<Boolean> New(Isolate* isolate, bool value);
  bool Value() const;
};

}  // namespace saauso

#endif  // INCLUDE_SAAUSO_PRIMITIVE_H_
