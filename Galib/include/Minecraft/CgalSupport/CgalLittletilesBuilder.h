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

#ifndef GALIB_INCLUDE_MINECRAFT_CGALSUPPORT_CGALLITTLETILEBUIDER_H
#define GALIB_INCLUDE_MINECRAFT_CGALSUPPORT_CGALLITTLETILEBUIDER_H

#include <vector>

#include "GalibNamespaceDef.h"
#include "Minecraft/CgalSupport/CgalTypeDef.h"
#include "Minecraft/LittleTiles.h"

namespace galib::minecraft::cgal_support {
class ChunkMesh {
 public:
  using container = GALIB_STD vector<LtSurfaceMesh>;
  using const_iterator = GALIB_STD vector<LtSurfaceMesh>::const_iterator;
  using iterator = GALIB_STD vector<LtSurfaceMesh>::iterator;
  using size_type = GALIB_STD vector<LtSurfaceMesh>::size_type;

 public:
  ChunkMesh() = default;
  ~ChunkMesh() = default;

 public:
  size_type addTilesFromChukTileEntities(
      const GALIB minecraft::littletiles::ChunkTileEntities&
          kChunkTileEntities);
  GALIB_NODISCARD const container& getMeshArray() const;
  void clear();

 private:
  container tiles_in_world_;
};

// class ObjFormatBuilder {
// public:
//     ObjFormatBuilder() = default;
//     ~ObjFormatBuilder() = default;
//
// private:
//     struct FaceData {
//         GALIB_STD vector<GALIB minecraft::littletiles::LittleTilesCoord>::size_type point_index;
//         const LtSurfaceMesh& desc_face;
//     };
//
// public:
//     bool exportToFile(const char* p_file_path, const GALIB_STD vector<LtSurfaceMesh>& meshes);
//
// public:
//     GALIB_STD vector<GALIB minecraft::littletiles::LittleTilesCoord> coord_array_;
//
// };

// 合并所有 tile 网格并写出 OBJ。
// kGeomCenter     : 把包围盒中心平移到原点（默认开启）。
// kNormalizeScale : 在居中的基础上再等比缩放到"最长边 = 1"，便于第三方软件查看；
//                   注意这会丢失"1 单位 = 1 方块"的原始比例，因此默认关闭。
void margeAndWriteToObj(const GALIB_STD vector<LtSurfaceMesh>& meshes,
                        const char* p_filename, bool geom_center = true,
                        bool normalize_scale = false);

void writeToOff(const GALIB_STD vector<LtSurfaceMesh>& meshes,
                const char* p_filename);

}  // namespace galib::minecraft::cgal_support

#endif  //GALIB_INCLUDE_MINECRAFT_CGALSUPPORT_CGALLITTLETILEBUIDER_H
