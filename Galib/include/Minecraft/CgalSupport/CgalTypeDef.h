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
 * Date Created: 04/28/2025
 */

#ifndef GALIB_MINECRAFT_CGALSUPPORT_CGALTYPEDEF_H
#define GALIB_MINECRAFT_CGALSUPPORT_CGALTYPEDEF_H

#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>
#include <CGAL/Point_set_3.h>
#include <CGAL/Surface_mesh/Surface_mesh.h>
#include <CGAL/Vector_3.h>

#include <vector>

#include "GalibNamespaceDef.h"
#include "Minecraft/MinecraftCoord.h"

namespace galib::minecraft::cgal_support {
#ifdef _WIN32
using FloatType = float;
using LtKernel = CGAL::Simple_cartesian<FloatType>;
using LtPoint3 = CGAL::Point_3<LtKernel>;
using LtVector3 = CGAL::Vector_3<LtKernel>;
using SurfaceMeshType = CGAL::Surface_mesh<CGAL::Point_3<LtKernel>>;
#elif __APPLE__
using FloatType = double;
using LtKernel = CGAL::Simple_cartesian<FloatType>;
using LtPoint3 = CGAL::Point_3<LtKernel>;
using LtVector3 = CGAL::Vector_3<LtKernel>;
using SurfaceMeshType = CGAL::Surface_mesh<CGAL::Point_3<LtKernel>>;
#endif

struct UVData {
  FloatType u;
  FloatType v;
};

class LtSurfaceMesh {
 public:
  LtSurfaceMesh() = default;
  explicit LtSurfaceMesh(const SurfaceMeshType& mesh);
  ~LtSurfaceMesh() = default;

  [[nodiscard]] SurfaceMeshType& surface_mesh();
  [[nodiscard]] const SurfaceMeshType& surface_mesh() const;

  // 逐顶点的"方块内本地坐标"（0..1，相对于该顶点所在的那个方块）。
  // 一个网格可能包含多个方块的几何（例如按方块类型分组后的完整方块），
  // 此时无法用网格级的方块坐标反推，必须逐顶点记录。
  // UV 依赖它，并且必须在导出归一化（居中/缩放）之前记录。
  void SetVertexLocalPosition(SurfaceMeshType::Vertex_index kVertex,
                              const LtPoint3& kLocalPosition);
  [[nodiscard]] bool has_vertex_local_positions() const;
  [[nodiscard]] const LtPoint3& VertexLocalPosition(
      SurfaceMeshType::Vertex_index kVertex) const;

  void set_block_id(const std::string& str);
  [[nodiscard]] const std::string& block_id() const;
  // tile 的染色；未染色的 tile 保持 has_tile_color_ = false
  void set_tile_color(std::int32_t kColor, bool kHasColor);
  [[nodiscard]] bool has_tile_color() const;
  [[nodiscard]] std::int32_t tile_color() const;
  void set_block_coord_in_world(
      const galib::minecraft::BlockCoordinate& kBlockCoord);
  const galib::minecraft::BlockCoordinate& block_coord_in_world() const;
  SurfaceMeshType GetMeshWithOffset(
      const galib::minecraft::BlockCoordinate& offset) const;
  void ApplyOffset(const galib::minecraft::BlockCoordinate& offset);
  std::string GetFormatBlockId() const;
  UVData CalculateFaceUv(const SurfaceMeshType::face_index& kFaceIndex);

 private:
  SurfaceMeshType surface_mesh_;
  std::vector<LtPoint3> vertex_local_positions_;
  std::string block_id_;
  std::int32_t tile_color_{0};
  bool has_tile_color_{false};
  galib::minecraft::BlockCoordinate block_coord_in_world_;
};
}  // namespace galib::minecraft::cgal_support

#endif  //GALIB_MINECRAFT_CGALSUPPORT_CGALTYPEDEF_H
