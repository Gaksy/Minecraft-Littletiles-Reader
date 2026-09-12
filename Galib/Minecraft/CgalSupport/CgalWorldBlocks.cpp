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
 * Date Created: 09/12/2026
 */

#include "Minecraft/CgalSupport/CgalWorldBlocks.h"

#include <array>
#include <map>
#include <utility>

#include "Log/GalibLog.h"
#include "Log/GalibText.h"

using galib::minecraft::BlockIdTable;
using galib::minecraft::ChunkBlocks;

namespace galib::minecraft::cgal_support {

namespace {

constexpr int kChunkSizeBlocks = 16;
constexpr int kWorldHeight = 256;

// 立方体六个面：4 个角的局部偏移（顺序决定法线方向，与 tile 的绕向一致）
struct FaceSpec {
  int neighbor[3];
  int corners[4][3];
};

constexpr FaceSpec kFaces[] = {
    {{1, 0, 0}, {{1, 0, 1}, {1, 0, 0}, {1, 1, 0}, {1, 1, 1}}},   // east
    {{-1, 0, 0}, {{0, 0, 0}, {0, 0, 1}, {0, 1, 1}, {0, 1, 0}}},  // west
    {{0, 0, 1}, {{0, 0, 1}, {1, 0, 1}, {1, 1, 1}, {0, 1, 1}}},   // south
    {{0, 0, -1}, {{1, 0, 0}, {0, 0, 0}, {0, 1, 0}, {1, 1, 0}}},  // north
    {{0, 1, 0}, {{0, 1, 1}, {1, 1, 1}, {1, 1, 0}, {0, 1, 0}}},   // up
    {{0, -1, 0}, {{0, 0, 0}, {1, 0, 0}, {1, 0, 1}, {0, 0, 1}}},  // down
};

}  // namespace

void BuildWorldBlockMeshes(const int kWorldOriginX, const int kWorldOriginZ,
                           const std::vector<ChunkBlocks>& kChunks,
                           const int kChunkSizeX, const int kChunkSizeZ,
                           const BlockIdTable& kBlockIdTable,
                           const bool kCullHiddenFaces,
                           std::vector<LtSurfaceMesh>* const p_desc_meshes) {
  if (!p_desc_meshes || kChunks.empty()) {
    return;
  }

  // 1. 把整片区域摊平成一张世界方块网格，方便跨区块做邻居剔除
  const int size_x = kChunkSizeX * kChunkSizeBlocks;
  const int size_z = kChunkSizeZ * kChunkSizeBlocks;
  const std::size_t plane = static_cast<std::size_t>(size_x) * size_z;
  std::vector<ChunkBlocks::State> grid(plane * kWorldHeight);
  const auto index_of = [size_x, plane](const int kX, const int kY,
                                        const int kZ) {
    return static_cast<std::size_t>(kX) +
           static_cast<std::size_t>(kZ) * size_x +
           static_cast<std::size_t>(kY) * plane;
  };

  for (int chunk_z = 0; chunk_z < kChunkSizeZ; ++chunk_z) {
    for (int chunk_x = 0; chunk_x < kChunkSizeX; ++chunk_x) {
      const std::size_t chunk_index =
          static_cast<std::size_t>(chunk_z) * kChunkSizeX + chunk_x;
      if (chunk_index >= kChunks.size()) {
        continue;
      }
      const ChunkBlocks& chunk = kChunks[chunk_index];
      for (int y = 0; y < kWorldHeight; ++y) {
        for (int z = 0; z < kChunkSizeBlocks; ++z) {
          for (int x = 0; x < kChunkSizeBlocks; ++x) {
            const ChunkBlocks::State& state = chunk.At(x, y, z);
            if (state.is_air() && !state.little_tiles_host()) {
              continue;
            }
            grid[index_of(chunk_x * kChunkSizeBlocks + x, y,
                          chunk_z * kChunkSizeBlocks + z)] = state;
          }
        }
      }
    }
  }

  const auto state_at = [&grid, &index_of, size_x, size_z](
                            const int kX, const int kY,
                            const int kZ) -> const ChunkBlocks::State& {
    static const ChunkBlocks::State kAir{};
    if (kX < 0 || kY < 0 || kZ < 0 || kX >= size_x || kY >= kWorldHeight ||
        kZ >= size_z) {
      return kAir;
    }
    return grid[index_of(kX, kY, kZ)];
  };

#ifdef GALIB_DEBUG
  {
    std::size_t filled = 0;
    std::size_t hosts = 0;
    for (const ChunkBlocks::State& state : grid) {
      if (!state.is_air()) {
        ++filled;
      }
      if (state.little_tiles_host()) {
        ++hosts;
      }
    }
    ProgressPrintf(
        Tr("[worldblocks] 网格 %dx%dx%d：非空气 %zu，LT 宿主 %zu\n",
           "[worldblocks] grid %dx%dx%d: %zu solid blocks, %zu LT hosts\n"),
        size_x, kWorldHeight, size_z, filled, hosts);
  }
#endif

  // 2. 按 (方块 id, meta) 分组，每组一个网格
  std::map<std::pair<std::uint16_t, std::uint8_t>, std::size_t> group_index;
#ifdef GALIB_DEBUG
  std::size_t emitted_blocks = 0;
  std::size_t emitted_faces = 0;
  std::size_t culled_faces = 0;
#endif
  // add_face 失败时 CGAL 不抛异常，只返回 null_face；必须自己统计，否则会静默丢面
  std::size_t rejected_faces = 0;
  for (int y = 0; y < kWorldHeight; ++y) {
    for (int z = 0; z < size_z; ++z) {
      for (int x = 0; x < size_x; ++x) {
        const ChunkBlocks::State& state = grid[index_of(x, y, z)];
        if (state.is_air() || state.little_tiles_host()) {
          continue;
        }
        const std::string block_name =
            kBlockIdTable.BlockName(state.block_id, state.meta);
        if (block_name.empty()) {
          continue;  // 表里没有这个 id
        }

        const auto key = std::make_pair(state.block_id, state.meta);
        auto found = group_index.find(key);
        if (found == group_index.end()) {
          LtSurfaceMesh mesh;
          mesh.set_block_id(block_name);
          found = group_index.emplace(key, p_desc_meshes->size()).first;
          p_desc_meshes->push_back(std::move(mesh));
        }
        LtSurfaceMesh& mesh = (*p_desc_meshes)[found->second];
        SurfaceMeshType& surface = mesh.surface_mesh();

        // 角点：世界坐标 = 世界原点 + 网格内坐标 + 0/1 偏移；本地坐标就是那个 0/1 偏移
        const int world_x = kWorldOriginX + x;
        const int world_z = kWorldOriginZ + z;
        // 立方体的 8 个角只在"这一个方块"内共享，绝不跨方块焊接。
        // CGAL 的 Euler::add_face 只接受"每个顶点都还在边界上、每条边都还不存在或为边界边"
        // 的环；跨方块焊接顶点后，先写完的那个方块会把自己的顶点变成内部顶点，
        // 于是相邻方块的这些面被静默拒绝（不抛异常、不返回失败），
        // 表现为"完整方块只剩一两个面"。每个方块独立成一张闭合曲面就没有这个问题。
        std::array<SurfaceMeshType::Vertex_index, 8> corners;
        corners.fill(SurfaceMeshType::null_vertex());
        const auto corner_index = [](const int kLocalX, const int kLocalY,
                                     const int kLocalZ) {
          return static_cast<std::size_t>(kLocalX + (kLocalY << 1) +
                                          (kLocalZ << 2));
        };
        const auto vertex_of =
            [&surface, &mesh, &corners, &corner_index, world_x, world_z, y](
                const int kLocalX, const int kLocalY, const int kLocalZ) {
              SurfaceMeshType::Vertex_index& cached =
                  corners[corner_index(kLocalX, kLocalY, kLocalZ)];
              if (cached != SurfaceMeshType::null_vertex()) {
                return cached;
              }
              cached = surface.add_vertex(
                  LtPoint3(world_x + kLocalX, y + kLocalY, world_z + kLocalZ));
              mesh.SetVertexLocalPosition(cached,
                                          LtPoint3(kLocalX, kLocalY, kLocalZ));
              return cached;
            };

        // kFaces 的下标与 TileFaceID 一致（EAST/WEST/SOUTH/NORTH/UP/DOWN），
        // 因此"邻居朝向本方块的那个面"就是 face_index ^ 1。
        for (std::size_t face_index = 0; face_index < 6; ++face_index) {
          const FaceSpec& face = kFaces[face_index];
          if (kCullHiddenFaces) {
            const ChunkBlocks::State& neighbor =
                state_at(x + face.neighbor[0], y + face.neighbor[1],
                         z + face.neighbor[2]);
            // 挡得住这个面的只有两种情况：
            //   1. 邻居是普通实心方块；
            //   2. 邻居是 LT 宿主，且它那一侧的整个面被 tile 铺满
            //      （例如一整块 tile 砌的实心方块）。
            // LT 宿主位置的方块 id 是 LittleTiles 自己的方块（实测 id 257，非空气），
            // 但它占的往往只是一小块几何（例如花盆），此时必须保留这个面。
            const std::uint8_t facing =
                static_cast<std::uint8_t>(1u << (face_index ^ 1));
            const bool blocked = !neighbor.is_air() &&
                                 (!neighbor.little_tiles_host() ||
                                  (neighbor.covered_faces() & facing) != 0);
            if (blocked) {
#ifdef GALIB_DEBUG
              ++culled_faces;
#endif
              continue;
            }
          }
          std::vector<SurfaceMeshType::Vertex_index> face_corners;
          face_corners.reserve(4);
          for (const auto& offset : face.corners) {
            face_corners.push_back(vertex_of(offset[0], offset[1], offset[2]));
          }
          if (surface.add_face(face_corners) == SurfaceMeshType::null_face()) {
            ++rejected_faces;
          }
#ifdef GALIB_DEBUG
          ++emitted_faces;
#endif
        }
#ifdef GALIB_DEBUG
        ++emitted_blocks;
#endif
      }
    }
  }

#ifdef GALIB_DEBUG
  ProgressPrintf(
      Tr("[worldblocks] 输出方块 %zu 个，面 %zu 个（邻居剔除 %zu 个，被 CGAL "
         "拒绝 "
         "%zu 个）\n",
         "[worldblocks] emitted %zu blocks, %zu faces (culled %zu, rejected by "
         "CGAL %zu)\n"),
      emitted_blocks, emitted_faces, culled_faces, rejected_faces);
  {
    // 分组网格里"实际保存下来的面/顶点"——若远少于 emitted_faces，说明 add_face 被 CGAL 拒绝了
    std::size_t stored_faces = 0;
    std::size_t stored_vertices = 0;
    for (const LtSurfaceMesh& mesh : *p_desc_meshes) {
      stored_faces += mesh.surface_mesh().number_of_faces();
      stored_vertices += mesh.surface_mesh().number_of_vertices();
    }
    ProgressPrintf(
        Tr("[worldblocks] 分组网格实际保存：%zu 个网格，面 %zu，顶点 %zu\n",
           "[worldblocks] stored in group meshes: %zu meshes, %zu faces, %zu "
           "vertices\n"),
        p_desc_meshes->size(), stored_faces, stored_vertices);
  }
#endif
}

}  // namespace galib::minecraft::cgal_support
