// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef SAAUSO_CODE_PYC_FILE_PARSER_H_
#define SAAUSO_CODE_PYC_FILE_PARSER_H_

#include <memory>

#include "handles/handles.h"

namespace saauso::internal {

class BinaryFileReader;
class CodeObject;
class PyList;
class PyString;

class PycFileParser {
 public:
  explicit PycFileParser(const char* filename);

  Handle<CodeObject> Parse();

 private:
  Handle<CodeObject> ParseCodeObject();

  Handle<PyString> ParseByteCodes();
  Handle<PyList> ParseConsts();
  Handle<PyList> ParseNames();
  Handle<PyList> ParseVarNames();
  Handle<PyList> ParseFreeVars();
  Handle<PyList> ParseCellVars();

  Handle<PyString> ParseFileName();
  Handle<PyString> ParseName();
  Handle<PyString> ParseNoTable();

  Handle<PyString> ParseString(bool is_long_string);
  Handle<PyList> ParseTuple();
  Handle<PyList> ParseTupleImpl();

  std::unique_ptr<BinaryFileReader> reader_;
};

}  // namespace saauso::internal

#endif  // SAAUSO_CODE_PYC_FILE_PARSER_H_
