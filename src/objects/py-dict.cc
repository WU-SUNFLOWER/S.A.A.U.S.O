// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/objects/py-dict.h"

#include <cstring>

#include "include/saauso-internal.h"
#include "include/saauso-maybe.h"
#include "src/execution/exception-utils.h"
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

void SetDictKey(Handle<PyDict> dict, int64_t index, Handle<PyObject> key) {
  dict->data_tagged()->Set(index << 1, key);
}

void SetDictVal(Handle<PyDict> dict, int64_t index, Handle<PyObject> value) {
  dict->data_tagged()->Set((index << 1) + 1, value);
}

uint64_t GetProbe(uint64_t index, uint64_t mask) {
  return (index + 1) & mask;
}

Maybe<int64_t> FindSlot(Handle<PyDict> dict,
                        Handle<PyObject> key,
                        bool* found,
                        Isolate* isolate) {
  HandleScope scope(isolate);

  uint64_t hash = 0;
  if (!PyObject::Hash(isolate, key).To(&hash)) {
    return kNullMaybe;
  }

  uint64_t mask = dict->capacity() - 1;
  uint64_t index = hash & mask;

  Handle<PyObject> curr_key = dict->KeyAtIndex(index, isolate);
  while (!curr_key.is_null()) {
    bool is_equal = false;
    ASSIGN_RETURN_ON_EXCEPTION(isolate, is_equal,
                               PyObject::EqualBool(isolate, curr_key, key));

    if (is_equal) {
      *found = true;
      return Maybe<int64_t>(static_cast<int64_t>(index));
    }
    index = GetProbe(index, mask);
    curr_key = dict->KeyAtIndex(index, isolate);
  }

  *found = false;
  return Maybe<int64_t>(static_cast<int64_t>(index));
}

Maybe<bool> RehashInto(Handle<PyDict> dict,
                       Handle<FixedArray> new_data,
                       uint64_t new_mask,
                       int64_t skip_index,
                       Isolate* isolate) {
  HandleScope scope(isolate);

  for (int64_t i = 0; i < dict->capacity(); ++i) {
    if (i == skip_index) {
      continue;
    }

    Handle<PyObject> key = dict->KeyAtIndex(i, isolate);
    if (key.is_null()) {
      continue;
    }

    uint64_t hash = 0;
    ASSIGN_RETURN_ON_EXCEPTION(isolate, hash, PyObject::Hash(isolate, key));

    uint64_t new_index = hash & new_mask;
    while (!new_data->Get(new_index << 1).is_null()) {
      new_index = GetProbe(new_index, new_mask);
    }

    new_data->Set(new_index << 1, key);
    new_data->Set((new_index << 1) + 1, dict->ValueAtIndex(i, isolate));
  }

  return Maybe<bool>(true);
}

}  // namespace

//////////////////////////////////////////////////////////////////////////

// static
Handle<PyDict> PyDict::New(Isolate* isolate, int64_t init_capacity) {
  return isolate->factory()->NewPyDict(init_capacity);
}

// static
Tagged<PyDict> PyDict::cast(Tagged<PyObject> object) {
  assert(IsPyDict(object));
  return Tagged<PyDict>::cast(object);
}

//////////////////////////////////////////////////////////////////////////

int64_t PyDict::capacity() const {
  return data_tagged()->capacity() >> 1;
}

Handle<PyObject> PyDict::KeyAtIndex(int64_t index, Isolate* isolate) const {
  DisallowHeapAllocation no_alloc(isolate);
  return handle(data_tagged()->Get(index << 1), isolate);
}

Handle<PyObject> PyDict::ValueAtIndex(int64_t index, Isolate* isolate) const {
  DisallowHeapAllocation no_alloc(isolate);
  return handle(data_tagged()->Get((index << 1) + 1), isolate);
}

// static
Handle<PyTuple> PyDict::ItemAtIndex(Handle<PyDict> dict,
                                    int64_t index,
                                    Isolate* isolate) {
  Handle<PyObject> key = dict->KeyAtIndex(index, isolate);
  // 如果当前槽位没有有效的键值对，直接返回null
  if (key.is_null()) {
    return Handle<PyTuple>::null();
  }

  Handle<PyTuple> result = PyTuple::New(isolate, 2);
  Handle<PyObject> value = dict->ValueAtIndex(index, isolate);
  result->SetInternal(0, key);
  result->SetInternal(1, value);
  return result;
}

