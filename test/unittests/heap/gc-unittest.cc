// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include <cstring>
#include <string>

#include "gtest/gtest.h"
#include "src/execution/isolate.h"
#include "src/handles/handle-scope-implementer.h"
#include "src/handles/handles.h"
#include "src/heap/heap.h"
#include "src/objects/py-dict.h"
#include "src/objects/py-float.h"
#include "src/objects/py-list.h"
#include "src/objects/py-object.h"
#include "src/objects/py-oddballs.h"
#include "src/objects/py-smi.h"
#include "src/objects/py-string.h"
#include "test/unittests/test-helpers.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace saauso::internal {

#define EXPECT_PY_TRUE(condition) EXPECT_TRUE(IsPyTrue(condition))

namespace {

void AllocateEphemeralStrings(int count) {
  for (int i = 0; i < count; ++i) {
    HandleScope inner_scope;
    auto s = std::string("ephemeral-").append(std::to_string(i));
    (void)PyString::NewInstance(s.c_str());
  }
}

void AllocateEphemeralListsWithStrings(Isolate* isolate,
                                       int list_count,
                                       int element_count) {
  for (int i = 0; i < list_count; ++i) {
    HandleScope inner_scope;
    auto list = PyList::New(isolate);
    for (int j = 0; j < element_count; ++j) {
      HandleScope elem_scope;
      auto s = std::string("l")
                   .append(std::to_string(i))
                   .append("-e")
                   .append(std::to_string(j));
      PyList::Append(list, PyString::NewInstance(s.c_str()));
    }
  }
}

}  // namespace

class GcTest : public VmTestBase {};

// 测试针对字符串的copy gc
TEST_F(GcTest, CopyGcTestForPyString) {
  HandleScope scope;

  char content[] = "Hello World";

  Handle<PyString> str1 = PyString::NewInstance(content);
  Address a1 = (*str1).ptr();
  isolate_->heap()->CollectGarbage();
  Handle<PyString> str2 = PyString::NewInstance(content);
  Address a2 = (*str1).ptr();

  Handle<PyObject> eq_res;
  ASSERT_TRUE(PyObject::Equal(isolate_, str1, str2).ToHandle(&eq_res));
  EXPECT_TRUE(IsPyTrue(*eq_res));

  EXPECT_EQ(std::strncmp(str1->buffer(), content, str1->length()), 0);

  EXPECT_NE(a1, a2);
}

// 测试针对list的copy gc
TEST_F(GcTest, CopyGcTestForPyList) {
  HandleScope scope;
  constexpr int kLength = 5;

  //////////////////////////////////////////

  Handle<PyList> list1 = PyList::New(isolate_, kLength);
  Address a1 = (*list1).ptr();

  for (auto i = 0; i < kLength; ++i) {
    HandleScope inner_scope;
    Handle<PyString> elem = PyString::NewInstance(std::to_string(i).c_str());
    PyList::Append(list1, elem);
  }

  //////////////////////////////////////////

  isolate_->heap()->CollectGarbage();

  //////////////////////////////////////////

  Handle<PyList> list2 = PyList::New(isolate_, kLength);

  for (auto i = 0; i < kLength; ++i) {
    HandleScope inner_scope;
    Handle<PyString> elem = PyString::NewInstance(std::to_string(i).c_str());
    PyList::Append(list2, elem);
  }

  //////////////////////////////////////////

  EXPECT_NE(a1, (*list1).ptr());
  bool eq = false;
  ASSERT_TRUE(PyObject::EqualBool(isolate_, list1, list2).To(&eq));
  EXPECT_TRUE(eq);
}

