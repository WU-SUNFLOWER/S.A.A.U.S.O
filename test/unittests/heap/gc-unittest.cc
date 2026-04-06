// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include <cstring>
#include <string>

#include "src/execution/isolate.h"
#include "src/handles/handle-scope-implementer.h"
#include "src/handles/handles.h"
#include "src/heap/factory.h"
#include "src/heap/spaces.h"
#include "src/objects/py-dict.h"
#include "src/objects/py-float.h"
#include "src/objects/py-list.h"
#include "src/objects/py-object.h"
#include "src/objects/py-oddballs.h"
#include "src/objects/py-smi.h"
#include "src/runtime/string-table.h"
#include "test/unittests/test-helpers.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace saauso::internal {

#define EXPECT_PY_TRUE(condition) EXPECT_TRUE(IsPyTrue(condition, isolate_))

namespace {

void AllocateEphemeralStrings(Isolate* isolate, int count) {
  for (int i = 0; i < count; ++i) {
    HandleScope inner_scope(isolate);
    auto s = std::string("ephemeral-").append(std::to_string(i));
    (void)PyString::New(isolate, s.c_str());
  }
}

void AllocateEphemeralListsWithStrings(Isolate* isolate,
                                       int list_count,
                                       int element_count) {
  for (int i = 0; i < list_count; ++i) {
    HandleScope inner_scope(isolate);
    auto list = PyList::New(isolate);
    for (int j = 0; j < element_count; ++j) {
      HandleScope elem_scope(isolate);
      auto s = std::string("l")
                   .append(std::to_string(i))
                   .append("-e")
                   .append(std::to_string(j));
      PyList::Append(list, PyString::New(isolate, s.c_str()), isolate);
    }
  }
}

}  // namespace

class GcTest : public VmTestBase {};

// 测试针对字符串的copy gc
TEST_F(GcTest, CopyGcTestForPyString) {
  HandleScope scope(isolate_);

  char content[] = "Hello World";

  Handle<PyString> str1 = PyString::New(isolate_, content);
  Address a1 = (*str1).ptr();
  isolate_->heap()->CollectGarbage();
  Handle<PyString> str2 = PyString::New(isolate_, content);
  Address a2 = (*str1).ptr();

  Handle<PyObject> eq_res;
  ASSERT_TRUE(PyObject::Equal(isolate_, str1, str2).ToHandle(&eq_res));
  EXPECT_TRUE(IsPyTrue(eq_res, isolate_));

  EXPECT_EQ(std::strncmp(str1->buffer(), content, str1->length()), 0);

  EXPECT_NE(a1, a2);
}

// 测试针对list的copy gc
TEST_F(GcTest, CopyGcTestForPyList) {
  HandleScope scope(isolate_);
  constexpr int kLength = 5;

  //////////////////////////////////////////

  Handle<PyList> list1 = PyList::New(isolate_, kLength);
  Address a1 = (*list1).ptr();

  for (auto i = 0; i < kLength; ++i) {
    HandleScope inner_scope(isolate_);
    Handle<PyString> elem = PyString::New(isolate_, std::to_string(i).c_str());
    PyList::Append(list1, elem, isolate_);
  }

  //////////////////////////////////////////

  isolate_->heap()->CollectGarbage();

  //////////////////////////////////////////

  Handle<PyList> list2 = PyList::New(isolate_, kLength);

  for (auto i = 0; i < kLength; ++i) {
    HandleScope inner_scope(isolate_);
    Handle<PyString> elem = PyString::New(isolate_, std::to_string(i).c_str());
    PyList::Append(list2, elem, isolate_);
  }

  //////////////////////////////////////////

  EXPECT_NE(a1, (*list1).ptr());
  bool eq = false;
  ASSERT_TRUE(PyObject::EqualBool(isolate_, list1, list2).To(&eq));
  EXPECT_TRUE(eq);
}

