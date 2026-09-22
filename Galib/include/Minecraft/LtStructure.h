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

#ifndef GALIB_MINECRAFT_LTSTRUCTURE_H
#define GALIB_MINECRAFT_LTSTRUCTURE_H

#include <cstddef>
#include <cstdint>
#include <functional>
#include <map>
#include <string>
#include <vector>

#include "GalibNamespaceDef.h"
#include "Minecraft/LittleTiles.h"
#include "Minecraft/MinecraftCoord.h"
#include "Minecraft/SnbtParser.h"

namespace galib::minecraft::littletiles {

// A LittleTiles "structure" (a whole building copied out in-game), coming from
// SNBT text.
//
// There are two dialects of that text, and both are still in use:
//
//   kModern20  Minecraft 1.16+ (1.20 / 1.21, LittleTiles 1.6+)
//              {boxes:4892,min:[I;9,0,6],s:{id:"fixed"},size:[I;173,63,52],
//               t:{"minecraft:oak_log[axis=y]":[[I;-1],[I;0,0,0,16,16,16]]},
//               tiles:1, c:[...]}
//              The tiles are a map: block state -> a stream of int arrays, where
//              a 1-element array is a colour marker for the boxes after it.
//
//   kLegacy12  Minecraft 1.12.2 (LittleTiles 1.5.x)
//              {tiles:[{boxes:[[I;0,0,0,16,16,16]],tile:{block:"minecraft:wool:5",
//               color:-1}}],grid:16,count:1,structure:{id:"fixed",name:"..."}}
//              One list entry per (block, colour) pair; the block name carries
//              the 1.12.2 metadata as a third colon-separated field.
//
// Both use the same box encoding as save files (`box`/`boxes` plus the
// angle-offset bit field, see DecodeBoxAngleData) and the same coordinate space:
// structure-space grid coordinates that span many blocks. So TileEntity is
// reused, and meshing only has to scale by the grid of the level a group came
// from and subtract the structure origin.
//
// A structure may embed child structures (children / c: doors, lamps, particle
// emitters, ...). Each level carries its own grid (LittleTiles stores the grid
// per level and only writes it when it is not the default 16), so the grid is
// kept per group. Only geometry is taken from children: their behavioural
// parameters (animations, open/closed state, ...) are carried through a
// conversion untouched, but are not interpreted here.
class LtStructure {
 public:
  enum class Dialect {
    kLegacy12,  // Minecraft 1.12.2 / LittleTiles 1.5.x
    kModern20,  // Minecraft 1.16+ (1.20 / 1.21) / LittleTiles 1.6+
  };

  // A group of boxes under one material (block name + optional tint).
  struct Group {
    // Dialect-native block name. In kLegacy12 this may carry the metadata
    // suffix ("minecraft:wool:5"), in kModern20 a block state
    // ("minecraft:oak_log[axis=y]").
    std::string block_id;
    std::int32_t color{0};
    bool has_color{false};
    // Grid of the level this group was read from (a child structure may use a
    // finer or coarser grid than its parent).
    int grid{16};
    std::vector<TileEntity> boxes;
    // kLegacy12 only: the remaining members of the tile compound (`tID`,
    // `invisible`, ...), kept verbatim so a conversion does not lose them.
    snbt::Value tile_extra;
    bool has_tile_extra{false};
  };

  // One level of the structure: the root, or one child structure.
  struct Node {
    std::vector<Group> groups;
    std::vector<Node> children;
    // Grid of this level (1 block = grid units here); 16 when the source did not
    // say, which is LittleTiles' default.
    int grid{16};
    bool has_grid{false};
    // Origin of the structure in grid units. Only the root usually has it;
    // meshing subtracts it so the model lands near the origin.
    std::vector<std::int32_t> min;
    bool has_min{false};
    std::vector<std::int32_t> size;
    bool has_size{false};
    // Structure metadata (legacy `structure` / modern `s`): id, name, door
    // animations, message text, ... Kept as parsed, only the container key is
    // renamed on conversion.
    snbt::Value structure;
    bool has_structure{false};
    // Members the reader does not model (legacy `pos`, modern `e`, anything
    // unknown). Carried through a conversion unchanged.
    std::map<std::string, snbt::Value> extra;
    // LittleTiles' `trans` flag (the structure contains translucent blocks).
    bool translucent{false};
  };

  struct ConvertOptions {
    // Translate block names between the two generations (1.12.2 "minecraft:wool:5"
    // <-> 1.20 "minecraft:lime_wool"). Off means "keep the names as they are",
    // which is only useful to diff two conversions.
    bool map_block_names{true};
    // LittleTiles writes `pos` (a list of block positions) for big levels so that
    // the inventory can render a low resolution box model. Recompute it instead
    // of carrying the source's stale one.
    bool recompute_low_resolution_pos{true};
  };

