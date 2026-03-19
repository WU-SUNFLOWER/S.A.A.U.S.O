// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef INCLUDE_SAAUSO_EMBEDDER_H_
#define INCLUDE_SAAUSO_EMBEDDER_H_

namespace saauso {

struct ApiAccess;

struct IsolateCreateParams {};

class Value {
 public:
  virtual ~Value() = default;
};

class Context;

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

  T* ptr_{nullptr};
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

  friend class Context;
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

  friend class Isolate;
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

#endif  // INCLUDE_SAAUSO_EMBEDDER_H_
