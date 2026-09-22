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

// Tests for the LittleTiles structure reader / writer: the two SNBT dialects and
// the conversion between them.
//
// Run with ctest, or directly:
//   ./lt_structure_test [<a real structure file>]
// A real structure (1.12.2 house / 1.20 blueprint) passed as the first argument
// additionally gets a full "read, convert, convert back, compare every box"
// round trip.

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#include "File/Utf8Path.h"
#include "Minecraft/BlockStateMap.h"
#include "Minecraft/LittleTiles.h"
#include "Minecraft/LtStructure.h"
#include "Minecraft/SnbtParser.h"
#include "Minecraft/TextureSupport/BlockTextureTable.h"

namespace {

using galib::minecraft::BlockStateMap;
using galib::minecraft::littletiles::DialectName;
using galib::minecraft::littletiles::EncodeBoxArray;
using galib::minecraft::littletiles::LtStructure;
using galib::minecraft::littletiles::ParseDialect;

int g_checks = 0;
int g_failures = 0;

void Check(const bool kOk, const std::string& kWhat) {
  ++g_checks;
  if (kOk) {
    std::printf("ok   %s\n", kWhat.c_str());
    return;
  }
  ++g_failures;
  std::printf("FAIL %s\n", kWhat.c_str());
}

void CheckEquals(const std::string& kActual, const std::string& kExpected,
                 const std::string& kWhat) {
  Check(kActual == kExpected,
        kWhat + " (got '" + kActual + "', expected '" + kExpected + "')");
}

void CheckEquals(const std::size_t kActual, const std::size_t kExpected,
                 const std::string& kWhat) {
  Check(kActual == kExpected, kWhat + " (got " + std::to_string(kActual) +
                                  ", expected " + std::to_string(kExpected) +
                                  ")");
}

std::string DataPath(const std::string& kName) {
  return std::string(TEST_DATA_DIR) + "/" + kName;
}

// Every box of a structure as one string (block, colour, encoded box). Groups
// are not compared as a whole: a conversion may merge or split groups of the
// same block without changing the tiles, and the box order is the part that must
// survive.
std::vector<std::string> BoxKeys(const LtStructure& kStructure) {
  std::vector<std::string> keys;
  kStructure.VisitGroups([&keys](const LtStructure::Group& kGroup) {
    for (const auto& box : kGroup.boxes) {
      // A colour of -1 is white to LittleTiles (LittleElement.isColored() is
      // `color != ColorUtils.WHITE`, and WHITE is 0xFFFFFFFF). An explicit
      // "color:-1" and "no colour at all" are therefore the same tile, and the
      // conversion is allowed to normalize one into the other - exactly what
      // LittleTiles' own OldLittleTilesDataParser does.
      const bool is_colored = kGroup.has_color && kGroup.color != -1;
      std::string key = kGroup.block_id + "|" +
                        (is_colored ? std::to_string(kGroup.color) : "-");
      for (const std::int32_t value : EncodeBoxArray(box)) {
        key += "," + std::to_string(value);
      }
      keys.push_back(key);
    }
  });
  std::sort(keys.begin(), keys.end());
  return keys;
}

std::size_t CountOccurrences(const std::string& kText,
                             const std::string& kNeedle) {
  std::size_t count = 0;
  for (std::size_t position = kText.find(kNeedle);
       position != std::string::npos;
       position = kText.find(kNeedle, position + kNeedle.size())) {
    ++count;
  }
  return count;
}

// Compares two box key sets and prints what differs, so that a failure points at
// the box instead of only at the count.
void CheckSameBoxes(const std::vector<std::string>& kActual,
                    const std::vector<std::string>& kExpected,
                    const std::string& kWhat) {
  if (kActual == kExpected) {
    Check(true, kWhat);
    return;
  }
  Check(false, kWhat + " (" + std::to_string(kActual.size()) + " vs " +
                   std::to_string(kExpected.size()) + " boxes)");
  std::size_t printed = 0;
  for (std::size_t i = 0; i < std::max(kActual.size(), kExpected.size()); ++i) {
    const std::string actual = i < kActual.size() ? kActual[i] : "(missing)";
    const std::string expected =
        i < kExpected.size() ? kExpected[i] : "(missing)";
    if (actual == expected) {
      continue;
    }
    std::printf("     actual:   %s\n     expected: %s\n", actual.c_str(),
                expected.c_str());
    if (++printed >= 5) {
      break;
    }
  }
}

void TestDialectDetection() {
  const LtStructure legacy =
      LtStructure::FromSnbtFile(DataPath("legacy_sample.txt"));
  Check(legacy.dialect() == LtStructure::Dialect::kLegacy12,
        "1.12.2 sample is detected as the legacy dialect");

  const LtStructure modern =
      LtStructure::FromSnbtFile(DataPath("modern_sample.txt"));
  Check(modern.dialect() == LtStructure::Dialect::kModern20,
        "1.20 sample is detected as the modern dialect");

  const galib::minecraft::snbt::Value not_a_structure =
      galib::minecraft::snbt::Parse("{foo:1}");
  bool threw = false;
  try {
    (void)LtStructure::DetectDialect(not_a_structure);
  } catch (const std::exception&) {
    threw = true;
  }
  Check(threw, "a document that is not a structure is rejected");
}

void TestLegacyParsing() {
  const LtStructure structure =
      LtStructure::FromSnbtFile(DataPath("legacy_sample.txt"));
  CheckEquals(structure.BoxCount(), 7, "legacy: box count");
  CheckEquals(structure.TileCount(), 6, "legacy: tile group count");
  CheckEquals(static_cast<std::size_t>(structure.child_group_count()), 2,
              "legacy: child structures");
  CheckEquals(static_cast<std::size_t>(structure.grid()), 32, "legacy: grid");
  CheckEquals(structure.min().x, 30, "legacy: min x");
  CheckEquals(structure.min().z, 28, "legacy: min z");
  CheckEquals(structure.size().x, 64, "legacy: size x");
  CheckEquals(structure.name(), std::string("Legacy Sample"), "legacy: name");

  // `block` + `meta` is the 1.12.2 way of spelling a block with metadata.
  bool has_meta_name = false;
  bool has_tile_extra = false;
  bool has_color = false;
  structure.VisitGroups([&](const LtStructure::Group& kGroup) {
    if (kGroup.block_id == "minecraft:log:1") {
      has_meta_name = true;
    }
    if (kGroup.has_tile_extra) {
      has_tile_extra = true;
    }
    if (kGroup.block_id == "minecraft:wool:5" && kGroup.has_color) {
      has_color = kGroup.color == -13487566;
    }
  });
  Check(has_meta_name, "legacy: `meta` is folded into the block name");
  Check(has_tile_extra, "legacy: unknown tile members are kept (tID, ...)");
  Check(has_color, "legacy: the tile colour is read");
}

void TestModernParsing() {
  const LtStructure structure =
      LtStructure::FromSnbtFile(DataPath("modern_sample.txt"));
  CheckEquals(structure.BoxCount(), 7, "modern: box count");
  CheckEquals(structure.TileCount(), 6, "modern: tile group count");
  CheckEquals(static_cast<std::size_t>(structure.child_group_count()), 2,
              "modern: child structures");
  CheckEquals(static_cast<std::size_t>(structure.grid()), 32, "modern: grid");
  CheckEquals(structure.name(), std::string("Legacy Sample"), "modern: name");

  bool has_quoted_key = false;
  bool no_color_at_end = false;
  structure.VisitGroups([&](const LtStructure::Group& kGroup) {
    if (kGroup.block_id == "minecraft:spruce_log[axis=y]") {
      has_quoted_key = true;
    }
    if (kGroup.block_id == "minecraft:diorite") {
      no_color_at_end = !kGroup.has_color;
    }
  });
  Check(has_quoted_key, "modern: a quoted key keeps its block states");
  Check(no_color_at_end, "modern: the [I;-1] marker means 'no colour'");
}

void TestBoxArrayEncoding() {
  // A real transformable box from a 1.12.2 house structure: 15 entries, the
  // 7th one negative (so LittleBox.create reads it as a transformable box).
  const std::vector<std::int32_t> source = {
      30,      287,         318,     32,      288,
      320,     -2133103178, 1900386, -917408, 1834850,
      -982944, 1900386,     -917408, 1834850, -982944};
  galib::minecraft::littletiles::TileEntity tile;
  galib::minecraft::littletiles::AngleOffset offsets[8];
  galib::minecraft::littletiles::Flipped flipped;
  const bool decoded = galib::minecraft::littletiles::DecodeBoxAngleData(
      source, offsets, &flipped);
  Check(decoded, "box encoding: a transformable box is decoded");
  tile.set_pos({30, 287, 318}, {32, 288, 320});
  tile.set_offset_data(offsets);
  tile.set_flipped_data(flipped);
  const std::vector<std::int32_t> encoded = EncodeBoxArray(tile);
  Check(encoded == source, "box encoding: re-encoding reproduces the array");

  // A plain box stays a plain box, and the legacy slice form (which carries no
  // information beyond it) comes back as the plain form.
  std::vector<std::int32_t> plain = {0, 0, 0, 16, 16, 16};
  galib::minecraft::littletiles::TileEntity plain_tile;
  galib::minecraft::littletiles::AngleOffset plain_offsets[8];
  (void)galib::minecraft::littletiles::DecodeBoxAngleData(plain, plain_offsets,
                                                          &flipped);
  plain_tile.set_pos({0, 0, 0}, {16, 16, 16});
  Check(EncodeBoxArray(plain_tile) == plain,
        "box encoding: a plain box has no angle data");
}

void TestBlockStateMap() {
  const BlockStateMap& map = BlockStateMap::Instance();
  Check(map.size() > 400, "block map: the table is loaded");
  CheckEquals(map.ToModern("minecraft:wool:5"),
              std::string("minecraft:lime_wool"),
              "block map: 1.12.2 metadata becomes a flattened name");
  CheckEquals(map.ToModern("minecraft:stone:3"),
              std::string("minecraft:diorite"),
              "block map: stone:3 becomes diorite");
  CheckEquals(map.ToModern("minecraft:log:1"),
              std::string("minecraft:spruce_log[axis=y]"),
              "block map: log:1 becomes a block state");
  CheckEquals(map.ToLegacy("minecraft:oak_log[axis=y]"),
              std::string("minecraft:log"),
              "block map: a block state becomes 1.12.2 metadata");
  CheckEquals(map.ToModern("kirosblocks:colored_brick_light_block:3"),
              std::string("kirosblocks:colored_brick_light_block"),
              "block map: an unknown mod block keeps its base name");
  CheckEquals(map.ToLegacy("minecraft:bamboo_block[axis=x]"),
              std::string("minecraft:bamboo_block"),
              "block map: an unknown 1.20 block loses its block states");
  CheckEquals(map.ToLegacy("minecraft:white_wool"),
              std::string("minecraft:wool"),
              "block map: wool:0 is spelled without metadata");
}

// The texture table has to bridge two naming eras: a pack built from a 1.12.2
// resource pack still says "minecraft:stone:2" / "minecraft:silver_concrete"
// while a 1.20 save says "minecraft:polished_granite" /
// "minecraft:light_gray_concrete".
void TestTextureNameFallback() {
  namespace fs = std::filesystem;
  using galib::minecraft::texture_support::BlockFaceTextures;
  using galib::minecraft::texture_support::BlockTextureTable;
  using galib::minecraft::texture_support::FaceDirection;

  const auto row = [](const std::string& kName, const std::string& kTexture) {
    std::string line = kName;
    for (int i = 0; i < 6; ++i) {
      line += "\t" + kTexture;
    }
    return line + "\n";
  };

  const fs::path table_path =
      fs::temp_directory_path() / "galib_block_textures_test.tsv";
  {
    std::ofstream out(table_path);
    out << "# block\tdown\tup\tnorth\tsouth\twest\teast\n";
    out << row("minecraft:stone:2", "blocks/stone_granite_smooth");
    out << row("minecraft:silver_concrete", "blocks/concrete_silver");
    out << row("minecraft:silver_stained_hardened_clay",
               "blocks/hardened_clay_stained_silver");
  }

  BlockTextureTable table;
  Check(table.LoadFromTsv(galib::Utf8String(table_path)),
        "textures: the table loads");

  BlockFaceTextures textures;
  Check(table.Lookup("minecraft:stone:2", &textures) &&
            textures.Path(FaceDirection::kUp) == "blocks/stone_granite_smooth",
        "textures: an exact name wins");
  Check(table.Lookup("minecraft:polished_granite", &textures) &&
            textures.Path(FaceDirection::kUp) == "blocks/stone_granite_smooth",
        "textures: a 1.20 name falls back through the block table");
  Check(table.Lookup("minecraft:light_gray_concrete", &textures) &&
            textures.Path(FaceDirection::kUp) == "blocks/concrete_silver",
        "textures: light_gray_* falls back to silver_*");
  Check(table.Lookup("minecraft:light_gray_terracotta", &textures) &&
            textures.Path(FaceDirection::kUp) ==
                "blocks/hardened_clay_stained_silver",
        "textures: two alias rules combine (colour + terracotta rename)");
  Check(!table.Lookup("minecraft:not_a_block", &textures),
        "textures: an unknown name is still reported as missing");

  std::error_code error;
  fs::remove(table_path, error);
}

void TestLegacyToModernConversion() {
  const LtStructure legacy =
      LtStructure::FromSnbtFile(DataPath("legacy_sample.txt"));
  LtStructure::ConvertReport report;
  const std::string modern_text =
      legacy.ToSnbt(LtStructure::Dialect::kModern20, {}, &report);

  Check(CountOccurrences(modern_text, "\"minecraft:lime_wool\"") == 1,
        "conversion: the wool group is written as lime_wool");
  Check(CountOccurrences(modern_text, "\"minecraft:spruce_log[axis=y]\"") == 1,
        "conversion: the log group keeps its block state");
  Check(CountOccurrences(modern_text, "[I;-13487566]") == 1,
        "conversion: the tile colour becomes a colour marker");
  Check(CountOccurrences(modern_text, "tID:") == 0,
        "conversion: a 1.12.2 only tile key is dropped for 1.20");
  Check(CountOccurrences(modern_text, "n:\"Legacy Sample\"") == 1,
        "conversion: the structure name is renamed to `n`");
  CheckEquals(report.renamed_blocks, 5,
              "conversion: the report counts the renamed blocks");
  Check(!report.unmapped_blocks.empty(),
        "conversion: the report lists block names without a counterpart");
  // Staying in the source dialect is a re-serialization, so nothing is
  // normalized away there.
  // The needle carries its separators: "color:-1" is a prefix of
  // "color:-13487566".
  CheckEquals(CountOccurrences(legacy.ToSnbt(LtStructure::Dialect::kLegacy12),
                               "color:-1,"),
              1,
              "conversion: an explicit color:-1 survives a same dialect write");

  const LtStructure modern =
      LtStructure::FromSnbt(galib::minecraft::snbt::Parse(modern_text));
  Check(modern.dialect() == LtStructure::Dialect::kModern20,
        "conversion: the output is recognised as the modern dialect");
  CheckEquals(modern.BoxCount(), legacy.BoxCount(),
              "conversion: the box count is preserved");
  CheckEquals(modern.TileCount(), legacy.TileCount(),
              "conversion: the tile group count is preserved");

  // Same dialect in and out: a re-serialization must be stable.
  const std::string again = modern.ToSnbt(LtStructure::Dialect::kModern20);
  CheckEquals(again, modern_text, "conversion: writing is idempotent");
}

void TestRoundTrip() {
  const LtStructure legacy =
      LtStructure::FromSnbtFile(DataPath("legacy_sample.txt"));
  const std::string modern_text =
      legacy.ToSnbt(LtStructure::Dialect::kModern20);
  const std::string legacy_text =
      LtStructure::FromSnbt(galib::minecraft::snbt::Parse(modern_text))
          .ToSnbt(LtStructure::Dialect::kLegacy12);
  const LtStructure back =
      LtStructure::FromSnbt(galib::minecraft::snbt::Parse(legacy_text));
  CheckSameBoxes(BoxKeys(back), BoxKeys(legacy),
                 "round trip: 1.12.2 -> 1.20 -> 1.12.2 keeps every box");

  const LtStructure modern =
      LtStructure::FromSnbtFile(DataPath("modern_sample.txt"));
  const std::string legacy2 = modern.ToSnbt(LtStructure::Dialect::kLegacy12);
  const LtStructure modern_back =
      LtStructure::FromSnbt(galib::minecraft::snbt::Parse(
          LtStructure::FromSnbt(galib::minecraft::snbt::Parse(legacy2))
              .ToSnbt(LtStructure::Dialect::kModern20)));
  CheckSameBoxes(BoxKeys(modern_back), BoxKeys(modern),
                 "round trip: 1.20 -> 1.12.2 -> 1.20 keeps every box");
}

// Optional: a real structure file, converted out and back, with every box and
// every byte of the structure metadata compared.
void TestRealFile(const std::string& kPath) {
  const LtStructure original = LtStructure::FromSnbtFile(kPath);
  const LtStructure::Dialect other =
      original.dialect() == LtStructure::Dialect::kLegacy12
          ? LtStructure::Dialect::kModern20
          : LtStructure::Dialect::kLegacy12;
  const std::string converted = original.ToSnbt(other);
  const std::string back_text =
      LtStructure::FromSnbt(galib::minecraft::snbt::Parse(converted))
          .ToSnbt(original.dialect());
  const LtStructure back =
      LtStructure::FromSnbt(galib::minecraft::snbt::Parse(back_text));

  std::printf("real file: %s (%s dialect, %zu boxes, %zu tile groups)\n",
              kPath.c_str(), DialectName(original.dialect()),
              original.BoxCount(), original.TileCount());
  CheckEquals(back.BoxCount(), original.BoxCount(), "real file: box count");
  CheckEquals(back.TileCount(), original.TileCount(),
              "real file: tile group count");
  CheckSameBoxes(BoxKeys(back), BoxKeys(original),
                 "real file: every box survives the round trip");
}

}  // namespace

int main(const int kArgc, char** const kArgv) {
  TestDialectDetection();
  TestLegacyParsing();
  TestModernParsing();
  TestBoxArrayEncoding();
  TestBlockStateMap();
  TestTextureNameFallback();
  TestLegacyToModernConversion();
  TestRoundTrip();
  if (kArgc > 1) {
    TestRealFile(kArgv[1]);
  }
  std::printf("\n%d checks, %d failure(s)\n", g_checks, g_failures);
  return g_failures == 0 ? 0 : 1;
}
