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

// The cube's six faces: local offsets of the 4 corners (the order determines the
// normal direction, consistent with the tile winding)
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

  // 1. Flatten the whole region into one world block grid, so neighbours can be
  // culled across chunks
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
        Tr("[worldblocks] grid %dx%dx%d: %zu solid blocks, %zu LT hosts\n"),
        size_x, kWorldHeight, size_z, filled, hosts);
  }
#endif

  // 2. Group by (block id, meta), one mesh per group
  std::map<std::pair<std::uint16_t, std::uint8_t>, std::size_t> group_index;
#ifdef GALIB_DEBUG
  std::size_t emitted_blocks = 0;
  std::size_t emitted_faces = 0;
  std::size_t culled_faces = 0;
#endif
  // When add_face fails CGAL throws no exception and only returns null_face; it must
  // be counted here, otherwise faces are silently dropped
  std::size_t rejected_faces = 0;
  for (int y = 0; y < kWorldHeight; ++y) {
    for (int z = 0; z < size_z; ++z) {
      for (int x = 0; x < size_x; ++x) {
        const ChunkBlocks::State& state = grid[index_of(x, y, z)];
        if (state.is_air() || state.little_tiles_host()) {
          continue;
        }
        // The id table only turns a numeric id into a block name so the texture
        // table can be consulted. Geometry does not need it, so an unresolvable id
        // still gets its cube: it ends up untextured, which is exactly the "white
        // model" a host wants when it has no assets package. Skipping here used to
        // make a missing block_ids.tsv look like "the plain-block option does
        // nothing".
        std::string block_name =
            kBlockIdTable.BlockName(state.block_id, state.meta);
        if (block_name.empty()) {
          block_name = "unknown_id_" + std::to_string(state.block_id) + "_" +
                       std::to_string(state.meta);
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

        // Corners: world coordinate = world origin + in-grid coordinate + 0/1 offset;
        // the local coordinate is that 0/1 offset
        const int world_x = kWorldOriginX + x;
        const int world_z = kWorldOriginZ + z;
        // The cube's 8 corners are shared only within "this one block" and are never
        // welded across blocks. CGAL's Euler::add_face only accepts a cycle where
        // "every vertex is still on the boundary and every edge does not exist yet or is
        // a boundary edge"; after welding vertices across blocks, the block written first
        // turns its own vertices into interior vertices, so these faces of the
        // neighbouring block are silently rejected (no exception, no failure return),
        // showing up as "a full block has only one or two faces left". Making each block
        // its own closed surface avoids this entirely.
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

        // The indices of kFaces match TileFaceID (EAST/WEST/SOUTH/NORTH/UP/DOWN), so
        // "the neighbour's face pointing at this block" is face_index ^ 1.
        for (std::size_t face_index = 0; face_index < 6; ++face_index) {
          const FaceSpec& face = kFaces[face_index];
          if (kCullHiddenFaces) {
            const ChunkBlocks::State& neighbor =
                state_at(x + face.neighbor[0], y + face.neighbor[1],
                         z + face.neighbor[2]);
            // Only two cases can hide this face:
            //   1. the neighbour is a plain solid block;
            //   2. the neighbour is an LT host whose entire face on that side is covered
            //      by tiles (for example a solid block built from a full tile).
            // The block id at an LT host position is LittleTiles' own block (measured id
            // 257, non-air), but it often occupies only a small piece of geometry (for
            // example a flower pot), in which case this face must be kept.
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
      Tr("[worldblocks] emitted %zu blocks, %zu faces (culled %zu, rejected by "
         "CGAL %zu)\n"),
      emitted_blocks, emitted_faces, culled_faces, rejected_faces);
  {
    // The faces/vertices actually stored in the group meshes - if far fewer than
    // emitted_faces, add_face was rejected by CGAL
    std::size_t stored_faces = 0;
    std::size_t stored_vertices = 0;
    for (const LtSurfaceMesh& mesh : *p_desc_meshes) {
      stored_faces += mesh.surface_mesh().number_of_faces();
      stored_vertices += mesh.surface_mesh().number_of_vertices();
    }
    ProgressPrintf(
        Tr("[worldblocks] stored in group meshes: %zu meshes, %zu faces, %zu "
           "vertices\n"),
        p_desc_meshes->size(), stored_faces, stored_vertices);
  }
#endif
}

}  // namespace galib::minecraft::cgal_support