TEST_F(GcTest, CopyGcTestForForwardingPointer) {
  HandleScope scope(isolate_);

  Handle<PyList> list1 = PyList::New(isolate_);
  Handle<PyList> list2 = PyList::New(isolate_);

  Handle<PyString> content = PyString::New(isolate_, "Hello World");
  PyList::Append(list1, content, isolate_);
  PyList::Append(list2, content, isolate_);

  isolate_->heap()->CollectGarbage();

  Handle<PyObject> eq_res;
  ASSERT_TRUE(PyObject::Equal(isolate_, list1, list2).ToHandle(&eq_res));
  EXPECT_PY_TRUE(*eq_res);
  ASSERT_TRUE(PyObject::Equal(isolate_, list1->Get(0, isolate_),
                              list2->Get(0, isolate_))
                  .ToHandle(&eq_res));
  EXPECT_PY_TRUE(*eq_res);
  ASSERT_TRUE(PyObject::Equal(isolate_, list1->Get(0, isolate_), content)
                  .ToHandle(&eq_res));
  EXPECT_PY_TRUE(*eq_res);
  ASSERT_TRUE(PyObject::Equal(isolate_, list2->Get(0, isolate_), content)
                  .ToHandle(&eq_res));
  EXPECT_PY_TRUE(*eq_res);
}

TEST_F(GcTest, CopyGcTestForPyDict) {
  HandleScope scope(isolate_);

  Handle<PyDict> dict = PyDict::New(isolate_);
  Address dict_addr_before = (*dict).ptr();

  constexpr int kCount = 32;
  for (int i = 0; i < kCount; ++i) {
    HandleScope inner_scope(isolate_);
    Handle<PyObject> key = PyString::New(
        isolate_, std::string("k").append(std::to_string(i)).c_str());
    Handle<PyObject> value = PyString::New(
        isolate_, std::string("v").append(std::to_string(i)).c_str());
    ASSERT_FALSE(PyDict::Put(dict, key, value, isolate_).IsNothing());
  }

  isolate_->heap()->CollectGarbage();

  EXPECT_NE(dict_addr_before, (*dict).ptr());
  EXPECT_EQ(dict->occupied(), kCount);

  for (int i = 0; i < kCount; ++i) {
    HandleScope inner_scope(isolate_);
    Handle<PyObject> query_key = PyString::New(
        isolate_, std::string("k").append(std::to_string(i)).c_str());
    Handle<PyObject> expected_value = PyString::New(
        isolate_, std::string("v").append(std::to_string(i)).c_str());

    Handle<PyObject> actual_value;
    bool found = false;
    ASSERT_TRUE(
        PyDict::Get(dict, query_key, actual_value, isolate_).To(&found));
    ASSERT_TRUE(found);
    ASSERT_FALSE(actual_value.is_null());
    Handle<PyObject> eq_res;
    ASSERT_TRUE(PyObject::Equal(isolate_, actual_value, expected_value)
                    .ToHandle(&eq_res));
    EXPECT_PY_TRUE(*eq_res);
  }
}

TEST_F(GcTest, MetaSingletonShouldNotMoveInMinorGc) {
  HandleScope scope(isolate_);

  // 关键点：
  // - None/True/False 属于 VM 的全局单例，分配在 meta space 上
  // - 新生代 GC（Scavenge）只搬迁 eden -> survivor 的对象
  //   因此 meta space 的对象地址必须保持稳定
  Address none_addr_before = isolate_->py_none_object().ptr();
  Address true_addr_before = isolate_->py_true_object().ptr();
  Address false_addr_before = isolate_->py_false_object().ptr();

  AllocateEphemeralStrings(isolate_, 2000);
  isolate_->heap()->CollectGarbage();
  AllocateEphemeralListsWithStrings(isolate_, 200, 8);
  isolate_->heap()->CollectGarbage();

  EXPECT_EQ(none_addr_before, isolate_->py_none_object().ptr());
  EXPECT_EQ(true_addr_before, isolate_->py_true_object().ptr());
  EXPECT_EQ(false_addr_before, isolate_->py_false_object().ptr());

  EXPECT_TRUE(IsPyTrue(isolate_->factory()->ToPyBoolean(true), isolate_));
  EXPECT_TRUE(IsPyFalse(isolate_->factory()->ToPyBoolean(false), isolate_));
}