TEST_F(GcTest, CopyGcTestForForwardingPointer) {
  HandleScope scope;

  Handle<PyList> list1 = PyList::New(isolate_);
  Handle<PyList> list2 = PyList::New(isolate_);

  Handle<PyString> content = PyString::NewInstance("Hello World");
  PyList::Append(list1, content);
  PyList::Append(list2, content);

  isolate_->heap()->CollectGarbage();

  Handle<PyObject> eq_res;
  ASSERT_TRUE(PyObject::Equal(isolate_, list1, list2).ToHandle(&eq_res));
  EXPECT_PY_TRUE(*eq_res);
  ASSERT_TRUE(PyObject::Equal(isolate_, list1->Get(0), list2->Get(0))
                  .ToHandle(&eq_res));
  EXPECT_PY_TRUE(*eq_res);
  ASSERT_TRUE(
      PyObject::Equal(isolate_, list1->Get(0), content).ToHandle(&eq_res));
  EXPECT_PY_TRUE(*eq_res);
  ASSERT_TRUE(
      PyObject::Equal(isolate_, list2->Get(0), content).ToHandle(&eq_res));
  EXPECT_PY_TRUE(*eq_res);
}

TEST_F(GcTest, CopyGcTestForPyDict) {
  HandleScope scope;

  Handle<PyDict> dict = PyDict::NewInstance();
  Address dict_addr_before = (*dict).ptr();

  constexpr int kCount = 32;
  for (int i = 0; i < kCount; ++i) {
    HandleScope inner_scope;
    Handle<PyObject> key = PyString::NewInstance(
        std::string("k").append(std::to_string(i)).c_str());
    Handle<PyObject> value = PyString::NewInstance(
        std::string("v").append(std::to_string(i)).c_str());
    ASSERT_FALSE(PyDict::Put(dict, key, value).IsNothing());
  }

  isolate_->heap()->CollectGarbage();

  EXPECT_NE(dict_addr_before, (*dict).ptr());
  EXPECT_EQ(dict->occupied(), kCount);

  for (int i = 0; i < kCount; ++i) {
    HandleScope inner_scope;
    Handle<PyObject> query_key = PyString::NewInstance(
        std::string("k").append(std::to_string(i)).c_str());
    Handle<PyObject> expected_value = PyString::NewInstance(
        std::string("v").append(std::to_string(i)).c_str());

    Tagged<PyObject> actual_value_tagged;
    bool found = false;
    ASSERT_TRUE(dict->GetTagged(query_key, actual_value_tagged).To(&found));
    ASSERT_TRUE(found);
    auto actual_value = handle(actual_value_tagged);
    ASSERT_FALSE(actual_value.is_null());
    Handle<PyObject> eq_res;
    ASSERT_TRUE(PyObject::Equal(isolate_, actual_value, expected_value)
                    .ToHandle(&eq_res));
    EXPECT_PY_TRUE(*eq_res);
  }
}

TEST_F(GcTest, MetaSingletonShouldNotMoveInMinorGc) {
  HandleScope scope;

  // 关键点：
  // - None/True/False 属于 VM 的全局单例，分配在 meta space 上
  // - 新生代 GC（Scavenge）只搬迁 eden -> survivor 的对象
  //   因此 meta space 的对象地址必须保持稳定
  Address none_addr_before = isolate_->py_none_object().ptr();
  Address true_addr_before = isolate_->py_true_object().ptr();
  Address false_addr_before = isolate_->py_false_object().ptr();

  AllocateEphemeralStrings(2000);
  isolate_->heap()->CollectGarbage();
  AllocateEphemeralListsWithStrings(isolate_, 200, 8);
  isolate_->heap()->CollectGarbage();

  EXPECT_EQ(none_addr_before, isolate_->py_none_object().ptr());
  EXPECT_EQ(true_addr_before, isolate_->py_true_object().ptr());
  EXPECT_EQ(false_addr_before, isolate_->py_false_object().ptr());

  EXPECT_TRUE(IsPyTrue(Isolate::ToPyBoolean(true)));
  EXPECT_TRUE(IsPyFalse(Isolate::ToPyBoolean(false)));
}

