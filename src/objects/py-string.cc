// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/objects/py-string.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <cstdint>
#include <cstring>

#include "src/execution/isolate.h"
#include "src/handles/handles.h"
#include "src/heap/heap.h"
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
}  // namespace

////////////////////////////////////////////////////////////

// static
Handle<PyString> PyString::NewInstance(int64_t str_length, bool in_meta_space) {
  // 计算出PyString结构体+字符串数据区+对齐所需要的总长度
  // 这里的+1是用来放置字符串末尾的哨兵的
  size_t object_size = ComputeObjectSize(str_length + 1);

  Tagged<PyString> object(Isolate::Current()->heap()->AllocateRaw(
      object_size, in_meta_space ? Heap::AllocationSpace::kMetaSpace
                                 : Heap::AllocationSpace::kNewSpace));

  // 初始化字段
  object->length_ = str_length;
  object->hash_ = kInvalidHashCache;

  // 设置哨兵位
  object->writable_buffer()[str_length] = '\0';

  // str类型没有__dict__，不需要初始化properties
  // >>> s = "ABC"
  // >>> s.__dict__
  // Traceback (most recent call last):
  //   File "<stdin>", line 1, in <module>
  // AttributeError: 'str' object has no attribute '__dict__'
  PyObject::SetProperties(object, Tagged<PyDict>::null());

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
Handle<PyString> PyString::Clone(Handle<PyString> other, bool in_meta_space) {
  return NewInstance(other->buffer(), other->length(), in_meta_space);
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
  Handle<PyString> result = PyString::NewInstance(buffer.data());
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
  Handle<PyString> result = PyString::NewInstance(buffer.data());
  return scope.Escape(result);
}

////////////////////////////////////////////////////////////

// static
Tagged<PyString> PyString::cast(Tagged<PyObject> object) {
  assert(IsPyString(object));
  return Tagged<PyString>::cast(object);
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
  EscapableHandleScope scope;

  assert(0 <= from && from <= to && to < self->length_);

  int sliced_length = to - from + 1;
  Handle<PyString> result = PyString::NewInstance(sliced_length);

  std::memcpy(result->writable_buffer(), self->buffer() + from, sliced_length);
  return scope.Escape(result);
}

Handle<PyString> PyString::Append(Handle<PyString> self,
                                  Handle<PyString> other) {
  EscapableHandleScope scope;

  int new_length = self->length_ + other->length();
  Handle<PyString> new_object(PyString::NewInstance(new_length));

  std::memcpy(new_object->writable_buffer(), self->buffer(), self->length_);
  std::memcpy(new_object->writable_buffer() + self->length_, other->buffer(),
              other->length());

  return scope.Escape(new_object);
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

bool PyString::Contains(Handle<PyString> pattern) const {
  return IndexOf(pattern) != kNotFound;
}

}  // namespace saauso::internal
