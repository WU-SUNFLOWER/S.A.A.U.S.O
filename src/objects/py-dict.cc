// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/objects/py-dict.h"

#include <cstring>

#include "include/saauso-internal.h"
#include "src/execution/isolate.h"
#include "src/handles/handles.h"
#include "src/handles/maybe-handles.h"
#include "src/heap/heap.h"
#include "src/objects/fixed-array.h"
#include "src/objects/py-dict-klass.h"
#include "src/objects/py-object.h"
#include "src/objects/py-oddballs.h"
#include "src/objects/py-tuple.h"
#include "src/utils/maybe.h"

namespace saauso::internal {

namespace {

constexpr double kMaxLoadFactor = 0.75;

#define GET_DICT_KEY(dict, index) (dict->data()->Get(index << 1))
#define GET_DICT_VAL(dict, index) (dict->data()->Get((index << 1) + 1))

#define SET_DICT_KEY(dict, index, key) dict->data()->Set(index << 1, key)
#define SET_DICT_VAL(dict, index, value) \
  dict->data()->Set((index << 1) + 1, value)

uint64_t GetProbe(uint64_t index, uint64_t mask) {
  return (index + 1) & mask;
}

template <typename DictT>
Maybe<int64_t> FindSlot(DictT dict, Handle<PyObject> key, bool* found) {
  uint64_t hash = 0;
  if (!PyObject::Hash(key).To(&hash)) {
    return kNullMaybe;
  }

  uint64_t mask = dict->capacity() - 1;
  uint64_t index = hash & mask;

  Tagged<PyObject> curr_key = GET_DICT_KEY(dict, index);
  while (!curr_key.is_null()) {
    bool is_equal = false;
    if (!PyObject::EqualBool(handle(curr_key), key).To(&is_equal)) {
      return kNullMaybe;
    }
    if (is_equal) {
      *found = true;
      return Maybe<int64_t>(static_cast<int64_t>(index));
    }
    index = GetProbe(index, mask);
    curr_key = GET_DICT_KEY(dict, index);
  }

  *found = false;
  return Maybe<int64_t>(static_cast<int64_t>(index));
}

template <typename DictT>
Maybe<bool> RehashInto(DictT dict,
                       Handle<FixedArray> new_data,
                       uint64_t new_mask,
                       int64_t skip_index) {
  for (int64_t i = 0; i < dict->capacity(); ++i) {
    if (i == skip_index) {
      continue;
    }

    Tagged<PyObject> key = GET_DICT_KEY(dict, i);
    if (key.is_null()) {
      continue;
    }

    uint64_t hash = 0;
    if (!PyObject::Hash(handle(key)).To(&hash)) {
      return kNullMaybe;
    }

    uint64_t new_index = hash & new_mask;
    while (!new_data->Get(new_index << 1).is_null()) {
      new_index = GetProbe(new_index, new_mask);
    }

    new_data->Set(new_index << 1, key);
    new_data->Set((new_index << 1) + 1, GET_DICT_VAL(dict, i));
  }
  return Maybe<bool>(true);
}

}  // namespace

//////////////////////////////////////////////////////////////////////////

// static
Handle<PyDict> PyDict::NewInstance(int64_t init_capacity) {
  // 因为探测公式取`index = (capacity_ - 1) & mask`，
  // 为了在按位与之后充分利用哈希表中的各个槽位，
  // 我们要求init_capacity必须取2的幂次方
  assert((init_capacity & (init_capacity - 1)) == 0);

  EscapableHandleScope scope;

  // 创建对象
  Handle<PyDict> object = NewInstanceWithoutAllocateData();

  // 创建数据区
  // 因为dict中同时要储存key和value，所以init_capacity要乘2
  Handle<FixedArray> data = FixedArray::NewInstance(init_capacity * 2);
  object->data_ = *data;

  return scope.Escape(object);
}

// static
Handle<PyDict> PyDict::Clone(Handle<PyDict> other) {
  EscapableHandleScope scope;

  Handle<PyDict> result = NewInstanceWithoutAllocateData();
  Handle<FixedArray> data = FixedArray::Clone(other->data());
  result->occupied_ = other->occupied();
  result->data_ = *data;

  return scope.Escape(result);
}

// static
Handle<PyDict> PyDict::NewInstanceWithoutAllocateData() {
  // 创建对象
  Handle<PyDict> object(Isolate::Current()->heap()->Allocate<PyDict>(
      Heap::AllocationSpace::kNewSpace));

  // 设置字段
  object->occupied_ = 0;

  // 绑定klass
  SetKlass(object, PyDictKlass::GetInstance());

  // float类型没有__dict__，不需要初始化properties
  // >>> dict().__dict__
  // Traceback (most recent call last):
  //   File "<stdin>", line 1, in <module>
  // AttributeError: 'dict' object has no attribute '__dict__'
  PyObject::SetProperties(*object, Tagged<PyDict>::null());

  return object;
}

// static
Tagged<PyDict> PyDict::cast(Tagged<PyObject> object) {
  assert(IsPyDict(object));
  return Tagged<PyDict>::cast(object);
}

//////////////////////////////////////////////////////////////////////////

int64_t PyDict::capacity() const {
  return data()->capacity() >> 1;
}

Handle<PyObject> PyDict::KeyAtIndex(int64_t index) const {
  return handle(data()->Get(index << 1));
}

Handle<PyObject> PyDict::ValueAtIndex(int64_t index) const {
  return handle(data()->Get((index << 1) + 1));
}

Handle<PyTuple> PyDict::ItemAtIndex(int64_t index) const {
  auto key = KeyAtIndex(index);
  // 如果当前槽位没有有效的键值对，直接返回null
  if (key.is_null()) {
    return Handle<PyTuple>::null();
  }
  auto result = PyTuple::NewInstance(2);
  auto value = ValueAtIndex(index);
  result->SetInternal(0, key);
  result->SetInternal(1, value);
  return result;
}

Handle<FixedArray> PyDict::data() const {
  return Handle<FixedArray>(Tagged<FixedArray>::cast(data_));
}

Maybe<bool> PyDict::ContainsMaybe(Handle<PyObject> key) const {
  Tagged<PyObject> value;
  if (!GetTaggedMaybe(*key).To(&value)) {
    return kNullMaybe;
  }
  return Maybe<bool>(!value.is_null());
}

Maybe<Tagged<PyObject>> PyDict::GetTaggedMaybe(Handle<PyObject> key) const {
  return GetTaggedMaybe(*key);
}

Maybe<Tagged<PyObject>> PyDict::GetTaggedMaybe(Tagged<PyObject> key) const {
  HandleScope scope;
  Handle<PyObject> handle_key(key);

  bool found = false;
  int64_t index = 0;
  if (!FindSlot(this, handle_key, &found).To(&index)) {
    return kNullMaybe;
  }
  if (!found) {
    return Maybe<Tagged<PyObject>>(Tagged<PyObject>::null());
  }
  return Maybe<Tagged<PyObject>>(GET_DICT_VAL(this, index));
}

Maybe<bool> PyDict::RemoveMaybe(Handle<PyObject> key) {
  HandleScope scope;

  bool found = false;
  int64_t index = 0;
  if (!FindSlot(this, key, &found).To(&index)) {
    return kNullMaybe;
  }
  if (!found) {
    return Maybe<bool>(false);
  }

  Handle<FixedArray> new_data = FixedArray::NewInstance(capacity() * 2);
  uint64_t new_mask = capacity() - 1;

  if (RehashInto(this, new_data, new_mask, index).IsNothing()) {
    return kNullMaybe;
  }

  data_ = *new_data;
  --occupied_;
  return Maybe<bool>(true);
}

// static
Maybe<bool> PyDict::PutMaybe(Handle<PyDict> object,
                             Handle<PyObject> key,
                             Handle<PyObject> value) {
  HandleScope scope;

  assert(!key.is_null());
  assert(!value.is_null());

  auto dict = Handle<PyDict>::cast(object);
  if (dict->occupied_ + 1 > dict->capacity() * kMaxLoadFactor) {
    bool expanded = false;
    if (!ExpandImplMaybe(dict).To(&expanded)) {
      return kNullMaybe;
    }
  }

  bool found = false;
  int64_t index = 0;
  if (!FindSlot(dict, key, &found).To(&index)) {
    return kNullMaybe;
  }

  if (found) {
    SET_DICT_VAL(dict, index, *value);
    return Maybe<bool>(false);
  }

  SET_DICT_KEY(dict, index, *key);
  SET_DICT_VAL(dict, index, *value);
  ++dict->occupied_;
  return Maybe<bool>(true);
}

// static
Handle<PyTuple> PyDict::GetKeyTuple(Handle<PyDict> dict) {
  EscapableHandleScope scope;

  int64_t out_length = dict->occupied();
  Handle<PyTuple> keys = PyTuple::NewInstance(out_length);

  Handle<FixedArray> data = dict->data();
  int64_t out_index = 0;
  for (auto i = 0; i < dict->capacity(); ++i) {
    Tagged<PyObject> key = data->Get(i << 1);
    if (key.is_null()) {
      continue;
    }
    keys->SetInternal(out_index++, key);
    if (out_index == out_length) {
      break;
    }
  }
  assert(out_index == out_length);

  return scope.Escape(keys);
}

// static
void PyDict::ExpandImpl(Handle<PyDict> dict) {
  bool expanded = false;
  ExpandImplMaybe(dict).To(&expanded);
}

// static
Maybe<bool> PyDict::ExpandImplMaybe(Handle<PyDict> dict) {
  HandleScope scope;

  int64_t new_capacity = dict->capacity() << 1;
  Handle<FixedArray> new_data = FixedArray::NewInstance(new_capacity * 2);
  uint64_t new_mask = new_capacity - 1;

  if (RehashInto(dict, new_data, new_mask, -1).IsNothing()) {
    return kNullMaybe;
  }

  dict->data_ = *new_data;
  return Maybe<bool>(true);
}

}  // namespace saauso::internal
