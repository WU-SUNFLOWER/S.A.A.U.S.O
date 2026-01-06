// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "code/pyc-file-parser.h"

#include <cassert>

#include "code/binary-file-reader.h"
#include "handles/handles.h"
#include "objects/code-object.h"
#include "objects/py-double.h"
#include "objects/py-integer.h"
#include "objects/py-list.h"
#include "objects/py-object.h"
#include "objects/py-string.h"
#include "runtime/universe.h"

namespace saauso::internal {

namespace {

constexpr char kRefFlagMask = 0x80;
constexpr char kObjectTypeMask = 0x7f;

constexpr char kStringFlag = 's';
constexpr char kInternedStringFlag = 't';
constexpr char kShortAsciiStringFlag = 'z';
constexpr char kShortAsciiInternedStringFlag = 'Z';

constexpr char kIntegerFlag = 'i';
constexpr char kDoubleFlag = 'g';
constexpr char kCodeObjectFlag = 'c';
constexpr char kTupleFlag = ')';

constexpr char kNoneObjectFlag = 'N';
constexpr char kTrueObjectFlag = 'T';
constexpr char kFalseObjectFlag = 'F';

constexpr char kInCacheObjectFlag = 'r';
constexpr char kInStringTableObjectFlag = 'R';

#define HAS_REF_FLAG(object_type) (((object_type) & kRefFlagMask) != 0)

#define GET_OBJECT_TYPE_CHAR(object_type) ((object_type) & kObjectTypeMask)

}  // namespace

PycFileParser::PycFileParser(const char* filename)
    : reader_(std::make_unique<BinaryFileReader>(filename)) {}

Handle<CodeObject> PycFileParser::Parse() {
  HandleScope scope;

  int magic_number [[maybe_unused]] = reader_->ReadInt32();
  int file_flag [[maybe_unused]] = reader_->ReadInt32();
  int timestamp [[maybe_unused]] = reader_->ReadInt32();
  int file_size [[maybe_unused]] = reader_->ReadInt32();

#ifdef _DEBUG
  printf("magic number is 0x%x\n", magic_number);
  printf("timestamp is %d\n", timestamp);
  printf("file size is %d\n", file_size);
#endif  // _DEBUG

  char object_type = reader_->ReadByte();
  bool ref_flag = (object_type & 0x80) != 0;
  object_type &= 0x7f;

  if (object_type != 'c') {
    return Handle<CodeObject>::Null();
  }

  Handle<PyList> string_table = PyList::NewInstance();
  Handle<PyList> cache = PyList::NewInstance();

  int index = 0;
  if (ref_flag) {
    // 先在cache中占个位
    index = cache->length();
    PyList::Append(cache, Handle<PyObject>::Null());
  }

  Handle<CodeObject> code_object = ParseCodeObject(string_table, cache);

  if (ref_flag) {
    // 对象构造完毕，缓存到cache
    cache->Set(index, code_object);
  }

  return code_object.EscapeFrom(&scope);
}

Handle<CodeObject> PycFileParser::ParseCodeObject(Handle<PyList> string_table,
                                                  Handle<PyList> cache) {
  HandleScope scope;

  int arg_count = reader_->ReadInt32();
  int posonly_arg_count = reader_->ReadInt32();
  int kwonly_arg_count = reader_->ReadInt32();
  int nlocals = reader_->ReadInt32();
  int stack_size = reader_->ReadInt32();
  int flags = reader_->ReadInt32();

  Handle<PyString> bytecodes = ParseByteCodes(string_table, cache);
  Handle<PyList> consts = ParseConsts(string_table, cache);
  Handle<PyList> names = ParseNames(string_table, cache);
  Handle<PyList> var_names = ParseVarNames(string_table, cache);
  Handle<PyList> free_vars = ParseFreeVars(string_table, cache);
  Handle<PyList> cell_vars = ParseCellVars(string_table, cache);

  Handle<PyString> file_name = ParseFileName(string_table, cache);
  Handle<PyString> module_name = ParseName(string_table, cache);
  int begin_line_no = reader_->ReadInt32();
  Handle<PyString> lnotab = ParseNoTable(string_table, cache);

  Handle<CodeObject> result = CodeObject::NewInstance(
      arg_count, posonly_arg_count, kwonly_arg_count, nlocals, stack_size,
      flags, bytecodes, names, consts, var_names, free_vars, cell_vars,
      file_name, module_name, begin_line_no, lnotab);

  return result.EscapeFrom(&scope);
}

Handle<PyString> PycFileParser::ParseByteCodes(Handle<PyList> string_table,
                                               Handle<PyList> cache) {
  char object_type = reader_->ReadByte();
  assert(GET_OBJECT_TYPE_CHAR(object_type) == kStringFlag);

  Handle<PyString> s = ParseString(true);
  if (HAS_REF_FLAG(object_type)) {
    PyList::Append(cache, s);
  }

  return s;
}

Handle<PyList> PycFileParser::ParseConsts(Handle<PyList> string_table,
                                          Handle<PyList> cache) {
  return ParseTuple(string_table, cache);
}

Handle<PyList> PycFileParser::ParseNames(Handle<PyList> string_table,
                                         Handle<PyList> cache) {
  return ParseTuple(string_table, cache);
}

Handle<PyList> PycFileParser::ParseVarNames(Handle<PyList> string_table,
                                            Handle<PyList> cache) {
  return ParseTuple(string_table, cache);
}

Handle<PyList> PycFileParser::ParseFreeVars(Handle<PyList> string_table,
                                            Handle<PyList> cache) {
  return ParseTuple(string_table, cache);
}

Handle<PyList> PycFileParser::ParseCellVars(Handle<PyList> string_table,
                                            Handle<PyList> cache) {
  return ParseTuple(string_table, cache);
}

Handle<PyString> PycFileParser::ParseFileName(Handle<PyList> string_table,
                                              Handle<PyList> cache) {
  return ParseName(string_table, cache);
}

Handle<PyString> PycFileParser::ParseString(bool is_long_string) {
  uint32_t length = 0;

  if (is_long_string) {
    length = static_cast<uint32_t>(reader_->ReadInt32());
  } else {
    length = static_cast<uint32_t>(static_cast<uint8_t>(reader_->ReadByte()));
  }

  auto str_value = std::make_unique<char[]>(length);
  for (uint32_t i = 0; i < length; ++i) {
    str_value[i] = reader_->ReadByte();
  }

  return PyString::NewInstance(str_value.get(), length);
}

Handle<PyList> PycFileParser::ParseTuple(Handle<PyList> string_table,
                                         Handle<PyList> cache) {
  char raw_object_type = reader_->ReadByte();
  char object_type = GET_OBJECT_TYPE_CHAR(raw_object_type);

  Handle<PyList> tuple;
  switch (object_type) {
    case kTupleFlag: {
      int cache_index = cache->length();
      if (HAS_REF_FLAG(object_type)) {
        PyList::Append(cache, Handle<PyObject>::Null());
      }

      tuple = ParseTupleImpl(string_table, cache);

      if (HAS_REF_FLAG(object_type)) {
        cache->Set(cache_index, tuple);
      }

      break;
    }
    case kInCacheObjectFlag: {
      int index = reader_->ReadInt32();
      tuple = Handle<PyList>::Cast(cache->Get(index));
    }
    default:
      printf("unknown object type: %c\n", object_type);
      exit(0);
  }

  return tuple;
}

Handle<PyList> PycFileParser::ParseTupleImpl(Handle<PyList> string_table,
                                             Handle<PyList> cache) {
  uint8_t length = reader_->ReadByte();
  Handle<PyList> list = PyList::NewInstance(length);

  for (int i = 0; i < length; ++i) {
    HandleScope scope;

    char raw_object_type = reader_->ReadByte();

    int index = 0;
    if (HAS_REF_FLAG(raw_object_type)) {
      index = cache->length();
      PyList::Append(cache, Handle<PyObject>::Null());
    }

    Handle<PyObject> object;
    char object_type = GET_OBJECT_TYPE_CHAR(raw_object_type);
    switch (object_type) {
      case kCodeObjectFlag:
        object = ParseCodeObject(string_table, cache);
        break;
      case kIntegerFlag:
        object = PyInteger::NewInstance(reader_->ReadInt32());
        break;
      case kDoubleFlag:
        object = PyDouble::NewInstance(reader_->ReadDouble());
        break;
      case kNoneObjectFlag:
        object = Handle<PyObject>(Universe::py_none_object_);
        break;
      case kTrueObjectFlag:
        object = Handle<PyObject>(Universe::py_true_object_);
        break;
      case kFalseObjectFlag:
        object = Handle<PyObject>(Universe::py_false_object_);
        break;
      case kInternedStringFlag:
        object = ParseString(true);
        break;
      case kShortAsciiStringFlag:
      case kShortAsciiInternedStringFlag:
        object = ParseString(false);
        break;
      case kStringFlag:
        object = ParseString(true);
        break;
      case kInStringTableObjectFlag:
        object = string_table->Get(reader_->ReadInt32());
        break;
      case kInCacheObjectFlag:
        object = cache->Get(reader_->ReadInt32());
        break;
      case kTupleFlag:
        object = ParseTuple(string_table, cache);
        break;
      default:
        printf("unknown object type: %c\n", object_type);
        exit(0);
    }

    list->Set(i, object);

    if (HAS_REF_FLAG(raw_object_type)) {
      cache->Set(index, object);
    }
  }

  return list;
}

Handle<PyString> PycFileParser::ParseName(Handle<PyList> string_table,
                                          Handle<PyList> cache) {
  char object_type = reader_->ReadByte();

  Handle<PyString> s;

  switch (GET_OBJECT_TYPE_CHAR(object_type)) {
    case kStringFlag:
    case kInternedStringFlag:
      s = Handle<PyString>(ParseString(true));
      break;
    case kShortAsciiStringFlag:
    case kShortAsciiInternedStringFlag:
      s = Handle<PyString>(ParseString(false));
      break;
    case kInCacheObjectFlag: {
      int index = reader_->ReadInt32();
      s = Handle<PyString>::Cast(string_table->Get(index));
      break;
    }
    case kInStringTableObjectFlag: {
      int index = reader_->ReadInt32();
      s = Handle<PyString>::Cast(cache->Get(index));
      break;
    }
  }

  if (HAS_REF_FLAG(object_type)) {
    PyList::Append(cache, s);
  }

  return s;
}

Handle<PyString> PycFileParser::ParseNoTable(Handle<PyList> string_table,
                                             Handle<PyList> cache) {
  char raw_object_type = reader_->ReadByte();
  char object_type = GET_OBJECT_TYPE_CHAR(raw_object_type);

  Handle<PyString> s;
  switch (object_type) {
    case kStringFlag:
    case kInternedStringFlag:
      s = ParseString(true);
      break;
    case kInCacheObjectFlag: {
      int index = reader_->ReadInt32();
      s = Handle<PyString>::Cast(cache->Get(index));
      break;
    }
    default:
      printf("expect a string for no table, but got %c\n", object_type);
      exit(0);
  }

  if (HAS_REF_FLAG(object_type)) {
    PyList::Append(cache, s);
  }

  return s;
}

}  // namespace saauso::internal
