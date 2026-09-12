#include <cctype>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <filesystem>
#include <string>
#include <vector>

#include "Log/GalibLog.h"
#include "Log/GalibText.h"
#include "Minecraft/Anvil.h"
#include "Minecraft/BlockIdTable.h"
#include "Minecraft/CgalSupport/CgalLittletilesBuilder.h"
#include "Minecraft/CgalSupport/CgalWorldBlocks.h"
#include "Minecraft/ChunkBlocks.h"
#include "Minecraft/LittleTiles.h"
#include "Minecraft/LtStructure.h"

using galib::minecraft::AnvilReader;
using galib::minecraft::BlockIdTable;
using galib::minecraft::ChunkBlocks;
using galib::minecraft::ChunkCoordinate;
using galib::minecraft::cgal_support::AddStructureToObjBuilder;
using galib::minecraft::cgal_support::BuildWorldBlockMeshes;
using galib::minecraft::cgal_support::ChunkMesh;
using galib::minecraft::cgal_support::LtSurfaceMesh;
using galib::minecraft::cgal_support::MergeAndWriteToObj;
using galib::minecraft::cgal_support::ObjExportOptions;
using galib::minecraft::cgal_support::ObjMeshBuilder;
using galib::minecraft::littletiles::ChunkTileEntities;
using galib::minecraft::littletiles::LtStructure;

using std::string;
using std::to_string;

#define OUT_OBJ_FILE_NAME "../out_file/marge_obj_from_chunk_"

