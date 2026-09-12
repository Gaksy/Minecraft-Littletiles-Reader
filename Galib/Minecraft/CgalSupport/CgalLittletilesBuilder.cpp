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
 * Date Created: 12/24/2024
 */

#include "Minecraft/CgalSupport/CgalLittletilesBuilder.h"

#include <Minecraft/CgalSupport/CgalLtSupport.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <filesystem>
#include <iomanip>
#include <limits>
#include <utility>

#include "Log/GalibLog.h"
#include "Log/GalibText.h"
#include "Minecraft/TextureSupport/BlockTextureTable.h"
#include "Minecraft/TextureSupport/TextureBaker.h"

using std::cerr;
using std::cout;
using std::distance;
using std::endl;
using std::find;
using std::map;
using std::ofstream;
using std::size_t;
using std::unordered_map;
using std::vector;

using galib::minecraft::cgal_support::ApplyGrid;
using galib::minecraft::cgal_support::ApplyWorldOffset;
using galib::minecraft::cgal_support::ChunkMesh;
using galib::minecraft::cgal_support::CleanupMesh;
using galib::minecraft::cgal_support::CreateMeshFromTileEntity;
using galib::minecraft::cgal_support::LtPoint3;
using galib::minecraft::cgal_support::LtSurfaceMesh;
using galib::minecraft::cgal_support::ObjExportOptions;
using galib::minecraft::cgal_support::ObjMeshBuilder;
using galib::minecraft::cgal_support::SurfaceMeshType;

using galib::minecraft::littletiles::BlockTileEntities;
using galib::minecraft::littletiles::BoxTileEnities;
using galib::minecraft::littletiles::ChunkTileEntities;
using galib::minecraft::littletiles::GridType;
using galib::minecraft::littletiles::TileEntity;

using galib::minecraft::texture_support::BlockFaceTextures;
using galib::minecraft::texture_support::BlockTextureTable;
using galib::minecraft::texture_support::ComputeFaceUv;
using galib::minecraft::texture_support::FaceDirection;
using galib::minecraft::texture_support::FaceDirectionFromNormal;
using galib::minecraft::texture_support::TextureBaker;

using CGAL::SM_Vertex_index;

size_t addTilesFromBlockTilesEntities(
    const BlockTileEntities& kBlockTileEntities,
    vector<LtSurfaceMesh>& mesh_array) {
  const GridType grid_type = kBlockTileEntities.grid();
  size_t processed_tile_count = 0;
  // BlockTile -> BoxTile -> Tile

  // For BlockTile 遍历 Block 中的所有 boxes
  for (BlockTileEntities::const_iterator box_it = kBlockTileEntities.cbegin();
       box_it != kBlockTileEntities.cend(); ++box_it) {
    // Get BoxTile entities 获取每个 Box Tile
    const BoxTileEnities& box_tile_entities = box_it->second;

    // For BoxTile 对于 Box Tile 中的每个 Tile，构建他的面
    for (BoxTileEnities::const_iterator tile_it = box_tile_entities.cbegin();
         tile_it != box_tile_entities.cend(); ++tile_it) {
      // Get Tile entities
      const TileEntity& tile_lt_entity = *tile_it;

      // 将 tile entities 转换为 cgal 网格
      LtSurfaceMesh tile_cgal_mesh;

      // 如有偏移且超出边界：用半空间裁剪（凸六面体 ∩ AABB），不再用 CGAL 布尔求交，
      // 避免布尔运算在共面面上产生的大量碎三角形与多余边。
      if (tile_lt_entity.is_offset_off_boundary()) {
        if (!ClipTileEntityToBox(tile_cgal_mesh, tile_lt_entity)) {
#ifdef GALIB_DEBUG
          galib::ProgressPrintf(
              "CgalLittletilesBuilder::addTilesFromBlockTilesEntities: clip "
              "result is empty, tile skipped\n");
#endif
          continue;
        }
      } else {
        CreateMeshFromTileEntity(tile_cgal_mesh, tile_lt_entity);
      }

      ApplyGrid(tile_cgal_mesh, grid_type);
      tile_cgal_mesh.set_block_coord_in_world(
          kBlockTileEntities.block_coordinate());
      tile_cgal_mesh.set_block_id(box_it->first.block_id);
      tile_cgal_mesh.set_tile_color(box_it->first.color,
                                    box_it->first.has_color);
      tile_cgal_mesh.ApplyOffset(tile_cgal_mesh.block_coord_in_world());
      mesh_array.push_back(tile_cgal_mesh);
      ++processed_tile_count;
    }
  }

  return processed_tile_count;
}

