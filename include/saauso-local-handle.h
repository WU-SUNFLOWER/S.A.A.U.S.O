// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef SAAUSO_SAAUSO_LOCAL_HANDLE_H_
#define SAAUSO_SAAUSO_LOCAL_HANDLE_H_

#include <cassert>
#include <type_traits>

namespace saauso {

class Isolate;
class Value;

namespace internal {
class Utils;
}

template <typename T>
class Local {
 public:
  Local() : val_(nullptr) {}

  bool IsEmpty() const { return val_ == nullptr; }
  T* operator->() const { return val_; }
  T* operator*() const { return this->operator->(); }

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
  // 默认构造函数：生成一个空的 handle
  MaybeLocal() = default;

  // 支持隐式地将 Local 转化为 MaybeLocal
  template <class S>
    requires std::is_base_of_v<T, S>
  MaybeLocal(Local<S> that) : local_(that) {}

  // 支持隐式地将指向派生类型 S 的 MaybeLocal 转换为指向基类 T 的 MaybeLocal
  template <class S>
    requires std::is_base_of_v<T, S>
  MaybeLocal(MaybeLocal<S> that) : local_(that.local_) {}

  bool IsEmpty() const { return local_.IsEmpty(); }

  // 将 MaybeLocal 解包为可以直接使用的 Local。
  // 如果 MaybeLocal 为空，本方法将返回 false，同时 out 将会被赋值为空 Local。
  template <class S>
  bool ToLocal(Local<S>* out) const {
    *out = local_;
    return !IsEmpty();
  }

  // 将 MaybeLocal 解包为可以直接使用的 Local。
  // 如果 MaybeLocal 为空，在 debug 模式下程序将直接崩溃。
  Local<T> ToLocalChecked() {
    assert(!IsEmpty() && "Empty MaybeLocal");
    return local_;
  }

  // 将 MaybeLocal 解包为可以直接使用的 Local。
  // 如果 MaybeLocal 为空，本方法将返回默认值 default_value。
  template <class S>
  Local<S> FromMaybe(Local<S> default_value) const {
    return IsEmpty() ? default_value : Local<S>(local_);
  }

  // 将指向基类 S 的 MaybeLocal 强转为指向派生类 T 的 MaybeLocal。
  // 使用此方法时，嵌入方应自行保证该 MaybeLocal 指向的的确是一个
  // 有效的派生类 T 的实例。
  template <class S>
  static MaybeLocal<T> Cast(MaybeLocal<S> that) {
    return MaybeLocal<T>{Local<T>::Cast(that.local_)};
  }

  // 效果等价于 MaybeLocal<T>::Cast。
  // 使用此方法时，嵌入方应自行保证该 MaybeLocal 指向的的确是一个
  // 有效的派生类 T 的实例。
  template <class S>
  MaybeLocal<S> As() const {
    return MaybeLocal<S>::Cast(*this);
  }

 private:
  Local<T> local_;

  template <typename S>
  friend class MaybeLocal;
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

}  // namespace saauso

#endif  // SAAUSO_SAAUSO_LOCAL_HANDLE_H_
