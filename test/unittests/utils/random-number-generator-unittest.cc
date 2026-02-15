// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include <cstdint>

#include "src/utils/random-number-generator.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace saauso::internal {

TEST(RandomNumberGeneratorTest, DeterministicForSameSeed) {
  RandomNumberGenerator rng1;
  RandomNumberGenerator rng2;
  rng1.Seed(123456789);
  rng2.Seed(123456789);

  for (int i = 0; i < 64; i++) {
    EXPECT_EQ(rng1.NextU64(), rng2.NextU64());
  }
}

TEST(RandomNumberGeneratorTest, NextDouble01Range) {
  RandomNumberGenerator rng;
  rng.Seed(0);

  for (int i = 0; i < 10000; i++) {
    double x = rng.NextDouble01();
    EXPECT_GE(x, 0.0);
    EXPECT_LT(x, 1.0);
  }
}

TEST(RandomNumberGeneratorTest, NextU64BoundedRange) {
  RandomNumberGenerator rng;
  rng.Seed(42);

  const uint64_t bounds[] = {1, 2, 3, 4, 5, 7, 8, 9, 10, 97, 1000,
                             (1ULL << 32), (1ULL << 63)};
  for (uint64_t bound : bounds) {
    for (int i = 0; i < 1000; i++) {
      uint64_t x = rng.NextU64Bounded(bound);
      EXPECT_LT(x, bound);
    }
  }
}

}  // namespace saauso::internal

