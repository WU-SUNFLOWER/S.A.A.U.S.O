// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef SAAUSO_HANDLES_HANDLES_H_
#define SAAUSO_HANDLES_HANDLES_H_

template <typename T>
class Handle {
 public:
  explicit Handle(T* address) : address_(address) {}

  T* operator->() const { return address_; }
  T& operator*() const { return *address_; }

  T* get() const { return address_; }

 private:
  T* address_;
};

class [[maybe_unused]] HandleScope {};

#endif  // SAAUSO_HANDLES_HANDLES_H_
