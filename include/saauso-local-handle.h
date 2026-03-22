// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef SAAUSO_SAAUSO_LOCAL_HANDLE_H_
#define SAAUSO_SAAUSO_LOCAL_HANDLE_H_

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