TEST_F(GcTest, PagedHeapMetadataShouldBeClosedForYoungAndMetaSpaces) {
  HandleScope scope(isolate_);

  NewSpace& new_space = isolate_->heap()->new_space();
  MetaSpace& meta_space = isolate_->heap()->meta_space();

  ASSERT_NE(new_space.eden_first_page(), nullptr);
  ASSERT_NE(new_space.survivor_first_page(), nullptr);
  EXPECT_TRUE(new_space.eden_first_page()->HasFlag(BasePage::Flag::kEdenPage));
  EXPECT_TRUE(
      new_space.survivor_first_page()->HasFlag(BasePage::Flag::kSurvivorPage));
  EXPECT_EQ(new_space.eden_first_page()->owner(), &new_space);
  EXPECT_EQ(new_space.survivor_first_page()->owner(), &new_space);

  BasePage* none_page =
      BasePage::FromAddress(isolate_->py_none_object().ptr());
  EXPECT_TRUE(none_page->HasFlag(BasePage::Flag::kMetaPage));
  EXPECT_EQ(none_page->owner(), &meta_space);
  EXPECT_TRUE(meta_space.Contains(isolate_->py_none_object().ptr()));
}

TEST_F(GcTest, StringTableEntriesStayReachableAcrossGc) {
  HandleScope scope(isolate_);

  Handle<PyString> before = ST(builtins, isolate_);
  Address before_addr = (*before).ptr();
  EXPECT_TRUE(isolate_->heap()->meta_space().Contains(before_addr));

  AllocateEphemeralStrings(isolate_, 2000);
  isolate_->heap()->CollectGarbage();
  AllocateEphemeralListsWithStrings(isolate_, 200, 8);
  isolate_->heap()->CollectGarbage();

  Handle<PyString> after = ST(builtins, isolate_);
  EXPECT_EQ(before_addr, (*after).ptr());
  EXPECT_TRUE(isolate_->heap()->meta_space().Contains((*after).ptr()));
  EXPECT_EQ(std::strncmp(after->buffer(), "__builtins__", after->length()), 0);
}

TEST_F(GcTest, CopyGcShouldScanAcrossMultipleSurvivorPages) {
  HandleScope scope(isolate_);

  Handle<PyList> root = PyList::New(isolate_);
  constexpr int kCount = 5000;
  for (int i = 0; i < kCount; ++i) {
    HandleScope inner_scope(isolate_);
    Handle<PyList> child = PyList::New(isolate_);
    PyList::Append(child, PyString::New(isolate_, std::to_string(i).c_str()),
                   isolate_);
    PyList::Append(root, Handle<PyObject>(child), isolate_);
  }

  isolate_->heap()->CollectGarbage();

  EXPECT_EQ(root->length(), kCount);
  for (int index : {0, 1024, 2048, 4096, kCount - 1}) {
    HandleScope inner_scope(isolate_);
    auto child = Handle<PyList>::cast(root->Get(index, isolate_));
    ASSERT_EQ(child->length(), 1);
    Handle<PyObject> eq_res;
    ASSERT_TRUE(PyObject::Equal(
                    isolate_, child->Get(0, isolate_),
                    PyString::New(isolate_, std::to_string(index).c_str()))
                    .ToHandle(&eq_res));
    EXPECT_PY_TRUE(*eq_res);
  }
}

