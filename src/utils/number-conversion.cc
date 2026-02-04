// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/utils/number-conversion.h"

#include <array>
#include <cassert>
#include <charconv>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <string>

#include "third_party/fast_float/fast_float.h"

namespace saauso::internal {

namespace {

bool IsAsciiSpace(char c) {
  // 仅识别 ASCII 空白字符，避免受本地化/Unicode 规则影响。
  switch (c) {
    case ' ':
    case '\t':
    case '\n':
    case '\r':
    case '\f':
    case '\v':
      return true;
    default:
      return false;
  }
}

bool IsAsciiDigit(char c) {
  return c >= '0' && c <= '9';
}

char ToLowerAscii(char c) {
  // 仅处理 ASCII 大小写映射。
  if (c >= 'A' && c <= 'Z') {
    return static_cast<char>(c - 'A' + 'a');
  }
  return c;
}

std::string_view TrimAsciiWhitespace(std::string_view s) {
  // 仅裁剪 ASCII 空白字符。
  size_t begin = 0;
  while (begin < s.size() && IsAsciiSpace(s[begin])) {
    ++begin;
  }
  size_t end = s.size();
  while (end > begin && IsAsciiSpace(s[end - 1])) {
    --end;
  }
  return s.substr(begin, end - begin);
}

bool ParseSignedInt(std::string_view s, int* out) {
  // 解析十进制有符号整数（仅用于浮点指数部分），并做溢出保护。
  if (s.empty()) {
    return false;
  }
  int sign = 1;
  if (s.front() == '+') {
    s.remove_prefix(1);
  } else if (s.front() == '-') {
    sign = -1;
    s.remove_prefix(1);
  }
  if (s.empty()) {
    return false;
  }
  int value = 0;
  for (char c : s) {
    if (!IsAsciiDigit(c)) {
      return false;
    }
    const int digit = c - '0';
    if (value > (std::numeric_limits<int>::max() - digit) / 10) {
      return false;
    }
    value = value * 10 + digit;
  }
  *out = value * sign;
  return true;
}

bool NormalizeUnderscores(std::string_view s, std::string* out) {
  // 规范化 '_' 分隔符：只允许出现在两个数字之间；其它情况视为非法。
  out->clear();
  out->reserve(s.size());

  for (size_t i = 0; i < s.size(); ++i) {
    const char c = s[i];
    if (c == '_') {
      if (i == 0 || i + 1 >= s.size()) {
        return false;
      }
      if (!IsAsciiDigit(s[i - 1]) || !IsAsciiDigit(s[i + 1])) {
        return false;
      }
      continue;
    }
    out->push_back(c);
  }
  return true;
}

int DigitValueForBase(char c) {
  if (c >= '0' && c <= '9') {
    return c - '0';
  }
  c = ToLowerAscii(c);
  if (c >= 'a' && c <= 'z') {
    return c - 'a' + 10;
  }
  return -1;
}

bool StartsWithIgnoreCase(std::string_view s, std::string_view prefix) {
  if (s.size() < prefix.size()) {
    return false;
  }
  for (size_t i = 0; i < prefix.size(); ++i) {
    if (ToLowerAscii(s[i]) != ToLowerAscii(prefix[i])) {
      return false;
    }
  }
  return true;
}

bool NormalizeUnderscoresForBase(std::string_view s, int base, std::string* out) {
  out->clear();
  out->reserve(s.size());

  for (size_t i = 0; i < s.size(); ++i) {
    const char c = s[i];
    if (c == '_') {
      if (i == 0 || i + 1 >= s.size()) {
        return false;
      }
      if (DigitValueForBase(s[i - 1]) < 0 ||
          DigitValueForBase(s[i - 1]) >= base ||
          DigitValueForBase(s[i + 1]) < 0 ||
          DigitValueForBase(s[i + 1]) >= base) {
        return false;
      }
      continue;
    }

    int digit = DigitValueForBase(c);
    if (digit < 0 || digit >= base) {
      return false;
    }
    out->push_back(ToLowerAscii(c));
  }
  return true;
}

void WriteStringToBuffer(std::string_view buffer, std::string_view text) {
  // 写入 text 并补 '\\0'，buffer 过小则 fail-fast。
  if (buffer.size() < text.size() + 1) {
    std::fprintf(stderr, "FatalError: number conversion buffer too small\n");
    std::exit(1);
  }
  char* out = const_cast<char*>(buffer.data());
  if (!text.empty()) {
    std::memcpy(out, text.data(), text.size());
  }
  out[text.size()] = '\0';
}

struct DecimalDigits {
  bool negative = false;
  std::string digits;
  int decimal_index = 0;
};

DecimalDigits ParseToDecimalDigits(std::string_view s) {
  // 将十进制浮点字符串拆解为“纯数字 + 小数点位置”，便于后续统一格式化。
  DecimalDigits rep;
  if (!s.empty() && s.front() == '-') {
    rep.negative = true;
    s.remove_prefix(1);
  }

  size_t epos = s.find_first_of("eE");
  std::string_view mantissa = s;
  int exponent = 0;
  if (epos != std::string_view::npos) {
    mantissa = s.substr(0, epos);
    std::string_view exp = s.substr(epos + 1);
    if (!ParseSignedInt(exp, &exponent)) {
      std::fprintf(stderr, "FatalError: invalid float exponent string\n");
      std::exit(1);
    }
  }

  size_t dot = mantissa.find('.');
  int digits_before_dot = dot == std::string_view::npos
                              ? static_cast<int>(mantissa.size())
                              : static_cast<int>(dot);
  rep.digits.reserve(mantissa.size());
  for (char c : mantissa) {
    if (c != '.') {
      rep.digits.push_back(c);
    }
  }
  rep.decimal_index = digits_before_dot + exponent;

  int stripped = 0;
  while (static_cast<size_t>(stripped) < rep.digits.size() &&
         rep.digits[static_cast<size_t>(stripped)] == '0') {
    ++stripped;
  }
  if (stripped > 0) {
    rep.digits.erase(0, static_cast<size_t>(stripped));
    rep.decimal_index -= stripped;
  }

  if (rep.digits.empty()) {
    rep.digits = "0";
    rep.decimal_index = 1;
    rep.negative = false;
  }
  return rep;
}

std::string FormatFixed(const DecimalDigits& rep) {
  // 以不带指数的 fixed 格式输出，并保证至少包含一位小数（例如 1.0）。
  std::string out;
  if (rep.negative) {
    out.push_back('-');
  }

  const int k = rep.decimal_index;
  const int n = static_cast<int>(rep.digits.size());

  if (k <= 0) {
    out.append("0.");
    out.append(static_cast<size_t>(-k), '0');
    out.append(rep.digits);
  } else if (k >= n) {
    out.append(rep.digits);
    out.append(static_cast<size_t>(k - n), '0');
  } else {
    out.append(rep.digits.substr(0, static_cast<size_t>(k)));
    out.push_back('.');
    out.append(rep.digits.substr(static_cast<size_t>(k)));
  }

  if (out.find('.') == std::string::npos) {
    out.append(".0");
  } else if (!out.empty() && out.back() == '.') {
    out.push_back('0');
  }

  return out;
}

std::string FormatScientific(const DecimalDigits& rep) {
  // 以科学计数法输出，指数使用 e±NN 的形式（至少两位指数数字）。
  std::string out;
  if (rep.negative) {
    out.push_back('-');
  }

  const int exp10 = rep.decimal_index - 1;
  out.push_back(rep.digits[0]);
  if (rep.digits.size() > 1) {
    out.push_back('.');
    out.append(rep.digits.substr(1));
  }

  out.push_back('e');
  if (exp10 >= 0) {
    out.push_back('+');
  } else {
    out.push_back('-');
  }
  int abs_exp = exp10 >= 0 ? exp10 : -exp10;

  std::array<char, 32> exp_buf{};
  auto [ptr, ec] =
      std::to_chars(exp_buf.data(), exp_buf.data() + exp_buf.size(), abs_exp);
  if (ec != std::errc()) {
    std::fprintf(stderr, "FatalError: failed to format float exponent\n");
    std::exit(1);
  }
  std::string_view exp_digits(exp_buf.data(),
                              static_cast<size_t>(ptr - exp_buf.data()));
  if (exp_digits.size() == 1) {
    out.push_back('0');
  }
  out.append(exp_digits);

  return out;
}

}  // namespace

std::optional<double> StringToDouble(std::string_view s) {
  const std::string_view trimmed = TrimAsciiWhitespace(s);
  if (trimmed.empty()) {
    return std::nullopt;
  }

  // 先单独处理符号位，以便识别 -inf 等形式。
  bool negative = false;
  std::string_view payload = trimmed;
  if (!payload.empty() && (payload.front() == '+' || payload.front() == '-')) {
    negative = (payload.front() == '-');
    payload.remove_prefix(1);
  }

  std::string lowered;
  lowered.reserve(payload.size());
  for (char c : payload) {
    lowered.push_back(ToLowerAscii(c));
  }

  if (lowered == "nan") {
    return std::numeric_limits<double>::quiet_NaN();
  }
  if (lowered == "inf" || lowered == "infinity") {
    double inf = std::numeric_limits<double>::infinity();
    return negative ? -inf : inf;
  }

  std::string normalized;
  // 解析前先去掉 '_' 分隔符；格式不合法则直接失败。
  if (!NormalizeUnderscores(trimmed, &normalized)) {
    return std::nullopt;
  }

  double value = 0.0;
  auto result = fast_float::from_chars(
      normalized.data(), normalized.data() + normalized.size(), value,
      fast_float::chars_format::general);
  if (result.ec != std::errc() ||
      result.ptr != normalized.data() + normalized.size()) {
    return std::nullopt;
  }
  return value;
}

StringToIntError StringToIntWithBase(std::string_view s,
                                     int base_input,
                                     int64_t* out) {
  assert(out != nullptr);

  if (!(base_input == 0 || (2 <= base_input && base_input <= 36))) {
    return StringToIntError::kInvalidBase;
  }

  s = TrimAsciiWhitespace(s);
  if (s.empty()) {
    return StringToIntError::kInvalid;
  }

  bool negative = false;
  if (s.front() == '+' || s.front() == '-') {
    negative = (s.front() == '-');
    s.remove_prefix(1);
  }
  if (s.empty()) {
    return StringToIntError::kInvalid;
  }

  int base = base_input;
  if (base == 0) {
    if (StartsWithIgnoreCase(s, "0x")) {
      base = 16;
      s.remove_prefix(2);
    } else if (StartsWithIgnoreCase(s, "0o")) {
      base = 8;
      s.remove_prefix(2);
    } else if (StartsWithIgnoreCase(s, "0b")) {
      base = 2;
      s.remove_prefix(2);
    } else {
      base = 10;
    }
  } else if (base == 16 && StartsWithIgnoreCase(s, "0x")) {
    s.remove_prefix(2);
  } else if (base == 8 && StartsWithIgnoreCase(s, "0o")) {
    s.remove_prefix(2);
  } else if (base == 2 && StartsWithIgnoreCase(s, "0b")) {
    s.remove_prefix(2);
  }

  if (s.empty()) {
    return StringToIntError::kInvalid;
  }

  std::string normalized;
  if (!NormalizeUnderscoresForBase(s, base, &normalized) || normalized.empty()) {
    return StringToIntError::kInvalid;
  }

  uint64_t unsigned_value = 0;
  auto parsed = std::from_chars(normalized.data(),
                                normalized.data() + normalized.size(),
                                unsigned_value, base);
  if (parsed.ec == std::errc::invalid_argument ||
      parsed.ptr != normalized.data() + normalized.size()) {
    return StringToIntError::kInvalid;
  }
  if (parsed.ec == std::errc::result_out_of_range) {
    return StringToIntError::kOverflow;
  }

  const uint64_t kLimit =
      negative ? static_cast<uint64_t>(std::numeric_limits<int64_t>::max()) + 1
               : static_cast<uint64_t>(std::numeric_limits<int64_t>::max());
  if (unsigned_value > kLimit) {
    return StringToIntError::kOverflow;
  }

  if (negative) {
    if (unsigned_value == kLimit) {
      *out = std::numeric_limits<int64_t>::min();
    } else {
      *out = -static_cast<int64_t>(unsigned_value);
    }
  } else {
    *out = static_cast<int64_t>(unsigned_value);
  }
  return StringToIntError::kOk;
}

std::optional<int64_t> StringToInt(std::string_view s) {
  int64_t value = 0;
  return StringToIntWithBase(s, 10, &value) == StringToIntError::kOk
             ? std::optional<int64_t>(value)
             : std::nullopt;
}

void DoubleToStringView(double n, std::string_view buffer) {
  if (std::isnan(n)) {
    WriteStringToBuffer(buffer, "nan");
    return;
  }
  if (std::isinf(n)) {
    if (std::signbit(n)) {
      WriteStringToBuffer(buffer, "-inf");
    } else {
      WriteStringToBuffer(buffer, "inf");
    }
    return;
  }
  if (n == 0.0) {
    if (std::signbit(n)) {
      WriteStringToBuffer(buffer, "-0.0");
    } else {
      WriteStringToBuffer(buffer, "0.0");
    }
    return;
  }

  // 寻找最短的十进制表示，使其经 fast_float 再解析后能精确回到原值。
  std::array<char, 128> tmp{};
  int len = -1;
  for (int precision = 1; precision <= 17; ++precision) {
    const int written =
        std::snprintf(tmp.data(), tmp.size(), "%.*g", precision, n);
    if (written < 0 || static_cast<size_t>(written) >= tmp.size()) {
      std::fprintf(stderr, "FatalError: failed to format double\n");
      std::exit(1);
    }

    double parsed = 0.0;
    auto parsed_result =
        fast_float::from_chars(tmp.data(), tmp.data() + written, parsed,
                               fast_float::chars_format::general);
    if (parsed_result.ec == std::errc() &&
        parsed_result.ptr == tmp.data() + written && parsed == n) {
      len = written;
      break;
    }
  }
  if (len < 0) {
    len = std::snprintf(tmp.data(), tmp.size(), "%.17g", n);
    if (len < 0 || static_cast<size_t>(len) >= tmp.size()) {
      std::fprintf(stderr, "FatalError: failed to format double\n");
      std::exit(1);
    }
  }
  std::string_view s(tmp.data(), static_cast<size_t>(len));

  DecimalDigits rep = ParseToDecimalDigits(s);
  const int exp10 = rep.decimal_index - 1;

  std::string out;
  // 与常见语言行为对齐：指数过大/过小使用科学计数法，其它使用 fixed。
  if (exp10 < -4 || exp10 >= 16) {
    out = FormatScientific(rep);
  } else {
    out = FormatFixed(rep);
  }

  WriteStringToBuffer(buffer, out);
}

void Int64ToStringView(int64_t n, std::string_view buffer) {
  char* out = const_cast<char*>(buffer.data());
  if (buffer.size() < 2) {
    std::fprintf(stderr, "FatalError: number conversion buffer too small\n");
    std::exit(1);
  }
  // 预留 1 字节写入 '\\0'，以便 buffer 始终可作为 C 字符串使用。
  auto result = std::to_chars(out, out + buffer.size() - 1, n);
  if (result.ec != std::errc()) {
    std::fprintf(stderr, "FatalError: failed to format int64\n");
    std::exit(1);
  }
  *result.ptr = '\0';
}

}  // namespace saauso::internal