namespace {

using Clock = std::chrono::steady_clock;

// 两个时间点之间的秒数，保留 1 位小数（用于耗时输出）
double ElapsedSeconds(const Clock::time_point kStart,
                      const Clock::time_point kEnd) {
  return std::chrono::duration<double>(kEnd - kStart).count();
}

// 启动时选界面语言：回车默认简体中文，输入 en/2/English 则切英文。
void AskLanguage() {
  printf("Language / 语言  [1] zh-CN  [2] en-US : ");
  fflush(stdout);

  char line[64];
  if (!fgets(line, sizeof(line), stdin)) {
    return;  // 非交互（管道）时保持默认中文
  }
  std::string answer(line);
  for (char& ch : answer) {
    ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
  }
  const bool english = answer.find('e') != std::string::npos ||
                       answer.find('2') != std::string::npos;
  galib::SetLanguage(english ? galib::Language::kEnUs : galib::Language::kZhCn);
  printf("%s\n",
         galib::Tr("已选择语言：简体中文", "Selected language: English"));
}

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

// 可执行文件所在目录。CLion 跑程序时的工作目录是构建目录，
// 相对路径必须能相对可执行文件（以及它的上一级 = 仓库根）解析，否则用户在
// 仓库根下习惯写的 "assets/xxx" 会因为工作目录不同而找不到。
std::filesystem::path ProgramDirectory(const char* const kProgramPath) {
  if (kProgramPath == nullptr || *kProgramPath == '\0') {
    return {};
  }
  std::error_code error;
  const std::filesystem::path absolute =
      std::filesystem::weakly_canonical(kProgramPath, error);
  return error ? std::filesystem::path() : absolute.parent_path();
}

// 自动探测素材根：环境变量 LITTLETILES_ASSETS 优先，其次是
// ./assets/1.12.2、../assets/1.12.2，再退到相对可执行文件的同样位置。
std::string DetectAssetsRoot(const std::filesystem::path& kProgramDir) {
  if (const char* const assets_env = std::getenv("LITTLETILES_ASSETS");
      assets_env != nullptr && *assets_env != '\0') {
    return assets_env;
  }

  std::vector<std::filesystem::path> candidates = {"assets/1.12.2",
                                                   "../assets/1.12.2"};
  if (!kProgramDir.empty()) {
    candidates.push_back(kProgramDir / "assets" / "1.12.2");
    candidates.push_back(kProgramDir.parent_path() / "assets" / "1.12.2");
  }
  for (const std::filesystem::path& candidate : candidates) {
    std::error_code error;
    if (std::filesystem::exists(candidate / "block_textures.tsv", error)) {
      const std::filesystem::path absolute =
          std::filesystem::weakly_canonical(candidate, error);
      return error ? candidate.string() : absolute.string();
    }
  }
  return {};
}

// 素材目录：回车走自动探测；也可以填材质包生成的合并素材根
// （见 tools/build_assets_from_pack.py，产物形如 assets/pack）。
// 相对路径依次按"当前工作目录 → 可执行文件目录 → 可执行文件上一级"解析，
// 这样在 CLion（工作目录 = 构建目录）里填 assets/pack 也能找到。
// 按整行读取，路径里有空格不用加引号（从 Finder 复制来的引号会自动去掉）。
std::string AskAssetsRoot(const std::filesystem::path& kProgramDir) {
  printf("%s", galib::Tr("素材目录（回车 = 自动探测）: ",
                         "assets root (blank = auto-detect): "));
  fflush(stdout);

  char line[512];
  if (!fgets(line, sizeof(line), stdin)) {
    return DetectAssetsRoot(kProgramDir);  // 非交互（管道）时直接走自动探测
  }
  std::string path(line);
  const std::string kSpaces = " \t\r\n";
  const std::size_t begin = path.find_first_not_of(kSpaces);
  path = (begin == std::string::npos)
             ? std::string()
             : path.substr(begin, path.find_last_not_of(kSpaces) - begin + 1);
  if (path.size() >= 2 && path.front() == '"' && path.back() == '"') {
    path = path.substr(1, path.size() - 2);
  }
  if (path.empty()) {
    return DetectAssetsRoot(kProgramDir);
  }

  std::vector<std::filesystem::path> candidates = {path};
  if (!kProgramDir.empty()) {
    candidates.push_back(kProgramDir / path);
    candidates.push_back(kProgramDir.parent_path() / path);
  }
  for (const std::filesystem::path& candidate : candidates) {
    std::error_code error;
    if (!std::filesystem::exists(candidate / "block_textures.tsv", error)) {
      continue;
    }
    const std::filesystem::path absolute =
        std::filesystem::weakly_canonical(candidate, error);
    return error ? candidate.string() : absolute.string();
  }

  printf("%s", galib::Tr("警告: 下面这些位置都没有 block_textures.tsv：\n",
                         "warning: no block_textures.tsv in any of these:\n"));
  for (const std::filesystem::path& candidate : candidates) {
    printf("        %s\n", candidate.string().c_str());
  }
  printf("%s", galib::Tr("      （填绝对路径最稳；现在退回自动探测）\n",
                         "      (an absolute path is safest; falling back to "
                         "auto-detect)\n"));
  return DetectAssetsRoot(kProgramDir);
}

}  // namespace

