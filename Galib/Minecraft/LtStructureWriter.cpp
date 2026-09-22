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
 * Date Created: 09/20/2026
 */

// Writing a LittleTiles structure back out as SNBT, in either dialect.
//
// The layout of both dialects was taken from the mod itself:
//
//   modern (1.16+, LittleTiles 1.6+)
//     LittleGroup.saveChild writes  s / t / grid / c / e,
//     LittleGroup.save adds          size / min / tiles / boxes / trans,
//     LittleCollection.save writes   t as block state -> [color, box, ...].
//
//   legacy (1.12.2, LittleTiles 1.5.x)
//     LittlePreview.saveChildPreviews / savePreview write
//                                    structure / tiles / count / children
//                                    (plus grid, min, size, pos at the root),
//     LittleNBTCompressionTools.writePreviews writes one `{boxes, tile}` entry
//     per (tile type, block, colour) group.
//
// so the output looks like what the game itself produces, and both 1.12.2 and
// 1.20 can load it back.

#include <array>
#include <cstdint>
#include <functional>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "Minecraft/BlockStateMap.h"
#include "Minecraft/LittleTiles.h"
#include "Minecraft/LtStructure.h"
#include "Minecraft/SnbtParser.h"

namespace galib::minecraft::littletiles {

namespace {

using snbt::Value;

// LittleTiles' default grid: a level whose grid is this one does not write the
// key at all (LittleGrid.set / LittleGridContext.set).
constexpr int kDefaultGrid = 16;

// The keys that rename when a structure crosses generations: LittleTiles'
// OldLittleTilesDataParser.convertStructureDataBase does exactly this while
// loading a 1.12.2 structure into 1.20.
struct KeyRename {
  const char* legacy;
  const char* modern;
};

constexpr KeyRename kStructureKeyRenames[] = {
    {"name", "n"},      // structure display name
    {"blocks", "b"},    // connected block positions
    {"signal", "ex"},   // external signal handlers
    {"parent", "k"},    // parent structure link
    {"children", "c"},  // child (item) list of the structure
};

// Structure type ids that LittleTiles itself renames while converting a 1.12.2
// structure (OldLittleTilesDataParser.convertStructureData). The behaviour
// payload of a door is re-encoded by the mod (animation timelines) and is only
// carried over here; the converter says so in its notes.
constexpr KeyRename kStructureTypeRenames[] = {
    {"door", "axis"},
    {"slidingDoor", "sliding"},
    {"doorActivator", "activator"},
    {"advancedDoor", "door"},
};

// LittlePreview.lowResolutionMode in 1.12.2: from this many previews on, the
// item keeps a list of block positions for a cheap inventory model.
constexpr std::size_t kLowResolutionMode = 2000;

const char* Rename(const std::string& kKey, const KeyRename* const kTable,
                   const std::size_t kCount, const bool kToModern) {
  for (std::size_t i = 0; i < kCount; ++i) {
    const char* const from = kToModern ? kTable[i].legacy : kTable[i].modern;
    if (kKey == from) {
      return kToModern ? kTable[i].modern : kTable[i].legacy;
    }
  }
  return nullptr;
}

void AppendMembers(
    const std::vector<std::pair<std::string, std::string>>& kMembers,
    std::string* const p_desc_out) {
  bool first = true;
  for (const auto& [key, value] : kMembers) {
    if (!first) {
      *p_desc_out += ',';
    }
    first = false;
    *p_desc_out += key;
    *p_desc_out += ':';
    *p_desc_out += value;
  }
}

std::string IntArrayText(const std::vector<std::int32_t>& kValues) {
  std::string text = "[I;";
  for (std::size_t i = 0; i < kValues.size(); ++i) {
    if (i != 0) {
      text += ',';
    }
    text += std::to_string(kValues[i]);
  }
  text += ']';
  return text;
}

std::string BoxText(const TileEntity& kTile) {
  return IntArrayText(EncodeBoxArray(kTile));
}

std::string NumberText(const std::int64_t kValue) {
  return std::to_string(kValue);
}

// Translate one block name for the target dialect and count what happened.
std::string ConvertBlockName(const std::string& kName,
                             const LtStructure::Dialect kTarget,
                             const LtStructure::ConvertOptions& kOptions,
                             LtStructure::ConvertReport* const p_desc_report) {
  if (!kOptions.map_block_names) {
    return kName;
  }
  const BlockStateMap& map = BlockStateMap::Instance();
  bool mapped = false;
  const std::string converted = kTarget == LtStructure::Dialect::kModern20
                                    ? map.ToModern(kName, &mapped)
                                    : map.ToLegacy(kName, &mapped);
  if (p_desc_report != nullptr) {
    if (mapped) {
      ++p_desc_report->renamed_blocks;
    } else {
      ++p_desc_report->unmapped_blocks[kName];
    }
  }
  return converted;
}

// The structure metadata of one level, with the key / type id renames applied.
// The rest of the compound (animations, message lines, ...) is passed through.
Value ConvertStructure(const Value& kStructure, const bool kToModern,
                       LtStructure::ConvertReport* const p_desc_report) {
  std::map<std::string, Value> members;
  for (const auto& [key, member] : kStructure.members()) {
    std::string renamed = key;
    if (key != "id") {
      if (const char* const replacement =
              Rename(key, kStructureKeyRenames, std::size(kStructureKeyRenames),
                     kToModern);
          replacement != nullptr) {
        renamed = replacement;
      }
    } else if (member.is_string()) {
      const std::string type_id = member.as_string();
      if (const char* const replacement =
              Rename(type_id, kStructureTypeRenames,
                     std::size(kStructureTypeRenames), kToModern);
          replacement != nullptr) {
        if (p_desc_report != nullptr) {
          p_desc_report->renamed_structures[type_id] = replacement;
        }
        members.emplace(renamed, Value::MakeString(replacement));
        continue;
      }
    }
    members.emplace(renamed, member);
  }
  return Value::MakeCompound(std::move(members));
}

std::size_t CountBoxes(const LtStructure::Node& kNode) {
  std::size_t count = 0;
  for (const LtStructure::Group& group : kNode.groups) {
    count += group.boxes.size();
  }
  for (const LtStructure::Node& child : kNode.children) {
    count += CountBoxes(child);
  }
  return count;
}

std::size_t CountTiles(const LtStructure::Node& kNode) {
  std::size_t count = kNode.groups.size();
  for (const LtStructure::Node& child : kNode.children) {
    count += CountTiles(child);
  }
  return count;
}

std::size_t LocalBoxCount(const LtStructure::Node& kNode) {
  std::size_t count = 0;
  for (const LtStructure::Group& group : kNode.groups) {
    count += group.boxes.size();
  }
  return count;
}

// A box whose coordinates are expressed in another grid: the 1.12.2 dialect
// carries one grid for the whole structure (a child inherits the root's), while
// the modern one stores the grid per level, so a level with a different grid has
// to be rescaled. Only an integer ratio is exact; anything else is reported.
bool ScaleBox(const TileEntity& kTile, const std::int32_t kRatio,
              TileEntity* const p_desc_out) {
  const auto scale = [kRatio](const double kValue, std::int16_t* const p_desc) {
    const double scaled = kValue * kRatio;
    if (scaled > 32767.0 || scaled < -32768.0) {
      return false;  // the offset no longer fits the 16 bit field
    }
    *p_desc = static_cast<std::int16_t>(scaled);
    return true;
  };

  LittleTilesCoord pos_1 = kTile.pos_1();
  LittleTilesCoord pos_2 = kTile.pos_2();
  pos_1.x *= kRatio;
  pos_1.y *= kRatio;
  pos_1.z *= kRatio;
  pos_2.x *= kRatio;
  pos_2.y *= kRatio;
  pos_2.z *= kRatio;
  *p_desc_out = kTile;
  p_desc_out->set_pos(pos_1, pos_2);

  AngleOffset offsets[8];
  for (std::size_t corner = 0; corner < 8; ++corner) {
    AngleOffset& offset = offsets[corner];
    offset = kTile.GetAngleOffset(static_cast<AngleID>(corner));
    if (!scale(offset.x_offset, &offset.x_offset) ||
        !scale(offset.y_offset, &offset.y_offset) ||
        !scale(offset.z_offset, &offset.z_offset)) {
      return false;
    }
  }
  p_desc_out->set_offset_data(offsets);
  return true;
}

// The `pos` list of a 1.12.2 preview: the block positions (grid units divided by
// the grid, LittleVec.getBlockPos) that hold at least one of this level's tiles -
// children have their own, and LittlePreview.savePreview only walks the level's
// own previews. Positions are written with integer truncation, in the order they
// occur, deduplicated, exactly like the mod does.
std::string LowResolutionPositions(const LtStructure::Node& kNode) {
  std::vector<std::array<std::int32_t, 3>> positions;
  std::set<std::array<std::int32_t, 3>> seen;
  for (const LtStructure::Group& group : kNode.groups) {
    const std::int32_t grid = static_cast<std::int32_t>(group.grid);
    for (const TileEntity& tile : group.boxes) {
      const std::array<std::int32_t, 3> position = {
          static_cast<std::int32_t>(tile.pos_1().x) / grid,
          static_cast<std::int32_t>(tile.pos_1().y) / grid,
          static_cast<std::int32_t>(tile.pos_1().z) / grid};
      if (seen.insert(position).second) {
        positions.push_back(position);
      }
    }
  }
  std::string text = "[";
  bool first = true;
  for (const std::array<std::int32_t, 3>& position : positions) {
    if (!first) {
      text += ',';
    }
    first = false;
    text += IntArrayText({position[0], position[1], position[2]});
  }
  text += ']';
  return text;
}

struct NodeWriter {
  const LtStructure::ConvertOptions& options;
  LtStructure::ConvertReport* report;
  // Grid the 1.12.2 output is written in: the root's, inherited by every child.
  std::int32_t legacy_grid{kDefaultGrid};
  // Notes already emitted, so that a structure with many levels reports a
  // problem once instead of once per level.
  std::set<std::string> notes;