ChunkMesh::size_type ChunkMesh::AddTilesFromChunkTileEntities(
    const ChunkTileEntities& kChunkTileEntities) {
  size_t processed_tile_count = 0;
#ifdef GALIB_DEBUG
  size_t all_tile_count = kChunkTileEntities.TileCount();
#endif
  for (auto block_it = kChunkTileEntities.cbegin();
       block_it != kChunkTileEntities.cend(); ++block_it) {
#ifdef GALIB_DEBUG
    galib::ProgressPrintf(
        "ChunkMesh::AddTilesFromChunkTileEntities build block (%zu / %zu): %d "
        "%d %d\n",
        processed_tile_count + 1, all_tile_count,
        block_it->block_coordinate().x, block_it->block_coordinate().y,
        block_it->block_coordinate().z);
#endif
    processed_tile_count +=
        addTilesFromBlockTilesEntities(*block_it, this->tiles_in_world_);
  }
  return processed_tile_count;
}

const ChunkMesh::container& ChunkMesh::mesh_array() const {
  return this->tiles_in_world_;
}

void ChunkMesh::Clear() { this->tiles_in_world_.clear(); }

namespace {

// 每个导出面附加的信息：材质下标与面朝向（用哪个面的贴图）
struct ExportedFaceInfo {
  std::size_t material_index{0};
  FaceDirection direction{FaceDirection::kUp};
  bool has_material{false};
};

// 材质名 = 贴图 + 生物群系染色 + tile 颜色：
//   blocks/grass_top + 0x91BD59 + 0xFFFFBE00 -> blocks_grass_top_t91bd59_cffffbe00
std::string MakeMaterialName(const std::string& kTexturePath,
                             const std::uint32_t kTintRgb,
                             const std::uint32_t kTileColor) {
  std::string name = kTexturePath;
  std::replace(name.begin(), name.end(), '/', '_');
  std::replace(name.begin(), name.end(), ':', '_');
  char suffix[32] = {};
  if (kTintRgb != 0x00FFFFFFu) {
    std::snprintf(suffix, sizeof(suffix), "_t%06x", kTintRgb);
    name += suffix;
  }
  if (kTileColor != 0xFFFFFFFFu) {
    std::snprintf(suffix, sizeof(suffix), "_c%08x", kTileColor);
    name += suffix;
  }
  return name;
}

// 由面的前三个顶点判定朝向（世界坐标；平移与等比缩放不改变方向）
FaceDirection FaceDirectionOf(const SurfaceMeshType& kMesh,
                              const SurfaceMeshType::face_index kFace) {
  std::array<SurfaceMeshType::Point, 3> points{};
  std::size_t count = 0;
  for (const SurfaceMeshType::vertex_index v :
       vertices_around_face(kMesh.halfedge(kFace), kMesh)) {
    if (count < points.size()) {
      points[count] = kMesh.point(v);
    }
    if (++count >= points.size()) {
      break;
    }
  }
  if (count < 3) {
    return FaceDirection::kUp;
  }
  const auto ab = points[1] - points[0];
  const auto ac = points[2] - points[0];
  const double nx = ab.y() * ac.z() - ab.z() * ac.y();
  const double ny = ab.z() * ac.x() - ab.x() * ac.z();
  const double nz = ab.x() * ac.y() - ab.y() * ac.x();
  return FaceDirectionFromNormal(nx, ny, nz);
}

// vt 去重用的量化键
long long QuantizeUvKey(const double kU, const double kV) {
  const long long u = static_cast<long long>(std::llround(kU * 1e7));
  const long long v = static_cast<long long>(std::llround(kV * 1e7));
  return u * 100000000LL + v;
}

}  // namespace

