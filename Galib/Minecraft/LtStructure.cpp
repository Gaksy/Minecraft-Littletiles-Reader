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

#include "Minecraft/LtStructure.h"

#include <algorithm>
#include <cctype>
#include <stdexcept>
#include <string>

#include "Minecraft/BlockStateMap.h"

namespace galib::minecraft::littletiles {

namespace {

// The fields a level may carry that this reader models itself; everything else
// is kept in Node::extra and written back out unchanged. Note that `tiles` means
// a list in the 1.12.2 dialect and a count in the modern one - both are handled.
constexpr const char* const kKnownNodeKeys[] = {
    "grid",       // grid of this level
    "min",        // origin, grid units
    "size",       // extent, grid units
    "tiles",      // kLegacy12: the tile list; kModern20: a count
    "boxes",      // kModern20: a count
    "count",      // kLegacy12: a count
    "children",   // kLegacy12: child structures
    "c",          // kModern20: child structures
    "structure",  // kLegacy12: structure metadata
    "s",          // kModern20: structure metadata
    "t",          // kModern20: the tile map
    "trans",      // kModern20: "contains translucent blocks"
};

bool IsKnownNodeKey(const std::string& kKey) {
  for (const char* const known : kKnownNodeKeys) {
    if (kKey == known) {
      return true;
    }
  }
  return false;
}

std::int32_t ReadInt(const snbt::Value& kValue) {
  return static_cast<std::int32_t>(kValue.as_int());
}

// One box array -> TileEntity (reusing the same semantics as the save-file path)
void ReadBox(const std::vector<std::int64_t>& kNumbers,
             TileEntity* const p_desc_tile) {
  if (kNumbers.size() < 6) {
    throw std::runtime_error("box array has fewer than 6 entries");
  }
  LittleTilesCoord pos_1;
  LittleTilesCoord pos_2;
  pos_1.x = static_cast<double>(kNumbers[0]);
  pos_1.y = static_cast<double>(kNumbers[1]);
  pos_1.z = static_cast<double>(kNumbers[2]);
  pos_2.x = static_cast<double>(kNumbers[3]);
  pos_2.y = static_cast<double>(kNumbers[4]);
  pos_2.z = static_cast<double>(kNumbers[5]);
  p_desc_tile->set_pos(pos_1, pos_2);

  if (kNumbers.size() <= 6) {
    return;
  }
  std::vector<std::int32_t> compact;
  compact.reserve(kNumbers.size());
  for (const std::int64_t number : kNumbers) {
    compact.push_back(static_cast<std::int32_t>(number));
  }
  AngleOffset offsets[8];
  Flipped flipped;
  if (DecodeBoxAngleData(compact, offsets, &flipped)) {
    p_desc_tile->set_offset_data(offsets);
    p_desc_tile->set_flipped_data(flipped);
  }
}

void AppendBox(const snbt::Value& kBoxValue,
               std::vector<TileEntity>* const p_desc_boxes) {
  p_desc_boxes->emplace_back();
  ReadBox(kBoxValue.AsIntArray(), &p_desc_boxes->back());
}

// `boxes` is a list of int arrays, `bBox` / `box` a single one; all three exist
// in the wild (see LittleTiles' LittleTile.loadTileCore and
// LittleNBTCompressionTools.ordinaryPreviewHandler).
void AppendBoxes(const snbt::Value& kHolder,
                 std::vector<TileEntity>* const p_desc_boxes) {
  if (!kHolder.is_list() || kHolder.array_type() != '\0') {
    AppendBox(kHolder, p_desc_boxes);
    return;
  }
  if (kHolder.items().empty()) {
    return;
  }
  // A plain list holding int arrays is the grouped form; a plain list of numbers
  // does not occur in NBT, so anything else is treated as a single box.
  if (!kHolder.item(0).is_list()) {
    AppendBox(kHolder, p_desc_boxes);
    return;
  }
  for (const snbt::Value& entry : kHolder.items()) {
    AppendBox(entry, p_desc_boxes);
  }
}

void ReadVec3(const snbt::Value& kValue, std::vector<std::int32_t>* p_desc_out,
              bool* const p_desc_has) {
  const std::vector<std::int64_t> numbers = kValue.AsIntArray();
  if (numbers.size() < 3) {
    return;
  }
  p_desc_out->clear();
  for (std::size_t i = 0; i < 3; ++i) {
    p_desc_out->push_back(static_cast<std::int32_t>(numbers[i]));
  }
  *p_desc_has = true;
}

std::string StructureName(const snbt::Value& kStructure,
                          LtStructure::Dialect kDialect) {
  // 1.12.2 writes `name`, 1.20 writes `n` (LittleTiles'
  // OldLittleTilesDataParser.convertStructureDataBase renames it).
  const char* const key =
      kDialect == LtStructure::Dialect::kLegacy12 ? "name" : "n";
  if (kStructure.has_member(key) && kStructure.member(key).is_string()) {
    return kStructure.member(key).as_string();
  }
  return {};
}

}  // namespace

const char* DialectName(const LtStructure::Dialect kDialect) {
  switch (kDialect) {
    case LtStructure::Dialect::kLegacy12:
      return "1.12.2";
    case LtStructure::Dialect::kModern20:
      return "1.20";
  }
  return "?";
}

bool ParseDialect(const std::string& kText, LtStructure::Dialect* p_desc_out) {
  // Case-insensitive compare against the spellings a user or a job file may use.
  std::string lower;
  lower.reserve(kText.size());
  for (const char ch : kText) {
    lower.push_back(
        static_cast<char>(std::tolower(static_cast<unsigned char>(ch))));
  }
  if (lower == "1.12.2" || lower == "1.12" || lower == "legacy" ||
      lower == "old") {
    *p_desc_out = LtStructure::Dialect::kLegacy12;
    return true;
  }
  if (lower == "1.20" || lower == "1.21" || lower == "1.16" ||
      lower == "modern" || lower == "new") {
    *p_desc_out = LtStructure::Dialect::kModern20;
    return true;
  }
  return false;
}

LtStructure::Dialect LtStructure::DetectDialect(const snbt::Value& kRoot) {
  if (!kRoot.is_compound()) {
    throw std::runtime_error(
        "the top level of the structure SNBT is not a compound tag");
  }
  // LittleTiles detects the version the same way: in the 1.12.2 dialect `tiles`
  // is the list of tiles, in the modern one it is a count and the tiles live in
  // the `t` map (see OldLittleTilesDataParser.isOld).
  if (kRoot.has_member("tiles")) {
    const snbt::Value& tiles = kRoot.member("tiles");
    if (tiles.is_list()) {
      return Dialect::kLegacy12;
    }
    if (tiles.is_int()) {
      return Dialect::kModern20;
    }
  }
  if (kRoot.has_member("t")) {
    return Dialect::kModern20;
  }
  if (kRoot.has_member("structure")) {
    // A structure with no tiles at all: the metadata key still tells the version.
    return Dialect::kLegacy12;
  }
  throw std::runtime_error(
      "not a LittleTiles structure: no 'tiles' list (1.12.2) and no 't' map "
      "(1.16+)");
}

LtStructure LtStructure::FromSnbt(const snbt::Value& kRoot,
                                  const Dialect kDialect) {
  if (!kRoot.is_compound()) {
    throw std::runtime_error(
        "the top level of the structure SNBT is not a compound tag");
  }
  LtStructure structure;
  structure.dialect_ = kDialect;
  structure.ReadNode(kRoot, &structure.root_);
  if (structure.root_.has_min) {
    structure.min_.x = structure.root_.min[0];
    structure.min_.y = structure.root_.min[1];
    structure.min_.z = structure.root_.min[2];
  }
  if (structure.root_.has_size) {
    structure.size_.x = structure.root_.size[0];
    structure.size_.y = structure.root_.size[1];
    structure.size_.z = structure.root_.size[2];
  }
  if (structure.root_.has_structure) {
    structure.name_ = StructureName(structure.root_.structure, kDialect);
  }
  return structure;
}

LtStructure LtStructure::FromSnbtFile(const std::string& kPath) {
  return FromSnbt(snbt::ParseFile(kPath));
}

LtStructure LtStructure::FromSnbtFile(const std::string& kPath,
                                      const Dialect kDialect) {
  return FromSnbt(snbt::ParseFile(kPath), kDialect);
}

void LtStructure::ReadNode(const snbt::Value& kValue, Node* const p_desc_node) {
  if (!kValue.is_compound()) {
    throw std::runtime_error("a structure level is not a compound tag");
  }

  // The grid is written per level and only when it is not LittleTiles' default
  // of 16 (LittleGrid.set in 1.20, LittleGridContext.set in 1.12.2).
  if (kValue.has_member("grid")) {
    const std::int32_t grid = ReadInt(kValue.member("grid"));
    if (grid > 0) {
      p_desc_node->grid = static_cast<int>(grid);
      p_desc_node->has_grid = true;
    }
  }
  if (kValue.has_member("min")) {
    ReadVec3(kValue.member("min"), &p_desc_node->min, &p_desc_node->has_min);
  }
  if (kValue.has_member("size")) {
    ReadVec3(kValue.member("size"), &p_desc_node->size, &p_desc_node->has_size);
  }

  const char* const structure_key =
      dialect_ == Dialect::kLegacy12 ? "structure" : "s";
  if (kValue.has_member(structure_key) &&
      kValue.member(structure_key).is_compound()) {
    p_desc_node->structure = kValue.member(structure_key);
    p_desc_node->has_structure = true;
  }
  if (kValue.has_member("trans")) {
    const snbt::Value& trans = kValue.member("trans");
    p_desc_node->translucent = !trans.is_int() || trans.as_int() != 0;
  }

  const char* const children_key =
      dialect_ == Dialect::kLegacy12 ? "children" : "c";
  if (kValue.has_member(children_key) &&
      kValue.member(children_key).is_list()) {
    for (const snbt::Value& child : kValue.member(children_key).items()) {
      Node node;
      // A child level inherits the parent grid in the 1.12.2 dialect: 1.12.2
      // never writes a grid for a child (LittlePreview.saveChildPreviews) and
      // hands the parent's context straight down when reading it back
      // (LittlePreviews.getChild). The modern dialect writes every level's grid
      // (LittleGroup.saveChild) and reads it per level, defaulting to 16.
      node.grid = dialect_ == Dialect::kLegacy12 ? p_desc_node->grid : 16;
      ReadNode(child, &node);
      if (dialect_ == Dialect::kLegacy12) {
        node.grid = p_desc_node->grid;
        node.has_grid = false;
      }
      if (node.grid != p_desc_node->grid) {
        ++child_grid_count_;
      }
      ++child_group_count_;
      p_desc_node->children.push_back(std::move(node));
    }
  }

  if (dialect_ == Dialect::kLegacy12) {
    ReadLegacyGroups(kValue, p_desc_node);
  } else {
    ReadModernGroups(kValue, p_desc_node);
  }

  for (const auto& [key, member] : kValue.members()) {
    if (IsKnownNodeKey(key)) {
      continue;
    }
    p_desc_node->extra.emplace(key, member);
  }
}

void LtStructure::ReadLegacyGroups(const snbt::Value& kValue,
                                   Node* const p_desc_node) {
  if (!kValue.has_member("tiles") || !kValue.member("tiles").is_list()) {
    return;
  }
  for (const snbt::Value& entry : kValue.member("tiles").items()) {
    if (!entry.is_compound()) {
      continue;
    }
    // The material of a tile entry sits in a `tile` compound; LittleTiles also
    // accepts the fields directly on the entry (LittleNBTCompressionTools.
    // collect -> createTile(tileNbt) when there is no `tile` key).
    const snbt::Value& material =
        entry.has_member("tile") ? entry.member("tile") : entry;
    if (!material.is_compound()) {
      continue;
    }

    Group group;
    group.grid = p_desc_node->grid;
    if (material.has_member("block") && material.member("block").is_string()) {
      group.block_id = material.member("block").as_string();
    }
    if (material.has_member("meta")) {
      const std::int32_t meta = ReadInt(material.member("meta"));
      if (meta != 0) {
        group.block_id += ":" + std::to_string(meta);
      }
    }
    if (material.has_member("color")) {
      group.color = ReadInt(material.member("color"));
      group.has_color = true;
    }

    // Anything else in the material compound (`tID`, `invisible`, ...) is kept
    // so that a conversion does not silently drop it.
    std::map<std::string, snbt::Value> extra;
    for (const auto& [key, member] : material.members()) {
      if (key == "block" || key == "color" || key == "meta") {
        continue;
      }
      extra.emplace(key, member);
    }
    if (!extra.empty()) {
      group.tile_extra = snbt::Value::MakeCompound(std::move(extra));
      group.has_tile_extra = true;
    }

    for (const char* const key : {"boxes", "bBox", "box"}) {
      if (entry.has_member(key)) {
        AppendBoxes(entry.member(key), &group.boxes);
      }
    }
    if (group.boxes.empty()) {
      continue;
    }
    p_desc_node->groups.push_back(std::move(group));
  }
}

void LtStructure::ReadModernGroups(const snbt::Value& kValue,
                                   Node* const p_desc_node) {
  if (!kValue.has_member("t") || !kValue.member("t").is_compound()) {
    return;
  }
  // `t` maps a block state to a stream of int arrays: every 1-element array is a
  // colour marker applying to the boxes that follow it until the next marker
  // (LittleCollection.save writes `[color]` in front of each tile).
  for (const auto& [block_state, stream] : kValue.member("t").members()) {
    if (!stream.is_list()) {
      throw std::runtime_error("t['" + block_state +
                               "'] is not a list of int arrays");
    }
    const std::size_t first_group = p_desc_node->groups.size();
    Group* p_current = nullptr;
    for (const snbt::Value& item : stream.items()) {
      const std::vector<std::int64_t> numbers = item.AsIntArray();
      if (numbers.size() == 1) {
        p_desc_node->groups.emplace_back();
        p_current = &p_desc_node->groups.back();
        p_current->block_id = block_state;
        p_current->grid = p_desc_node->grid;
        p_current->color = static_cast<std::int32_t>(numbers[0]);
        p_current->has_color = numbers[0] != -1;
        continue;
      }
      if (numbers.size() >= 6) {
        if (p_current == nullptr) {
          // LittleTiles would fail on this (it starts a tile only at a colour
          // marker); be forgiving and treat the boxes as uncoloured.
          p_desc_node->groups.emplace_back();
          p_current = &p_desc_node->groups.back();
          p_current->block_id = block_state;
          p_current->grid = p_desc_node->grid;
        }
        ReadBox(numbers, &p_current->boxes.emplace_back());
        continue;
      }
      throw std::runtime_error("t['" + block_state +
                               "'] holds an int array of " +
                               std::to_string(numbers.size()) +
                               " entries, which is neither a colour nor a box");
    }
    // A colour marker with no boxes after it produces an empty group; LittleTiles
    // drops those on load, so drop them here as well.
    while (p_desc_node->groups.size() > first_group &&
           p_desc_node->groups.back().boxes.empty()) {
      p_desc_node->groups.pop_back();
    }
  }
}

std::size_t LtStructure::BoxCount() const {
  std::size_t count = 0;
  VisitGroups([&count](const Group& kGroup) { count += kGroup.boxes.size(); });
  return count;
}

std::size_t LtStructure::TileCount() const {
  std::size_t count = 0;
  VisitGroups([&count](const Group&) { ++count; });
  return count;
}

void LtStructure::VisitGroups(
    const std::function<void(const Group&)>& kVisitor) const {
  // Depth first, file order, so that a conversion keeps the source order.
  std::function<void(const Node&)> visit_node = [&](const Node& kNode) {
    for (const Group& group : kNode.groups) {
      kVisitor(group);
    }
    for (const Node& child : kNode.children) {
      visit_node(child);
    }
  };
  visit_node(root_);
}

std::vector<std::string> LtStructure::UnmappedBlockNames(
    const Dialect kTarget) const {
  if (kTarget == dialect_) {
    return {};
  }
  const BlockStateMap& map = BlockStateMap::Instance();
  std::vector<std::string> names;
  VisitGroups([&](const Group& kGroup) {
    bool mapped = false;
    if (kTarget == Dialect::kModern20) {
      (void)map.ToModern(kGroup.block_id, &mapped);
    } else {
      (void)map.ToLegacy(kGroup.block_id, &mapped);
    }
    if (!mapped &&
        std::find(names.begin(), names.end(), kGroup.block_id) == names.end()) {
      names.push_back(kGroup.block_id);
    }
  });
  return names;
}

}  // namespace galib::minecraft::littletiles
