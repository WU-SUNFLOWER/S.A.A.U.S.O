// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef SAAUSO_CODE_PYC_FILE_PARSER_H_
#define SAAUSO_CODE_PYC_FILE_PARSER_H_

#include <cstdint>
#include <memory>
#include <span>

#include "src/handles/handles.h"

namespace saauso::internal {

class BinaryFileReader;
class PyCodeObject;
class PyList;
class PyString;

class PycFileParser {
 public:
  explicit PycFileParser(const char* filename);
  explicit PycFileParser(std::span<const uint8_t> bytes);
  ~PycFileParser();

  Handle<PyCodeObject> Parse();

 private:
  Handle<PyCodeObject> ParseCodeObject(Handle<PyList> string_table,
                                       Handle<PyList> cache);

  Handle<PyString> ParseByteCodes(Handle<PyList> string_table,
                                  Handle<PyList> cache);
  Handle<PyList> ParseConsts(Handle<PyList> string_table, Handle<PyList> cache);
  Handle<PyList> ParseNames(Handle<PyList> string_table, Handle<PyList> cache);
  Handle<PyList> ParseVarNames(Handle<PyList> string_table,
                               Handle<PyList> cache);
  Handle<PyList> ParseFreeVars(Handle<PyList> string_table,
                               Handle<PyList> cache);
  Handle<PyList> ParseCellVars(Handle<PyList> string_table,
                               Handle<PyList> cache);

  Handle<PyString> ParseFileName(Handle<PyList> string_table,
                                 Handle<PyList> cache);
  Handle<PyString> ParseName(Handle<PyList> string_table, Handle<PyList> cache);
  Handle<PyString> ParseNoTable(Handle<PyList> string_table,
                                Handle<PyList> cache);

  Handle<PyString> ParseString(bool is_long_string);
  Handle<PyList> ParseTuple(Handle<PyList> string_table, Handle<PyList> cache);
  Handle<PyList> ParseTupleImpl(Handle<PyList> string_table,
                                Handle<PyList> cache);

  std::unique_ptr<BinaryFileReader> reader_;
};

}  // namespace saauso::internal

#endif  // SAAUSO_CODE_PYC_FILE_PARSER_H_
