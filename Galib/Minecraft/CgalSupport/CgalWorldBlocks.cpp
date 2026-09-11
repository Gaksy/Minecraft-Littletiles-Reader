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

#include <cstdint>
#include <map>
#include <unordered_map>
#include <utility>

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
            if (state.is_air() && !state.little_tiles_host) {
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

  // 2. 按 (方块 id, meta) 分组，每组一个网格
  std::map<std::pair<std::uint16_t, std::uint8_t>, std::size_t> group_index;
  // 每组一张"世界坐标 -> 顶点"表，用来在同一个方块内共享顶点。
  // 坐标都是整数，打包成 64 位做键（x/z 各 21 位、y 22 位足够覆盖任意区域）。
  std::vector<std::unordered_map<std::uint64_t, SurfaceMeshType::Vertex_index>>
      group_vertices;
  const auto pack_position = [](const int kX, const int kY, const int kZ) {
    return (static_cast<std::uint64_t>(kX) << 42) |
           (static_cast<std::uint64_t>(kY) << 21) |
           static_cast<std::uint64_t>(kZ);
  };

  for (int y = 0; y < kWorldHeight; ++y) {
    for (int z = 0; z < size_z; ++z) {
      for (int x = 0; x < size_x; ++x) {
        const ChunkBlocks::State& state = grid[index_of(x, y, z)];
        if (state.is_air() || state.little_tiles_host) {
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
          group_vertices.emplace_back();
        }
        LtSurfaceMesh& mesh = (*p_desc_meshes)[found->second];
        SurfaceMeshType& surface = mesh.surface_mesh();
        std::unordered_map<std::uint64_t, SurfaceMeshType::Vertex_index>&
            vertices = group_vertices[found->second];

        // 角点：世界坐标 = 世界原点 + 网格内坐标 + 0/1 偏移；本地坐标就是那个 0/1 偏移
        const int world_x = kWorldOriginX + x;
        const int world_z = kWorldOriginZ + z;
        const auto vertex_of = [&surface, &mesh, &vertices, &pack_position](
                                   const int kX, const int kY, const int kZ,
                                   const int kLocalX, const int kLocalY,
                                   const int kLocalZ) {
          const std::uint64_t key = pack_position(kX, kY, kZ);
          const auto found_vertex = vertices.find(key);
          if (found_vertex != vertices.end()) {
            return found_vertex->second;
          }
          const SurfaceMeshType::Vertex_index v =
              surface.add_vertex(LtPoint3(kX, kY, kZ));
          mesh.SetVertexLocalPosition(v, LtPoint3(kLocalX, kLocalY, kLocalZ));
          vertices.emplace(key, v);
          return v;
        };

        for (const FaceSpec& face : kFaces) {
          if (kCullHiddenFaces) {
            const ChunkBlocks::State& neighbor =
                state_at(x + face.neighbor[0], y + face.neighbor[1],
                         z + face.neighbor[2]);
            if (!neighbor.is_air()) {
              continue;
            }
          }
          std::vector<SurfaceMeshType::Vertex_index> corners;
          corners.reserve(4);
          for (const auto& corner : face.corners) {
            corners.push_back(vertex_of(world_x + corner[0], y + corner[1],
                                        world_z + corner[2], corner[0],
                                        corner[1], corner[2]));
          }
          surface.add_face(corners);
        }
      }
    }
  }
}

}  // namespace galib::minecraft::cgal_support
