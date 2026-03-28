// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/objects/py-string.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>

#include "src/execution/isolate.h"
#include "src/handles/handles.h"
#include "src/heap/factory.h"
#include "src/objects/py-dict.h"
#include "src/objects/py-float.h"
#include "src/objects/py-smi.h"
#include "src/objects/py-string-klass.h"
#include "src/utils/number-conversion.h"
#include "third_party/rapidhash/rapidhash.h"

namespace saauso::internal {

namespace {
constexpr uint64_t kInvalidHashCache = 0;
constexpr uint64_t kFallbackHashCache = kInvalidHashCache + 1;
constexpr int kNumberToStringBufferSize = 32;

// 在计算对象大小时，字符串buffer末尾的'\0'哨兵也要算进去
constexpr int kSentinelValueSizeInBytes = 1;
}  // namespace

////////////////////////////////////////////////////////////

// static
Handle<PyString> PyString::New(Isolate* isolate,
                               int64_t str_length,
                               bool in_meta_space) {
  return isolate->factory()->NewRawStringLike(
      PyStringKlass::GetInstance(isolate), str_length, in_meta_space);
}

// static
Handle<PyString> PyString::New(Isolate* isolate,
                               const char* source,
                               int64_t str_length,
                               bool in_meta_space) {
  return isolate->factory()->NewString(source, str_length, in_meta_space);
}

// static
Handle<PyString> PyString::New(Isolate* isolate,
                               const char* source,
                               bool in_meta_space) {
  return isolate->factory()->NewString(source, std::strlen(source),
                                       in_meta_space);
}

// static
Handle<PyString> PyString::Clone(Isolate* isolate,
                                 Handle<PyString> other,
                                 bool in_meta_space) {
  return isolate->factory()->NewString(other->buffer(), other->length(),
                                       in_meta_space);
}

////////////////////////////////////////////////////////////

// static
Handle<PyString> PyString::FromPySmi(Tagged<PySmi> smi) {
  return FromInt(PySmi::ToInt(smi));
}

// static
Handle<PyString> PyString::FromInt(int64_t n) {
  EscapableHandleScope scope;

  std::array<char, kNumberToStringBufferSize> buffer{};
  Int64ToStringView(n, std::string_view(buffer.data(), buffer.size()));
  Handle<PyString> result = PyString::New(Isolate::Current(), buffer.data());
  return scope.Escape(result);
}

// static
Handle<PyString> PyString::FromPyFloat(Handle<PyFloat> py_float) {
  return FromDouble(py_float->value());
}

// static
Handle<PyString> PyString::FromDouble(double n) {
  EscapableHandleScope scope;

  std::array<char, kNumberToStringBufferSize> buffer{};
  DoubleToStringView(n, std::string_view(buffer.data(), buffer.size()));
  Handle<PyString> result = PyString::New(Isolate::Current(), buffer.data());
  return scope.Escape(result);
}

// static
Handle<PyString> PyString::FromStdString(std::string source) {
  return PyString::New(Isolate::Current(), source.data(),
                       static_cast<int64_t>(source.size()));
}

////////////////////////////////////////////////////////////

// static
Tagged<PyString> PyString::cast(Tagged<PyObject> object) {
  assert(IsPyString(object));
  return Tagged<PyString>::cast(object);
}

// static
size_t PyString::ComputeObjectSize(int64_t str_length) {
  return ObjectSizeAlign(sizeof(PyString) + str_length +
                         kSentinelValueSizeInBytes);
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
  if (Tagged<PyString>(this) == other) {
    return true;
  }

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

std::string PyString::ToStdString() const {
  return std::string(buffer(), static_cast<size_t>(length()));
}

Handle<PyString> PyString::Slice(Handle<PyString> self, int64_t from) {
  return Slice(self, from, self->length() - 1);
}

Handle<PyString> PyString::Slice(Handle<PyString> self,
                                 int64_t from,
                                 int64_t to) {
  EscapableHandleScope scope;

  assert(0 <= from && from <= to && to < self->length_);

  int sliced_length = to - from + 1;
  Handle<PyString> result = PyString::New(Isolate::Current(), sliced_length);

  std::memcpy(result->writable_buffer(), self->buffer() + from, sliced_length);
  return scope.Escape(result);
}

Handle<PyString> PyString::Append(Handle<PyString> self,
                                  Handle<PyString> other,
                                  Isolate* isolate) {
  return isolate->factory()->NewConsString(self, other);
}

int64_t PyString::IndexOf(Handle<PyString> pattern) const {
  return IndexOf(pattern, 0, length());
}

int64_t PyString::IndexOf(Handle<PyString> pattern,
                          int64_t begin,
                          int64_t end) const {
  assert(0 <= begin && begin <= end && end <= length());

  size_t length = static_cast<size_t>(end - begin);
  size_t pattern_length = static_cast<size_t>(pattern->length());

  int64_t offset = StringSearch::IndexOfSubstring(
      std::string_view(buffer() + begin, length),
      std::string_view(pattern->buffer(), pattern_length));
  return offset == StringSearch::kNotFound ? kNotFound : begin + offset;
}

int64_t PyString::LastIndexOf(Handle<PyString> pattern) const {
  return LastIndexOf(pattern, 0, length());
}

int64_t PyString::LastIndexOf(Handle<PyString> pattern,
                              int64_t begin,
                              int64_t end) const {
  assert(0 <= begin && begin <= end && end <= length());

  size_t length = static_cast<size_t>(end - begin);
  size_t pattern_length = static_cast<size_t>(pattern->length());

  int64_t offset = StringSearch::LastIndexOfSubstring(
      std::string_view(buffer() + begin, length),
      std::string_view(pattern->buffer(), pattern_length));
  return offset == StringSearch::kNotFound ? kNotFound : begin + offset;
}

bool PyString::Contains(Handle<PyString> pattern) const {
  return IndexOf(pattern) != kNotFound;
}

}  // namespace saauso::internal
