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
Handle<PyString> PyString::NewInstance(int64_t str_length, bool in_meta_space) {
  // 计算出PyString结构体+字符串数据区+对齐所需要的总长度
  size_t object_size = ComputeObjectSize(str_length);

  Tagged<PyString> object(Universe::heap_->AllocateRaw(
      object_size, in_meta_space ? Heap::AllocationSpace::kMetaSpace
                                 : Heap::AllocationSpace::kNewSpace));

  // 初始化字段
  object->length_ = str_length;
  object->hash_ = kInvalidHashCache;

  // 绑定klass
  PyObject::SetKlass(object, PyStringKlass::GetInstance());

  return Handle<PyString>(object);
}

// static
Handle<PyString> PyString::NewInstance(const char* source,
                                       int64_t str_length,
                                       bool in_meta_space) {
  Handle<PyString> object = NewInstance(str_length, in_meta_space);

  std::memcpy(object->writable_buffer(), source, str_length);

  return object;
}

// static
Handle<PyString> PyString::NewInstance(const char* source, bool in_meta_space) {
  return NewInstance(source, std::strlen(source), in_meta_space);
}

// static
Tagged<PyString> PyString::Cast(Tagged<PyObject> object) {
  assert(IsPyString(object));
  return Tagged<PyString>::Cast(object);
}

// static
size_t PyString::ComputeObjectSize(int64_t str_length) {
  return ObjectSizeAlign(sizeof(PyString) + str_length);
}

////////////////////////////////////////////////////////////

void PyString::Set(int64_t index, char value) {
  assert(0 <= index && index < length_);

  writable_buffer()[index] = value;
}

char PyString::Get(int64_t index) const {
  assert(0 <= index && index < length_);

  return buffer()[index];
}

uint64_t PyString::GetHash() {
  if (hash_ == kInvalidHashCache) {
    hash_ = rapidhash(buffer(), length_);
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

  return std::memcmp(buffer(), other->buffer(), length_) == 0;
}

bool PyString::IsLessThan(Tagged<PyString> other) {
  // 比较长度取较小值，避免越界
  int min_len = std::min(length_, other->length());

  // 使用 memcmp 进行二进制安全的字典序比较
  int cmp = std::memcmp(buffer(), other->buffer(), min_len);

  // 如果前缀部分不相等，直接返回比较结果
  if (cmp != 0) {
    return cmp < 0;
  }

  // 如果前缀部分相等，长度较短的字符串更小
  return length_ < other->length();
}

bool PyString::IsGreaterThan(Tagged<PyString> other) {
  int min_len = std::min(length_, other->length());
  int cmp = std::memcmp(buffer(), other->buffer(), min_len);

  if (cmp != 0) {
    return cmp > 0;
  }

  // 如果前缀相等，长度较长的字符串更大
  return length_ > other->length();
}

Handle<PyString> PyString::Slice(Handle<PyString> self,
                                 int64_t from,
                                 int64_t to) {
  HandleScope scope;

  assert(0 <= from && from <= to && to < self->length_);

  int sliced_length = to - from + 1;
  Handle<PyString> result = PyString::NewInstance(sliced_length);

  std::memcpy(result->writable_buffer(), self->buffer() + from, sliced_length);
  return result;
}

Handle<PyString> PyString::Append(Handle<PyString> self,
                                  Handle<PyString> other) {
  HandleScope scope;

  int new_length = self->length_ + other->length();
  Handle<PyString> new_object(PyString::NewInstance(new_length));

  std::memcpy(new_object->writable_buffer(), self->buffer(), self->length_);
  std::memcpy(new_object->writable_buffer() + self->length_, other->buffer(),
              other->length());

  return new_object;
}

}  // namespace saauso::internal
