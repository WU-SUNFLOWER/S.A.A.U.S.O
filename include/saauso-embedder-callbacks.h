#ifndef INCLUDE_SAAUSO_EMBEDDER_CALLBACKS_H_
#define INCLUDE_SAAUSO_EMBEDDER_CALLBACKS_H_

#include <string>
#include <string_view>

#include "saauso-embedder-core.h"

namespace saauso {

class Function final : public Value {
 public:
  static Local<Function> New(Isolate* isolate,
                             FunctionCallback callback,
                             std::string_view name);
  MaybeLocal<Value> Call(Local<Context> context,
                         Local<Value> receiver,
                         int argc,
                         Local<Value> argv[]);
};

class Exception {
 public:
  static Local<Value> TypeError(Local<String> msg);
  static Local<Value> RuntimeError(Local<String> msg);
};

class FunctionCallbackInfo {
 public:
  FunctionCallbackInfo() = default;

  int Length() const;
  Local<Value> operator[](int index) const;
  bool GetIntegerArg(int index, int64_t* out) const;
  bool GetStringArg(int index, std::string* out) const;
  Local<Value> Receiver() const;
  Isolate* GetIsolate() const;
  void ThrowRuntimeError(std::string_view message) const;
  void SetReturnValue(Local<Value> value);

 private:
  void* impl_{nullptr};

  friend struct ApiAccess;
};

class TryCatch {
 public:
  explicit TryCatch(Isolate* isolate);
  TryCatch(const TryCatch&) = delete;
  TryCatch& operator=(const TryCatch&) = delete;
  ~TryCatch();

  bool HasCaught() const;
  void Reset();
  Local<Value> Exception() const;

 private:
  Isolate* isolate_{nullptr};
  TryCatch* previous_{nullptr};
  Local<Value> exception_;

  friend struct ApiAccess;
};

}  // namespace saauso

#endif  // INCLUDE_SAAUSO_EMBEDDER_CALLBACKS_H_
