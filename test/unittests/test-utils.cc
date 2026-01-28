// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "test/unittests/test-utils.h"

#include <cstring>
#include <span>

#include "src/code/cpython312-pyc-compiler.h"
#include "src/code/cpython312-pyc-file-parser.h"
#include "src/handles/handles.h"
#include "src/objects/py-code-object.h"
#include "src/objects/py-list.h"
#include "src/objects/py-object.h"
#include "src/objects/py-oddballs.h"
#include "src/objects/py-string.h"
#include "src/runtime/isolate.h"

namespace saauso::internal {

Handle<PyObject> PyNoneObject() {
  return handle(Isolate::Current()->py_none_object());
}

Handle<PyObject> PyTrueObject() {
  return handle(Isolate::Current()->py_true_object());
}

Handle<PyObject> PyFalseObject() {
  return handle(Isolate::Current()->py_false_object());
}

::testing::AssertionResult IsPyStringEqual(Handle<PyString> s,
                                           std::string_view expected) {
  if (s.IsNull()) {
    return ::testing::AssertionFailure() << "PyString is null";
  }

  const int64_t expected_len = static_cast<int64_t>(expected.size());
  if (s->length() != expected_len) {
    return ::testing::AssertionFailure()
           << "length mismatch: expected=" << expected_len
           << " actual=" << s->length();
  }

  if (std::memcmp(s->buffer(), expected.data(), expected.size()) != 0) {
    return ::testing::AssertionFailure() << "buffer mismatch";
  }

  return ::testing::AssertionSuccess();
}

::testing::AssertionResult IsPyObjectEqual(Handle<PyObject> x,
                                           Handle<PyObject> y) {
  if (x.IsNull() || y.IsNull()) {
    return ::testing::AssertionFailure()
           << "null input: x=" << (x.IsNull() ? "null" : "non-null")
           << " y=" << (y.IsNull() ? "null" : "non-null");
  }

  auto result = PyObject::Equal(x, y);
  if (!IsPyTrue(Tagged<PyObject>(result))) {
    return ::testing::AssertionFailure() << "PyObject::Equal returned false";
  }
  return ::testing::AssertionSuccess();
}

::testing::AssertionResult IsPyTrueCondition(Handle<PyObject> condition) {
  if (condition.IsNull()) {
    return ::testing::AssertionFailure() << "condition is null";
  }
  if (!IsPyTrue(condition)) {
    return ::testing::AssertionFailure() << "condition is not PyTrue";
  }
  return ::testing::AssertionSuccess();
}

void AppendExpected(Handle<PyList> list, Handle<PyObject> value) {
  PyList::Append(list, value);
}

Handle<PyCodeObject> CompileScript312(std::string_view source,
                                      std::string_view file_name) {
  std::vector<uint8_t> pyc =
      CompilePythonSourceToPycBytes312(source, file_name);
  CPython312PycFileParser parser(
      std::span<const uint8_t>(pyc.data(), pyc.size()));
  return parser.Parse();
}

void PutInt32LE(std::vector<uint8_t>& out, int32_t v) {
  out.push_back(static_cast<uint8_t>(v & 0xff));
  out.push_back(static_cast<uint8_t>((v >> 8) & 0xff));
  out.push_back(static_cast<uint8_t>((v >> 16) & 0xff));
  out.push_back(static_cast<uint8_t>((v >> 24) & 0xff));
}

void PutByte(std::vector<uint8_t>& out, uint8_t v) {
  out.push_back(v);
}

void PutLongString(std::vector<uint8_t>& out, const std::string& s) {
  PutInt32LE(out, static_cast<int32_t>(s.size()));
  out.insert(out.end(), s.begin(), s.end());
}

}  // namespace saauso::internal