TEST_F(GcTest, CopyGcShouldPreserveDeepObjectGraph) {
  HandleScope scope(isolate_);

  // 这个用例主要验证“遍历约定（iterate）”是否正确：
  // dict -> fixed_array (entries) -> (key/value) -> list -> fixed_array ->
  // string
  Handle<PyDict> dict = PyDict::New(isolate_);
  Handle<PyObject> key = PyString::New(isolate_, "payload");

  Handle<PyList> list = PyList::New(isolate_);
  constexpr int kCount = 64;
  for (int i = 0; i < kCount; ++i) {
    HandleScope inner_scope(isolate_);
    PyList::Append(list, PyString::New(isolate_, std::to_string(i).c_str()),
                   isolate_);
  }
  ASSERT_FALSE(
      PyDict::Put(dict, key, Handle<PyObject>(list), isolate_).IsNothing());

  Address dict_addr_before = (*dict).ptr();
  Address list_addr_before = (*list).ptr();

  AllocateEphemeralListsWithStrings(isolate_, 120, 6);
  isolate_->heap()->CollectGarbage();

  // dict/list 作为 GC ROOT（被 Handle 持有）应当在 minor GC 后存活且被搬迁。
  EXPECT_NE(dict_addr_before, (*dict).ptr());
  EXPECT_NE(list_addr_before, (*list).ptr());

  Handle<PyObject> payload;
  bool found = false;
  ASSERT_TRUE(PyDict::Get(dict, key, payload, isolate_).To(&found));
  ASSERT_TRUE(found);
  ASSERT_FALSE(payload.is_null());
  auto payload_list = Handle<PyList>::cast(payload);
  EXPECT_EQ(payload_list->length(), kCount);

  for (int i = 0; i < kCount; ++i) {
    HandleScope inner_scope(isolate_);
    auto expected = PyString::New(isolate_, std::to_string(i).c_str());
    Handle<PyObject> eq_res;
    ASSERT_TRUE(
        PyObject::Equal(isolate_, payload_list->Get(i, isolate_), expected)
            .ToHandle(&eq_res));
    EXPECT_PY_TRUE(*eq_res);
  }
}

TEST_F(GcTest, EscapedHandleShouldSurviveAcrossGc) {
  HandleScope scope(isolate_);

  // 关键点：
  // - Escape 会把 inner scope 中的 handle “提升”到 outer scope
  // - 在 minor GC 中，handle slot 会被更新为新地址
  auto create_escaped_string = []() -> Handle<PyString> {
    EscapableHandleScope inner_scope(isolate_);
    auto s = PyString::New(isolate_, "escaped-string");
    return inner_scope.Escape(s);
  };

  Handle<PyString> escaped = create_escaped_string();
  Address addr_before = (*escaped).ptr();

  AllocateEphemeralStrings(isolate_, 4000);
  isolate_->heap()->CollectGarbage();

  EXPECT_NE(addr_before, (*escaped).ptr());
  EXPECT_EQ(
      std::strncmp(escaped->buffer(), "escaped-string", escaped->length()), 0);
}

TEST_F(GcTest, CopyGcShouldHandleSelfReferenceInContainer) {
  HandleScope scope(isolate_);

  // 这个用例验证“环形引用”在 Scavenge 下不会造成错误：
  // list[0] 指向 list 本身，GC 需要正确更新该内部指针为新地址。
  Handle<PyList> list = PyList::New(isolate_);
  PyList::Append(list, Handle<PyObject>(list), isolate_);
  PyList::Append(list, PyString::New(isolate_, "tail"), isolate_);

  Address before = (*list).ptr();
  isolate_->heap()->CollectGarbage();

  EXPECT_NE(before, (*list).ptr());
  EXPECT_EQ(list->length(), 2);

  Tagged<PyObject> elem0 = list->GetTagged(0);
  ASSERT_FALSE(elem0.is_null());
  EXPECT_EQ((*list).ptr(), elem0.ptr());
}

TEST_F(GcTest, CopyGcShouldNotCorruptSmiValues) {
  HandleScope scope(isolate_);

  // 关键点：
  // - Smi 不是真正的堆对象，不应该被 Scavenge 搬迁
  // - 同时它作为 list 元素被存储在 fixed array 中，需要保证读取解码正确
  Handle<PyList> list = PyList::New(isolate_);
  constexpr int kCount = 50;
  for (int i = 0; i < kCount; ++i) {
    HandleScope inner_scope(isolate_);
    Handle<PySmi> smi(PySmi::FromInt(i), isolate_);
    PyList::Append(list, Handle<PyObject>(smi), isolate_);
  }

  isolate_->heap()->CollectGarbage();

  for (int i = 0; i < kCount; ++i) {
    HandleScope inner_scope(isolate_);
    auto v = list->GetTagged(i);
    ASSERT_TRUE(IsPySmi(v));
    EXPECT_EQ(PySmi::ToInt(Tagged<PySmi>::cast(v)), i);
  }
}

}  // namespace saauso::internal
