// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/objects/py-dict.h"

#include <cstring>

#include "include/saauso-internal.h"
#include "include/saauso-maybe.h"
#include "src/execution/isolate.h"
#include "src/handles/handles.h"
#include "src/handles/maybe-handles.h"
#include "src/heap/factory.h"
#include "src/objects/fixed-array.h"
#include "src/objects/py-dict-klass.h"
#include "src/objects/py-object.h"
#include "src/objects/py-oddballs.h"
#include "src/objects/py-tuple.h"

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
Maybe<int64_t> FindSlot(DictT dict, Tagged<PyObject> key, bool* found) {
  HandleScope scope;
  Handle<PyObject> key_handle(key);

  uint64_t hash = 0;
  if (!PyObject::Hash(Isolate::Current(), key_handle).To(&hash)) {
    return kNullMaybe;
  }

  uint64_t mask = dict->capacity() - 1;
  uint64_t index = hash & mask;

  Tagged<PyObject> curr_key = GET_DICT_KEY(dict, index);
  while (!curr_key.is_null()) {
    bool is_equal = false;
    if (!PyObject::EqualBool(Isolate::Current(), handle(curr_key), key_handle)
             .To(&is_equal)) {
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
    if (!PyObject::Hash(Isolate::Current(), handle(key)).To(&hash)) {
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
  return Isolate::Current()->factory()->NewPyDict(init_capacity);
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

Maybe<bool> PyDict::ContainsKey(Handle<PyObject> key) const {
  Tagged<PyObject> value;
  bool found = false;
  if (!GetTagged(*key, value).To(&found)) {
    return kNullMaybe;
  }
  return Maybe<bool>(found);
}

Maybe<bool> PyDict::Get(Handle<PyObject> key, Handle<PyObject>& out) const {
  return Get(*key, out);
}

Maybe<bool> PyDict::Get(Tagged<PyObject> key, Handle<PyObject>& out) const {
  assert(!key.is_null());

  out = Handle<PyObject>::null();

  bool found = false;
  int64_t index = 0;
  if (!FindSlot(this, key, &found).To(&index)) {
    return kNullMaybe;
  }

  if (!found) {
    return Maybe<bool>(false);
  }

  out = handle(GET_DICT_VAL(this, index));
  assert(!out.is_null());
  return Maybe<bool>(true);
}

Maybe<bool> PyDict::GetTagged(Handle<PyObject> key,
                              Tagged<PyObject>& out) const {
  return GetTagged(*key, out);
}

Maybe<bool> PyDict::GetTagged(Tagged<PyObject> key,
                              Tagged<PyObject>& out) const {
  assert(!key.is_null());

  out = Tagged<PyObject>::null();

  bool found = false;
  int64_t index = 0;
  if (!FindSlot(this, key, &found).To(&index)) {
    return kNullMaybe;
  }

  if (!found) {
    return Maybe<bool>(false);
  }

  out = GET_DICT_VAL(this, index);
  assert(!out.is_null());
  return Maybe<bool>(true);
}

Maybe<bool> PyDict::Remove(Handle<PyObject> key) {
  HandleScope scope;

  bool found = false;
  int64_t index = 0;
  if (!FindSlot(this, *key, &found).To(&index)) {
    return kNullMaybe;
  }
  if (!found) {
    return Maybe<bool>(false);
  }

  Handle<FixedArray> new_data =
      Isolate::Current()->factory()->NewFixedArray(capacity() * 2);
  uint64_t new_mask = capacity() - 1;

  if (RehashInto(this, new_data, new_mask, index).IsNothing()) {
    return kNullMaybe;
  }

  data_ = *new_data;
  --occupied_;
  return Maybe<bool>(true);
}

// static
Maybe<bool> PyDict::Put(Handle<PyDict> object,
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
  if (!FindSlot(dict, *key, &found).To(&index)) {
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
  Handle<FixedArray> new_data =
      Isolate::Current()->factory()->NewFixedArray(new_capacity * 2);
  uint64_t new_mask = new_capacity - 1;

  if (RehashInto(dict, new_data, new_mask, -1).IsNothing()) {
    return kNullMaybe;
  }

  dict->data_ = *new_data;
  return Maybe<bool>(true);
}

}  // namespace saauso::internal