  // What a conversion changed, so that a host can tell the user what to check.
  struct ConvertReport {
    std::size_t groups{0};
    std::size_t boxes{0};
    std::size_t levels{0};
    // Block names found in the table and translated.
    std::size_t renamed_blocks{0};
    // Block names with no counterpart in the target version, kept as-is (minus
    // metadata / block states). The map counts occurrences per name.
    std::map<std::string, std::size_t> unmapped_blocks;
    // Structure type ids translated (legacy door -> modern axis, ...).
    std::map<std::string, std::string> renamed_structures;
    // Things the converter cannot translate for the user.
    std::vector<std::string> notes;
  };

  LtStructure() = default;

  // Detect which dialect a parsed structure is written in. Throws
  // std::runtime_error when the root is not a LittleTiles structure at all.
  [[nodiscard]] static Dialect DetectDialect(const snbt::Value& kRoot);

  // The explicit overload is there for a host that already knows the version (or
  // wants to force the reading of a file whose dialect detection is ambiguous).
  [[nodiscard]] static LtStructure FromSnbt(const snbt::Value& kRoot,
                                            Dialect kDialect);
  [[nodiscard]] static LtStructure FromSnbt(const snbt::Value& kRoot) {
    return FromSnbt(kRoot, DetectDialect(kRoot));
  }
  [[nodiscard]] static LtStructure FromSnbtFile(const std::string& kPath);
  [[nodiscard]] static LtStructure FromSnbtFile(const std::string& kPath,
                                                Dialect kDialect);

  // Serialize back to SNBT text. kTarget may be the same dialect the structure
  // was read in (a normalizing round-trip) or the other one (a conversion);
  // block names are translated unless the option says otherwise.
  //
  // The "no options" shorthand is a separate overload on purpose: a default
  // argument that value-initialises the nested ConvertOptions (either `{}` or
  // `ConvertOptions()`) is rejected by GCC with "default member initializer ...
  // required before the end of its enclosing class", because ConvertOptions
  // carries default member initializers and the enclosing class is still
  // incomplete there. Inside a member function body that restriction does not
  // apply, so the overload below (a complete-class context) is portable.
  [[nodiscard]] std::string ToSnbt(Dialect kTarget,
                                   const ConvertOptions& kOptions,
                                   ConvertReport* p_desc_report = nullptr) const;
  [[nodiscard]] std::string ToSnbt(Dialect kTarget) const {
    return ToSnbt(kTarget, ConvertOptions{}, nullptr);
  }

  [[nodiscard]] Dialect dialect() const { return dialect_; }
  [[nodiscard]] const Node& root() const { return root_; }
  // Root grid (1 block = grid() units); a child group may use its own.
  [[nodiscard]] int grid() const { return root_.grid; }
  // Root origin in grid units (zero when the source carried no `min`).
  [[nodiscard]] const BlockCoordinate& min() const { return min_; }
  [[nodiscard]] const BlockCoordinate& size() const { return size_; }
  [[nodiscard]] const std::string& name() const { return name_; }
  // Total number of boxes / tile groups over all levels.
  [[nodiscard]] std::size_t BoxCount() const;
  [[nodiscard]] std::size_t TileCount() const;
  // Number of child structures over all levels (the root's own children plus
  // their descendants).
  [[nodiscard]] int child_group_count() const { return child_group_count_; }
  // Number of child levels that use a different grid than their parent (they are
  // meshed with their own grid; a non-zero count used to be mis-scaled).
  [[nodiscard]] int child_grid_count() const { return child_grid_count_; }
  // Visits every group of every level, depth first, in file order.
  void VisitGroups(const std::function<void(const Group&)>& kVisitor) const;

  // The block names a conversion could not translate (empty when everything was
  // either mapped or already valid in the target version). Used by the CLI to
  // warn without running a conversion.
  [[nodiscard]] std::vector<std::string> UnmappedBlockNames(
      Dialect kTarget) const;

 private:
  void ReadNode(const snbt::Value& kValue, Node* p_desc_node);
  void ReadLegacyGroups(const snbt::Value& kValue, Node* p_desc_node);
  void ReadModernGroups(const snbt::Value& kValue, Node* p_desc_node);

  Dialect dialect_{Dialect::kLegacy12};
  Node root_;
  // Root origin / size in grid units, mirrored for the convenience accessors.
  BlockCoordinate min_{};
  BlockCoordinate size_{};
  std::string name_;
  int child_group_count_{0};
  int child_grid_count_{0};
};

// Human readable name of a dialect ("1.12.2" / "1.20"), for messages and file
// names.
[[nodiscard]] const char* DialectName(LtStructure::Dialect kDialect);

// Parse a dialect name typed by a user or written in a job file. Accepts
// "1.12.2", "1.12", "legacy", "old" and "1.20", "1.21", "modern", "new".
[[nodiscard]] bool ParseDialect(const std::string& kText,
                                LtStructure::Dialect* p_desc_out);

}  // namespace galib::minecraft::littletiles

#endif  // GALIB_MINECRAFT_LTSTRUCTURE_H