struct ObjMeshBuilder::Impl {
  explicit Impl(const ObjExportOptions& kOptions)
      : options(kOptions), baker(kOptions.assets_root) {
    has_textures =
        !options.assets_root.empty() &&
        texture_table.LoadFromTsv(options.assets_root + "/block_textures.tsv");
  }

  // 把一张网格并入合并结果，并记录它的材质与逐面 UV 信息
  void AddMesh(const LtSurfaceMesh& kMesh) {
    using Point = SurfaceMeshType::Point;

    {  // 并入这一张网格
      const SurfaceMeshType& current_mesh = kMesh.surface_mesh();
      const auto block_coord = kMesh.block_coord_in_world();
      const std::string block_id = kMesh.block_id();
      // 网格若记录了逐顶点本地坐标就用它（一个网格含多个方块时必须靠它算 UV），
      // 否则退化为"世界坐标 − 方块坐标"（LittleTiles 每个 tile 一个网格的情形）。
      const bool has_vertex_local = kMesh.has_vertex_local_positions();

      BlockFaceTextures face_textures;
      const bool has_block_textures =
          has_textures && texture_table.Lookup(block_id, &face_textures) &&
          !face_textures.empty();

      // 首先为当前mesh的所有顶点在合并mesh中创建对应顶点
      std::vector<SurfaceMeshType::Vertex_index>
          current_mesh_vertices_in_marged;
      for (const SurfaceMeshType::vertex_index& v : current_mesh.vertices()) {
        LtPoint3 current_point = current_mesh.point(v);
        SurfaceMeshType::Vertex_index new_vertex =
            marged_mesh.add_vertex(current_point);
        current_mesh_vertices_in_marged.push_back(new_vertex);
        vertex_index_map[v] = new_vertex;  // 映射原始顶点索引到新顶点索引
        // 记录"方块内本地坐标"：世界坐标 = 本地坐标 + 方块坐标。
        // UV 必须用它来算，不能等导出归一化（居中/缩放）之后再从坐标反推。
        if (has_vertex_local) {
          vertex_local_positions.push_back(kMesh.VertexLocalPosition(v));
        } else {
          vertex_local_positions.emplace_back(
              current_point.x() - block_coord.x,
              current_point.y() - block_coord.y,
              current_point.z() - block_coord.z);
        }
      }

      // 然后添加面到合并的mesh中
      for (const SurfaceMeshType::face_index& f : current_mesh.faces()) {
        std::vector<SurfaceMeshType::Vertex_index> face_vertices;

        // 获取当前面的所有顶点
        CGAL::Vertex_around_face_iterator<SurfaceMeshType> vbegin, vend;
        for (boost::tie(vbegin, vend) =
                 vertices_around_face(current_mesh.halfedge(f), current_mesh);
             vbegin != vend; ++vbegin) {
          SurfaceMeshType::Vertex_index original_vertex = *vbegin;
          // 通过映射找到在合并mesh中的对应顶点
          auto it_vertex = vertex_index_map.find(original_vertex);
          if (it_vertex != vertex_index_map.end()) {
            face_vertices.push_back(it_vertex->second);
          }
        }

        // 保留 n 边形：平面面片现在是四边形/多边形，不再强制拆成三角形
        // （CGAL 的 Surface_mesh 支持多边形面，OBJ 也直接支持）
        if (face_vertices.size() >= 3) {
          const std::size_t faces_before = marged_mesh.number_of_faces();
          // 使用try-catch防止添加无效的面
          try {
            marged_mesh.add_face(face_vertices);
          } catch (...) {
#ifdef GALIB_DEBUG
            // 这条是"数据有问题"的警告，不随进度开关关闭
            printf("%s",
                   galib::Tr("警告: 无法添加面，可能是重复面或无效几何\n",
                             "warning: cannot add face (duplicate or invalid "
                             "geometry)\n"));
#endif
          }

          // 只有真正加进去的面才记录附加信息，保证与网格的面顺序对齐
          if (marged_mesh.number_of_faces() > faces_before) {
            ExportedFaceInfo info;
            if (has_block_textures) {
              const FaceDirection direction = FaceDirectionOf(current_mesh, f);
              const std::string& texture_path = face_textures.Path(direction);
              if (texture_path.empty()) {
                ++missing_texture_faces;
              } else {
                // 生物群系染色：只有模型标了 tintindex 的面才需要
                std::uint32_t tint_rgb = 0x00FFFFFFu;
                const int tint_index = face_textures.Tint(direction);
                std::uint32_t tint_argb = 0;
                if (tint_index >= 0 &&
                    baker.ResolveTintColor(block_id, tint_index, &tint_argb)) {
                  tint_rgb = tint_argb & 0x00FFFFFFu;
                }
                // tile 自带染色（LittleTiles 的彩色 tile）
                const std::uint32_t tile_color =
                    kMesh.has_tile_color()
                        ? static_cast<std::uint32_t>(kMesh.tile_color())
                        : 0xFFFFFFFFu;

                const std::string name =
                    MakeMaterialName(texture_path, tint_rgb, tile_color);
                auto found = material_index.find(name);
                if (found == material_index.end()) {
                  found =
                      material_index.emplace(name, material_names.size()).first;
                  material_names.push_back(name);
                  material_textures.push_back(texture_path);
                  material_tints.push_back(tint_rgb);
                  material_tile_colors.push_back(tile_color);
                }
                info.material_index = found->second;
                info.direction = direction;
                info.has_material = true;
              }
            }
            face_infos.push_back(info);
          }
        }
      }

      // 清理当前mesh的顶点映射，为下一个mesh准备
      vertex_index_map.clear();
    }
  }

