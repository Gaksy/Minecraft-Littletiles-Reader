#include <cstdio>
#include <functional>
#include <stdexcept>
#include <string>
#include <vector>

#include "Minecraft/ChunkBlocks.h"
#include "Minecraft/WorldProfile.h"

using namespace nbt;
using namespace galib::minecraft;
int failures = 0;
void Check(bool value, const char* name) {
  if (!value) {
    ++failures;
    std::fprintf(stderr, "FAIL: %s\n", name);
  }
}
void Reject(const std::function<void()>& action, const char* needle) {
  try {
    action();
    Check(false, needle);
  } catch (const std::exception& e) {
    Check(std::string(e.what()).find(needle) != std::string::npos, e.what());
  }
}
tag_compound Modern(int data_version = 3465) {
  return tag_compound{{"DataVersion", tag_int(data_version)},
                      {"sections", tag_list{}}};
}
tag_compound Chunk(
    tag_list palette, bool data = true,
    std::vector<std::int64_t> packed = std::vector<std::int64_t>(256, 0)) {
  tag_compound states{{"palette", tag_list(palette)}};
  if (data) states["data"] = tag_long_array(std::move(packed));
  return tag_compound{
      {"sections",
       tag_list{tag_compound{{"Y", tag_byte(-4)},
                             {"block_states", tag_compound(states)}}}}};
}
tag_compound Entry(const char* name, const char* axis = nullptr) {
  tag_compound state{{"Name", tag_string(name)}};
  if (axis) state["Properties"] = tag_compound{{"axis", tag_string(axis)}};
  return state;
}
int main() {
  Check(CanonicalBlockState("m:b[z=2,a=1]") == "m:b[a=1,z=2]",
        "canonical property order");
  Check(IsAirBlockState("minecraft:cave_air") &&
            IsAirBlockState("minecraft:void_air"),
        "all air variants");
  Check(DetectWorldProfile(Modern()).minecraft_version == "1.20.1",
        "modern profile");
  auto legacy = tag_compound{{"DataVersion", tag_int(1343)},
                             {"Level", tag_compound{{"Sections", tag_list{}}}}};
  Check(DetectWorldProfile(legacy).minecraft_version == "1.12.2",
        "legacy profile");
  Reject([&] { DetectWorldProfile(Modern(), "1.12.2"); }, "does not match");
  Reject([&] { DetectWorldProfile(Modern(9999)); },
         "unsupported chunk DataVersion");
  auto inferred = Modern();
  inferred.erase("DataVersion");
  Check(DetectWorldProfile(inferred).inferred,
        "missing metadata is inference only");
  auto conflict = Modern();
  conflict["Sections"] = tag_list{};
  Reject([&] { DetectWorldProfile(conflict); }, "layout conflict");
  ChunkBlocks blocks;
  const tag_list palette{Entry("minecraft:oak_log", "x"),
                         Entry("minecraft:oak_log", "y"),
                         Entry("minecraft:oak_log", "z")};
  std::vector<std::int64_t> data(256, 0);
  data[0] = 0x210;
  Check(blocks.ReadFromChunkLevel(Chunk(palette, true, data)), "palette read");
  Check(blocks.min_y() == -64 && blocks.size_y() == 16,
        "stored sections determine height, not the Overworld default");
  Check(blocks.BlockName(blocks.At(0, -64, 0)) == "minecraft:oak_log[axis=x]",
        "axis x retained");
  Check(blocks.BlockName(blocks.At(1, -64, 0)) == "minecraft:oak_log[axis=y]",
        "axis y retained");
  Check(blocks.BlockName(blocks.At(2, -64, 0)) == "minecraft:oak_log[axis=z]",
        "axis z retained");
  blocks.ReadFromChunkLevel(
      Chunk(tag_list{Entry("minecraft:cave_air")}, false));
  Check(blocks.At(0, -64, 0).is_air(), "cave air is not a solid cube");
  blocks.ReadFromChunkLevel(Chunk(tag_list{Entry("minecraft:stone")}, false));
  Check(blocks.BlockName(blocks.At(15, -49, 15)) == "minecraft:stone",
        "singleton palette without data");
  blocks.ReadFromChunkLevel(Chunk(tag_list{Entry("minecraft:stone")}, true, {}));
  Check(blocks.BlockName(blocks.At(15, -49, 15)) == "minecraft:stone",
        "singleton palette with an empty data array");
  Reject([&] { blocks.ReadFromChunkLevel(Chunk(palette, false)); },
         "missing block_states.data");
  Reject([&] { blocks.ReadFromChunkLevel(Chunk(palette, true, {0})); },
         "array length");
  data[0] = 15;
  Reject([&] { blocks.ReadFromChunkLevel(Chunk(palette, true, data)); },
         "index out of range");
  // 17 states need 5 bits. Index 12 starts the second long (padding must not
  // spill across words as it did in older storage layouts).
  tag_list wider;
  for (int i = 0; i < 17; ++i)
    wider.push_back(Entry(("m:block_" + std::to_string(i)).c_str()));
  data.assign(342, 0);
  data[1] = 16;
  blocks.ReadFromChunkLevel(Chunk(wider, true, data));
  Check(blocks.BlockName(blocks.At(12, -64, 0)) == "m:block_16",
        "5-bit padded packing boundary");
  std::printf("world profiles and palettes: %d failures\n", failures);
  return failures ? 1 : 0;
}
