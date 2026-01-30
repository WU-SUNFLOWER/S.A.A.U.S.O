// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef TEST_UNITTESTS_TEST_UTILS_H_
#define TEST_UNITTESTS_TEST_UTILS_H_

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include "testing/gtest/include/gtest/gtest.h"

namespace saauso::internal {

template <typename T>
class Handle;

class Isolate;
class PyCodeObject;
class PyList;
class PyObject;
class PyString;

Handle<PyObject> PyNoneObject();
Handle<PyObject> PyTrueObject();
Handle<PyObject> PyFalseObject();

::testing::AssertionResult IsPyStringEqual(Handle<PyString> s,
                                           std::string_view expected);

::testing::AssertionResult IsPyObjectEqual(Handle<PyObject> x,
                                           Handle<PyObject> y);

::testing::AssertionResult IsPyTrueCondition(Handle<PyObject> condition);

void AppendExpected(Handle<PyList> list, Handle<PyObject> value);

Handle<PyCodeObject> CompileScript312(Isolate* isolate,
                                      std::string_view source,
                                      std::string_view file_name);

void PutInt32LE(std::vector<uint8_t>& out, int32_t v);
void PutByte(std::vector<uint8_t>& out, uint8_t v);
void PutLongString(std::vector<uint8_t>& out, const std::string& s);

}  // namespace saauso::internal

#endif  // TEST_UNITTESTS_TEST_UTILS_H_
