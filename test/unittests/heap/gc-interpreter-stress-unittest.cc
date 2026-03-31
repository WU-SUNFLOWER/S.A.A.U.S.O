// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include <string_view>

#include "src/execution/isolate.h"
#include "src/heap/factory.h"
#include "src/handles/handles.h"
#include "src/objects/py-list.h"
#include "src/objects/py-smi.h"
#include "src/objects/py-string.h"
#include "test/unittests/test-helpers.h"
#include "test/unittests/test-utils.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace saauso::internal {

namespace {

constexpr std::string_view kTestFileName = kInterpreterTestFileName;

int ComputeExpectedChecksumForSysgcLoop() {
  int checksum = 0;
  int l_len = 0;

  for (int i = 0; i < 240; ++i) {
    int tmp_len = 10;
    int dt_len = 4;

    if (i < 32) {
      l_len += 1;
    } else {
      l_len = 32;
    }

    checksum += tmp_len + dt_len + l_len;
  }
  return checksum;
}

}  // namespace

TEST_F(BasicInterpreterTest, SysgcInLoopShouldNotCorruptListAndDict) {
  HandleScope scope;

  constexpr std::string_view kSource = R"(
d = {}
l = []
i = 0
checksum = 0

while i < 240:
  tmp = []
  j = 0
  while j < 10:
    tmp.append(str(i + j))
    j += 1

  dt = {}
  dt[0] = str(i)
  dt[1] = str(i + 1)
  dt[2] = str(i + 2)
  dt[3] = str(i + 3)

  if i < 32:
    l.append(str(i))
    d[i] = str(i)
  else:
    l.pop()
    l.append(str(i % 32))

  checksum += len(tmp)
  checksum += len(dt)
  checksum += len(l)

  if (i % 3) == 0:
    sysgc()
  i += 1

print(len(l))
print(d[0])
print(d[31])
print(checksum)
)";

  RunScript(kSource, kTestFileName);

  auto expected_printv_result = PyList::New(isolate_);
  AppendExpected(expected_printv_result, isolate_->factory()->NewSmiFromInt(32));
  AppendExpected(expected_printv_result, PyString::New(isolate_, "0"));
  AppendExpected(expected_printv_result, PyString::New(isolate_, "31"));
  AppendExpected(expected_printv_result,
                isolate_->factory()->NewSmiFromInt(
                    ComputeExpectedChecksumForSysgcLoop()));
  ExpectPrintResult(expected_printv_result);
}

TEST_F(BasicInterpreterTest, SysgcDuringNestedCallsShouldNotCrashOrHang) {
  HandleScope scope;

  constexpr std::string_view kSource = R"(
def work(n, acc):
  i = 0
  while i < 60:
    tmp = []
    j = 0
    while j < 12:
      tmp.append(str(i + j))
      j += 1
    if (i % 4) == 0:
      sysgc()
    acc += len(tmp)
    i += 1
  if n == 0:
    return acc
  return work(n - 1, acc)

print(work(5, 0))
)";

  RunScript(kSource, kTestFileName);

  auto expected_printv_result = PyList::New(isolate_);
  AppendExpected(expected_printv_result,
                 isolate_->factory()->NewSmiFromInt(6 * 60 * 12));
  ExpectPrintResult(expected_printv_result);
}

TEST_F(BasicInterpreterTest, SysgcInsideDictIterationShouldNotBreakViews) {
  HandleScope scope;

  constexpr std::string_view kSource = R"(
d = {"x": 1, "y": 2, "z": 3}
s = 0
cnt = 0
for v in d.values():
  t = []
  t.append(str(v))
  t.append(str(v + 1))
  sysgc()
  s += v
  cnt += 1
print(s)
print(cnt)
)";

  RunScript(kSource, kTestFileName);

  auto expected_printv_result = PyList::New(isolate_);
  AppendExpected(expected_printv_result, isolate_->factory()->NewSmiFromInt(6));
  AppendExpected(expected_printv_result, isolate_->factory()->NewSmiFromInt(3));
  ExpectPrintResult(expected_printv_result);
}

}  // namespace saauso::internal