  // 归一化（居中/缩放）并写出 OBJ、MTL 与贴图
  bool WriteToFile(const char* const kFilename) {
    using Point = SurfaceMeshType::Point;
    // 归一化：把包围盒中心平移到原点；如需要，再等比缩放到最长边 = 1
    if (marged_mesh.number_of_vertices() > 0 &&
        (options.geom_center || options.normalize_scale)) {
      CGAL::Bbox_3 bbox;
      bool first = true;

      // 计算所有顶点的包围盒
      for (const SurfaceMeshType::vertex_index& v : marged_mesh.vertices()) {
        const Point& p = marged_mesh.point(v);
        if (first) {
          bbox = p.bbox();
          first = false;
        } else {
          bbox = bbox + p.bbox();
        }
      }

      // 计算包围盒中心与缩放系数（以最长边为基准，保持长宽比）
      const double center_x = (bbox.xmin() + bbox.xmax()) / 2.0;
      const double center_y = (bbox.ymin() + bbox.ymax()) / 2.0;
      const double center_z = (bbox.zmin() + bbox.zmax()) / 2.0;

      const double extent = std::max(
          bbox.xmax() - bbox.xmin(),
          std::max(bbox.ymax() - bbox.ymin(), bbox.zmax() - bbox.zmin()));
      double scale = 1.0;
      if (options.normalize_scale && extent > 1e-12) {
        scale = 1.0 / extent;
      }

      // 先平移到原点，再按需缩放
      for (const SurfaceMeshType::vertex_index& v : marged_mesh.vertices()) {
        Point& p = marged_mesh.point(v);
        p = Point((p.x() - center_x) * scale, (p.y() - center_y) * scale,
                  (p.z() - center_z) * scale);
      }
    }

#ifdef GALIB_DEBUG
    // 检查合并后的网格
    galib::ProgressPrintf(galib::Tr("合并后网格统计: ", "merged mesh: "));
    galib::ProgressPrintf(galib::Tr("顶点数: %u\n", "vertices: %u\n"),
                          marged_mesh.number_of_vertices());
    galib::ProgressPrintf(galib::Tr("面数: %u\n", "faces: %u\n"),
                          marged_mesh.number_of_faces());
#endif

    // 导出为OBJ文件
    // 注意：ofstream 不会创建目录，必须先把输出目录建出来，否则会直接失败
    const std::filesystem::path output_path(kFilename);
    if (output_path.has_parent_path()) {
      std::error_code create_error;
      std::filesystem::create_directories(output_path.parent_path(),
                                          create_error);
      if (create_error) {
        std::cerr << galib::Tr("无法创建输出目录: ",
                               "cannot create output dir: ")
                  << output_path.parent_path() << " —— "
                  << create_error.message() << std::endl;
        return false;
      }
    }

    std::ofstream out(output_path);
    if (!out) {
      std::error_code path_error;
      std::cerr << galib::Tr("无法打开文件: ", "cannot open file: ")
                << std::filesystem::weakly_canonical(output_path, path_error)
                << std::endl;
      return false;
    }

    // 输出顶点
    // 提高精度：默认流精度只有 6 位有效数字，坐标在千级（未居中的世界坐标）时
    // 量化步长可达 0.01 方块，会静默改变几何。
    out << std::setprecision(9);
    const std::string obj_stem = output_path.stem().string();
    const std::string mtl_filename = obj_stem + ".mtl";
    // 贴图统一放进 OBJ 旁边的同名子目录，避免几十上百张 PNG 和 OBJ 混在一起；
    // MTL 仍与 OBJ 同级（Blender 按 mtllib 的路径找 MTL，map_Kd 再相对 MTL 解析）。
    const std::string texture_dir_name = obj_stem + "_textures";
    // 只有材质与逐面信息都对齐时才写贴图坐标，否则退回纯几何输出
    const bool write_materials =
        !material_names.empty() &&
        face_infos.size() == marged_mesh.number_of_faces() &&
        vertex_local_positions.size() == marged_mesh.number_of_vertices();
    if (write_materials) {
      out << "mtllib " << mtl_filename << "\n";
    }

    for (const SurfaceMeshType::vertex_index& v : marged_mesh.vertices()) {
      const LtPoint3& p = marged_mesh.point(v);
      out << "v " << p.x() << " " << p.y() << " " << p.z() << "\n";
    }

    // 逐面算出每个角的贴图坐标。UV 由"方块内本地坐标 + 面朝向"决定：
    // 本地坐标在建网格时记录，不能用导出归一化之后的坐标反推。
    std::vector<std::vector<std::size_t>>
        face_uv_indices;  // 每个角对应的 vt 下标（1 起）
    if (write_materials) {
      std::unordered_map<long long, std::size_t> uv_dedup;
      std::vector<std::pair<double, double>> uv_values;
      std::size_t face_index = 0;
      for (const SurfaceMeshType::face_index& f : marged_mesh.faces()) {
        const ExportedFaceInfo& info = face_infos[face_index++];
        std::vector<std::size_t> uv_indices;
        if (info.has_material) {
          for (const SurfaceMeshType::vertex_index v :
               vertices_around_face(marged_mesh.halfedge(f), marged_mesh)) {
            const LtPoint3& local = vertex_local_positions[v.idx()];
            double u = 0.0;
            double uv_v = 0.0;
            ComputeFaceUv(info.direction, local.x(), local.y(), local.z(), &u,
                          &uv_v);
            // OBJ 的 vt 以左下角为原点，而 v = 0 在贴图顶部，因此翻转一次
            const double vt_u = u;
            const double vt_v = 1.0 - uv_v;
            const long long key = QuantizeUvKey(vt_u, vt_v);
            auto found = uv_dedup.find(key);
            if (found == uv_dedup.end()) {
              uv_values.emplace_back(vt_u, vt_v);
              found = uv_dedup.emplace(key, uv_values.size())
                          .first;  // OBJ 下标从 1 开始
            }
            uv_indices.push_back(found->second);
          }
        }
        face_uv_indices.push_back(std::move(uv_indices));
      }
      for (const auto& uv : uv_values) {
        out << "vt " << uv.first << " " << uv.second << "\n";
      }
    }

    // 输出面（按材质分组，切换材质时写 usemtl）
    std::size_t face_index = 0;
    std::size_t current_material = std::numeric_limits<std::size_t>::max();
    for (const SurfaceMeshType::face_index& f : marged_mesh.faces()) {
      if (write_materials) {
        const ExportedFaceInfo& info = face_infos[face_index];
        if (info.has_material && info.material_index != current_material) {
          out << "usemtl " << material_names[info.material_index] << "\n";
          current_material = info.material_index;
        }
      }

      out << "f";
      std::size_t corner = 0;
      for (const SurfaceMeshType::vertex_index v :
           vertices_around_face(marged_mesh.halfedge(f), marged_mesh)) {
        out << " " << (v.idx() + 1);
        if (write_materials && corner < face_uv_indices[face_index].size()) {
          out << "/" << face_uv_indices[face_index][corner];
        }
        ++corner;
      }
      out << "\n";
      ++face_index;
    }

    out.close();

    // 写 MTL：每个材质把 (贴图 × 生物群系染色 × tile 颜色) 烘焙成一张 PNG，
    // 统一放到 OBJ 旁边的 <obj_stem>_textures/ 目录，保证输出可以整体搬走。
    std::size_t baked_texture_count = 0;
    if (write_materials) {
      const std::filesystem::path mtl_path =
          output_path.parent_path() / mtl_filename;
      const std::filesystem::path texture_dir =
          output_path.parent_path() / texture_dir_name;
      std::error_code texture_dir_error;
      std::filesystem::create_directories(texture_dir, texture_dir_error);
      if (texture_dir_error) {
        std::cerr << galib::Tr("无法创建贴图目录: ",
                               "cannot create texture dir: ")
                  << texture_dir << " —— " << texture_dir_error.message()
                  << std::endl;
      }
      std::ofstream mtl(mtl_path);
      if (mtl) {
        mtl << galib::Tr("# 由 LittleTilesReader 生成\n",
                         "# Generated by LittleTilesReader\n");
        for (std::size_t i = 0; i < material_names.size(); ++i) {
          const std::string& texture_path = material_textures[i];
          const std::string png_name = material_names[i] + ".png";
          const std::filesystem::path target = texture_dir / png_name;

          // 无染色时直接复制原贴图，避免多做一次无意义的编解码
          const bool needs_bake = material_tints[i] != 0x00FFFFFFu ||
                                  material_tile_colors[i] != 0xFFFFFFFFu;
          std::string bake_error;
          bool texture_ready = false;
          if (needs_bake) {
            texture_ready = baker.Bake(texture_path, material_tints[i],
                                       material_tile_colors[i], target.string(),
                                       &bake_error);
          }
          if (!texture_ready) {
            const std::filesystem::path source =
                std::filesystem::path(options.assets_root) / "textures" /
                (texture_path + ".png");
            std::error_code copy_error;
            if (std::filesystem::exists(source)) {
              std::filesystem::copy_file(
                  source, target,
                  std::filesystem::copy_options::overwrite_existing,
                  copy_error);
              texture_ready = !copy_error;
            }
          }
          if (texture_ready) {
            ++baked_texture_count;
          } else {
            std::cerr << galib::Tr("警告: 贴图处理失败 ",
                                   "warning: texture failed: ")
                      << texture_path;
            if (!bake_error.empty()) {
              std::cerr << " —— " << bake_error;
            }
            std::cerr << std::endl;
          }
          mtl << "\nnewmtl " << material_names[i] << "\n"
              << "Ka 1.000 1.000 1.000\n"
              << "Kd 1.000 1.000 1.000\n"
              << "d 1.0\n"
              << "map_Kd " << texture_dir_name << "/" << png_name << "\n";
        }
        mtl.close();
      }
    }

    std::error_code path_error;
    std::cout << galib::Tr("合并的网格已导出到: ", "exported merged mesh to: ")
              << std::filesystem::weakly_canonical(output_path, path_error)
              << "\n";
    if (write_materials) {
      std::cout << galib::Tr("  材质 ", "  materials ") << material_names.size()
                << galib::Tr(" 个，已写出贴图 ", ", textures written ")
                << baked_texture_count << galib::Tr(" 张到 ", " into ")
                << texture_dir_name << "/";
      if (missing_texture_faces > 0) {
        std::cout << galib::Tr("（另有 ", " (") << missing_texture_faces
                  << galib::Tr(" 个面没解析到贴图）",
                               " faces without texture)");
      }
      std::cout << std::endl;
    } else if (!options.assets_root.empty()) {
      std::cout
          << galib::Tr(
                 "  未导出贴图：请检查素材目录是否包含 block_textures.tsv（",
                 "  no textures exported: check that the assets root has "
                 "block_textures.tsv (")
          << options.assets_root << "）" << std::endl;
    }
    return true;
  }

