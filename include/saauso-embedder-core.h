#ifndef INCLUDE_SAAUSO_EMBEDDER_CORE_H_
#define INCLUDE_SAAUSO_EMBEDDER_CORE_H_

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

namespace saauso {

namespace internal {
class Utils;
}

struct ApiAccess;

struct IsolateCreateParams {};

class Isolate;
class Context;
class String;
class Integer;
class Float;
class Boolean;
class Script;
class Function;
class Object;
class List;
class Tuple;
class Exception;
class TryCatch;
class FunctionCallbackInfo;

using FunctionCallback = void (*)(FunctionCallbackInfo& info);

template <typename T>
class MaybeLocal;

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

template <typename T>
class Local {
 public:
  Local() : val_(nullptr) {}

  bool IsEmpty() const { return val_ == nullptr; }
  T* operator->() const { return val_; }
  T& operator*() const { return *val_; }

  template <typename S>
  static Local<T> Cast(Local<S> that) {
    return Local<T>(reinterpret_cast<T*>(that.val_));
  }

 private:
  explicit Local(T* val) : val_(val) {}

  template <typename>
  friend class Local;
  friend class internal::Utils;
  friend struct ApiAccess;
  T* val_;
};

template <typename T>
class MaybeLocal {
 public:
  MaybeLocal() = default;
  MaybeLocal(Local<T> local) : local_(local) {}

  bool IsEmpty() const { return local_.IsEmpty(); }
  bool ToLocal(Local<T>* out) const {
    if (out == nullptr || IsEmpty()) {
      return false;
    }
    *out = local_;
    return true;
  }
  Local<T> ToLocalChecked() const { return local_; }

 private:
  Local<T> local_;
};

class Isolate {
 public:
  static Isolate* New(
      const IsolateCreateParams& params = IsolateCreateParams());
  void Dispose();
  void ThrowException(Local<Value> exception);
  size_t ValueRegistrySizeForTesting() const;

  Isolate(const Isolate&) = delete;
  Isolate& operator=(const Isolate&) = delete;

 private:
  Isolate() = default;

  void* internal_isolate_{nullptr};
  void* entered_contexts_{nullptr};
  TryCatch* try_catch_top_{nullptr};

  friend class Context;
  friend class TryCatch;
  friend struct ApiAccess;
};

class HandleScope {
 public:
  explicit HandleScope(Isolate* isolate);
  HandleScope(const HandleScope&) = delete;
  HandleScope& operator=(const HandleScope&) = delete;
  ~HandleScope();

 private:
  void* impl_{nullptr};
};

class EscapableHandleScope {
 public:
  explicit EscapableHandleScope(Isolate* isolate);
  EscapableHandleScope(const EscapableHandleScope&) = delete;
  EscapableHandleScope& operator=(const EscapableHandleScope&) = delete;
  ~EscapableHandleScope();

  Local<Value> Escape(Local<Value> value);

 private:
  void* impl_{nullptr};
};

class Context final : public Value {
 public:
  static Local<Context> New(Isolate* isolate);

  void Enter();
  void Exit();
  bool Set(Local<String> key, Local<Value> value);
  MaybeLocal<Value> Get(Local<String> key);
  Local<Object> Global();

 private:
  friend class Isolate;
  friend class Script;
  friend struct ApiAccess;
};

class ContextScope {
 public:
  explicit ContextScope(Local<Context> context);
  ContextScope(const ContextScope&) = delete;
  ContextScope& operator=(const ContextScope&) = delete;
  ~ContextScope();

 private:
  Local<Context> context_;
};

}  // namespace saauso

#endif  // INCLUDE_SAAUSO_EMBEDDER_CORE_H_
