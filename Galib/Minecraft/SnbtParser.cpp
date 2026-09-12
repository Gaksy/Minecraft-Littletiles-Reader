/*
 * Copyright (c) 2024 Gaksy (Fuhongren)
 *
 * This work is licensed under the GNU Lesser General Public License v3.0.
 * You may obtain a copy of the license at https://www.gnu.org/licenses/lgpl-3.0.html.
 *
 * This source code form is subject to the terms of the LGPL v3.0 license.
 * If a copy of the LGPL was not distributed with this file, you can obtain one
 * at the above license URL.
 */

/*
 * Author: Gaksy
 * Date Created: 09/13/2026
 */

#include "Minecraft/SnbtParser.h"

#include <cctype>
#include <fstream>
#include <sstream>
#include <stdexcept>

namespace galib::minecraft::snbt {

namespace {

bool IsUnquotedChar(const char kCh) {
  return std::isalnum(static_cast<unsigned char>(kCh)) != 0 || kCh == '_' ||
         kCh == '.' || kCh == '+' || kCh == '-';
}

}  // namespace

std::vector<std::int64_t> Value::AsIntArray() const {
  std::vector<std::int64_t> result;
  result.reserve(items_.size());
  for (const Value& value : items_) {
    result.push_back(value.as_int());
  }
  return result;
}

// 递归下降解析器。LittleTiles 的结构文件是单行巨型文本，
// 出错时把位置和上下文一起报出来，否则根本没法查。
class Parser {
 public:
  explicit Parser(const std::string& kText) : text_(kText) {}

  Value ParseDocument() {
    Value value = ParseValue();
    SkipSpaces();
    if (position_ != text_.size()) {
      Fail("文档末尾有多余内容");
    }
    return value;
  }

 private:
  [[noreturn]] void Fail(const std::string& kWhat) const {
    const std::size_t begin = position_ > 60 ? position_ - 60 : 0;
    const std::size_t length =
        (position_ + 60 < text_.size() ? position_ + 60 : text_.size()) - begin;
    std::ostringstream message;
    message << "SNBT 解析失败（位置 " << position_ << "）：" << kWhat << "\n"
            << "  上下文: ..." << text_.substr(begin, length) << "...";
    throw std::runtime_error(message.str());
  }

  void SkipSpaces() {
    while (position_ < text_.size() &&
           std::isspace(static_cast<unsigned char>(text_[position_])) != 0) {
      ++position_;
    }
  }

  char Peek() {
    SkipSpaces();
    return position_ < text_.size() ? text_[position_] : '\0';
  }

  void Expect(const char kCh) {
    SkipSpaces();
    if (position_ >= text_.size() || text_[position_] != kCh) {
      Fail(std::string("期望 '") + kCh + "'");
    }
    ++position_;
  }

  Value ParseValue() {
    const char ch = Peek();
    switch (ch) {
      case '{':
        return ParseCompound();
      case '[':
        return ParseListOrArray();
      case '"':
        return ParseQuotedString();
      case '\0':
        Fail("内容意外结束");
      default:
        break;
    }
    if (ch == '-' || ch == '+' ||
        std::isdigit(static_cast<unsigned char>(ch))) {
      return ParseNumber();
    }
    return ParseUnquotedString();
  }

  Value ParseCompound() {
    Value result;
    result.kind_ = Value::Kind::kCompound;
    Expect('{');
    if (Peek() == '}') {
      ++position_;
      return result;
    }
    while (true) {
      const std::string key = Peek() == '"' ? ParseQuotedString().as_string()
                                            : ParseUnquotedString().as_string();
      Expect(':');
      result.members_[key] = ParseValue();
      const char next = Peek();
      if (next == ',') {
        ++position_;
        continue;
      }
      if (next == '}') {
        ++position_;
        return result;
      }
      Fail("复合标签里期望 ',' 或 '}'");
    }
  }