  void Note(const std::string& kText) {
    if (notes.insert(kText).second) {
      report->notes.push_back(kText);
    }
  }

  // ---- modern (1.16+ / 1.20) -----------------------------------------------
  //
  // {s:{...},t:{"block":[color,box,...]},grid:...,c:[...],e:{...},
  //  min:[I;...],size:[I;...],tiles:n,boxes:n,trans:1b}
  void WriteModern(const LtStructure::Node& kNode, const bool kIsRoot,
                   std::string* const p_desc_out) {
    std::vector<std::pair<std::string, std::string>> members;
    if (kNode.has_structure) {
      members.emplace_back(
          "s", snbt::ToSnbt(ConvertStructure(kNode.structure,
                                             /*kToModern=*/true, report)));
    }
    members.emplace_back("t", ModernTileMap(kNode));
    if (kNode.grid != kDefaultGrid) {
      // The modern dialect stores the grid of every level and only skips the
      // default (LittleGroup.saveChild -> LittleGrid.set).
      members.emplace_back("grid", NumberText(kNode.grid));
    }
    if (!kNode.children.empty()) {
      std::string children = "[";
      for (std::size_t i = 0; i < kNode.children.size(); ++i) {
        if (i != 0) {
          children += ',';
        }
        WriteModern(kNode.children[i], /*kIsRoot=*/false, &children);
      }
      children += ']';
      members.emplace_back("c", std::move(children));
    }
    // A level's origin and extent are part of the data, so they are kept wherever
    // the source had them: LittleGroup.save only writes them for the root, but
    // dropping them on a child level would lose information.
    if (kNode.has_min) {
      members.emplace_back(
          "min", IntArrayText({kNode.min[0], kNode.min[1], kNode.min[2]}));
    }
    if (kNode.has_size) {
      members.emplace_back(
          "size", IntArrayText({kNode.size[0], kNode.size[1], kNode.size[2]}));
    }
    for (auto& member : ExtraMembers(kNode, /*kModern=*/true)) {
      members.push_back(std::move(member));
    }
    if (kIsRoot) {
      // The counts are what LittleGroup.save adds on top of saveChild, i.e. for
      // the whole structure only.
      members.emplace_back("tiles", NumberText(CountTiles(kNode)));
      members.emplace_back("boxes", NumberText(CountBoxes(kNode)));
      if (kNode.translucent) {
        members.emplace_back("trans", "1b");
      }
    }

    *p_desc_out += '{';
    AppendMembers(members, p_desc_out);
    *p_desc_out += '}';
  }