Handle<FixedArray> PyDict::data(Isolate* isolate) const {
  return handle(data_tagged(), isolate);
}

Tagged<FixedArray> PyDict::data_tagged() const {
  return Tagged<FixedArray>::cast(data_);
}

void PyDict::set_data(Handle<FixedArray> data) {
  set_data(*data);
}

void PyDict::set_data(Tagged<FixedArray> data) {
  data_ = data;
  WRITE_BARRIER(Tagged<PyObject>(this), &data_, data);
}

// static
Maybe<bool> PyDict::ContainsKey(Handle<PyDict> dict,
                                Handle<PyObject> key,
                                Isolate* isolate) {
  bool found = false;
  RETURN_ON_EXCEPTION(isolate, FindSlot(dict, key, &found, isolate));
  return Maybe<bool>(found);
}

// static
Maybe<bool> PyDict::Get(Handle<PyDict> dict,
                        Handle<PyObject> key,
                        Handle<PyObject>& out,
                        Isolate* isolate) {
  assert(!key.is_null());

  out = Handle<PyObject>::null();

  bool found = false;
  int64_t index = 0;
  ASSIGN_RETURN_ON_EXCEPTION(isolate, index,
                             FindSlot(dict, key, &found, isolate));

  if (!found) {
    return Maybe<bool>(false);
  }

  out = dict->ValueAtIndex(index, isolate);
  assert(!out.is_null());

  return Maybe<bool>(true);
}

// static
Maybe<bool> PyDict::Remove(Handle<PyDict> dict,
                           Handle<PyObject> key,
                           Isolate* isolate) {
  HandleScope scope(isolate);

  bool found = false;
  int64_t index = 0;
  ASSIGN_RETURN_ON_EXCEPTION(isolate, index,
                             FindSlot(dict, key, &found, isolate));
  if (!found) {
    return Maybe<bool>(false);
  }

  Handle<FixedArray> new_data =
      isolate->factory()->NewFixedArray(dict->capacity() << 1);
  uint64_t new_mask = dict->capacity() - 1;

  RETURN_ON_EXCEPTION(isolate,
                      RehashInto(dict, new_data, new_mask, index, isolate));

  dict->set_data(new_data);
  --dict->occupied_;
  return Maybe<bool>(true);
}

// static
Maybe<bool> PyDict::Put(Handle<PyDict> object,
                        Handle<PyObject> key,
                        Handle<PyObject> value,
                        Isolate* isolate) {
  HandleScope scope(isolate);

  assert(!key.is_null());
  assert(!value.is_null());

  auto dict = Handle<PyDict>::cast(object);
  if (dict->occupied_ + 1 > dict->capacity() * kMaxLoadFactor) {
    bool expanded = false;
    ASSIGN_RETURN_ON_EXCEPTION(isolate, expanded, ExpandImpl(dict, isolate));
  }

  bool found = false;
  int64_t index = 0;
  ASSIGN_RETURN_ON_EXCEPTION(isolate, index,
                             FindSlot(dict, key, &found, isolate));

  {
    DisallowHeapAllocation no_alloc(isolate);

    if (found) {
      SetDictVal(dict, index, value);
      return Maybe<bool>(false);
    }

    SetDictKey(dict, index, key);
    SetDictVal(dict, index, value);
    ++dict->occupied_;
  }

  return Maybe<bool>(true);
}

// static
Handle<PyTuple> PyDict::GetKeyTuple(Handle<PyDict> dict, Isolate* isolate) {
  EscapableHandleScope scope(isolate);

  int64_t out_length = dict->occupied();
  Handle<PyTuple> keys = PyTuple::New(isolate, out_length);

  int64_t out_index = 0;
  for (auto i = 0; i < dict->capacity(); ++i) {
    Handle<PyObject> key = dict->KeyAtIndex(i, isolate);
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
Maybe<bool> PyDict::ExpandImpl(Handle<PyDict> dict, Isolate* isolate) {
  HandleScope scope(isolate);

  int64_t new_capacity = dict->capacity() << 1;
  Handle<FixedArray> new_data =
      isolate->factory()->NewFixedArray(new_capacity * 2);
  uint64_t new_mask = new_capacity - 1;

  RETURN_ON_EXCEPTION(isolate,
                      RehashInto(dict, new_data, new_mask, -1, isolate));

  dict->set_data(new_data);
  return Maybe<bool>(true);
}

}  // namespace saauso::internal
