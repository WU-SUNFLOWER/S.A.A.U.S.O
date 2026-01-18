// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef SAAUSO_CODE_CPYTHON312_PYC_FILE_PARSER_H_
#define SAAUSO_CODE_CPYTHON312_PYC_FILE_PARSER_H_

#include <cstdint>
#include <memory>
#include <span>

#include "src/handles/handles.h"

namespace saauso::internal {

class BinaryFileReader;
class PyCodeObject;
class PyList;
class PyObject;
class PyString;

class CPython312PycFileParser {
 public:
  explicit CPython312PycFileParser(const char* filename);
  explicit CPython312PycFileParser(std::span<const uint8_t> bytes);
  ~CPython312PycFileParser();

  Handle<PyCodeObject> Parse();

 private:
  Handle<PyObject> ParseObject(Handle<PyList> string_table, Handle<PyList> cache);
  Handle<PyCodeObject> ParseCodeObject(Handle<PyList> string_table,
                                       Handle<PyList> cache);

  Handle<PyString> ParseStringLong();
  Handle<PyString> ParseStringShort();
  Handle<PyString> ParseString(bool is_long_string);

  int ReadInt32();
  uint16_t ReadUInt16();

  std::unique_ptr<BinaryFileReader> reader_;
};

}  // namespace saauso::internal

#endif  // SAAUSO_CODE_CPYTHON312_PYC_FILE_PARSER_H_

