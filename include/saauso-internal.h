// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef INCLUDE_SAAUSO_INTERNAL_H_
#define INCLUDE_SAAUSO_INTERNAL_H_

#include <cstddef>
#include <cstdint>

namespace saauso::internal {

inline constexpr int kByteSize = 1;
inline constexpr intptr_t kByteMask = 0xFF;
inline constexpr int kBitsPerByte = 8;

using Address = uintptr_t;
inline constexpr Address kNullAddress = 0;

// Tag information for Smi.
// 若Address以二进制位1结尾，则代表它是一个Smi
inline constexpr int kSmiTag = 1;
inline constexpr int kSmiTagSize = 1;
inline constexpr intptr_t kSmiTagMask = (1 << kSmiTagSize) - 1;

// Tag information for fowarding pointers stored in object headers.
// 若MarkWord以二进制位10结尾，则代表它是一个forwarding指针
inline constexpr int kForwardingTag = 0b10;
inline constexpr int kForwardingTagSize = 2;
inline constexpr intptr_t kForwardingTagMask = (1 << kForwardingTagSize) - 1;

inline bool AddressIsSmi(Address addr) {
  return (addr & kSmiTagMask) == kSmiTag;
}

inline int64_t AddressToSmi(Address addr) {
  return static_cast<intptr_t>(addr) >> kSmiTagSize;
}

inline Address SmiToAddress(int64_t smi) {
  return (smi << kSmiTagSize) | kSmiTag;
}

// Desired alignment for tagged pointers.
inline constexpr int kObjectAlignmentBits = 3;
inline constexpr intptr_t kObjectAlignment = 1 << kObjectAlignmentBits;
inline constexpr intptr_t kObjectAlignmentMask = kObjectAlignment - 1;

inline size_t ObjectSizeAlign(size_t size) {
  return (size + kObjectAlignmentMask) & ~kObjectAlignmentMask;
}

inline bool IsObjectSizeAligned(size_t size) {
  return (size & kObjectAlignmentMask) == 0;
}

}  // namespace saauso::internal

#endif  // INCLUDE_SAAUSO_INTERNAL_H_
