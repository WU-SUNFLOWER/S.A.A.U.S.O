// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef SAAUSO_CODE_BINARY_FILE_READER_H_
#define SAAUSO_CODE_BINARY_FILE_READER_H_

#include <cstdio>

namespace saauso::internal {

class BinaryFileReader {
 public:
  explicit BinaryFileReader(const char* filename);

  ~BinaryFileReader();

  char ReadByte();
  int ReadInt32();
  double ReadDouble();

  void Unread();

  void Close();

 private:
  static constexpr int kBufferLength = 256;

  FILE* file_{nullptr};
  size_t index_{0};
  char buffer_[kBufferLength];
};

}  // namespace saauso::internal

#endif  // SAAUSO_CODE_BINARY_FILE_READER_H_