  ObjExportOptions options;
  BlockTextureTable texture_table;
  TextureBaker baker;
  bool has_textures{false};
  SurfaceMeshType marged_mesh;
  unordered_map<SurfaceMeshType::Vertex_index, SurfaceMeshType::Vertex_index>
      vertex_index_map;
  std::vector<std::string> material_names;
  std::vector<std::string> material_textures;
  std::vector<std::uint32_t> material_tints;
  std::vector<std::uint32_t> material_tile_colors;
  std::unordered_map<std::string, std::size_t> material_index;
  std::vector<ExportedFaceInfo> face_infos;
  std::vector<LtPoint3> vertex_local_positions;
  std::size_t missing_texture_faces{0};
};

ObjMeshBuilder::ObjMeshBuilder(const ObjExportOptions& kOptions)
    : impl_(new Impl(kOptions)) {}

ObjMeshBuilder::~ObjMeshBuilder() = default;

void ObjMeshBuilder::AddMesh(const LtSurfaceMesh& kMesh) {
  impl_->AddMesh(kMesh);
}

bool ObjMeshBuilder::WriteToFile(const char* const kFilename) {
  return impl_->WriteToFile(kFilename);
}

void galib::minecraft::cgal_support::MergeAndWriteToObj(
    const vector<LtSurfaceMesh>& meshes, const char* const p_filename,
    const ObjExportOptions& options) {
  ObjMeshBuilder builder(options);
  for (const LtSurfaceMesh& mesh : meshes) {
    builder.AddMesh(mesh);
  }
  builder.WriteToFile(p_filename);
}

