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

// Recursive-descent parser. A LittleTiles structure file is a single-line gigantic
// text, so errors report both the position and the surrounding context; otherwise
// they would be impossible to debug.
class Parser {
 public:
  explicit Parser(const std::string& kText) : text_(kText) {}

  Value ParseDocument() {
    Value value = ParseValue();
    SkipSpaces();
    if (position_ != text_.size()) {
      Fail("trailing content after the end of the document");
    }
    return value;
  }

 private:
  [[noreturn]] void Fail(const std::string& kWhat) const {
    const std::size_t begin = position_ > 60 ? position_ - 60 : 0;
    const std::size_t length =
        (position_ + 60 < text_.size() ? position_ + 60 : text_.size()) - begin;
    std::ostringstream message;
    message << "SNBT parse failed (position " << position_ << "): " << kWhat
            << "\n"
            << "  context: ..." << text_.substr(begin, length) << "...";
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
      Fail(std::string("expected '") + kCh + "'");
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
        Fail("unexpected end of input");
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
      Fail("expected ',' or '}' in compound tag");
    }
  }

  Value ParseListOrArray() {
    Value result;
    result.kind_ = Value::Kind::kList;
    Expect('[');
    SkipSpaces();
    // Typed array: [I;...] / [B;...] / [L;...]
    if (position_ + 1 < text_.size() &&
        (text_[position_] == 'I' || text_[position_] == 'B' ||
         text_[position_] == 'L') &&
        text_[position_ + 1] == ';') {
      result.array_type_ = text_[position_];
      position_ += 2;
      if (Peek() == ']') {  // empty array: [I;]
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
        Fail("expected ',' or ']' in array");
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
      Fail("expected ',' or ']' in list");
    }
  }

  Value ParseQuotedString() {
    Value result;
    result.kind_ = Value::Kind::kString;
    Expect('"');
    std::string text;
    while (true) {
      if (position_ >= text_.size()) {
        Fail("unterminated string (no closing quote)");
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
        Fail("escape character at end of input");
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
          text.push_back(escaped);  // \" \\ \' etc. are kept as-is
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
      Fail("expected a value here");
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
      Fail("malformed number");
    }
    // Type suffix: b/B s/S l/L -> integer; f/F d/D -> float
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
    throw std::runtime_error("cannot open SNBT file: " + kPath);
  }
  std::ostringstream buffer;
  buffer << input.rdbuf();
  return Parse(buffer.str());
}

}  // namespace galib::minecraft::snbt