  // t: block state -> [color, box, color, box, ...], one key per block state and
  // one colour marker in front of every tile group (LittleCollection.save).
  [[nodiscard]] std::string ModernTileMap(const LtStructure::Node& kNode) {
    // std::map keeps the output deterministic; a conversion may also merge two
    // groups of the same block state that were separate in the source.
    std::map<std::string, std::string> by_block;
    for (const LtStructure::Group& group : kNode.groups) {
      const std::string block = ConvertBlockName(
          group.block_id, LtStructure::Dialect::kModern20, options, report);
      std::string& stream = by_block[block];
      if (!stream.empty()) {
        stream += ',';  // the previous tile group's last box
      }
      stream += IntArrayText({group.has_color ? group.color : -1});
      for (const TileEntity& tile : group.boxes) {
        stream += ',';
        stream += BoxText(tile);
      }
    }
    std::string text = "{";
    bool first = true;
    for (const auto& [block, stream] : by_block) {
      if (!first) {
        text += ',';
      }
      first = false;
      text += snbt::ToSnbt(Value::MakeString(block));
      text += ":[";
      text += stream;
      text += ']';
    }
    text += '}';
    return text;
  }

  // ---- legacy (1.12.2) -----------------------------------------------------
  //
  // {structure:{...},grid:...,min:[I;...],size:[I;...],
  //  tiles:[{boxes:[[I;...],...],tile:{block:"...",color:...}}, ...],
  //  count:n,children:[...]}
  void WriteLegacy(const LtStructure::Node& kNode, const bool kIsRoot,
                   std::string* const p_desc_out) {
    std::vector<std::pair<std::string, std::string>> members;
    if (kNode.has_structure) {
      members.emplace_back("structure",
                           snbt::ToSnbt(ConvertStructure(
                               kNode.structure, /*kToModern=*/false, report)));
    }
    if (kIsRoot && kNode.grid != kDefaultGrid) {
      // 1.12.2 writes the grid at the root only; a child inherits it
      // (LittlePreview.saveChildPreviews writes no grid and
      // LittlePreviews.getChild hands the parent context down).
      members.emplace_back("grid", NumberText(kNode.grid));
    }
    // As in the modern dialect: the root normally carries these, but a level that
    // has them keeps them (1.12.2 ignores a child's, so this is only about not
    // losing data on a round trip).
    if (kNode.has_min) {
      members.emplace_back(
          "min", IntArrayText({kNode.min[0], kNode.min[1], kNode.min[2]}));
    }
    if (kNode.has_size) {
      members.emplace_back(
          "size", IntArrayText({kNode.size[0], kNode.size[1], kNode.size[2]}));
    }
    members.emplace_back("tiles", LegacyTileList(kNode));
    // `count` is LittlePreviews.size(), i.e. the number of boxes of this level
    // (a preview holds exactly one box); children count separately.
    members.emplace_back("count", NumberText(LocalBoxCount(kNode)));
    if (kIsRoot && options.recompute_low_resolution_pos &&
        CountBoxes(kNode) >= kLowResolutionMode) {
      members.emplace_back("pos", LowResolutionPositions(kNode));
    }
    if (!kNode.children.empty()) {
      std::string children = "[";
      for (std::size_t i = 0; i < kNode.children.size(); ++i) {
        if (i != 0) {
          children += ',';
        }
        WriteLegacy(kNode.children[i], /*kIsRoot=*/false, &children);
      }
      children += ']';
      members.emplace_back("children", std::move(children));
    }
    for (auto& member : ExtraMembers(kNode, /*kModern=*/false)) {
      members.push_back(std::move(member));
    }

    *p_desc_out += '{';
    AppendMembers(members, p_desc_out);
    *p_desc_out += '}';
  }

