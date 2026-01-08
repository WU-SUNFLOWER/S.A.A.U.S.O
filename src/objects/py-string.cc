// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/objects/py-string.h"

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstring>

#include "src/handles/handles.h"
#include "src/heap/heap.h"
#include "src/objects/py-string-klass.h"
#include "src/runtime/universe.h"
#include "third_party/rapidhash/rapidhash.h"

namespace saauso::internal {

namespace {
constexpr uint64_t kInvalidHashCache = 0;
constexpr uint64_t kFallbackHashCache = kInvalidHashCache + 1;
}  // namespace

////////////////////////////////////////////////////////////

// static
Handle<PyString> PyString::NewInstance(int64_t length) {
  Handle<PyString> object(
      Universe::heap_->Allocate<PyString>(Heap::AllocationSpace::kNewSpace));

  object->length_ = length;
  object->hash_ = kInvalidHashCache;

  // 在虚拟机堆上分配一块空间，用于存放实际的字符串
  object->buffer_ = nullptr;
  object->buffer_ = static_cast<char*>(Universe::heap_->AllocateRaw(
      sizeof(char) * length, Heap::AllocationSpace::kNewSpace));

  // 绑定klass
  PyObject::SetKlass(*object, PyStringKlass::GetInstance());

  return object;
}

// static
Handle<PyString> PyString::NewInstance(const char* source, int64_t length) {
  Handle<PyString> object(NewInstance(length));

  std::memcpy(object->buffer_, source, length);

  return object;
}

// static
Handle<PyString> PyString::NewInstance(const char* source) {
  return NewInstance(source, std::strlen(source));
}

// static
Tagged<PyString> PyString::Cast(Tagged<PyObject> object) {
  assert(IsPyString(object));
  return Tagged<PyString>::Cast(object);
}

////////////////////////////////////////////////////////////

void PyString::Set(int64_t index, char value) {
  assert(0 <= index && index < length_);

  buffer_[index] = value;
}

char PyString::Get(int64_t index) const {
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

bool PyString::IsEqualTo(Tagged<PyString> other) {
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

bool PyString::IsLessThan(Tagged<PyString> other) {
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

bool PyString::IsGreaterThan(Tagged<PyString> other) {
  int min_len = std::min(length_, other->length());
  int cmp = std::memcmp(buffer_, other->buffer(), min_len);

  if (cmp != 0) {
    return cmp > 0;
  }

  // 如果前缀相等，长度较长的字符串更大
  return length_ > other->length();
}

Handle<PyString> PyString::Slice(Handle<PyString> self, int64_t from, int64_t to) {
  HandleScope scope;

  assert(0 <= from && from <= to && to < self->length_);

  int sliced_length = to - from + 1;
  Handle<PyString> result = PyString::NewInstance(sliced_length);

  std::memcpy(result->buffer_, self->buffer_ + from, sliced_length);
  return result;
}

Handle<PyString> PyString::Append(Handle<PyString> self, Handle<PyString> other) {
  HandleScope scope;

  int new_length = self->length_ + other->length();
  Handle<PyString> new_object(PyString::NewInstance(new_length));

  std::memcpy(new_object->buffer_, self->buffer_, self->length_);
  std::memcpy(new_object->buffer_ + self->length_, other->buffer(),
              other->length());

  return new_object;
}

}  // namespace saauso::internal
