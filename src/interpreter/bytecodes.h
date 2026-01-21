// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include <cstdint>

namespace saauso::internal {

// 参见CPython源代码中的Include/opcode.h

#define BYTECODE_LIST(V) \
  V(Cache, 0)            \
  V(PopTop, 1)           \
  V(PushNull, 2)         \
  V(Nop, 9)              \
  V(ReturnValue, 83)     \
  V(StoreName, 90)       \
  V(StoreGlobal, 97)     \
  V(LoadConst, 100)      \
  V(LoadName, 101)       \
  V(CompareOp, 107)      \
  V(JumpForward, 110)    \
  V(JumpIfFalse, 114)    \
  V(JumpIfTrue, 115)     \
  V(IsOp, 117)           \
  V(LoadGlobal, 116)     \
  V(ContainsOp, 118)     \
  V(ReturnConst, 121)    \
  V(BinaryOp, 122)       \
  V(LoadFast, 124)       \
  V(StoreFast, 125)      \
  V(DeleteFast, 126)     \
  V(MakeFunction, 132)   \
  V(JumpBackward, 140)   \
  V(Resume, 151)         \
  V(Call, 171)

#define DECL_BYTECODES(name, value) inline constexpr uint8_t name = value;
BYTECODE_LIST(DECL_BYTECODES)
#undef DECL_BYTECODES

enum CompareOpType {
  kLT = 0,  // <
  kLE,      // <=
  kEQ,      // ==
  kNE,      // !=
  kGT,      // >
  kGE,      // >=
};

enum BinaryOpType {
  kAdd = 0,                     // +
  kAnd = 1,                     // &
  kFloorDivide = 2,             // 向下整除//
  kLshift = 3,                  // <<
  kMatrixMultiply = 4,          // @
  kMultiply = 5,                // *
  kRemainder = 6,               // %
  kOr = 7,                      // |
  kPower = 8,                   // **
  kRshift = 9,                  // >>
  kSubtract = 10,               // -
  kTrueDivide = 11,             // 除法/
  kXor = 12,                    // ^
  kInplaceAdd = 13,             // +=
  kInplaceAnd = 14,             // &=
  kInplaceFloorDivide = 15,     // 向下整除//=
  kInplaceLshift = 16,          //<<=
  kInplaceMatrixMultiply = 17,  //@=
  kInplaceMultiply = 18,        //*=
  kInplaceRemainder = 19,       //%=
  kInplaceOr = 20,              //|=
  kInplacePower = 21,           //**=
  kInplaceRshift = 22,          //>>=
  kInplaceSubtract = 23,        //-=
  kInplaceTrueDivide = 24,      // 除法/=
  kInplaceXor = 25,             //^=
};

enum MakeFunctionOpArgMask {
  kDefaults = 0x01,
  kKwDefaults = 0x02,
  kAnnotations = 0x04,
  kClosure = 0x08,
};

}  // namespace saauso::internal
