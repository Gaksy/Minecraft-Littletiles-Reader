// Regression tests for local LittleTiles geometry in world tile entities.
#include <algorithm>
#include <cstdio>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "Minecraft/Anvil.h"
#include "Minecraft/LittleTiles.h"
#include "nbt_tags.h"

namespace {
using namespace nbt;
using galib::minecraft::littletiles::BlockTileEntities;
using galib::minecraft::littletiles::EncodeBoxArray;
int failures = 0;
int checks = 0;
void Check(bool ok, const std::string& what) {
  ++checks;
  std::printf("%s %s\n", ok ? "ok  " : "FAIL", what.c_str());
  if (!ok) ++failures;
}
tag_int_array Box(int x = 0) {
  return tag_int_array(std::vector<std::int32_t>{x, 0, 0, x + 1, 16, 16});
}
tag_compound Tile(int x = 0) {
  return tag_compound{{"block", tag_string("minecraft:leaves:4")}, {"box", Box(x)}};
}
tag_compound Collection(int x = 0) {
  return tag_compound{{"tiles", tag_list{Tile(x)}}};
}
tag_compound Root(const tag_compound& content) {
  return tag_compound{{"id", tag_string("minecraft:littletilestileentity")},
                      {"x", tag_int(959)}, {"y", tag_int(27)},
                      {"z", tag_int(-2008)}, {"content", tag_compound(content)}};
}
std::vector<std::string> Keys(const BlockTileEntities& block) {
  std::vector<std::string> keys;
  for (auto it = block.cbegin(); it != block.cend(); ++it) {
    const auto& group = *it;
    for (const auto& tile : group.second) {
      std::string key = group.first.block_id + "|" +
          (group.first.has_color ? std::to_string(group.first.color) : "-");
      for (const auto value : EncodeBoxArray(tile)) key += "," + std::to_string(value);
      keys.push_back(key);
    }
  }
  std::sort(keys.begin(), keys.end());
  return keys;
}
void TestCollections() {
  BlockTileEntities block;
  std::size_t groups = 0;
  Check(block.ReadBlockTileNbt(Root(Collection()), &groups) == 1 && groups == 1,
        "top-level-only remains unchanged");
  const auto top_keys = Keys(block);
  tag_compound child = Collection();
  child["structure"] = tag_compound{{"id", tag_string("fixed")},
      {"blocks", tag_int_array(std::vector<std::int32_t>{6,-3,3, 6,-3,3, 0,0,0})}};
  tag_compound content{{"tiles", tag_list{}}, {"children", tag_list{child}}};
  Check(block.ReadBlockTileNbt(Root(content), &groups) == 1 && groups == 1,
        "fixed child-only geometry is read once, not expanded from blocks");
  Check(Keys(block) == top_keys, "fixed metadata does not move local geometry");
  Check(block.grid() == 16 && block.block_coordinate().x == 959 &&
        block.block_coordinate().y == 27 && block.block_coordinate().z == -2008,
        "default grid and world block position are preserved");
  child.erase("structure");
  child["coord"] = tag_int_array(std::vector<std::int32_t>{6,-3,3});
  child["index"] = tag_int(0);
  child["type"] = tag_int(0);
  content["children"] = tag_list{child};
  block.ReadBlockTileNbt(Root(content));
  Check(Keys(block) == top_keys, "coord links to the main TE; it does not translate boxes");
  content["tiles"] = tag_list{Tile(1), Tile(2)};
  child["tiles"] = tag_list{Tile(3)};
  content["children"] = tag_list{child, Collection(4)};
  Check(block.ReadBlockTileNbt(Root(content), &groups) == 4 && groups == 1 &&
        block.TileCount() == 4,
        "duplicate material entries append across parent and multiple children");
  content["children"] = tag_list{};
  Check(block.ReadBlockTileNbt(Root(content)) == 2, "empty children preserve parent tiles");
  content.erase("tiles");
  Check(block.ReadBlockTileNbt(Root(content)) == 0 && block.TileCount() == 0,
        "missing tiles plus empty children is valid and clears previous geometry");
  content["children"] = tag_list{tag_compound{{"children", tag_list{Collection(5)}}}};
  Check(block.ReadBlockTileNbt(Root(content)) == 1,
        "nested child collections work without a parent tiles field");
  auto full = Tile();
  full["box"] = tag_int_array(std::vector<std::int32_t>{0,0,0,16,16,16});
  content["children"] = tag_list{tag_compound{{"tiles", tag_list{full}}}};
  block.ReadBlockTileNbt(Root(content));
  Check(block.covered_face_mask() == 63, "child geometry participates in coverage masks");
}
void TestProperties() {
  BlockTileEntities block;
  auto colored = Tile();
  colored["color"] = tag_int(-123456);
  colored["block"] = tag_string("minecraft:wool");
  colored["meta"] = tag_int(5);
  colored.erase("box");
  const std::vector<std::int32_t> angle = {30,287,318,32,288,320,-2133103178,
      1900386,-917408,1834850,-982944,1900386,-917408,1834850,-982944};
  colored["boxes"] = tag_list{Box(2), tag_int_array(std::vector<std::int32_t>(angle))};
  auto content = tag_compound{{"children", tag_list{tag_compound{
      {"tiles", tag_list{colored}}}}}};
  auto root = Root(content);
  root["grid"] = tag_int(32);
  Check(block.ReadBlockTileNbt(root) == 2 && block.grid() == 32,
        "box lists and explicit grid survive in children");
  const auto& group = *block.cbegin();
  Check(group.first.block_id == "minecraft:wool:5" && group.first.has_color &&
        group.first.color == -123456, "legacy metadata and material color survive");
  Check(EncodeBoxArray(group.second[1]) == angle,
        "corner offsets and flip flags round trip exactly in a child");
  Check(group.second[0].has_color() && group.second[0].color() == -123456,
        "child color is also applied to decoded tiles");
  auto other_color = colored;
  other_color["color"] = tag_int(-654321);
  content["tiles"] = tag_list{colored};
  content["children"] = tag_list{tag_compound{
      {"tiles", tag_list{colored, other_color}}}};
  std::size_t groups = 0;
  Check(block.ReadBlockTileNbt(Root(content), &groups) == 6 && groups == 2,
        "legacy same-color groups append while differently tinted groups stay distinct");
}
void TestModern() {
  BlockTileEntities block;
  auto modern = tag_compound{{"minecraft:stone", tag_list{
      tag_int_array(std::vector<std::int32_t>{-1}),
      tag_int_array(std::vector<std::int32_t>{123,0,0,0,8,16,16})}}};
  auto tinted = tag_compound{{"minecraft:stone", tag_list{
      tag_int_array(std::vector<std::int32_t>{-123}),
      tag_int_array(std::vector<std::int32_t>{456,8,0,0,16,16,16})}}};
  auto content = tag_compound{{"tiles", tag_compound(modern)}, {"children", tag_list{
      tag_compound{{"tiles", tag_compound(modern)}},
      tag_compound{{"tiles", tag_compound(tinted)}}}}};
  std::size_t groups = 0;
  Check(block.ReadBlockTileNbt(Root(content), &groups) == 3 && groups == 2,
        "modern parent and child streams append without an early return");
  Check(block.covered_face_mask() == 63,
        "modern face-cache prefix is stripped before decoding child geometry");
}
void TestErrors() {
  BlockTileEntities block;
  auto bad = tag_compound{{"tiles", tag_list{tag_compound{
      {"box", Box()}}}}};
  auto content = tag_compound{{"tiles", tag_int(42)}, {"children", tag_list{
      bad, Collection(1), tag_compound{{"tiles", tag_list{Tile(2)}}}}}};
  std::ostringstream errors;
  auto* previous = std::cerr.rdbuf(errors.rdbuf());
  const auto count = block.ReadBlockTileNbt(Root(content));
  std::cerr.rdbuf(previous);
  Check(count == 2 && block.TileCount() == 2,
        "malformed parent or child entries do not discard valid sibling geometry");
  Check(errors.str().find("content.tiles") != std::string::npos &&
        errors.str().find("content.children[0].tiles[0]") != std::string::npos,
        "malformed collections emit contextual stderr diagnostics");
  bool threw = false;
  try { block.ReadBlockTileNbt(tag_compound{{"id", tag_string("bad")}}); }
  catch (const std::exception& e) {
    threw = std::string(e.what()).find("Invalid block tile data") != std::string::npos;
  }
  Check(threw, "invalid block envelope still fails with the original diagnostic");
}
}  // namespace
int main() {
  try {
    TestCollections(); TestProperties(); TestModern(); TestErrors();
  } catch (const std::exception& e) {
    std::printf("UNEXPECTED: %s\n", e.what());
    return 1;
  }
  std::printf("%d checks, %d failures\n", checks, failures);
  return failures == 0 ? 0 : 1;
}