  // One entry per group, each carrying a `boxes` list - the shape
  // LittleNBTCompressionTools.writePreviews produces.
  [[nodiscard]] std::string LegacyTileList(const LtStructure::Node& kNode) {
    std::string text = "[";
    bool first_entry = true;
    for (const LtStructure::Group& group : kNode.groups) {
      const std::string block = ConvertBlockName(
          group.block_id, LtStructure::Dialect::kLegacy12, options, report);
      std::vector<TileEntity> scaled;
      const std::vector<TileEntity>* p_boxes = &group.boxes;
      if (group.grid != legacy_grid) {
        const std::int32_t ratio =
            legacy_grid > group.grid && legacy_grid % group.grid == 0
                ? legacy_grid / group.grid
                : 0;
        bool ok = ratio > 0;
        if (ok) {
          scaled.reserve(group.boxes.size());
          for (const TileEntity& tile : group.boxes) {
            scaled.emplace_back();
            if (!ScaleBox(tile, ratio, &scaled.back())) {
              ok = false;
              break;
            }
          }
        }
        if (!ok) {
          Note("a level uses grid " + NumberText(group.grid) +
               ", which cannot be expressed in the single grid of a 1.12.2 "
               "structure; its boxes were kept as they are (in game they will "
               "be scaled wrongly)");
          scaled.clear();
        } else {
          p_boxes = &scaled;
        }
      }

      if (!first_entry) {
        text += ',';
      }
      first_entry = false;
      text += "{boxes:[";
      for (std::size_t i = 0; i < p_boxes->size(); ++i) {
        if (i != 0) {
          text += ',';
        }
        text += BoxText((*p_boxes)[i]);
      }
      text += "],tile:{block:";
      text += snbt::ToSnbt(Value::MakeString(block));
      if (group.has_color) {
        text += ",color:";
        text += NumberText(group.color);
      }
      if (group.has_tile_extra) {
        // `tID`, `invisible`, ... exactly as they were read (minus the enclosing
        // braces).
        const std::string extra = snbt::ToSnbt(group.tile_extra);
        if (extra.size() >= 2 && extra.front() == '{' && extra.back() == '}') {
          const std::string inner = extra.substr(1, extra.size() - 2);
          if (!inner.empty()) {
            text += ',';
            text += inner;
          }
        }
      }
      text += "}}";
    }
    text += ']';
    return text;
  }

