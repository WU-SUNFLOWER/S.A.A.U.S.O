// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/utils/random-number-generator.h"

#include <chrono>
#include <limits>

namespace saauso::internal {

namespace {

// splitmix64 作为 seed 扩展器：
// - 输入一个 64-bit seed，输出均匀性更好的 64-bit 序列；
// - 用于把单 seed 扩展成 xorshift128+ 的 128-bit 初始状态。
uint64_t SplitMix64(uint64_t* x) {
  uint64_t z = (*x += 0x9e3779b97f4a7c15ULL);
  z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ULL;
  z = (z ^ (z >> 27)) * 0x94d049bb133111ebULL;
  return z ^ (z >> 31);
}

}  // namespace

/////////////////////////////////////////////////////////////////////////////////

RandomNumberGenerator::RandomNumberGenerator() : s0_(0), s1_(0) {
  SetSeed(NowSeed());
}

RandomNumberGenerator::RandomNumberGenerator(uint64_t seed) : s0_(0), s1_(0) {
  SetSeed(seed);
}

void RandomNumberGenerator::SetSeed(uint64_t seed) {
  uint64_t x = seed;
  s0_ = SplitMix64(&x);
  s1_ = SplitMix64(&x);
  if (s0_ == 0 && s1_ == 0) {
    s1_ = 1;
  }
}

uint64_t RandomNumberGenerator::NextU64() {
  uint64_t s1 = s0_;
  const uint64_t s0 = s1_;
  s0_ = s0;

  s1 ^= s1 << 23;
  s1_ = s1 ^ s0 ^ (s1 >> 17) ^ (s0 >> 26);
  return s1_ + s0;
}

uint32_t RandomNumberGenerator::NextU32() {
  return static_cast<uint32_t>(NextU64() >> 32);
}

double RandomNumberGenerator::NextDouble01() {
  constexpr double kInvPow2_53 = 1.0 / 9007199254740992.0;  // 2^53
  uint64_t x = NextU64();
  return static_cast<double>(x >> 11) * kInvPow2_53;
}

uint64_t RandomNumberGenerator::NextU64Bounded(uint64_t bound) {
  if (bound == 0) {
    return 0;
  }

  // 通过拒绝采样避免 x % bound 的取模偏差：
  // 令 limit 为最大可接受值（>=limit 的样本丢弃），保证范围能整除 bound。
  const uint64_t max = std::numeric_limits<uint64_t>::max();
  const uint64_t limit = max - (max % bound);
  uint64_t x = 0;
  do {
    x = NextU64();
  } while (x >= limit);
  return x % bound;
}

uint64_t RandomNumberGenerator::NowSeed() {
  auto now = std::chrono::high_resolution_clock::now().time_since_epoch();
  return static_cast<uint64_t>(
      std::chrono::duration_cast<std::chrono::nanoseconds>(now).count());
}

}  // namespace saauso::internal
