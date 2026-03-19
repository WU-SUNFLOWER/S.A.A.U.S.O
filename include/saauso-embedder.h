// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef INCLUDE_SAAUSO_EMBEDDER_H_
#define INCLUDE_SAAUSO_EMBEDDER_H_

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
  virtual ~Value() = default;

  bool IsString() const;
  bool IsInteger() const;

 protected:
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
  explicit Context(Isolate* isolate) : isolate_(isolate) {}

  Isolate* isolate_{nullptr};
  void* impl_{nullptr};

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
  static Local<String> New(Isolate* isolate, std::string_view value);
  std::string Value() const;
};

class Integer final : public Value {
 public:
  static Local<Integer> New(Isolate* isolate, int64_t value);
  int64_t Value() const;
};

class Script final : public Value {
 public:
  static MaybeLocal<Script> Compile(Isolate* isolate, Local<String> source);
  MaybeLocal<Value> Run(Local<Context> context);
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

#endif  // INCLUDE_SAAUSO_EMBEDDER_H_