TEST_F(GcTest, CopyGcShouldPreserveDeepObjectGraph) {
  HandleScope scope;

  // 这个用例主要验证“遍历约定（iterate）”是否正确：
  // dict -> fixed_array (entries) -> (key/value) -> list -> fixed_array ->
  // string
  Handle<PyDict> dict = PyDict::NewInstance();
  Handle<PyObject> key = PyString::NewInstance("payload");

  Handle<PyList> list = PyList::New(isolate_);
  constexpr int kCount = 64;
  for (int i = 0; i < kCount; ++i) {
    HandleScope inner_scope;
    PyList::Append(list, PyString::NewInstance(std::to_string(i).c_str()));
  }
  ASSERT_FALSE(PyDict::Put(dict, key, Handle<PyObject>(list)).IsNothing());

  Address dict_addr_before = (*dict).ptr();
  Address list_addr_before = (*list).ptr();

  AllocateEphemeralListsWithStrings(isolate_, 120, 6);
  isolate_->heap()->CollectGarbage();

  // dict/list 作为 GC ROOT（被 Handle 持有）应当在 minor GC 后存活且被搬迁。
  EXPECT_NE(dict_addr_before, (*dict).ptr());
  EXPECT_NE(list_addr_before, (*list).ptr());

  Tagged<PyObject> payload_tagged;
  bool found = false;
  ASSERT_TRUE(dict->GetTagged(key, payload_tagged).To(&found));
  ASSERT_TRUE(found);
  auto payload = handle(payload_tagged);
  ASSERT_FALSE(payload.is_null());
  auto payload_list = Handle<PyList>::cast(payload);
  EXPECT_EQ(payload_list->length(), kCount);

  for (int i = 0; i < kCount; ++i) {
    HandleScope inner_scope;
    auto expected = PyString::NewInstance(std::to_string(i).c_str());
    Handle<PyObject> eq_res;
    ASSERT_TRUE(PyObject::Equal(isolate_, payload_list->Get(i), expected)
                    .ToHandle(&eq_res));
    EXPECT_PY_TRUE(*eq_res);
  }
}

TEST_F(GcTest, EscapedHandleShouldSurviveAcrossGc) {
  HandleScope scope;

  // 关键点：
  // - Escape 会把 inner scope 中的 handle “提升”到 outer scope
  // - 在 minor GC 中，handle slot 会被更新为新地址
  auto create_escaped_string = []() -> Handle<PyString> {
    EscapableHandleScope inner_scope;
    auto s = PyString::NewInstance("escaped-string");
    return inner_scope.Escape(s);
  };

  Handle<PyString> escaped = create_escaped_string();
  Address addr_before = (*escaped).ptr();

  AllocateEphemeralStrings(4000);
  isolate_->heap()->CollectGarbage();

  EXPECT_NE(addr_before, (*escaped).ptr());
  EXPECT_EQ(
      std::strncmp(escaped->buffer(), "escaped-string", escaped->length()), 0);
}

TEST_F(GcTest, CopyGcShouldHandleSelfReferenceInContainer) {
  HandleScope scope;

  // 这个用例验证“环形引用”在 Scavenge 下不会造成错误：
  // list[0] 指向 list 本身，GC 需要正确更新该内部指针为新地址。
  Handle<PyList> list = PyList::New(isolate_);
  PyList::Append(list, Handle<PyObject>(list));
  PyList::Append(list, PyString::NewInstance("tail"));

  Address before = (*list).ptr();
  isolate_->heap()->CollectGarbage();

  EXPECT_NE(before, (*list).ptr());
  EXPECT_EQ(list->length(), 2);

  auto elem0 = list->Get(0);
  ASSERT_FALSE(elem0.is_null());
  EXPECT_EQ((*list).ptr(), (*elem0).ptr());
}

TEST_F(GcTest, CopyGcShouldNotCorruptSmiValues) {
  HandleScope scope;

  // 关键点：
  // - Smi 不是真正的堆对象，不应该被 Scavenge 搬迁
  // - 同时它作为 list 元素被存储在 fixed array 中，需要保证读取解码正确
  Handle<PyList> list = PyList::New(isolate_);
  constexpr int kCount = 50;
  for (int i = 0; i < kCount; ++i) {
    HandleScope inner_scope;
    Handle<PySmi> smi(PySmi::FromInt(i));
    PyList::Append(list, Handle<PyObject>(smi));
  }

  isolate_->heap()->CollectGarbage();

  for (int i = 0; i < kCount; ++i) {
    HandleScope inner_scope;
    auto v = list->Get(i);
    ASSERT_TRUE(IsPySmi(v));
    EXPECT_EQ(PySmi::ToInt(Handle<PySmi>::cast(v)), i);
  }
}

}  // namespace saauso::internal
