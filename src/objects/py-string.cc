// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "objects/py-string.h"

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstring>

#include "handles/handles.h"
#include "heap/heap.h"
#include "runtime/universe.h"
#include "third_party/rapidhash/rapidhash.h"

namespace saauso::internal {

namespace {
constexpr uint64_t kInvalidHashCache = 0;
constexpr uint64_t kFallbackHashCache = kInvalidHashCache + 1;
}  // namespace

////////////////////////////////////////////////////////////

// static
Handle<PyString> PyString::NewInstance(int length) {
  Handle<PyString> object(Universe::heap_->Allocate<PyString>());

  object->length_ = length;
  object->hash_ = kInvalidHashCache;

  // 在虚拟机堆上分配一块空间，用于存放实际的字符串
  object->buffer_ = nullptr;
  object->buffer_ =
      static_cast<char*>(Universe::heap_->AllocateRaw(sizeof(char) * length));

  return object;
}

// static
Handle<PyString> PyString::NewInstance(const char* source, int length) {
  Handle<PyString> object(NewInstance(length));

  std::memcpy(object->buffer_, source, length);

  return object;
}

// static
PyString* PyString::Cast(PyObject* object) {
  return reinterpret_cast<PyString*>(object);
}

////////////////////////////////////////////////////////////

void PyString::Set(int index, char value) {
  assert(0 <= index && index < length_);

  buffer_[index] = value;
}

char PyString::Get(int index) const {
  assert(0 <= index && index < length_);

  return buffer_[index];
}

uint64_t PyString::GetHash() {
  if (hash_ == kInvalidHashCache) {
    hash_ = rapidhash(buffer_, length_);
    if (hash_ == kInvalidHashCache) {
      hash_ = kFallbackHashCache;
    }
  }
  return hash_;
}

bool PyString::HasHashCache() const {
  return hash_ != kInvalidHashCache;
}

bool PyString::IsEqualTo(PyString* other) {
  if (length_ != other->length()) {
    return false;
  }

  if (HasHashCache() && other->HasHashCache()) {
    if (GetHash() != other->GetHash()) {
      return false;
    }
  }

  return std::memcmp(buffer_, other->buffer(), length_) == 0;
}

bool PyString::IsLessThan(PyString* other) {
  // 比较长度取较小值，避免越界
  int min_len = std::min(length_, other->length());

  // 使用 memcmp 进行二进制安全的字典序比较
  int cmp = std::memcmp(buffer_, other->buffer(), min_len);

  // 如果前缀部分不相等，直接返回比较结果
  if (cmp != 0) {
    return cmp < 0;
  }

  // 如果前缀部分相等，长度较短的字符串更小
  return length_ < other->length();
}

bool PyString::IsLargerThan(PyString* other) {
  int min_len = std::min(length_, other->length());
  int cmp = std::memcmp(buffer_, other->buffer(), min_len);

  if (cmp != 0) {
    return cmp > 0;
  }

  // 如果前缀相等，长度较长的字符串更大
  return length_ > other->length();
}

// static
Handle<PyString> PyString::Slice(Handle<PyString> string, int from, int to) {
  assert(0 <= from && from <= to && to < string->length_);

  int sliced_length = to - from + 1;
  Handle<PyString> result = PyString::NewInstance(sliced_length);

  std::memcpy(result->buffer_, string->buffer_ + from, sliced_length);
  return result;
}

// static
Handle<PyString> PyString::Append(Handle<PyString> string,
                                  Handle<PyString> other) {
  int new_length = string->length_ + other->length();
  Handle<PyString> new_object(PyString::NewInstance(new_length));

  std::memcpy(new_object->buffer_, string->buffer_, string->length_);
  std::memcpy(new_object->buffer_ + string->length_, other->buffer(),
              other->length());

  return new_object;
}

}  // namespace saauso::internal
