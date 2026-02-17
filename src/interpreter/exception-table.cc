// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/interpreter/exception-table.h"

#include <cassert>

#include "src/objects/py-code-object.h"
#include "src/objects/py-string.h"

namespace saauso::internal {

namespace {

int ParseExceptionTableVarint(const char* data, int length, int& index) {
  assert(index < length);
  uint8_t b = static_cast<uint8_t>(data[index++]);
  int val = b & 63;
  while (b & 64) {
    assert(index < length);
    val <<= 6;
    b = static_cast<uint8_t>(data[index++]);
    val |= b & 63;
  }
  return val;
}

}  // namespace

bool ExceptionTable::LookupHandler(Handle<PyCodeObject> code_object,
                                   int instruction_offset_in_bytes,
                                   ExceptionHandlerInfo& out) {
  HandleScope scope;

  Handle<PyString> table = code_object->exception_table();
  if (table.is_null() || table->length() == 0) {
    return false;
  }

  const char* data = table->buffer();
  const int length = static_cast<int>(table->length());

  int index = 0;
  while (index < length) {
    const int start = ParseExceptionTableVarint(data, length, index) * 2;
    const int size = ParseExceptionTableVarint(data, length, index) * 2;
    const int end = start + size;
    const int target = ParseExceptionTableVarint(data, length, index) * 2;
    const int depth_and_lasti = ParseExceptionTableVarint(data, length, index);

    if (start <= instruction_offset_in_bytes &&
        instruction_offset_in_bytes < end) {
      out.stack_depth = depth_and_lasti >> 1;
      out.handler_pc = target;
      out.need_push_lasti = (depth_and_lasti & 1) != 0;
      return true;
    }
  }

  return false;
}

}  // namespace saauso::internal
