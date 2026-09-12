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

// SNBT 值（Minecraft 的文本 NBT 方言）。
//
// 只需要覆盖 LittleTiles 结构导出真正用到的部分：compound、list、类型化数组
// （`[I;...]`）、字符串、整数、浮点。类型后缀（`1b`/`1.0f`/`2d`）在解析时归一化
// 成整数或浮点，不再保留原始类型——目前没有需要区分 byte/short/int 的场景。
class Value {
 public:
  enum class Kind {
    kInt,       // 含 byte/short/int/long 及其后缀
    kFloat,     // float/double
    kString,    // 带引号或不带引号的字符串
    kList,      // 普通列表，或类型化数组（看 array_type）
    kCompound,  // { key:value, ... }
  };

  Value() = default;

  [[nodiscard]] Kind kind() const { return kind_; }
  [[nodiscard]] bool is_int() const { return kind_ == Kind::kInt; }
  [[nodiscard]] bool is_string() const { return kind_ == Kind::kString; }
  [[nodiscard]] bool is_list() const { return kind_ == Kind::kList; }
  [[nodiscard]] bool is_compound() const { return kind_ == Kind::kCompound; }

  // 类型化数组的标记：'I' / 'B' / 'L'；普通列表为 '\0'
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
  // 取复合标签的成员；键不存在时抛 std::out_of_range（与 std::map::at 一致）
  [[nodiscard]] const Value& member(const std::string& kKey) const {
    return members_.at(kKey);
  }
  // 取列表/数组的第 kIndex 个元素；越界抛 std::out_of_range
  [[nodiscard]] const Value& item(std::size_t kIndex) const {
    return items_.at(kIndex);
  }
  [[nodiscard]] std::size_t size() const {
    return kind_ == Kind::kCompound ? members_.size() : items_.size();
  }
  // 列表/数组里每个元素都当整数取（`[I;...]` 用）
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

// 解析一段 SNBT 文本；失败时抛 std::runtime_error，消息里带出错位置
// （LittleTiles 导出的结构是**单行**大文件，没有位置信息几乎无法排查）。
[[nodiscard]] Value Parse(const std::string& kText);

// 从文件读入并解析（整份读进内存；实测 800 KB 的房屋结构约 0.1 s）。
[[nodiscard]] Value ParseFile(const std::string& kPath);

}  // namespace galib::minecraft::snbt

#endif  // GALIB_MINECRAFT_SNBTPARSER_H
