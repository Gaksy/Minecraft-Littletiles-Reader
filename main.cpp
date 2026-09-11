#include <cstdio>
#include <cstdlib>
#include <exception>
#include <filesystem>
#include <string>
#include <vector>

#include "Minecraft/Anvil.h"
#include "Minecraft/BlockIdTable.h"
#include "Minecraft/CgalSupport/CgalLittletilesBuilder.h"
#include "Minecraft/CgalSupport/CgalWorldBlocks.h"
#include "Minecraft/ChunkBlocks.h"
#include "Minecraft/LittleTiles.h"

using galib::minecraft::AnvilReader;
using galib::minecraft::BlockIdTable;
using galib::minecraft::ChunkBlocks;
using galib::minecraft::ChunkCoordinate;
using galib::minecraft::cgal_support::BuildWorldBlockMeshes;
using galib::minecraft::cgal_support::ChunkMesh;
using galib::minecraft::cgal_support::LtSurfaceMesh;
using galib::minecraft::cgal_support::MergeAndWriteToObj;
using galib::minecraft::cgal_support::ObjExportOptions;
using galib::minecraft::cgal_support::ObjMeshBuilder;
using galib::minecraft::littletiles::ChunkTileEntities;

using std::string;
using std::to_string;

#define OUT_OBJ_FILE_NAME "../out_file/marge_obj_from_chunk_"

namespace {

// 读取一行 y/n 回答；直接回车（空行）使用推荐默认值。
// 注意：调用前必须先清掉上一个 scanf 残留的换行符，否则会立刻读到空行。
bool askYesNo(const char* const kPQuestion, const bool kDefaultValue) {
  printf("%s (y/n) [%c]: ", kPQuestion, kDefaultValue ? 'y' : 'n');
  fflush(stdout);

  char answer[64];
  if (!fgets(answer, sizeof(answer), stdin)) {
    return kDefaultValue;
  }

  for (const char* p = answer; *p != '\0'; ++p) {
    if (*p == 'y' || *p == 'Y') {
      return true;
    }
    if (*p == 'n' || *p == 'N') {
      return false;
    }
  }
  return kDefaultValue;
}

// 素材根目录：环境变量 LITTLETILES_ASSETS 优先；否则依次尝试
// ./assets/1.12.2（从仓库根运行）与 ../assets/1.12.2（从构建目录运行）
std::string DetectAssetsRoot() {
  if (const char* const assets_env = std::getenv("LITTLETILES_ASSETS")) {
    return assets_env;
  }
  if (std::filesystem::exists("assets/1.12.2/block_textures.tsv")) {
    return "assets/1.12.2";
  }
  if (std::filesystem::exists("../assets/1.12.2/block_textures.tsv")) {
    return "../assets/1.12.2";
  }
  return {};
}

}  // namespace

