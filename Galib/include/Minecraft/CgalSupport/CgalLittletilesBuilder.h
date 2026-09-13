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

#include <memory>
#include <vector>

#include "GalibNamespaceDef.h"
#include "Minecraft/CgalSupport/CgalTypeDef.h"
#include "Minecraft/LittleTiles.h"
#include "Minecraft/LtStructure.h"

namespace galib::minecraft::cgal_support {
class ChunkMesh {
 public:
  using container = std::vector<LtSurfaceMesh>;
  using const_iterator = std::vector<LtSurfaceMesh>::const_iterator;
  using iterator = std::vector<LtSurfaceMesh>::iterator;
  using size_type = std::vector<LtSurfaceMesh>::size_type;

 public:
  ChunkMesh() = default;
  ~ChunkMesh() = default;

 public:
  size_type AddTilesFromChunkTileEntities(
      const galib::minecraft::littletiles::ChunkTileEntities&
          kChunkTileEntities);
  [[nodiscard]] const container& mesh_array() const;
  void Clear();

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
//         std:: vector<galib:: minecraft::littletiles::LittleTilesCoord>::size_type point_index;
//         const LtSurfaceMesh& desc_face;
//     };
//
// public:
//     bool exportToFile(const char* p_file_path, const std:: vector<LtSurfaceMesh>& meshes);
//
// public:
//     std:: vector<galib:: minecraft::littletiles::LittleTilesCoord> coord_array_;
//
// };

// OBJ export options.
struct ObjExportOptions {
  // Move the centre of the bounding box to the origin (enabled by default).
  bool geom_center{true};
  // On top of centring, uniformly scale so the longest edge = 1, which is convenient
  // for viewing in third-party software; it loses the original "1 unit = 1 block"
  // scale, so it is disabled by default.
  bool normalize_scale{false};
  // Assets root (must contain block_textures.tsv and textures/, see AssetsPackage).
  // Specified by the host; when empty only geometry is exported and no
  // vt / usemtl / mtl is written.
  std::string assets_root;
  // Output directory for the texture products (the PNGs land here).
  // Empty = the default behaviour, a <obj name>_textures/ folder next to the OBJ.
  // When set, the MTL's map_Kd is automatically written as the relative path from the
  // MTL's directory to this one.
  // The assets root is always read-only; products are never written back there.
  std::string material_output_dir;
  // Suppress this builder's own summary and warning output (stdout / stderr). A host
  // that speaks a machine-readable protocol (the UI's NDJSON progress) sets this so
  // human text never lands inside its stream; failures are still reported through the
  // return value and Stats.
  bool quiet{false};
};

// Merge all tile meshes and write an OBJ (optionally writing the MTL and the
// required textures as well).
void MergeAndWriteToObj(const std::vector<LtSurfaceMesh>& meshes,
                        const char* p_filename,
                        const ObjExportOptions& options = ObjExportOptions());

// Incremental OBJ builder: AddMesh batch by batch (each batch can be released as
// soon as it has been added) and finally WriteToFile.
// Purpose: avoid holding all meshes at once during large-area exports (hundreds of
// chunks, hundreds of thousands of tiles) - collecting every mesh into a vector first
// and handing it to MergeAndWriteToObj would get the process killed for excessive
// memory usage.
class ObjMeshBuilder {
 public:
  // What a call to WriteToFile actually produced. Lets a host (CLI, UI) report the
  // result without parsing the human-readable log lines.
  struct Stats {
    std::size_t vertices{0};
    std::size_t faces{0};
    std::size_t materials{0};
    std::size_t textures_written{0};
    std::size_t missing_texture_faces{0};
    bool wrote_materials{false};
    // Why WriteToFile returned false. Empty on success. A host that silences the
    // human-readable output (quiet) still needs to say what went wrong.
    std::string error;
  };

  explicit ObjMeshBuilder(const ObjExportOptions& kOptions);
  ~ObjMeshBuilder();

  ObjMeshBuilder(const ObjMeshBuilder&) = delete;
  ObjMeshBuilder& operator=(const ObjMeshBuilder&) = delete;

  // Merge one mesh into the result (vertices, faces, materials and per-face UV information)
  void AddMesh(const LtSurfaceMesh& kMesh);

  // Normalize and write the OBJ (including mtllib/vt/usemtl plus the MTL and textures)
  bool WriteToFile(const char* kFilename);

  // Valid after WriteToFile.
  const Stats& stats() const;

 private:
  struct Impl;  // defined in the .cpp
  std::unique_ptr<Impl> impl_;
};

void WriteToOff(const std::vector<LtSurfaceMesh>& meshes,
                const char* p_filename);

// Convert a LittleTiles structure (an SNBT structure file) into meshes and hand them
// to the OBJ builder.
//
// It follows the same geometry chain as the save-file path (half-space clipping ->
// mesh -> UV); only the coordinates differ: a structure uses structure-space grid
// coordinates spanning many blocks, so two things are done here:
//   1. Scale by grid into block units and subtract the structure origin min;
//   2. Record, per vertex, the "relative coordinate inside the block cell it belongs
//      to", so UVs can be sampled by position (one mesh spans several cells, so unlike
//      the save-file path the mesh-level block coordinate cannot be used to derive it).
// Child structures (children) have already been expanded by LtStructure. Returns the
// number of meshes merged.
std::size_t AddStructureToObjBuilder(
    const galib::minecraft::littletiles::LtStructure& kStructure,
    ObjMeshBuilder* p_desc_builder);

}  // namespace galib::minecraft::cgal_support

#endif  //GALIB_INCLUDE_MINECRAFT_CGALSUPPORT_CGALLITTLETILEBUIDER_H
