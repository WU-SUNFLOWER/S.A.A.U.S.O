// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/code/cpython312-pyc-file-parser.h"

#include <cassert>
#include <cinttypes>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>

#include "src/execution/isolate.h"
#include "src/handles/handles.h"
#include "src/heap/factory.h"
#include "src/objects/py-code-object.h"
#include "src/objects/py-dict.h"
#include "src/objects/py-float.h"
#include "src/objects/py-list.h"
#include "src/objects/py-object.h"
#include "src/objects/py-oddballs.h"
#include "src/objects/py-smi.h"
#include "src/objects/py-string.h"
#include "src/objects/py-tuple.h"
#include "src/utils/binary-file-reader.h"

// CPython 3.12 .pyc 文件格式（高层概览）
// -----------------------------------------------------------------------------
// CPython 会把每个模块编译为一个顶层 `code` object，并将其使用 CPython 的
// `marshal` 协议序列化后写入 .pyc。整体结构可以理解为：
//
//   [ 16-byte header ][ marshal(code object payload) ... ]
//
// 1) 16-byte header（PEP 552 的思路，3.12 仍延续此布局）
//   - int32 magic_number (little-endian)
//       由 `PyImport_GetMagicNumber()` 生成，用于区分 pyc 版本/字节码版本。
//   - int32 flags (little-endian)
//       低位含义（与 PEP552 一致）：
//         * flags & 0x00000001 == 0: timestamp-based pyc
//             header 接下来 8 字节一般是：
//               - int32 timestamp
//               - int32 source_size
//         * flags & 0x00000001 != 0: hash-based pyc
//             header 接下来 8 字节一般是：
//               - 8 bytes hash (e.g. SipHash/Hash of source)
//
//   本项目的单元测试/内嵌 compiler 目前构造的是最简单的 timestamp-based
//   header：
//     magic, flags=0, header1=0, header2=source_size
//   因此解析器仅顺序读取并忽略这 4 个 int32，随后直接进入 marshal payload。
//
// 2) marshal payload（核心）
//   payload 的第一个对象必须是 code object（tag 字符为 'c'）。
//
//   marshal 是一种紧凑的二进制序列化格式。每个对象以 1 字节的“类型 tag”
//   开始，其中最高位 0x80 是“引用缓存标记（ref flag）”，低 7 位才是实际的
//   tag 字符：
//
//     raw_tag = (ref? 0x80 : 0) | tag_char
//
//   ref flag 的作用：
//     - marshal 在序列化时，会把某些对象（例如字符串/容器/递归结构）放入
//     cache，
//       后续若再次出现同一个对象，会用 'r' + index
//       复用，避免重复序列化与支持自引用。
//     - 解析时必须支持“先占位、后回填”的语义：遇到 ref_flag 的对象，先在 cache
//       里 append 一个 placeholder（null），等对象真正解析完成后再 set 回去。
//
// 3) 字符串表 string_table（interned strings）
//   marshal 对“被 intern 的字符串”会进入一个独立的 string_table：
//     - 't' / 'A' / 'Z' 等 tag 代表 interned string，会 append 到 string_table
//     - 'R' + index 用于从 string_table 复用
//
// 4) CPython 3.12 code object 的字段顺序（marshal('c') 的 payload）
//   该顺序与 CPython `marshal.c` 中写入 code object 的字段顺序一致（这里列出本
//   解析器依赖的最小字段集）：
//     int32 argcount
//     int32 posonlyargcount
//     int32 kwonlyargcount
//     int32 stacksize
//     int32 flags
//     bytes  code (co_code)
//     tuple  consts (co_consts)
//     tuple  names (co_names)
//     tuple  localsplusnames (co_localsplusnames)
//     bytes|None localspluskinds (co_localspluskinds)
//     str    filename (co_filename)
//     str    name (co_name)
//     str    qualname (co_qualname)
//     int32  firstlineno (co_firstlineno)
//     bytes|None linetable (co_linetable)
//     bytes|None exceptiontable (co_exceptiontable)
//
//   localspluskinds 的每个字节描述对应 localsplusnames[i]
//   的“槽位类型”，本解析器 使用最常用的两个 bit 来拆分 freevars / cellvars：
//     - kind & 0x80 != 0  => freevar
//     - kind & 0x40 != 0  => cellvar
//
// 5) 本解析器的落地策略（映射到 S.A.A.U.S.O 对象系统）
//   - 统一使用 `Handle<T>` 构造临时对象，避免 GC 移动导致悬垂引用。
//   - 将 marshal tuple 映射为 `PyTuple`，marshal list 映射为 `PyList`。
//   - 将 marshal 的 int/long 在当前 MVP 里限制为 int32 范围，并用 `PySmi`
//   表示。
//   - code object 落地到 `PyCodeObject::New(...)`，并补齐：
//       * nlocals_（由 localsplusnames 长度推导）
//       * free_vars_ / cell_vars_（由 localspluskinds 推导）
//       * 可空字段（localspluskinds/linetable/exceptiontable）用 null 表示
//
namespace saauso::internal {

namespace {

constexpr uint8_t kRefFlagMask = 0x80;
constexpr uint8_t kObjectTypeMask = 0x7f;

constexpr char kCodeObjectFlag = 'c';
constexpr char kIntegerFlag = 'i';
constexpr char kLongFlag = 'l';
constexpr char kDoubleFlag = 'g';

constexpr char kNoneObjectFlag = 'N';
constexpr char kTrueObjectFlag = 'T';
constexpr char kFalseObjectFlag = 'F';
constexpr char kNullObjectFlag = '0';

constexpr char kBytesFlag = 's';
constexpr char kUnicodeFlag = 'u';
constexpr char kInternedStringFlag = 't';
constexpr char kAsciiStringFlag = 'a';
constexpr char kAsciiInternedStringFlag = 'A';
constexpr char kShortAsciiStringFlag = 'z';
constexpr char kShortAsciiInternedStringFlag = 'Z';

constexpr char kSmallTupleFlag = ')';
constexpr char kTupleFlag = '(';
constexpr char kListFlag = '[';
constexpr char kDictFlag = '{';

constexpr char kInCacheObjectFlag = 'r';
constexpr char kInStringTableObjectFlag = 'R';

inline bool HasRefFlag(uint8_t object_type) {
  return (object_type & kRefFlagMask) != 0;
}

inline char GetObjectTypeChar(uint8_t object_type) {
  return static_cast<char>(object_type & kObjectTypeMask);
}

}  // namespace

CPython312PycFileParser::CPython312PycFileParser(const char* filename,
                                                 Isolate* isolate)
    : isolate_(isolate),
      reader_(std::make_unique<BinaryFileReader>(filename)) {}

CPython312PycFileParser::CPython312PycFileParser(std::span<const uint8_t> bytes,
                                                 Isolate* isolate)
    : isolate_(isolate), reader_(std::make_unique<BinaryFileReader>(bytes)) {}

CPython312PycFileParser::~CPython312PycFileParser() = default;

int CPython312PycFileParser::ReadInt32() {
  return reader_->ReadInt32();
}

uint16_t CPython312PycFileParser::ReadUInt16() {
  uint16_t b1 =
      static_cast<uint16_t>(static_cast<uint8_t>(reader_->ReadByte()));
  uint16_t b2 =
      static_cast<uint16_t>(static_cast<uint8_t>(reader_->ReadByte()));
  return static_cast<uint16_t>((b2 << 8) | b1);
}

Handle<PyString> CPython312PycFileParser::ParseStringLong() {
  return ParseString(true);
}

Handle<PyString> CPython312PycFileParser::ParseStringShort() {
  return ParseString(false);
}

Handle<PyString> CPython312PycFileParser::ParseString(bool is_long_string) {
  uint32_t length = 0;
  if (is_long_string) {
    length = static_cast<uint32_t>(ReadInt32());
  } else {
    length = static_cast<uint32_t>(static_cast<uint8_t>(reader_->ReadByte()));
  }

  auto bytes = std::make_unique<char[]>(length);
  for (uint32_t i = 0; i < length; ++i) {
    bytes[i] = reader_->ReadByte();
  }
  return PyString::New(isolate_, bytes.get(), static_cast<int64_t>(length),
                       false);
}

Handle<PyCodeObject> CPython312PycFileParser::Parse() {
  EscapableHandleScope scope;

  // .pyc header: magic_number + flags + (timestamp/hash, size/hash)。
  // 解析器当前只需要跳过它们，真正的语义由 marshal payload 决定。
  int magic_number [[maybe_unused]] = ReadInt32();
  int flags [[maybe_unused]] = ReadInt32();
  int header1 [[maybe_unused]] = ReadInt32();
  int header2 [[maybe_unused]] = ReadInt32();

  uint8_t raw_object_type =
      static_cast<uint8_t>(static_cast<unsigned char>(reader_->ReadByte()));
  bool ref_flag = HasRefFlag(raw_object_type);
  char object_type = GetObjectTypeChar(raw_object_type);

  // marshal payload 的第一个对象应当是 'c'（code object）。
  if (object_type != kCodeObjectFlag) {
    return Handle<PyCodeObject>::null();
  }

  // marshal 的两个核心表：
  // - string_table: 存放被 intern 的字符串（由 't'/'A'/'Z' 等 tag 推入）
  // - cache: ref_flag 置位的对象会进入 cache，后续可由 'r' 复用
  Handle<PyList> string_table = PyList::New(isolate_);
  Handle<PyList> cache = PyList::New(isolate_);

  int index = 0;
  if (ref_flag) {
    index = static_cast<int>(cache->length());
    PyList::Append(cache, Handle<PyObject>::null(), isolate_);
  }

  Handle<PyCodeObject> code_object = ParseCodeObject(string_table, cache);

  if (ref_flag) {
    cache->Set(index, code_object);
  }

  return scope.Escape(code_object);
}

Handle<PyCodeObject> CPython312PycFileParser::ParseCodeObject(
    Handle<PyList> string_table,
    Handle<PyList> cache) {
  EscapableHandleScope scope;

  // 这里的读取顺序必须严格匹配 CPython3.12 marshal 对 code object 的写入顺序。
  int arg_count = ReadInt32();
  int posonly_arg_count = ReadInt32();
  int kwonly_arg_count = ReadInt32();
  int stack_size = ReadInt32();
  int flags = ReadInt32();

  auto bytecodes = Handle<PyString>::cast(ParseObject(string_table, cache));
  auto consts = Handle<PyTuple>::cast(ParseObject(string_table, cache));
  auto names = Handle<PyTuple>::cast(ParseObject(string_table, cache));

  auto localsplusnames =
      Handle<PyTuple>::cast(ParseObject(string_table, cache));
  auto localspluskinds =
      Handle<PyString>::cast(ParseObject(string_table, cache));

  auto file_name = Handle<PyString>::cast(ParseObject(string_table, cache));
  auto co_name = Handle<PyString>::cast(ParseObject(string_table, cache));
  auto qual_name = Handle<PyString>::cast(ParseObject(string_table, cache));
  int begin_line_no = ReadInt32();

  Handle<PyString> line_table;
  {
    auto obj = ParseObject(string_table, cache);
    if (!obj.is_null()) {
      line_table = Handle<PyString>::cast(obj);
    } else {
      line_table = Handle<PyString>::null();
    }
  }

  Handle<PyString> exception_table;
  {
    auto obj = ParseObject(string_table, cache);
    if (!obj.is_null()) {
      exception_table = Handle<PyString>::cast(obj);
    } else {
      exception_table = Handle<PyString>::null();
    }
  }

  Handle<PyCodeObject> result = PyCodeObject::New(
      isolate_, arg_count, posonly_arg_count, kwonly_arg_count, stack_size,
      flags, bytecodes, consts, names, localsplusnames, localspluskinds,
      file_name, co_name, qual_name, begin_line_no, line_table,
      exception_table);

  return scope.Escape(result);
}

Handle<PyObject> CPython312PycFileParser::ParseObject(
    Handle<PyList> string_table,
    Handle<PyList> cache) {
  EscapableHandleScope scope;

  // marshal 的每个对象以一个 raw_tag 开始：高 1bit 是 ref_flag，低 7bit 是
  // tag。
  uint8_t raw_object_type =
      static_cast<uint8_t>(static_cast<unsigned char>(reader_->ReadByte()));
  bool ref_flag = HasRefFlag(raw_object_type);
  char object_type = GetObjectTypeChar(raw_object_type);

  int cache_index = 0;
  if (ref_flag) {
    cache_index = static_cast<int>(cache->length());
    PyList::Append(cache, Handle<PyObject>::null(), isolate_);
  }

  Handle<PyObject> object;
  switch (object_type) {
    case kCodeObjectFlag:
      object = ParseCodeObject(string_table, cache);
      break;
    case kIntegerFlag:
      object = isolate_->factory()->NewSmiFromInt(ReadInt32());
      break;
    case kLongFlag: {
      int n = ReadInt32();
      int sign = 1;
      if (n < 0) {
        sign = -1;
        n = -n;
      }

      int64_t value = 0;
      int64_t base = 1;
      for (int i = 0; i < n; ++i) {
        uint16_t digit = static_cast<uint16_t>(ReadUInt16() & 0x7fff);
        value += static_cast<int64_t>(digit) * base;
        base <<= 15;
      }
      value *= sign;
      if (value > INT32_MAX || value < INT32_MIN) {
        std::fprintf(stderr, "marshal long out of int32 range: %" PRId64 "\n",
                     value);
        std::exit(1);
      }
      object = isolate_->factory()->NewSmiFromInt(value);
      break;
    }
    case kDoubleFlag:
      object = PyFloat::New(isolate_, reader_->ReadDouble());
      break;
    case kNoneObjectFlag:
      object = isolate_->factory()->py_none_object();
      break;
    case kTrueObjectFlag:
      object = Handle<PyBoolean>(isolate_->py_true_object());
      break;
    case kFalseObjectFlag:
      object = Handle<PyBoolean>(isolate_->py_false_object());
      break;
    case kBytesFlag:
    case kUnicodeFlag:
    case kInternedStringFlag:
    case kAsciiStringFlag:
    case kAsciiInternedStringFlag:
      object = ParseStringLong();
      if (object_type == kInternedStringFlag ||
          object_type == kAsciiInternedStringFlag) {
        PyList::Append(string_table, object, isolate_);
      }
      break;
    case kShortAsciiStringFlag:
    case kShortAsciiInternedStringFlag:
      object = ParseStringShort();
      if (object_type == kShortAsciiInternedStringFlag) {
        PyList::Append(string_table, object, isolate_);
      }
      break;
    case kInStringTableObjectFlag: {
      int index = ReadInt32();
      object = string_table->Get(index);
      break;
    }
    case kInCacheObjectFlag: {
      int index = ReadInt32();
      object = cache->Get(index);
      break;
    }
    case kSmallTupleFlag: {
      uint8_t length =
          static_cast<uint8_t>(static_cast<unsigned char>(reader_->ReadByte()));
      auto tuple = PyTuple::New(isolate_, length);
      if (ref_flag) {
        cache->Set(cache_index, tuple);
      }
      for (uint8_t i = 0; i < length; ++i) {
        auto value = ParseObject(string_table, cache);
        tuple->SetInternal(i,
                           value.is_null() ? Tagged<PyObject>::null() : *value);
      }
      object = tuple;
      break;
    }
    case kTupleFlag: {
      int length = ReadInt32();
      auto tuple = PyTuple::New(isolate_, length);
      if (ref_flag) {
        cache->Set(cache_index, tuple);
      }
      for (int i = 0; i < length; ++i) {
        auto value = ParseObject(string_table, cache);
        tuple->SetInternal(i,
                           value.is_null() ? Tagged<PyObject>::null() : *value);
      }
      object = tuple;
      break;
    }
    case kListFlag: {
      int length = ReadInt32();
      auto list = PyList::New(isolate_, length);
      if (ref_flag) {
        cache->Set(cache_index, list);
      }
      for (int i = 0; i < length; ++i) {
        PyList::Append(list, ParseObject(string_table, cache), isolate_);
      }
      object = list;
      break;
    }
    case kDictFlag: {
      auto dict = PyDict::New(isolate_);
      if (ref_flag) {
        cache->Set(cache_index, dict);
      }
      while (true) {
        auto key = ParseObject(string_table, cache);
        if (key.is_null()) {
          break;
        }
        auto value = ParseObject(string_table, cache);
        if (PyDict::Put(dict, key, value, isolate_).IsNothing()) {
          std::fprintf(stderr, "marshal parser: failed to insert dict entry\n");
          std::exit(1);
        }
      }
      object = dict;
      break;
    }
    case kNullObjectFlag:
      object = Handle<PyObject>::null();
      break;
    default:
      std::fprintf(stderr, "marshal parser: unrecognized type : %c\n",
                   object_type);
      std::exit(1);
  }

  if (ref_flag) {
    cache->Set(cache_index, object);
  }

  return scope.Escape(object);
}

}  // namespace saauso::internal