int main() {
  printf("Hello, There is LittleTile Reader\n");

  printf("please key in region folder:");
  char region_folder[256];
  scanf("%255s", region_folder);  // 明确上限，避免超长路径写越界
  printf("region folder path is %s\n", region_folder);

  ChunkCoordinate::NumericType chunk_x = 0;
  ChunkCoordinate::NumericType chunk_z = 0;
  printf("chunk x: ");
  scanf("%d", &chunk_x);
  printf("chunk z: ");
  scanf("%d", &chunk_z);

  int chunk_radius = 0;
  printf("scan radius in chunks (0 = only this chunk): ");
  scanf("%d", &chunk_radius);
  if (chunk_radius < 0) {
    chunk_radius = 0;
  }

  // 丢掉上一个 scanf 残留的换行符，后面的选项按整行读取
  {
    int remaining = 0;
    while ((remaining = getchar()) != '\n' && remaining != EOF) {
    }
  }

  const bool include_world_blocks =
      askYesNo("Also export plain (non-LittleTiles) blocks?", true);
  const bool cull_hidden_faces =
      include_world_blocks
          ? askYesNo("Skip faces hidden by neighbouring blocks?", true)
          : false;
  // 居中：把包围盒中心移到原点（推荐，导入第三方软件后一按 Frame Selected 就能看到）
  const bool is_need_geometry_center =
      askYesNo("Move the model center to the origin?", true);
  // 单位缩放：会把最长边压成 1，丢失"1 单位 = 1 方块"的真实尺寸，默认不做
  const bool is_need_normalize_scale = askYesNo(
      "Also scale the longest edge to 1 unit (changes the real size)?", false);

  const std::string assets_root = DetectAssetsRoot();

  ObjExportOptions export_options;
  export_options.geom_center = is_need_geometry_center;
  export_options.normalize_scale = is_need_normalize_scale;
  export_options.assets_root = assets_root;

  AnvilReader anvil_reader;
  anvil_reader.SetRegionFolder(region_folder);

  // 增量累加：每个区块处理完就把网格并入结果并释放，
  // 否则几百个区块的网格会同时驻留导致内存爆掉。
  ObjMeshBuilder obj_builder(export_options);

  const int span = chunk_radius * 2 + 1;
  std::vector<ChunkBlocks> world_blocks;
  if (include_world_blocks) {
    world_blocks.resize(static_cast<std::size_t>(span) * span);
  }

  // 逐区块读取：LittleTiles 的 tile +（可选）普通方块
  std::size_t total_tiles = 0;
  int found_chunks = 0;
  int missing_chunks = 0;
  for (int offset_z = -chunk_radius; offset_z <= chunk_radius; ++offset_z) {
    for (int offset_x = -chunk_radius; offset_x <= chunk_radius; ++offset_x) {
      const ChunkCoordinate coord{chunk_x + offset_x, chunk_z + offset_z};
      AnvilReader::ChunkDataReference reference{};
      bool has_chunk = true;
      try {
        reference = anvil_reader.GetChunkDataReference(coord);
      } catch (const std::exception&) {
        has_chunk = false;  // 区块不存在，或所在 region 文件缺失
      }
      if (!has_chunk) {
        ++missing_chunks;
        continue;
      }
      ++found_chunks;

      // LittleTiles 的 tile
      try {
        ChunkTileEntities chunk_tiles;
        chunk_tiles.ReadChunk(reference);
        total_tiles += chunk_tiles.TileCount();
        ChunkMesh chunk_mesh;
        chunk_mesh.AddTilesFromChunkTileEntities(chunk_tiles);
        for (const LtSurfaceMesh& mesh : chunk_mesh.mesh_array()) {
          obj_builder.AddMesh(mesh);
        }
      } catch (const std::exception&) {
        // 该区块没有 LittleTiles 数据
      }

      // 普通方块
      if (include_world_blocks && reference.p_chunk_level) {
        ChunkBlocks& blocks =
            world_blocks[static_cast<std::size_t>(offset_z + chunk_radius) *
                             span +
                         (offset_x + chunk_radius)];
        blocks.ReadFromChunkLevel(*reference.p_chunk_level);
        if (reference.p_chunk_level->has_key("TileEntities")) {
          blocks.MarkLittleTilesHosts(
              reference.p_chunk_level->at("TileEntities").as<nbt::tag_list>());
        }
      }
    }
  }
  printf("chunk 数: 找到 %d 个，缺失 %d 个；LittleTiles tile 共 %zu 个\n",
         found_chunks, missing_chunks, total_tiles);

  // 普通方块 -> 完整立方体网格（按方块类型分组合并）
  if (include_world_blocks && assets_root.empty()) {
    printf("警告: 未找到素材目录，跳过普通方块导出\n");
  } else if (include_world_blocks) {
    BlockIdTable block_id_table;
    if (block_id_table.LoadFromTsv(assets_root + "/block_ids.tsv")) {
      std::vector<LtSurfaceMesh> world_meshes;
      // 世界原点：区域左下角方块坐标（chunk * 16）
      BuildWorldBlockMeshes((chunk_x - chunk_radius) * 16,
                            (chunk_z - chunk_radius) * 16, world_blocks, span,
                            span, block_id_table, cull_hidden_faces,
                            &world_meshes);
      for (const LtSurfaceMesh& mesh : world_meshes) {
        obj_builder.AddMesh(mesh);
      }
    } else {
      printf("警告: 读不到 %s/block_ids.tsv，跳过普通方块导出\n",
             assets_root.c_str());
    }
  }

  string obj_file_path = OUT_OBJ_FILE_NAME;
  obj_file_path.append(to_string(chunk_x));
  obj_file_path.append("_");
  obj_file_path.append(to_string(chunk_z));
  if (chunk_radius > 0) {
    obj_file_path.append("_to_");
    obj_file_path.append(to_string(chunk_x + chunk_radius));
    obj_file_path.append("_");
    obj_file_path.append(to_string(chunk_z + chunk_radius));
  }
  obj_file_path.append(".obj");

  obj_builder.WriteToFile(obj_file_path.c_str());
  return EXIT_SUCCESS;
}
