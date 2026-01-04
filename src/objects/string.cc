// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "objects/string.h"

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

// static
String* String::NewInstance(int length) {
  HandleScope scope;

  Handle<String> object(Universe::heap_->Allocate<String>());
  object->set_shape(Universe::string_shape_);
  object->length_ = length;
  object->buffer_ =
      static_cast<char*>(Universe::heap_->AllocateRaw(sizeof(char) * length));
  object->hash_ = kInvalidHashCache;

  return object.get();
}

// static
String* String::NewInstance(const char* source, int length) {
  HandleScope scope;
  Handle<String> object(NewInstance(length));

  std::memcpy(object->buffer_, source, length);

  return object.get();
}

void String::Set(int index, char value) {
  assert(0 <= index && index < length_);

  buffer_[index] = value;
}

char String::Get(int index) const {
  assert(0 <= index && index < length_);

  return buffer_[index];
}

uint64_t String::GetHash() {
  if (hash_ == kInvalidHashCache) {
    hash_ = rapidhash(buffer_, length_);
    if (hash_ == kInvalidHashCache) {
      hash_ = kFallbackHashCache;
    }
  }
  return hash_;
}

bool String::HasHashCache() const {
  return hash_ != kInvalidHashCache;
}

bool String::IsEqualTo(String* other) {
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

bool String::IsLessThan(String* other) {
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

bool String::IsLargerThan(String* other) {
  int min_len = std::min(length_, other->length());
  int cmp = std::memcmp(buffer_, other->buffer(), min_len);

  if (cmp != 0) {
    return cmp > 0;
  }

  // 如果前缀相等，长度较长的字符串更大
  return length_ > other->length();
}

String* String::Slice(int from, int to) const {
  assert(0 <= from && from <= to && to < length_);

  return String::NewInstance(buffer_ + from, to - from + 1);
}

String* String::Append(String* other) const {
  HandleScope scope;

  int new_length = length_ + other->length();
  Handle<String> new_object(String::NewInstance(new_length));

  std::memcpy(new_object->buffer_, buffer_, length_);
  std::memcpy(new_object->buffer_ + length_, other->buffer(), other->length());

  return new_object.get();
}

}  // namespace saauso::internal
