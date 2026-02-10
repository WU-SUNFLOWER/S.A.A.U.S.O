// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/utils/binary-file-reader.h"

namespace saauso::internal {

BinaryFileReader::BinaryFileReader(const char* filename) {
  mode_ = Mode::kFile;
  file_ = fopen(filename, "rb");
  fread(buffer_, kBufferLength * sizeof(char), 1, file_);
}

BinaryFileReader::BinaryFileReader(std::span<const uint8_t> bytes) {
  mode_ = Mode::kMemory;
  bytes_ = bytes.data();
  size_ = bytes.size();
}

BinaryFileReader::~BinaryFileReader() {
  Close();
}

char BinaryFileReader::ReadByte() {
  if (mode_ == Mode::kMemory) {
    if (pos_ >= size_) {
      return 0;
    }
    return static_cast<char>(bytes_[pos_++]);
  }

  if (index_ >= kBufferLength) {
    index_ = 0;
    fread(buffer_, kBufferLength * sizeof(char), 1, file_);
  }
  return buffer_[index_++];
}

int BinaryFileReader::ReadInt32() {
  constexpr int kByteMask = 0xff;

  int b1 = ReadByte() & kByteMask;
  int b2 = ReadByte() & kByteMask;
  int b3 = ReadByte() & kByteMask;
  int b4 = ReadByte() & kByteMask;

  return b4 << 24 | b3 << 16 | b2 << 8 | b1;
}

double BinaryFileReader::ReadDouble() {
  char bytes[sizeof(double)];
  for (char& byte : bytes) {
    byte = ReadByte();
  }

  return *(reinterpret_cast<double*>(bytes));
}

void BinaryFileReader::Unread() {
  if (mode_ == Mode::kMemory) {
    if (pos_ > 0) {
      --pos_;
    }
    return;
  }
  if (index_ > 0) {
    --index_;
  }
}

void BinaryFileReader::Close() {
  if (file_ != nullptr) {
    fclose(file_);
    file_ = nullptr;
  }
}

};  // namespace saauso::internal
