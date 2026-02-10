// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef SAAUSO_CODE_BINARY_FILE_READER_H_
#define SAAUSO_CODE_BINARY_FILE_READER_H_

#include <cstdint>
#include <cstdio>
#include <span>

namespace saauso::internal {

class BinaryFileReader {
 public:
  explicit BinaryFileReader(const char* filename);
  explicit BinaryFileReader(std::span<const uint8_t> bytes);

  ~BinaryFileReader();

  char ReadByte();
  int ReadInt32();
  double ReadDouble();

  void Unread();

  void Close();

 private:
  enum class Mode {
    kFile,
    kMemory,
  };

  static constexpr int kBufferLength = 256;

  Mode mode_{Mode::kFile};

  FILE* file_{nullptr};
  size_t index_{0};
  char buffer_[kBufferLength];

  const uint8_t* bytes_{nullptr};
  size_t size_{0};
  size_t pos_{0};
};

}  // namespace saauso::internal

#endif  // SAAUSO_CODE_BINARY_FILE_READER_H_
