// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/objects/py-dict.h"

#include <cstring>

#include "include/saauso-internal.h"
#include "src/handles/handles.h"
#include "src/heap/heap.h"
#include "src/objects/fixed-array.h"
#include "src/objects/py-dict-klass.h"
#include "src/objects/py-object.h"
#include "src/objects/py-oddballs.h"
#include "src/objects/py-tuple.h"
#include "src/runtime/isolate.h"

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

bool PyDict::Contains(Handle<PyObject> key) const {
  return !Get(key).is_null();
}

Handle<PyObject> PyDict::Get(Handle<PyObject> key) const {
  return Get(*key);
}

Handle<PyObject> PyDict::Get(Tagged<PyObject> key) const {
  return handle(GetImpl(key));
}

Tagged<PyObject> PyDict::GetTagged(Handle<PyObject> key) const {
  return GetTagged(*key);
}

Tagged<PyObject> PyDict::GetTagged(Tagged<PyObject> key) const {
  return GetImpl(key);
}

Tagged<PyObject> PyDict::GetImpl(Tagged<PyObject> key) const {
  HandleScope scope;
  Handle<PyObject> handle_key(key);

  uint64_t hash = PyObject::Hash(handle_key);
  uint64_t mask = capacity() - 1;
  uint64_t index = hash & mask;

  auto curr_key = GET_DICT_KEY(this, index);
  while (!curr_key.is_null()) {
    if (PyObject::Equal(handle(curr_key), handle_key)->value()) {
      return GET_DICT_VAL(this, index);
    }
    index = GetProbe(index, mask);
    curr_key = GET_DICT_KEY(this, index);
  }

  return Tagged<PyObject>::null();
}

void PyDict::Remove(Handle<PyObject> key) {
  HandleScope scope;

  uint64_t hash = PyObject::Hash(key);
  uint64_t mask = capacity() - 1;
  uint64_t index = hash & mask;

  auto curr_key = GET_DICT_KEY(this, index);
  while (!curr_key.is_null()) {
    if (PyObject::Equal(handle(curr_key), key)->value()) {
      // 找到目标，进行删除
      // 1. 将当前位置置空
      SET_DICT_KEY(this, index, Tagged<PyObject>::null());
      SET_DICT_VAL(this, index, Tagged<PyObject>::null());
      --occupied_;

      // 2. 整理后续元素（backward shift deletion）
      uint64_t hole = index;
      uint64_t next = GetProbe(hole, mask);

      Handle<PyObject> next_key = handle(GET_DICT_KEY(this, next));
      while (!(*next_key).is_null()) {
        uint64_t next_hash = PyObject::Hash(next_key);
        uint64_t ideal = next_hash & mask;

        // 计算距离
        uint64_t dist_next = (next - ideal) & mask;
        uint64_t dist_hole = (hole - ideal) & mask;

        // 如果hole更接近ideal，则移动next到hole
        if (dist_hole < dist_next) {
          SET_DICT_KEY(this, hole, *next_key);
          SET_DICT_VAL(this, hole, GET_DICT_VAL(this, next));

          SET_DICT_KEY(this, next, Tagged<PyObject>::null());
          SET_DICT_VAL(this, next, Tagged<PyObject>::null());

          hole = next;
        }

        next = GetProbe(next, mask);
        next_key = handle(GET_DICT_KEY(this, next));
      }
      return;
    }
    index = GetProbe(index, mask);
    curr_key = GET_DICT_KEY(this, index);
  }
}

// static
void PyDict::Put(Handle<PyObject> object,
                 Handle<PyObject> key,
                 Handle<PyObject> value) {
  HandleScope scope;

  assert(!key.is_null());
  assert(!value.is_null());

  auto dict = Handle<PyDict>::cast(object);
  // 超出装填因子系数，自动进行扩容
  if (dict->occupied_ + 1 > dict->capacity() * kMaxLoadFactor) {
    ExpandImpl(dict);
  }

  uint64_t hash = PyObject::Hash(key);
  uint64_t mask = dict->capacity() - 1;
  uint64_t index = hash & mask;

  Handle<PyObject> curr_key(GET_DICT_KEY(dict, index));
  while (!curr_key.is_null()) {
    if (PyObject::Equal(curr_key, key)->value()) {
      SET_DICT_VAL(dict, index, *value);
      return;
    }
    index = GetProbe(index, mask);
    curr_key = handle(GET_DICT_KEY(dict, index));
  }

  // 插入新元素
  SET_DICT_KEY(dict, index, *key);
  SET_DICT_VAL(dict, index, *value);

  // 更新计数器
  ++dict->occupied_;
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
  HandleScope scope;

  int64_t new_capacity = dict->capacity() << 1;
  Handle<FixedArray> new_data = FixedArray::NewInstance(new_capacity * 2);
  uint64_t new_mask = new_capacity - 1;

  // rehash所有旧数据
  for (auto index = 0; index < dict->capacity(); ++index) {
    if (GET_DICT_KEY(dict, index).is_null()) {
      continue;
    }

    Handle<PyObject> key(GET_DICT_KEY(dict, index));
    uint64_t hash = PyObject::Hash(key);
    uint64_t new_index = hash & new_mask;

    // 在new_data中寻找一个合适的位置放置键值对
    while (!new_data->Get(new_index << 1).is_null()) {
      new_index = GetProbe(new_index, new_mask);
    }

    new_data->Set(new_index << 1, *key);
    new_data->Set((new_index << 1) + 1, GET_DICT_VAL(dict, index));
  }

  dict->data_ = *new_data;
}

}  // namespace saauso::internal
