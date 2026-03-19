#ifndef INCLUDE_SAAUSO_EMBEDDER_OBJECTS_H_
#define INCLUDE_SAAUSO_EMBEDDER_OBJECTS_H_

#include "saauso-embedder-core.h"

namespace saauso {

class Object : public Value {
 public:
  static Local<Object> New(Isolate* isolate);
  bool Set(Local<String> key, Local<Value> value);
  MaybeLocal<Value> Get(Local<String> key);
  MaybeLocal<Value> CallMethod(Local<Context> context,
                               Local<String> name,
                               int argc,
                               Local<Value> argv[]);
};

class List : public Value {
 public:
  static Local<List> New(Isolate* isolate);
  int64_t Length() const;
  bool Push(Local<Value> value);
  bool Set(int64_t index, Local<Value> value);
  MaybeLocal<Value> Get(int64_t index) const;
};

class Tuple : public Value {
 public:
  static MaybeLocal<Tuple> New(Isolate* isolate, int argc, Local<Value> argv[]);
  int64_t Length() const;
  MaybeLocal<Value> Get(int64_t index) const;
};

}  // namespace saauso

#endif  // INCLUDE_SAAUSO_EMBEDDER_OBJECTS_H_
