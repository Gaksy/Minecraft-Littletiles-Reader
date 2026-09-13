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
#include <memory>
#include <optional>
#include <utility>

#include "File/Utf8Path.h"
#include "Log/GalibLog.h"
#include "Log/GalibText.h"
#include "Minecraft/TextureSupport/AssetsPackage.h"
#include "Minecraft/TextureSupport/BlockTextureTable.h"
#include "Minecraft/TextureSupport/MaterialManager.h"

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
using galib::minecraft::texture_support::AssetsPackage;
using galib::minecraft::texture_support::ComputeFaceUv;
using galib::minecraft::texture_support::FaceDirection;
using galib::minecraft::texture_support::FaceDirectionFromNormal;
using galib::minecraft::texture_support::MaterialManager;

using CGAL::SM_Vertex_index;

size_t addTilesFromBlockTilesEntities(
    const BlockTileEntities& kBlockTileEntities,
    vector<LtSurfaceMesh>& mesh_array) {
  const GridType grid_type = kBlockTileEntities.grid();
  size_t processed_tile_count = 0;
  // BlockTile -> BoxTile -> Tile

  // For BlockTile: iterate over all boxes in the block
  for (BlockTileEntities::const_iterator box_it = kBlockTileEntities.cbegin();
       box_it != kBlockTileEntities.cend(); ++box_it) {
    // Get BoxTile entities: get each Box Tile
    const BoxTileEnities& box_tile_entities = box_it->second;

    // For BoxTile: for each Tile in the Box Tile, build its faces
    for (BoxTileEnities::const_iterator tile_it = box_tile_entities.cbegin();
         tile_it != box_tile_entities.cend(); ++tile_it) {
      // Get Tile entities
      const TileEntity& tile_lt_entity = *tile_it;

      // Convert the tile entity into a CGAL mesh
      LtSurfaceMesh tile_cgal_mesh;

      // If it has offsets and goes out of bounds: use half-space clipping (convex
      // hexahedron intersected with an AABB) instead of a CGAL boolean intersection,
      // avoiding the many sliver triangles and superfluous edges that boolean operations
      // produce on coplanar faces.
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

// Extra information attached to each exported face: material index and face direction
// (which face's texture to use)
struct ExportedFaceInfo {
  std::size_t material_index{0};
  FaceDirection direction{FaceDirection::kUp};
  bool has_material{false};
};

// Determine the direction from the face's first three vertices (world coordinates;
// translation and uniform scaling do not change the direction)
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

// Quantized key used to deduplicate vt
long long QuantizeUvKey(const double kU, const double kV) {
  const long long u = static_cast<long long>(std::llround(kU * 1e7));
  const long long v = static_cast<long long>(std::llround(kV * 1e7));
  return u * 100000000LL + v;
}

}  // namespace

struct ObjMeshBuilder::Impl {
  explicit Impl(const ObjExportOptions& kOptions)
      : options(kOptions) {
    // The assets root is the library's only assets entry point: path assembly, mapping
    // table and tint all live in AssetsPackage. A failed open only degrades to "no
    // materials written", with the reason left for the host to print (a missing assets
    // directory is a common case).
    if (!options.assets_root.empty()) {
      package_ = AssetsPackage::Open(options.assets_root, &package_open_error_);
      if (package_) {
        materials_ = std::make_unique<MaterialManager>(*package_);
      }
    }
  }

  // Merge one mesh into the combined result and record its material and per-face UV
  // information
  void AddMesh(const LtSurfaceMesh& kMesh) {
    using Point = SurfaceMeshType::Point;

    {  // merge this one mesh
      const SurfaceMeshType& current_mesh = kMesh.surface_mesh();
      const auto block_coord = kMesh.block_coord_in_world();
      const std::string block_id = kMesh.block_id();
      // If the mesh recorded per-vertex local positions, use them (essential for
      // computing UVs when one mesh contains several blocks); otherwise fall back to
      // "world coordinate - block coordinate" (the case where LittleTiles has one mesh
      // per tile).
      const bool has_vertex_local = kMesh.has_vertex_local_positions();

      BlockFaceTextures face_textures;
      const bool has_block_textures =
          package_ && package_->Lookup(block_id, &face_textures) &&
          !face_textures.empty();

      // First create a corresponding vertex in the merged mesh for every vertex of the
      // current mesh
      std::vector<SurfaceMeshType::Vertex_index>
          current_mesh_vertices_in_marged;
      for (const SurfaceMeshType::vertex_index& v : current_mesh.vertices()) {
        LtPoint3 current_point = current_mesh.point(v);
        SurfaceMeshType::Vertex_index new_vertex =
            marged_mesh.add_vertex(current_point);
        current_mesh_vertices_in_marged.push_back(new_vertex);
        vertex_index_map[v] = new_vertex;  // map the original vertex index to the new vertex index
        // Record the "in-block local position": world coordinate = local coordinate +
        // block coordinate. UVs must be computed from it, and it cannot be derived from
        // coordinates after the export normalization (centring/scaling).
        if (has_vertex_local) {
          vertex_local_positions.push_back(kMesh.VertexLocalPosition(v));
        } else {
          vertex_local_positions.emplace_back(
              current_point.x() - block_coord.x,
              current_point.y() - block_coord.y,
              current_point.z() - block_coord.z);
        }
      }

      // Then add the faces to the merged mesh
      for (const SurfaceMeshType::face_index& f : current_mesh.faces()) {
        std::vector<SurfaceMeshType::Vertex_index> face_vertices;

        // Get all vertices of the current face
        CGAL::Vertex_around_face_iterator<SurfaceMeshType> vbegin, vend;
        for (boost::tie(vbegin, vend) =
                 vertices_around_face(current_mesh.halfedge(f), current_mesh);
             vbegin != vend; ++vbegin) {
          SurfaceMeshType::Vertex_index original_vertex = *vbegin;
          // Find the corresponding vertex in the merged mesh through the map
          auto it_vertex = vertex_index_map.find(original_vertex);
          if (it_vertex != vertex_index_map.end()) {
            face_vertices.push_back(it_vertex->second);
          }
        }

        // Keep n-gons: planar patches are now quads/polygons and are no longer forced
        // into triangles (CGAL's Surface_mesh supports polygon faces, and OBJ supports
        // them directly too)
        if (face_vertices.size() >= 3) {
          const std::size_t faces_before = marged_mesh.number_of_faces();
          // Use try-catch to guard against adding an invalid face
          try {
            marged_mesh.add_face(face_vertices);
          } catch (...) {
#ifdef GALIB_DEBUG
            // This is a "data is broken" warning and is not silenced by the progress
            // switch
            printf("%s",
                   galib::Tr("warning: cannot add face (duplicate or invalid "
                             "geometry)\n"));
#endif
          }

          // Only faces that were actually added get extra information recorded, keeping
          // them aligned with the mesh's face order
          if (marged_mesh.number_of_faces() > faces_before) {
            ExportedFaceInfo info;
            if (has_block_textures) {
              const FaceDirection direction = FaceDirectionOf(current_mesh, f);
              const std::string& texture_path = face_textures.Path(direction);
              if (texture_path.empty()) {
                ++missing_texture_faces;
              } else {
                // Biome tint: only faces whose model declares a tintindex need it
                std::uint32_t tint_rgb = 0x00FFFFFFu;
                const int tint_index = face_textures.Tint(direction);
                std::uint32_t tint_argb = 0;
                if (tint_index >= 0 &&
                    package_->TintOverride(block_id, tint_index, &tint_argb)) {
                  tint_rgb = tint_argb & 0x00FFFFFFu;
                }
                // The tile's own tint (LittleTiles coloured tiles)
                const std::uint32_t tile_color =
                    kMesh.has_tile_color()
                        ? static_cast<std::uint32_t>(kMesh.tile_color())
                        : 0xFFFFFFFFu;

                // The dedup key is the structured (texture, tint, tile colour); the name
                // is only used to write the MTL
                info.material_index =
                    materials_->Claim(texture_path, tint_rgb, tile_color);
                info.direction = direction;
                info.has_material = true;
              }
            }
            face_infos.push_back(info);
          }
        }
      }

      // Clear the current mesh's vertex map, preparing for the next mesh
      vertex_index_map.clear();
    }
  }

  // Normalize (centre/scale) and write the OBJ, MTL and textures
  bool WriteToFile(const std::string& kFilename) {
    using Point = SurfaceMeshType::Point;
    // Normalize: move the bounding-box centre to the origin; if requested, also
    // uniformly scale so the longest edge = 1
    if (marged_mesh.number_of_vertices() > 0 &&
        (options.geom_center || options.normalize_scale)) {
      CGAL::Bbox_3 bbox;
      bool first = true;

      // Compute the bounding box of all vertices
      for (const SurfaceMeshType::vertex_index& v : marged_mesh.vertices()) {
        const Point& p = marged_mesh.point(v);
        if (first) {
          bbox = p.bbox();
          first = false;
        } else {
          bbox = bbox + p.bbox();
        }
      }

      // Compute the bounding-box centre and scale factor (based on the longest edge,
      // preserving the aspect ratio)
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

      // Translate to the origin first, then scale if needed
      for (const SurfaceMeshType::vertex_index& v : marged_mesh.vertices()) {
        Point& p = marged_mesh.point(v);
        p = Point((p.x() - center_x) * scale, (p.y() - center_y) * scale,
                  (p.z() - center_z) * scale);
      }
    }

#ifdef GALIB_DEBUG
    // Check the merged mesh
    galib::ProgressPrintf(galib::Tr("merged mesh: "));
    galib::ProgressPrintf(galib::Tr("vertices: %u\n"),
                          marged_mesh.number_of_vertices());
    galib::ProgressPrintf(galib::Tr("faces: %u\n"),
                          marged_mesh.number_of_faces());
#endif

    // Export to an OBJ file
    // Note: ofstream does not create directories, so the output directory must be created
    // first, otherwise this fails outright
    // UTF-8 in, native path out: the output folder and the object name are chosen by
    // the host and may contain non-ASCII characters.
    const std::filesystem::path output_path = galib::Utf8Path(kFilename);
    if (output_path.has_parent_path()) {
      std::error_code create_error;
      std::filesystem::create_directories(output_path.parent_path(),
                                          create_error);
      if (create_error) {
        stats_.error = "cannot create output dir: " +
                       galib::Utf8String(output_path.parent_path()) + " : " +
                       create_error.message();
        if (!options.quiet) {
          std::cerr << galib::Tr("cannot create output dir: ")
                    << galib::Utf8String(output_path.parent_path()) << " : "
                    << create_error.message() << std::endl;
        }
        return false;
      }
    }

    std::ofstream out(output_path);
    if (!out) {
      std::error_code path_error;
      stats_.error =
          "cannot open file: " +
          galib::Utf8String(
              std::filesystem::weakly_canonical(output_path, path_error));
      if (!options.quiet) {
        std::cerr << galib::Tr("cannot open file: ")
                  << galib::Utf8String(std::filesystem::weakly_canonical(
                         output_path, path_error))
                  << std::endl;
      }
      return false;
    }

    // Output vertices
    // Increase precision: the default stream precision is only 6 significant digits, and
    // with coordinates in the thousands (uncentred world coordinates) the quantization
    // step can reach 0.01 blocks, silently changing the geometry.
    out << std::setprecision(9);
    const std::string obj_stem = galib::Utf8String(output_path.stem());
    const std::string mtl_filename = obj_stem + ".mtl";
    // Default destination of the textures: a same-named subdirectory next to the OBJ, so
    // dozens or hundreds of PNGs do not get mixed in with the OBJ; the MTL stays next to
    // the OBJ (Blender finds the MTL via the mtllib path and then resolves map_Kd
    // relative to the MTL). The host can point elsewhere with
    // ObjExportOptions::material_output_dir (see docs/assets-package.md section 4.3).
    const std::string texture_dir_name = obj_stem + "_textures";
    // Texture coordinates are written only when the materials and per-face information
    // are both aligned; otherwise it falls back to geometry-only output
    const bool write_materials =
        materials_ != nullptr && materials_->size() > 0 &&
        face_infos.size() == marged_mesh.number_of_faces() &&
        vertex_local_positions.size() == marged_mesh.number_of_vertices();
    if (write_materials) {
      out << "mtllib " << mtl_filename << "\n";
    }

    for (const SurfaceMeshType::vertex_index& v : marged_mesh.vertices()) {
      const LtPoint3& p = marged_mesh.point(v);
      out << "v " << p.x() << " " << p.y() << " " << p.z() << "\n";
    }

    // Compute the texture coordinate of each corner face by face. UVs are determined by
    // "in-block local coordinate + face direction": the local coordinate is recorded
    // while building the mesh and cannot be derived from coordinates after the export
    // normalization.
    std::vector<std::vector<std::size_t>>
        face_uv_indices;  // the vt index each corner maps to (1-based)
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
            // OBJ vt has its origin at the bottom-left while v = 0 is at the top of the
            // texture, so flip once
            const double vt_u = u;
            const double vt_v = 1.0 - uv_v;
            const long long key = QuantizeUvKey(vt_u, vt_v);
            auto found = uv_dedup.find(key);
            if (found == uv_dedup.end()) {
              uv_values.emplace_back(vt_u, vt_v);
              found = uv_dedup.emplace(key, uv_values.size())
                          .first;  // OBJ indices start at 1
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

    // Output faces (grouped by material; usemtl is written when the material changes)
    std::size_t face_index = 0;
    std::size_t current_material = std::numeric_limits<std::size_t>::max();
    for (const SurfaceMeshType::face_index& f : marged_mesh.faces()) {
      if (write_materials) {
        const ExportedFaceInfo& info = face_infos[face_index];
        if (info.has_material && info.material_index != current_material) {
          out << "usemtl " << materials_->Name(info.material_index) << "\n";
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

    // Write the MTL and textures: materials are registered and baked by
    // MaterialManager - one source texture is decoded only once and each tinted variant
    // is baked from the decode cache; the products land in the host-specified output
    // directory (the default is described above). The assets root is always read-only
    // and products are never written back there.
    std::size_t baked_texture_count = 0;
    std::string texture_dir_label = texture_dir_name;
    if (write_materials) {
      const std::filesystem::path mtl_path =
          output_path.parent_path() / galib::Utf8Path(mtl_filename);
      const std::filesystem::path texture_dir =
          options.material_output_dir.empty()
              ? output_path.parent_path() / galib::Utf8Path(texture_dir_name)
              : galib::Utf8Path(options.material_output_dir);

      // map_Kd is resolved relative to the MTL; the host-specified directory may not be
      // under the OBJ's subtree, so a relative path is computed once
      //
      // lexically_relative() rather than relative(): the latter touches the file
      // system and, on this toolchain, fails with ERROR_ACCESS_DENIED for every
      // path (ASCII ones included), which silently degraded map_Kd to an absolute
      // path. The lexical form is a pure string operation - both paths are already
      // absolute - and gives the wanted "same folder -> plain name" result.
      const std::filesystem::path relative =
          texture_dir.lexically_relative(output_path.parent_path());
      std::string map_kd_prefix =
          relative.empty() ? galib::Utf8GenericString(texture_dir)
                           : galib::Utf8GenericString(relative);
      if (map_kd_prefix.empty()) {
        map_kd_prefix = ".";
      }
      texture_dir_label = map_kd_prefix;

      std::string material_error;
      baked_texture_count =
          materials_->WriteTextures(galib::Utf8String(texture_dir),
                                    &material_error);
      materials_->WriteMtl(galib::Utf8String(mtl_path), map_kd_prefix,
                           &material_error);
      if (!material_error.empty() && !options.quiet) {
        std::cerr << material_error;
      }
      if (!material_error.empty()) {
        stats_.error += material_error;
      }
    }

    if (!options.quiet) {
      std::error_code path_error;
      std::cout << galib::Tr("exported merged mesh to: ")
                << galib::Utf8String(
                       std::filesystem::weakly_canonical(output_path,
                                                         path_error))
                << "\n";
      if (write_materials) {
        std::cout << galib::Tr("  materials ") << materials_->size()
                  << galib::Tr(", textures written ") << baked_texture_count
                  << galib::Tr(" into ") << texture_dir_label << "/"
                  << galib::Tr(" (source textures decoded ")
                  << materials_->decode_count() << galib::Tr(" times)");
        if (missing_texture_faces > 0) {
          std::cout << galib::Tr(" (") << missing_texture_faces
                    << galib::Tr(" faces without texture)");
        }
        std::cout << std::endl;
      } else if (!options.assets_root.empty()) {
        // A failed open and "this block is not in the table" are two different
        // things, so they are reported separately here
        std::cout
            << galib::Tr(
                   "  no textures exported: check that the assets root has "
                   "block_textures.tsv (")
            << options.assets_root << ")";
        if (!package_open_error_.empty()) {
          std::cout << " : " << package_open_error_;
        }
        std::cout << std::endl;
      }
    }
    stats_.vertices = marged_mesh.number_of_vertices();
    stats_.faces = marged_mesh.number_of_faces();
    stats_.materials = materials_ == nullptr ? 0 : materials_->size();
    stats_.textures_written = baked_texture_count;
    stats_.missing_texture_faces = missing_texture_faces;
    stats_.wrote_materials = write_materials;
    return true;
  }

  ObjExportOptions options;
  // The assets package is the only assets entry point; materials_ exists only when the
  // package is valid
  std::optional<AssetsPackage> package_;
  std::unique_ptr<MaterialManager> materials_;
  std::string package_open_error_;  // reason the assets package failed to open (for
                                    // reporting to the host)
  SurfaceMeshType marged_mesh;
  unordered_map<SurfaceMeshType::Vertex_index, SurfaceMeshType::Vertex_index>
      vertex_index_map;
  std::vector<ExportedFaceInfo> face_infos;
  std::vector<LtPoint3> vertex_local_positions;
  std::size_t missing_texture_faces{0};
  ObjMeshBuilder::Stats stats_;  // filled by WriteToFile
};

ObjMeshBuilder::ObjMeshBuilder(const ObjExportOptions& kOptions)
    : impl_(new Impl(kOptions)) {}

ObjMeshBuilder::~ObjMeshBuilder() = default;

void ObjMeshBuilder::AddMesh(const LtSurfaceMesh& kMesh) {
  impl_->AddMesh(kMesh);
}

bool ObjMeshBuilder::WriteToFile(const std::string& kFilename) {
  return impl_->WriteToFile(kFilename);
}

const ObjMeshBuilder::Stats& ObjMeshBuilder::stats() const {
  return impl_->stats_;
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
      // The same clipping strategy as the save-file path: clip only when the offset goes
      // outside its own box
      if (tile.is_offset_off_boundary()) {
        if (!ClipTileEntityToBox(tile_mesh, tile)) {
          continue;
        }
      } else {
        CreateMeshFromTileEntity(tile_mesh, tile);
      }
      tile_mesh.set_block_id(group.block_id);
      tile_mesh.set_tile_color(group.color, group.has_color);

      // grid -> block units, and translate to the structure origin; also record the
      // "relative coordinate inside the cell it lies in" (for UVs). A mesh inside a
      // structure spans several block cells, so the mesh-level block coordinate cannot be
      // used to derive it and it must be recorded per vertex.
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
