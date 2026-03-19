// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef INCLUDE_SAAUSO_EMBEDDER_H_
#define INCLUDE_SAAUSO_EMBEDDER_H_

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

namespace saauso {

struct ApiAccess;

struct IsolateCreateParams {};

class Isolate;
class Context;
class String;
class Integer;
class Script;
class TryCatch;

template <typename T>
class MaybeLocal;

class Value {
 public:
  // 判断该值是否可按字符串语义读取。
  bool IsString() const;
  // 判断该值是否可按整数语义读取。
  bool IsInteger() const;
  // 尝试按字符串语义读取值；成功返回 true 并写入 out。
  bool ToString(std::string* out) const;
  // 尝试按整数语义读取值；成功返回 true 并写入 out。
  bool ToInteger(int64_t* out) const;

 protected:
  virtual ~Value() = default;
  Value() = default;

 private:
  void* impl_{nullptr};
  Isolate* isolate_{nullptr};

  friend struct ApiAccess;
};

template <typename T>
class Local {
 public:
  Local() = default;

  bool IsEmpty() const { return ptr_ == nullptr; }
  T* operator->() const { return ptr_; }
  T& operator*() const { return *ptr_; }

  template <typename S>
  static Local<T> Cast(Local<S> that) {
    return Local<T>(static_cast<T*>(that.ptr_));
  }

 private:
  explicit Local(T* ptr) : ptr_(ptr) {}

  template <typename>
  friend class Local;
  friend class Context;
  friend class Isolate;
  friend struct ApiAccess;

  T* ptr_{nullptr};
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
  size_t ValueRegistrySizeForTesting() const;

  Isolate(const Isolate&) = delete;
  Isolate& operator=(const Isolate&) = delete;

 private:
  Isolate() = default;

  void* internal_isolate_{nullptr};
  Context* default_context_{nullptr};
  void* values_{nullptr};
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

class Context final : public Value {
 public:
  static Local<Context> New(Isolate* isolate);

  void Enter();
  void Exit();

 private:
  explicit Context(Isolate* isolate) { (void)isolate; }

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

class String final : public Value {
 public:
  // 在当前 Isolate 中创建一个字符串值对象。
  static Local<String> New(Isolate* isolate, std::string_view value);
  // 读取字符串内容；若值不可读为字符串则返回空串。
  std::string Value() const;
};

class Integer final : public Value {
 public:
  // 在当前 Isolate 中创建一个整数值对象。
  static Local<Integer> New(Isolate* isolate, int64_t value);
  // 读取整数内容；若值不可读为整数则返回 0。
  int64_t Value() const;
};

class Script final : public Value {
 public:
  // 编译脚本文本并返回脚本对象；失败时返回空 MaybeLocal。
  static MaybeLocal<Script> Compile(Isolate* isolate, Local<String> source);
  // 在给定上下文执行脚本；失败时返回空 MaybeLocal，并由 TryCatch 捕获异常。
  MaybeLocal<Value> Run(Local<Context> context);
};

class TryCatch {
 public:
  explicit TryCatch(Isolate* isolate);
  TryCatch(const TryCatch&) = delete;
  TryCatch& operator=(const TryCatch&) = delete;
  ~TryCatch();

  // 返回当前是否已捕获到异常。
  bool HasCaught() const;
  // 清空当前捕获状态，不影响后续异常捕获能力。
  void Reset();
  // 返回当前已捕获异常对象；若未捕获则返回空 Local。
  Local<Value> Exception() const;

 private:
  Isolate* isolate_{nullptr};
  TryCatch* previous_{nullptr};
  Local<Value> exception_;

  friend struct ApiAccess;
};

}  // namespace saauso

#endif  // INCLUDE_SAAUSO_EMBEDDER_H_