  // Members of a level that are neither geometry nor structure metadata. `pos`
  // is recomputed instead of carried when asked for, because it caches the block
  // positions of the tiles.
  [[nodiscard]] std::vector<std::pair<std::string, std::string>> ExtraMembers(
      const LtStructure::Node& kNode, const bool kModern) {
    std::vector<std::pair<std::string, std::string>> members;
    for (const auto& [key, value] : kNode.extra) {
      if (key == "pos" && kModern) {
        continue;  // a 1.12.2 only low resolution cache; 1.20 has no use for it
      }
      if (key == "pos" && !kModern && options.recompute_low_resolution_pos) {
        continue;
      }
      if (key == "tiles" || key == "boxes" || key == "count") {
        continue;  // counts are recomputed, never carried
      }
      members.emplace_back(key, snbt::ToSnbt(value));
    }
    return members;
  }
};

}  // namespace

std::string LtStructure::ToSnbt(const Dialect kTarget,
                                const ConvertOptions& kOptions,
                                ConvertReport* const p_desc_report) const {
  ConvertReport local_report;
  ConvertReport* const report =
      p_desc_report != nullptr ? p_desc_report : &local_report;
  *report = ConvertReport{};
  report->groups = TileCount();
  report->boxes = BoxCount();
  report->levels = static_cast<std::size_t>(child_group_count_) + 1;

  // A round-trip in the same dialect only re-serializes: the block names are
  // already in that version's spelling.
  ConvertOptions options = kOptions;
  options.map_block_names = kOptions.map_block_names && kTarget != dialect_;

  NodeWriter writer{options, report};
  writer.legacy_grid = static_cast<std::int32_t>(root_.grid);
  std::string text;
  if (kTarget == Dialect::kModern20) {
    writer.WriteModern(root_, /*kIsRoot=*/true, &text);
  } else {
    writer.WriteLegacy(root_, /*kIsRoot=*/true, &text);
  }

  if (!report->renamed_structures.empty()) {
    std::string note = "structure type id translated: ";
    bool first = true;
    for (const auto& [from, to] : report->renamed_structures) {
      if (!first) {
        note += ", ";
      }
      first = false;
      note += from + " -> " + to;
    }
    note +=
        "; the behaviour payload of a door (animation curves, rotation) is "
        "passed through unchanged and may need attention in game";
    report->notes.push_back(std::move(note));
  }
  if (!report->unmapped_blocks.empty()) {
    report->notes.push_back(
        "block names with no counterpart in the target version keep their base "
        "name (metadata / block states dropped); in game they load as "
        "LittleTiles' missing block");
  }
  return text;
}

}  // namespace galib::minecraft::littletiles
