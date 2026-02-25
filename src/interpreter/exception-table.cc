// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/interpreter/exception-table.h"

#include <cassert>
#include <cstdint>
#include <limits>

#include "src/interpreter/bytecodes.h"
#include "src/objects/py-code-object.h"
#include "src/objects/py-string.h"

namespace saauso::internal {

namespace {

using ByteOffset = int;
using CodeUnitOffset = int;

constexpr uint8_t kVarintPayloadMask = 0x3F;
constexpr uint8_t kVarintContinueMask = 0x40;
constexpr int kVarintPayloadBits = 6;

constexpr int kStackDepthShift = 1;
constexpr int kLastiFlagMask = 0x1;

int DecodeStackDepth(int packed_depth_and_lasti) {
  return packed_depth_and_lasti >> kStackDepthShift;
}

bool DecodeNeedPushLasti(int packed_depth_and_lasti) {
  return (packed_depth_and_lasti & kLastiFlagMask) != 0;
}

bool TryCodeUnitsToBytes(CodeUnitOffset code_units, ByteOffset& out_bytes) {
  if (code_units < 0) {
    return false;
  }

  const int64_t bytes = static_cast<int64_t>(code_units) * kBytecodeSizeInBytes;
  if (bytes > std::numeric_limits<ByteOffset>::max()) {
    return false;
  }
  out_bytes = static_cast<ByteOffset>(bytes);
  return true;
}

bool ParseExceptionTableVarint(const char* data,
                               int length,
                               int& index,
                               int& out) {
  if (index >= length) {
    return false;
  }

  uint8_t b = static_cast<uint8_t>(data[index++]);
  int64_t val = b & kVarintPayloadMask;
  while (b & kVarintContinueMask) {
    if (index >= length) {
      return false;
    }

    if (val > (std::numeric_limits<int>::max() >> kVarintPayloadBits)) {
      return false;
    }
    val <<= kVarintPayloadBits;

    b = static_cast<uint8_t>(data[index++]);
    val |= b & kVarintPayloadMask;
  }

  out = static_cast<int>(val);
  return true;
}

}  // namespace

bool ExceptionTable::LookupHandler(Handle<PyCodeObject> code_object,
                                   int instruction_offset_in_bytes,
                                   ExceptionHandlerInfo& out) {
  HandleScope scope;

  if (instruction_offset_in_bytes < 0) {
    return false;
  }
  assert(instruction_offset_in_bytes % kBytecodeSizeInBytes == 0);

  Handle<PyString> table = code_object->exception_table();
  if (table.is_null() || table->length() == 0) {
    return false;
  }

  const char* data = table->buffer();
  const int length = static_cast<int>(table->length());

  int index = 0;
  while (index < length) {
    int start_units_raw = 0;
    int size_units_raw = 0;
    int target_units_raw = 0;
    int packed_depth_and_lasti = 0;
    if (!ParseExceptionTableVarint(data, length, index, start_units_raw) ||
        !ParseExceptionTableVarint(data, length, index, size_units_raw) ||
        !ParseExceptionTableVarint(data, length, index, target_units_raw) ||
        !ParseExceptionTableVarint(data, length, index,
                                   packed_depth_and_lasti)) {
      return false;
    }

    const CodeUnitOffset start_units = start_units_raw;
    const CodeUnitOffset size_units = size_units_raw;
    const CodeUnitOffset target_units = target_units_raw;

    ByteOffset start_bytes = 0;
    ByteOffset size_bytes = 0;
    ByteOffset target_bytes = 0;
    if (!TryCodeUnitsToBytes(start_units, start_bytes) ||
        !TryCodeUnitsToBytes(size_units, size_bytes) ||
        !TryCodeUnitsToBytes(target_units, target_bytes)) {
      return false;
    }

    const int64_t end64 = static_cast<int64_t>(start_bytes) + size_bytes;
    if (end64 > std::numeric_limits<ByteOffset>::max()) {
      return false;
    }
    const ByteOffset end_bytes = static_cast<ByteOffset>(end64);

    const ByteOffset instruction_offset_bytes = instruction_offset_in_bytes;
    if (start_bytes <= instruction_offset_bytes &&
        instruction_offset_bytes < end_bytes) {
      out.stack_depth = DecodeStackDepth(packed_depth_and_lasti);
      out.handler_pc = target_bytes;
      out.need_push_lasti = DecodeNeedPushLasti(packed_depth_and_lasti);
      return true;
    }
  }

  return false;
}

}  // namespace saauso::internal