  Value ParseListOrArray() {
    Value result;
    result.kind_ = Value::Kind::kList;
    Expect('[');
    SkipSpaces();
    // 类型化数组：[I;...] / [B;...] / [L;...]
    if (position_ + 1 < text_.size() &&
        (text_[position_] == 'I' || text_[position_] == 'B' ||
         text_[position_] == 'L') &&
        text_[position_ + 1] == ';') {
      result.array_type_ = text_[position_];
      position_ += 2;
      if (Peek() == ']') {  // 空数组：[I;]
        ++position_;
        return result;
      }
      while (true) {
        result.items_.push_back(ParseNumber());
        const char next = Peek();
        if (next == ',') {
          ++position_;
          continue;
        }
        if (next == ']') {
          ++position_;
          return result;
        }
        Fail("数组里期望 ',' 或 ']'");
      }
    }
    if (Peek() == ']') {
      ++position_;
      return result;
    }
    while (true) {
      result.items_.push_back(ParseValue());
      const char next = Peek();
      if (next == ',') {
        ++position_;
        continue;
      }
      if (next == ']') {
        ++position_;
        return result;
      }
      Fail("列表里期望 ',' 或 ']'");
    }
  }

  Value ParseQuotedString() {
    Value result;
    result.kind_ = Value::Kind::kString;
    Expect('"');
    std::string text;
    while (true) {
      if (position_ >= text_.size()) {
        Fail("字符串没有结束的引号");
      }
      const char ch = text_[position_++];
      if (ch == '"') {
        break;
      }
      if (ch != '\\') {
        text.push_back(ch);
        continue;
      }
      if (position_ >= text_.size()) {
        Fail("转义符后面没有内容");
      }
      const char escaped = text_[position_++];
      switch (escaped) {
        case 'n':
          text.push_back('\n');
          break;
        case 't':
          text.push_back('\t');
          break;
        case 'r':
          text.push_back('\r');
          break;
        default:
          text.push_back(escaped);  // \" \\ \' 等原样保留
          break;
      }
    }
    result.text_ = text;
    return result;
  }

  Value ParseUnquotedString() {
    SkipSpaces();
    const std::size_t begin = position_;
    while (position_ < text_.size() && IsUnquotedChar(text_[position_])) {
      ++position_;
    }
    if (begin == position_) {
      Fail("这里需要一个值");
    }
    Value result;
    result.kind_ = Value::Kind::kString;
    result.text_ = text_.substr(begin, position_ - begin);
    return result;
  }

  Value ParseNumber() {
    SkipSpaces();
    const std::size_t begin = position_;
    if (position_ < text_.size() &&
        (text_[position_] == '+' || text_[position_] == '-')) {
      ++position_;
    }
    bool is_real = false;
    while (position_ < text_.size()) {
      const char ch = text_[position_];
      if (std::isdigit(static_cast<unsigned char>(ch)) != 0) {
        ++position_;
        continue;
      }
      if (ch == '.' && !is_real) {
        is_real = true;
        ++position_;
        continue;
      }
      break;
    }
    const std::string digits = text_.substr(begin, position_ - begin);
    if (digits.empty() || digits == "-" || digits == "+") {
      Fail("数字格式不对");
    }
    // 类型后缀：b/B s/S l/L → 整型；f/F d/D → 浮点
    char suffix = '\0';
    if (position_ < text_.size()) {
      const char ch = text_[position_];
      if (ch == 'b' || ch == 'B' || ch == 's' || ch == 'S' || ch == 'l' ||
          ch == 'L' || ch == 'f' || ch == 'F' || ch == 'd' || ch == 'D') {
        suffix = ch;
        ++position_;
      }
    }
    Value result;
    if (is_real || suffix == 'f' || suffix == 'F' || suffix == 'd' ||
        suffix == 'D') {
      result.kind_ = Value::Kind::kFloat;
      result.real_ = std::stod(digits);
      result.integer_ = static_cast<std::int64_t>(result.real_);
    } else {
      result.kind_ = Value::Kind::kInt;
      result.integer_ = std::stoll(digits);
      result.real_ = static_cast<double>(result.integer_);
    }
    result.text_ = digits;
    return result;
  }

  const std::string& text_;
  std::size_t position_{0};
};

Value Parse(const std::string& kText) {
  Parser parser(kText);
  return parser.ParseDocument();
}

Value ParseFile(const std::string& kPath) {
  std::ifstream input(kPath, std::ios::binary);
  if (!input) {
    throw std::runtime_error("无法打开 SNBT 文件: " + kPath);
  }
  std::ostringstream buffer;
  buffer << input.rdbuf();
  return Parse(buffer.str());
}

}  // namespace galib::minecraft::snbt
