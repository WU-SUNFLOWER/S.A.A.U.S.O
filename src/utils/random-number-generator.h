// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef SAAUSO_UTILS_RANDOM_NUMBER_GENERATOR_H_
#define SAAUSO_UTILS_RANDOM_NUMBER_GENERATOR_H_

#include <cstdint>

namespace saauso::internal {

// 伪随机数生成器（PRNG）。
//
// 当前实现基于 xorshift128+：
// - 速度快、实现简单，适合 random 模块的 MVP 版本与 benchmark 场景。
// - 该 PRNG 不保证密码学安全性，禁止用于安全场景。
//
// 该类型位于 src/utils/，因此它只能依赖标准库与 utils 层能力，
// 不能访问 VM 上层（Isolate/Heap/Handle 等）。
class RandomNumberGenerator {
 public:
  RandomNumberGenerator();
  explicit RandomNumberGenerator(uint64_t seed);

  // 使用 seed 初始化内部状态。
  //
  // 说明：
  // - 该方法会用 splitmix64 将单个 seed 扩展为 128-bit 状态，
  // 避免弱 seed 与全 0 状态。
  void SetSeed(uint64_t seed);

  // 生成一个 64-bit 无符号随机数。
  uint64_t NextU64();

  // 生成一个 32-bit 无符号随机数。
  uint32_t NextU32();

  // 生成一个 double，范围为 [0.0, 1.0)。
  double NextDouble01();

  // 生成一个范围为 [0, bound) 的无偏随机数。
  // 前置条件：bound > 0。
  uint64_t NextU64Bounded(uint64_t bound);

  // 根据当前的系统时间获取一个最新的随机数种子
  static uint64_t NowSeed();

 private:
  uint64_t s0_;
  uint64_t s1_;
};

}  // namespace saauso::internal

#endif  // SAAUSO_UTILS_RANDOM_NUMBER_GENERATOR_H_
