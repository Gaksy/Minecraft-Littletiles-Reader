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

#include <filesystem>
#include <iomanip>

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

using galib::minecraft::littletiles::BlockTileEntities;
using galib::minecraft::littletiles::BoxTileEnities;
using galib::minecraft::littletiles::ChunkTileEntities;
using galib::minecraft::littletiles::GridType;
using galib::minecraft::littletiles::TileEntity;

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
          printf(
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
    printf(
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

void galib::minecraft::cgal_support::MergeAndWriteToObj(
    const vector<LtSurfaceMesh>& meshes, const char* const p_filename,
    const bool geom_center, const bool normalize_scale) {
  using SurfaceMeshType = SurfaceMeshType;
  using Point = SurfaceMeshType::Point;
  using Vector = CGAL::Vector_3<CGAL::Simple_cartesian<double>>;

  SurfaceMeshType marged_mesh;
  // 使用顶点坐标作为键值可能不够精确，改用容差比较或者直接映射原始顶点索引
  unordered_map<SurfaceMeshType::Vertex_index, SurfaceMeshType::Vertex_index>
      vertex_index_map;

  for (vector<LtSurfaceMesh>::const_iterator it = meshes.cbegin();
       it != meshes.cend(); ++it) {
    const SurfaceMeshType& current_mesh = it->surface_mesh();

    // 首先为当前mesh的所有顶点在合并mesh中创建对应顶点
    std::vector<SurfaceMeshType::Vertex_index> current_mesh_vertices_in_marged;
    for (const SurfaceMeshType::vertex_index& v : current_mesh.vertices()) {
      LtPoint3 current_point = current_mesh.point(v);
      SurfaceMeshType::Vertex_index new_vertex =
          marged_mesh.add_vertex(current_point);
      current_mesh_vertices_in_marged.push_back(new_vertex);
      vertex_index_map[v] = new_vertex;  // 映射原始顶点索引到新顶点索引
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
        // 使用try-catch防止添加无效的面
        try {
          marged_mesh.add_face(face_vertices);
        } catch (...) {
#ifdef GALIB_DEBUG
          printf("警告: 无法添加面，可能是重复面或无效几何\n");
#endif
        }
      }
    }

    // 清理当前mesh的顶点映射，为下一个mesh准备
    vertex_index_map.clear();
  }

  // 归一化：把包围盒中心平移到原点；如需要，再等比缩放到最长边 = 1
  if (marged_mesh.number_of_vertices() > 0 &&
      (geom_center || normalize_scale)) {
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
    if (normalize_scale && extent > 1e-12) {
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
  printf("合并后网格统计: ");
  printf("顶点数: %u\n", marged_mesh.number_of_vertices());
  printf("面数: %u\n", marged_mesh.number_of_faces());
#endif

  // 导出为OBJ文件
  // 注意：ofstream 不会创建目录，必须先把输出目录建出来，否则会直接失败
  const std::filesystem::path output_path(p_filename);
  if (output_path.has_parent_path()) {
    std::error_code create_error;
    std::filesystem::create_directories(output_path.parent_path(),
                                        create_error);
    if (create_error) {
      std::cerr << "无法创建输出目录: " << output_path.parent_path() << " —— "
                << create_error.message() << std::endl;
      return;
    }
  }

  std::ofstream out(output_path);
  if (!out) {
    std::error_code path_error;
    std::cerr << "无法打开文件: "
              << std::filesystem::weakly_canonical(output_path, path_error)
              << std::endl;
    return;
  }

  // 输出顶点
  // 提高精度：默认流精度只有 6 位有效数字，坐标在千级（未居中的世界坐标）时
  // 量化步长可达 0.01 方块，会静默改变几何。
  out << std::setprecision(9);
  for (const SurfaceMeshType::vertex_index& v : marged_mesh.vertices()) {
    const LtPoint3& p = marged_mesh.point(v);
    out << "v " << p.x() << " " << p.y() << " " << p.z() << std::endl;
  }

  // 输出面
  for (const SurfaceMeshType::face_index& f : marged_mesh.faces()) {
    out << "f";
    CGAL::Vertex_around_face_iterator<SurfaceMeshType> vbegin, vend;
    for (boost::tie(vbegin, vend) =
             vertices_around_face(marged_mesh.halfedge(f), marged_mesh);
         vbegin != vend; ++vbegin) {
      // OBJ文件索引从1开始
      out << " " << (vbegin->idx() + 1);
    }
    out << std::endl;
  }

  out.close();
  std::error_code path_error;
  std::cout << "合并的网格已导出到: "
            << std::filesystem::weakly_canonical(output_path, path_error)
            << std::endl;
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
