// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef SAAUSO_HEAP_SPACES_H_
#define SAAUSO_HEAP_SPACES_H_

#include "include/saauso-internal.h"

namespace saauso::internal {

class Space {
  virtual Address AllocateRaw(size_t size_in_bytes) = 0;
  virtual bool Contains(Address) = 0;
};

class SemiSpace : public Space {
 public:
  SemiSpace() = default;

  void Setup(Address start, size_t size);
  void TearDown();

  Address AllocateRaw(size_t size_in_bytes) override;
  bool Contains(Address addr) override;

 private:
  Address base_;
  Address top_;
  Address end_;
};

class NewSpace : public Space {
 public:
  NewSpace() = default;

  void Setup(Address star, size_t size);
  void TearDown();

  Address AllocateRaw(size_t size_in_bytes) override;
  bool Contains(Address addr) override;

  void Flip();

  SemiSpace& eden_space() { return eden_space_; }
  SemiSpace& survivor_space() { return survivor_space_; }

 private:
  SemiSpace eden_space_;
  SemiSpace survivor_space_;
};

class MetaSpace : public SemiSpace {};

// TODO: 实现分代式GC
class OldSpace : public Space {
 public:
  Address AllocateRaw(size_t size_in_bytes) override;
  bool Contains(Address) override;
};

}  // namespace saauso::internal

#endif
