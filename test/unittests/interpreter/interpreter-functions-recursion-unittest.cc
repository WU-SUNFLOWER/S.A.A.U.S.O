// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include <string_view>

#include "src/execution/isolate.h"
#include "src/handles/handles.h"
#include "src/heap/factory.h"
#include "src/objects/py-list.h"
#include "src/objects/py-smi.h"
#include "test/unittests/test-helpers.h"
#include "test/unittests/test-utils.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace saauso::internal {

namespace {

constexpr std::string_view kTestFileName = kInterpreterTestFileName;

}  // namespace

TEST_F(BasicInterpreterTest, FibRecursion) {
  HandleScope scope(isolate_);

  constexpr std::string_view kSource = R"(
def F(n):
    if n == 0:
        return 0
    elif n == 1:
        return 1
    else:
        return F(n - 1) + F(n - 2)

i = 0
while True:
  print(F(i))
  i += 1
  if i > 5:
    break
)";

  RunScript(kSource, kTestFileName);

  auto expected_printv_result = PyList::New(isolate_);
  AppendExpected(expected_printv_result, isolate_->factory()->NewSmiFromInt(0));
  AppendExpected(expected_printv_result, isolate_->factory()->NewSmiFromInt(1));
  AppendExpected(expected_printv_result, isolate_->factory()->NewSmiFromInt(1));
  AppendExpected(expected_printv_result, isolate_->factory()->NewSmiFromInt(2));
  AppendExpected(expected_printv_result, isolate_->factory()->NewSmiFromInt(3));
  AppendExpected(expected_printv_result, isolate_->factory()->NewSmiFromInt(5));
  ExpectPrintResult(expected_printv_result);
}

TEST_F(BasicInterpreterTest, MergeSortRecursion) {
  HandleScope scope(isolate_);

  constexpr std::string_view kSource = R"(
def merge(left, right):
    result = []
    i = 0
    j = 0
    while i < len(left):
        if j >= len(right):
            break
        if left[i] <= right[j]:
            result.append(left[i])
            i += 1
        else:
            result.append(right[j])
            j += 1
    while i < len(left):
        result.append(left[i])
        i += 1
    while j < len(right):
        result.append(right[j])
        j += 1
    return result

def merge_sort(a):
    n = len(a)
    if n <= 1:
        return a
    mid = 0
    t = n
    while t > 1:
        t -= 2
        mid += 1
    left = []
    right = []
    i = 0
    while i < mid:
        left.append(a[i])
        i += 1
    while i < n:
        right.append(a[i])
        i += 1
    return merge(merge_sort(left), merge_sort(right))

data = [7, 3, 5, 2, 9, 1, 4, 8, 6, 0]
print(merge_sort(data))
)";

  RunScript(kSource, kTestFileName);

  auto expected_printv_result = PyList::New(isolate_);
  auto expected_sorted = PyList::New(isolate_);
  PyList::Append(expected_sorted, isolate_->factory()->NewSmiFromInt(0),
                 isolate_);
  PyList::Append(expected_sorted, isolate_->factory()->NewSmiFromInt(1),
                 isolate_);
  PyList::Append(expected_sorted, isolate_->factory()->NewSmiFromInt(2),
                 isolate_);
  PyList::Append(expected_sorted, isolate_->factory()->NewSmiFromInt(3),
                 isolate_);
  PyList::Append(expected_sorted, isolate_->factory()->NewSmiFromInt(4),
                 isolate_);
  PyList::Append(expected_sorted, isolate_->factory()->NewSmiFromInt(5),
                 isolate_);
  PyList::Append(expected_sorted, isolate_->factory()->NewSmiFromInt(6),
                 isolate_);
  PyList::Append(expected_sorted, isolate_->factory()->NewSmiFromInt(7),
                 isolate_);
  PyList::Append(expected_sorted, isolate_->factory()->NewSmiFromInt(8),
                 isolate_);
  PyList::Append(expected_sorted, isolate_->factory()->NewSmiFromInt(9),
                 isolate_);
  AppendExpected(expected_printv_result, expected_sorted);
  ExpectPrintResult(expected_printv_result);
}

}  // namespace saauso::internal