int main(int argc, char** argv) {
  printf("=== LittleTiles Reader ===\n");
  AskLanguage();

  const std::filesystem::path program_dir =
      ProgramDirectory(argc > 0 ? argv[0] : nullptr);

  printf(
      "%s",
      galib::Tr(
          "存档 region 目录，或 LittleTiles 结构文件（.txt/.struct）: ",
          "region folder, or a LittleTiles structure file (.txt/.struct): "));
  char region_folder[256];
  scanf("%255s", region_folder);  // 明确上限，避免超长路径写越界
  printf(galib::Tr("存档目录: %s\n", "region folder path: %s\n"),
         region_folder);

  // 给的是文件（.txt/.struct）就走 LittleTiles 结构模式：结构自带坐标，
  // 不需要区块坐标与扫描半径，也谈不上"周围的普通方块"。
  std::error_code path_error_code;
  const bool is_structure_file =
      std::filesystem::is_regular_file(region_folder, path_error_code);

  ChunkCoordinate::NumericType chunk_x = 0;
  ChunkCoordinate::NumericType chunk_z = 0;
  int chunk_radius = 0;
  if (!is_structure_file) {
    printf("%s", galib::Tr("区块 x: ", "chunk x: "));
    scanf("%d", &chunk_x);
    printf("%s", galib::Tr("区块 z: ", "chunk z: "));
    scanf("%d", &chunk_z);
    printf("%s", galib::Tr("扫描半径（0 = 只处理这一个区块）: ",
                           "scan radius in chunks (0 = only this chunk): "));
    scanf("%d", &chunk_radius);
    if (chunk_radius < 0) {
      chunk_radius = 0;
    }
  }

  // 丢掉上一个 scanf 残留的换行符，后面的选项按整行读取
  {
    int remaining = 0;
    while ((remaining = getchar()) != '\n' && remaining != EOF) {
    }
  }

  const bool include_world_blocks =
      is_structure_file
          ? false
          : askYesNo(galib::Tr("是否同时导出普通方块（非 LittleTiles）？",
                               "Also export plain (non-LittleTiles) blocks?"),
                     true);
  const bool cull_hidden_faces =
      include_world_blocks
          ? askYesNo(galib::Tr("是否剔除被相邻方块挡住的面？",
                               "Skip faces hidden by neighbouring blocks?"),
                     true)
          : false;
  // 居中：把包围盒中心移到原点（推荐，导入第三方软件后一按 Frame Selected 就能看到）
  const bool is_need_geometry_center =
      askYesNo(galib::Tr("是否把模型中心移到原点？",
                         "Move the model center to the origin?"),
               true);
  // 单位缩放：会把最长边压成 1，丢失"1 单位 = 1 方块"的真实尺寸，默认不做
  const bool is_need_normalize_scale = askYesNo(
      galib::Tr(
          "是否再把最长边缩放到 1 个单位（会改变真实尺寸）？",
          "Also scale the longest edge to 1 unit (changes the real size)?"),
      false);
  // 进度提示与耗时统计一起开关：两者都是"看过程"的，脚本化/服务化时通常都不要。
  const bool show_progress = askYesNo(
      galib::Tr("是否打印进度提示与耗时？", "Print progress and timing?"),
      true);
  galib::SetProgressEnabled(show_progress);

  // AskAssetsRoot 已经在找到时统一成绝对路径：后面的读取
  // （block_ids.tsv / block_textures.tsv / 贴图）都按这个字符串拼。
  const std::string assets_root = AskAssetsRoot(program_dir);
  printf(galib::Tr("素材目录: %s\n", "assets root: %s\n"),
         assets_root.empty()
             ? galib::Tr("(未找到，只导出几何)", "(not found, geometry only)")
             : assets_root.c_str());

  // 计时从这里开始：前面是人工输入，不计入处理耗时。
  const Clock::time_point process_start = Clock::now();

  ObjExportOptions export_options;
  export_options.geom_center = is_need_geometry_center;
  export_options.normalize_scale = is_need_normalize_scale;
  export_options.assets_root = assets_root;

  // ---- LittleTiles 结构（SNBT）模式 ----
  if (is_structure_file) {
    const LtStructure structure = LtStructure::FromSnbtFile(region_folder);
    printf(
        galib::Tr(
            "结构: %s，grid=%d，盒子 %zu 个，材质分组 %zu 个，子结构 %d 个\n",
            "structure: %s, grid=%d, %zu boxes, %zu material groups, %d "
            "child structures\n"),
        structure.name().empty() ? "(未命名)" : structure.name().c_str(),
        structure.grid(), structure.BoxCount(), structure.groups().size(),
        structure.child_group_count());

    ObjMeshBuilder structure_builder(export_options);
    const std::size_t mesh_count =
        AddStructureToObjBuilder(structure, &structure_builder);

    // 输出文件名：优先用结构名（去掉不适合当文件名的字符），否则用文件主名
    std::string out_name =
        structure.name().empty()
            ? std::filesystem::path(region_folder).stem().string()
            : structure.name();
    for (char& ch : out_name) {
      const bool is_ok = std::isalnum(static_cast<unsigned char>(ch)) != 0 ||
                         ch == '_' || ch == '-';
      if (!is_ok) {
        ch = '_';
      }
    }
    const std::string obj_path =
        std::string("../out_file/") + out_name + ".obj";
    structure_builder.WriteToFile(obj_path.c_str());

    if (show_progress) {
      printf(galib::Tr("结构导出完成: %zu 个网格，总耗时 %.1f 秒\n",
                       "structure export done: %zu meshes, %.1f s\n"),
             mesh_count, ElapsedSeconds(process_start, Clock::now()));
    }
    return EXIT_SUCCESS;
  }

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
      if (show_progress) {
        const int chunk_index =
            (offset_z + chunk_radius) * span + (offset_x + chunk_radius) + 1;
        printf(galib::Tr("[进度] 区块 (%d, %d) —— %d/%d\n",
                         "[progress] chunk (%d, %d) - %d/%d\n"),
               coord.x, coord.z, chunk_index, span * span);
      }
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
      ChunkTileEntities chunk_tiles;
      bool has_tiles = true;
      try {
        chunk_tiles.ReadChunk(reference);
        total_tiles += chunk_tiles.TileCount();
        ChunkMesh chunk_mesh;
        chunk_mesh.AddTilesFromChunkTileEntities(chunk_tiles);
        for (const LtSurfaceMesh& mesh : chunk_mesh.mesh_array()) {
          obj_builder.AddMesh(mesh);
        }
      } catch (const std::exception&) {
        // 该区块没有 LittleTiles 数据
        has_tiles = false;
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
              reference.p_chunk_level->at("TileEntities").as<nbt::tag_list>(),
              coord);
        }
        // 哪些面被 tile 整面铺满——完整方块的邻居剔除要靠它，
        // 否则花盆这种只占一小块的 LT 结构会把下面方块的面剔掉。
        if (has_tiles) {
          blocks.MarkLittleTilesCoverage(chunk_tiles);
        }
      }
    }
  }
  printf(galib::Tr("区块: 找到 %d 个，缺失 %d 个；LittleTiles tile 共 %zu 个\n",
                   "chunks: %d found, %d missing; %zu LittleTiles tiles\n"),
         found_chunks, missing_chunks, total_tiles);
  const Clock::time_point read_end = Clock::now();

  // 普通方块 -> 完整立方体网格（按方块类型分组合并）
  if (include_world_blocks && assets_root.empty()) {
    printf("%s",
           galib::Tr("警告: 未找到素材目录，跳过普通方块导出\n",
                     "warning: no assets root, skipping plain block export\n"));
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
      printf(galib::Tr("警告: 读不到 %s/block_ids.tsv，跳过普通方块导出\n",
                       "warning: cannot read %s/block_ids.tsv, skipping plain "
                       "block export\n"),
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

  const Clock::time_point build_end = Clock::now();
  obj_builder.WriteToFile(obj_file_path.c_str());
  const Clock::time_point write_end = Clock::now();

  // 总耗时包含写出文件的时间（大范围导出里写 OBJ 往往占相当一部分）
  if (show_progress) {
    printf(
        galib::Tr(
            "总耗时: %.1f 秒（读取与建网格 %.1f 秒，普通方块网格 %.1f 秒，写出"
            "文件 %.1f 秒）\n",
            "total: %.1f s (read & mesh %.1f s, plain blocks %.1f s, write "
            "%.1f s)\n"),
        ElapsedSeconds(process_start, write_end),
        ElapsedSeconds(process_start, read_end),
        ElapsedSeconds(read_end, build_end),
        ElapsedSeconds(build_end, write_end));
  }
  return EXIT_SUCCESS;
}