void writeMeshToOff(const LtSurfaceMesh& mesh, const char* const p_filename) {
  vector<LtPoint3> v_array;
}

void galib::minecraft::cgal_support::WriteToOff(
    const std::vector<LtSurfaceMesh>& meshes, const char* const p_filename) {
  for (size_t i = 0; i < meshes.size(); ++i) {
    // Generate a unique filename for each mesh
    std::string mesh_filename =
        std::string(p_filename) + "_" + std::to_string(i) + ".off";
    writeMeshToOff(meshes[i], mesh_filename.c_str());
    std::cout << "Written mesh " << i << " to " << mesh_filename << std::endl;
  }
}

std::size_t galib::minecraft::cgal_support::AddStructureToObjBuilder(
    const littletiles::LtStructure& kStructure,
    ObjMeshBuilder* const p_desc_builder) {
  if (p_desc_builder == nullptr) {
    return 0;
  }
  const double grid = static_cast<double>(kStructure.grid());
  const double origin_x = static_cast<double>(kStructure.min().x);
  const double origin_y = static_cast<double>(kStructure.min().y);
  const double origin_z = static_cast<double>(kStructure.min().z);
  std::size_t mesh_count = 0;

  for (const littletiles::LtStructure::Group& group : kStructure.groups()) {
    for (const littletiles::TileEntity& tile : group.boxes) {
      LtSurfaceMesh tile_mesh;
      // 与存档路径一致的裁剪策略：偏移后超出自身盒子时才裁
      if (tile.is_offset_off_boundary()) {
        if (!ClipTileEntityToBox(tile_mesh, tile)) {
          continue;
        }
      } else {
        CreateMeshFromTileEntity(tile_mesh, tile);
      }
      tile_mesh.set_block_id(group.block_id);
      tile_mesh.set_tile_color(group.color, group.has_color);

      // grid → 方块单位，并平移到结构原点；同时记录"所在单元内的相对坐标"（UV 用）。
      // 结构里的网格会横跨多个方块单元，不能用网格级方块坐标反推，必须逐顶点记。
      SurfaceMeshType& mesh = tile_mesh.surface_mesh();
      for (const SurfaceMeshType::vertex_index& vertex : mesh.vertices()) {
        const LtPoint3 point = mesh.point(vertex);
        const double x = (point.x() - origin_x) / grid;
        const double y = (point.y() - origin_y) / grid;
        const double z = (point.z() - origin_z) / grid;
        mesh.point(vertex) = LtPoint3(x, y, z);
        tile_mesh.SetVertexLocalPosition(
            vertex,
            LtPoint3(x - std::floor(x), y - std::floor(y), z - std::floor(z)));
      }
      p_desc_builder->AddMesh(tile_mesh);
      ++mesh_count;
    }
  }
  return mesh_count;
}
