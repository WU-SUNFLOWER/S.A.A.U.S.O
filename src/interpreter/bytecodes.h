// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include <cstdint>

namespace saauso::internal {

// 参见CPython源代码中的Include/opcode.h

#define BYTECODE_LIST(V)   \
  V(Cache, 0)              \
  V(PopTop, 1)             \
  V(PushNull, 2)           \
  V(EndFor, 4)             \
  V(Nop, 9)                \
  V(BinarySubscr, 25)      \
  V(StoreSubscr, 60)       \
  V(GetIter, 68)           \
  V(LoadBuildClass, 71)    \
  V(ReturnValue, 83)       \
  V(StoreName, 90)         \
  V(UnpackSequence, 92)    \
  V(ForIter, 93)           \
  V(StoreAttr, 95)         \
  V(StoreGlobal, 97)       \
  V(LoadConst, 100)        \
  V(LoadName, 101)         \
  V(BuildTuple, 102)       \
  V(BuildList, 103)        \
  V(BuildMap, 105)         \
  V(LoadAttr, 106)         \
  V(CompareOp, 107)        \
  V(JumpForward, 110)      \
  V(JumpIfFalse, 114)      \
  V(JumpIfTrue, 115)       \
  V(IsOp, 117)             \
  V(LoadGlobal, 116)       \
  V(ContainsOp, 118)       \
  V(ReturnConst, 121)      \
  V(BinaryOp, 122)         \
  V(LoadFast, 124)         \
  V(StoreFast, 125)        \
  V(DeleteFast, 126)       \
  V(PopJumpIfNotNone, 128) \
  V(PopJumpIfNone, 129)    \
  V(MakeFunction, 132)     \
  V(MakeCell, 135)         \
  V(LoadClosure, 136)      \
  V(LoadDeref, 137)        \
  V(StoreDeref, 138)       \
  V(JumpBackward, 140)     \
  V(CopyFreeVars, 149)     \
  V(Resume, 151)           \
  V(BuildConstKeyMap, 156) \
  V(ListExtend, 162)       \
  V(DictMerge, 164)        \
  V(CallFunctionEx, 142)   \
  V(Call, 171)             \
  V(KwNames, 172)          \
  V(CallIntrinsic1, 173)

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

enum UnaryIntrinsic {
  kInvalid = 0,             // 对应 INTRINSIC_1_INVALID
  kPrint = 1,               // 对应 INTRINSIC_PRINT
  kImportStar = 2,          // 对应 INTRINSIC_IMPORT_STAR
  kStopIterationError = 3,  // 对应 INTRINSIC_STOPITERATION_ERROR
  kAsyncGenWrap = 4,        // 对应 INTRINSIC_ASYNC_GEN_WRAP
  kUnaryPositive = 5,       // 对应 INTRINSIC_UNARY_POSITIVE
  kListToTuple = 6,         // 对应 INTRINSIC_LIST_TO_TUPLE
  kTypeVar = 7,             // 对应 INTRINSIC_TYPEVAR
  kParamSpec = 8,           // 对应 INTRINSIC_PARAMSPEC
  kTypeVarTuple = 9,        // 对应 INTRINSIC_TYPEVARTUPLE
  kSubscriptGeneric = 10,   // 对应 INTRINSIC_SUBSCRIPT_GENERIC
  kTypeAlias = 11,          // 对应 INTRINSIC_TYPEALIAS
};

}  // namespace saauso::internal
