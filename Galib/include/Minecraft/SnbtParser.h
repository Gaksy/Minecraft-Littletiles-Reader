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

#ifndef GALIB_MINECRAFT_SNBTPARSER_H
#define GALIB_MINECRAFT_SNBTPARSER_H

#include <cstdint>
#include <map>
#include <string>
#include <vector>

#include "GalibNamespaceDef.h"

namespace galib::minecraft::snbt {

// An SNBT value (Minecraft's textual NBT dialect).
//
// Only the parts that LittleTiles structure exports actually use need to be
// covered: compound, list, typed arrays (`[I;...]`), strings, integers, floats.
// Type suffixes (`1b`/`1.0f`/`2d`) are normalized to integers or floats during
// parsing and the original type is not preserved - there is currently no case
// that needs to distinguish byte/short/int.
class Value {
 public:
  enum class Kind {
    kInt,       // includes byte/short/int/long and their suffixes
    kFloat,     // float/double
    kString,    // quoted or unquoted string
    kList,      // plain list, or typed array (see array_type)
    kCompound,  // { key:value, ... }
  };

  Value() = default;

  [[nodiscard]] Kind kind() const { return kind_; }
  [[nodiscard]] bool is_int() const { return kind_ == Kind::kInt; }
  [[nodiscard]] bool is_string() const { return kind_ == Kind::kString; }
  [[nodiscard]] bool is_list() const { return kind_ == Kind::kList; }
  [[nodiscard]] bool is_compound() const { return kind_ == Kind::kCompound; }

  // Marker of a typed array: 'I' / 'B' / 'L'; '\0' for a plain list
  [[nodiscard]] char array_type() const { return array_type_; }

  [[nodiscard]] std::int64_t as_int() const { return integer_; }
  [[nodiscard]] double as_double() const { return real_; }
  [[nodiscard]] const std::string& as_string() const { return text_; }
  [[nodiscard]] const std::vector<Value>& items() const { return items_; }
  [[nodiscard]] const std::map<std::string, Value>& members() const {
    return members_;
  }

  [[nodiscard]] bool has_member(const std::string& kKey) const {
    return members_.find(kKey) != members_.end();
  }
  // Get a member of a compound tag; throws std::out_of_range when the key does
  // not exist (consistent with std::map::at)
  [[nodiscard]] const Value& member(const std::string& kKey) const {
    return members_.at(kKey);
  }
  // Get the kIndex-th element of a list/array; throws std::out_of_range when out of bounds
  [[nodiscard]] const Value& item(std::size_t kIndex) const {
    return items_.at(kIndex);
  }
  [[nodiscard]] std::size_t size() const {
    return kind_ == Kind::kCompound ? members_.size() : items_.size();
  }
  // Take every element of a list/array as an integer (used for `[I;...]`)
  [[nodiscard]] std::vector<std::int64_t> AsIntArray() const;

 private:
  friend class Parser;

  Kind kind_{Kind::kInt};
  char array_type_{'\0'};
  std::int64_t integer_{0};
  double real_{0.0};
  std::string text_;
  std::vector<Value> items_;
  std::map<std::string, Value> members_;
};

// Parse a piece of SNBT text; throws std::runtime_error on failure with the error
// position in the message (a structure exported by LittleTiles is a **single-line**
// large file, and without position information it is nearly impossible to debug).
[[nodiscard]] Value Parse(const std::string& kText);

// Read from a file and parse (the whole file is read into memory; a measured
// 800 KB house structure takes about 0.1 s).
[[nodiscard]] Value ParseFile(const std::string& kPath);

}  // namespace galib::minecraft::snbt

#endif  // GALIB_MINECRAFT_SNBTPARSER_H
