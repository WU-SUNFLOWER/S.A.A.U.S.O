#ifndef INCLUDE_SAAUSO_EMBEDDER_VALUES_H_
#define INCLUDE_SAAUSO_EMBEDDER_VALUES_H_

#include <string>
#include <string_view>

#include "saauso-embedder-core.h"

namespace saauso {

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

class Script final : public Value {
 public:
  static MaybeLocal<Script> Compile(Isolate* isolate, Local<String> source);
  MaybeLocal<Value> Run(Local<Context> context);
};

}  // namespace saauso

#endif  // INCLUDE_SAAUSO_EMBEDDER_VALUES_H_
