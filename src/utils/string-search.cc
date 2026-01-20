// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/utils/string-search.h"

#include <algorithm>
#include <array>
#include <cstring>
#include <vector>

namespace saauso::internal {

namespace {

int64_t IndexOfNaive(std::string_view subject, std::string_view pattern) {
  const int64_t n = static_cast<int64_t>(subject.size());
  const int64_t m = static_cast<int64_t>(pattern.size());

  if (m == 0) {
    return 0;
  }
  if (m > n) {
    return -1;
  }

  const char* const s = subject.data();
  const char* const p = pattern.data();
  for (int64_t i = 0; i <= n - m; ++i) {
    if (std::memcmp(s + i, p, static_cast<size_t>(m)) == 0) {
      return i;
    }
  }
  return -1;
}

void PreprocessGoodSuffix(const uint8_t* pattern,
                          int64_t m,
                          std::vector<int64_t>* shift) {
  shift->assign(static_cast<size_t>(m + 1), m);
  std::vector<int64_t> bpos(static_cast<size_t>(m + 1), 0);

  int64_t i = m;
  int64_t j = m + 1;
  bpos[static_cast<size_t>(i)] = j;
  while (i > 0) {
    while (j <= m && pattern[i - 1] != pattern[j - 1]) {
      if ((*shift)[static_cast<size_t>(j)] == m) {
        (*shift)[static_cast<size_t>(j)] = j - i;
      }
      j = bpos[static_cast<size_t>(j)];
    }
    --i;
    --j;
    bpos[static_cast<size_t>(i)] = j;
  }

  j = bpos[0];
  for (i = 0; i <= m; ++i) {
    if ((*shift)[static_cast<size_t>(i)] == m) {
      (*shift)[static_cast<size_t>(i)] = j;
    }
    if (i == j) {
      j = bpos[static_cast<size_t>(j)];
    }
  }
}

int64_t IndexOfBoyerMoore(std::string_view subject, std::string_view pattern) {
  const int64_t n = static_cast<int64_t>(subject.size());
  const int64_t m = static_cast<int64_t>(pattern.size());

  if (m == 0) {
    return 0;
  }
  if (m > n) {
    return -1;
  }

  const uint8_t* const txt =
      reinterpret_cast<const uint8_t*>(subject.data());
  const uint8_t* const pat =
      reinterpret_cast<const uint8_t*>(pattern.data());

  std::array<int64_t, 256> last{};
  last.fill(-1);
  for (int64_t i = 0; i < m; ++i) {
    last[pat[i]] = i;
  }

  std::vector<int64_t> good_suffix_shift;
  PreprocessGoodSuffix(pat, m, &good_suffix_shift);

  int64_t s = 0;
  while (s <= n - m) {
    int64_t j = m;
    while (j > 0 && pat[j - 1] == txt[s + j - 1]) {
      --j;
    }

    if (j == 0) {
      return s;
    }

    const uint8_t bad_char = txt[s + j - 1];
    int64_t bc_shift = j - 1 - last[bad_char];
    if (bc_shift < 1) {
      bc_shift = 1;
    }

    const int64_t gs_shift = good_suffix_shift[static_cast<size_t>(j)];
    s += std::max(bc_shift, gs_shift);
  }

  return -1;
}

}  // namespace

int64_t IndexOfSubstring(std::string_view subject, std::string_view pattern) {
  if (static_cast<int64_t>(pattern.size()) < kBMMinPatternLength) {
    return IndexOfNaive(subject, pattern);
  }
  return IndexOfBoyerMoore(subject, pattern);
}

}  // namespace saauso::internal

