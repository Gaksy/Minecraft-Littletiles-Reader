#include <fstream>
#include "LittleTilesObjBuilder.h"
#include "../Exception.h"

using galib::Exception::GalibErrorCode;
using galib::Exception::GalibException;

#include "CGAL/Polygon_mesh_processing/triangulate_faces.h"
#include "CGAL/Polygon_mesh_processing/corefinement.h"

using galib::minecraft::littletiles::ChunkFaceBuilder;
using galib::minecraft::littletiles::geometry::LtKernel;
using galib::minecraft::littletiles::geometry::LtMesh;
using galib::minecraft::littletiles::geometry::LtPoint;
using galib::minecraft::littletiles::BoxesTiles;
using galib::minecraft::Coord::Integer3dCoord;
using galib::minecraft::littletiles::GridType;
using galib::minecraft::Coord::ChunkCoord;
using galib::minecraft::littletiles::ObjFormatBuilder;
using galib::minecraft::Coord::Float3dCoord;
using galib::minecraft::Coord::WorldBlockCoord;
ChunkFaceBuilder::ChunkFaceBuilder()
{
}

bool ChunkFaceBuilder::SetChunkTiles(const ChunkTiles& kDescChunkTiles, const galib::minecraft::Coord::WorldBlockCoord& offset)
{
    if (kDescChunkTiles.IsEmpty()) {
        return false;
    }

    std::string block_id;

    LtMesh mesh;
    LtMesh aabb_mesh;
    LtMesh cuted_mesh;
    GridType grid;
    WorldBlockCoord block_coord;

    for (ChunkTiles::const_iterator block_it = kDescChunkTiles.cbegin(); block_it != kDescChunkTiles.cend(); ++block_it) {
        if (block_it->IsEmpty()) { continue; }
        grid = block_it->GetGrid();
        block_coord = block_it->GetBlockCoord();
        block_coord.x += offset.x;
        block_coord.y += offset.y;
        block_coord.z += offset.z;
        //printf("%d %d %d\n", block_coord.x, block_coord.y, block_coord.z);
        for (BlockTiles::const_iterator boxes_it = block_it->cbegin(); boxes_it != block_it->cend(); ++boxes_it) {
            block_id = boxes_it->first;
            
            // Add to map
            MeshMap::iterator mesh_map_it = mesh_map_.insert(MeshMapPair(block_id, MeshArray())).first;
            MeshArray& desc_mesh_array = mesh_map_it->second;

            for (BoxesTiles::const_iterator tile_it = boxes_it->second.cbegin(); tile_it != boxes_it->second.cend(); ++tile_it) {
                mesh.clear();
                aabb_mesh.clear();
                cuted_mesh.clear();

                if (!tile_it->HasAnyOffsetEnable()) {
                    try {
                        GetMeshFromTileBoxDataWithoutOffset(mesh, *tile_it);
                        ToBlockSize(mesh, grid);
                        ToBlockCoord(mesh, block_coord);
                        desc_mesh_array.push_back(mesh);
                    }
                    catch (...) { ; }
                    continue;
                }
               
                // Get face array
                std::vector<ChunkFaceBuilder::LtFaceData> face_array;
                GetFaceArray(face_array, tile_it);

                // Processed face array
                std::vector<ChunkFaceBuilder::FaceBaseData> done_facearray;
                bool overflow = false;

                // PROCESSED
                for (ChunkFaceBuilder::LtFaceData& face_data : face_array) {

                    if (!face_data.HasAnyOffset()) {    // if don't have any offset
                        if (face_data.IsFace()) {
                            done_facearray.push_back(face_data.to_base());
                        }
                        continue;
                    }

                    
                    ApplyAngleOffset(face_data.p_1, face_data.p_1_state);
                    ApplyAngleOffset(face_data.p_2, face_data.p_2_state);
                    ApplyAngleOffset(face_data.p_3, face_data.p_3_state);
                    ApplyAngleOffset(face_data.p_4, face_data.p_4_state);

                    if (!face_data.IsFace()) {
                        continue;
                    }

                    if (!face_data.IsShouldTriangle()) {
                        done_facearray.push_back(face_data.to_base());
                        continue;
                    }

                    FaceBaseData triangle_1;
                    FaceBaseData triangle_2;
                    SplitFace(face_data, triangle_1, triangle_2);

                    if (triangle_1.IsFace()) {
                        done_facearray.push_back(triangle_1);
                    }
                    if (triangle_2.IsFace()) {
                        done_facearray.push_back(triangle_2);
                    }
                }

                // Cut use CGAL
                if (!done_facearray.empty() && tile_it->IsOffsetWithinBounds()) {
                    try {
                        // To CGAL mesh
                        GetMeshFromFaceBaseArray(mesh, done_facearray);
                        for (auto face : mesh.faces()) {
                            CGAL::Polygon_mesh_processing::triangulate_face(face, mesh);
                        }
                        //std::cout << mesh << std::endl;

                        // Get aabb mesh
                        GetMeshFromTileBoxDataWithoutOffset(aabb_mesh, *tile_it);
                        for (auto face : aabb_mesh.faces()) {
                            CGAL::Polygon_mesh_processing::triangulate_face(face, aabb_mesh);
                        }

                        if (CGAL::Polygon_mesh_processing::corefine_and_compute_intersection(mesh, aabb_mesh, cuted_mesh)) {
                            ToBlockSize(cuted_mesh, grid);
                            ToBlockCoord(cuted_mesh, block_coord);
                            desc_mesh_array.push_back(cuted_mesh);
                        }
                    }
                    catch (...) {
                        continue;
                    }
                }
                else {
                    GetMeshFromFaceBaseArray(mesh, done_facearray);
                    ToBlockSize(mesh, grid);
                    ToBlockCoord(mesh, block_coord);
                    desc_mesh_array.push_back(mesh);
                }
            }

            if (desc_mesh_array.empty()) {
                mesh_map_.erase(mesh_map_it);
            }
        }
    }

    chunk_coord_ = kDescChunkTiles.GetChunkCoord();
    return true;
}

const ChunkCoord& ChunkFaceBuilder::GetChunkCoord() const
{
    return chunk_coord_;
}

ChunkFaceBuilder::mesh_map_constIterator ChunkFaceBuilder::cbegin() const
{
    IsEmptyAndTherow("Get cbegin error");
    return mesh_map_.cbegin();
}
ChunkFaceBuilder::mesh_map_constIterator ChunkFaceBuilder::cend() const
{
    IsEmptyAndTherow("Get cend error");
    return mesh_map_.cend();
}

void ChunkFaceBuilder::Clear()
{
    mesh_map_.clear();
}

bool ChunkFaceBuilder::IsEmpty() const
{
    return mesh_map_.empty();
}

void ChunkFaceBuilder::IsEmptyAndTherow(const char* const kPMessage) const
{
    if (IsEmpty()) {
        throw GalibException(GalibErrorCode::minecraft_object_uninitialized, kPMessage, "galib::minecraft::littletiles::ChunkFaceBuilder");
    }
}

void ChunkFaceBuilder::GetMeshFromTileBoxDataWithoutOffset(LtMesh& desc_mesh, const BoxesTiles::TileBoxData& kTileBoxData)
{
    desc_mesh.clear();

    LtMesh::Vertex_index v1 = desc_mesh.add_vertex(LtKernel::Point_3(kTileBoxData.x_1, kTileBoxData.y_1, kTileBoxData.z_2));
    LtMesh::Vertex_index v2 = desc_mesh.add_vertex(LtKernel::Point_3(kTileBoxData.x_1, kTileBoxData.y_1, kTileBoxData.z_1));
    LtMesh::Vertex_index v3 = desc_mesh.add_vertex(LtKernel::Point_3(kTileBoxData.x_2, kTileBoxData.y_1, kTileBoxData.z_1));
    LtMesh::Vertex_index v4 = desc_mesh.add_vertex(LtKernel::Point_3(kTileBoxData.x_2, kTileBoxData.y_1, kTileBoxData.z_2));
    LtMesh::Vertex_index v5 = desc_mesh.add_vertex(LtKernel::Point_3(kTileBoxData.x_1, kTileBoxData.y_2, kTileBoxData.z_1));
    LtMesh::Vertex_index v6 = desc_mesh.add_vertex(LtKernel::Point_3(kTileBoxData.x_1, kTileBoxData.y_2, kTileBoxData.z_2));
    LtMesh::Vertex_index v7 = desc_mesh.add_vertex(LtKernel::Point_3(kTileBoxData.x_2, kTileBoxData.y_2, kTileBoxData.z_2));
    LtMesh::Vertex_index v8 = desc_mesh.add_vertex(LtKernel::Point_3(kTileBoxData.x_2, kTileBoxData.y_2, kTileBoxData.z_1));

    desc_mesh.add_face(v1, v2, v3, v4);  // DOWN
    desc_mesh.add_face(v5, v6, v7, v8);  // UP
    desc_mesh.add_face(v8, v3, v2, v5);  // NORTH
    desc_mesh.add_face(v6, v1, v4, v7);  // SOUTH
    desc_mesh.add_face(v5, v2, v1, v6);  // WEST
    desc_mesh.add_face(v7, v4, v3, v8);  // EAST

    return;
}

void ChunkFaceBuilder::GetMeshFromFaceBaseArray(LtMesh& desc_mesh, const std::vector<ChunkFaceBuilder::FaceBaseData>& kFaceArray) {
    desc_mesh.clear();



    // Helper lambda to add a vertex and return its index
    auto add_vertex = [&desc_mesh](float x, float y, float z) {
        LtKernel::Point_3 point(x, y, z);
        return desc_mesh.add_vertex(point);
    };

    // Map to keep track of vertex indices for each unique vertex
    std::unordered_map<LtKernel::Point_3, LtMesh::Vertex_index> vertex_map;

    // Helper function to get or add vertex and return its index
    auto get_or_add_vertex = [&vertex_map, &desc_mesh, &add_vertex](const LtKernel::Point_3& p) {
        LtKernel::Point_3 point(p);
        auto it = vertex_map.find(point);
        if (it != vertex_map.end()) {
            return it->second;
        }
        else {
            LtMesh::Vertex_index index = add_vertex(p.x(), p.y(), p.z());
            vertex_map[point] = index;
            return index;
        }
    };

    for (const FaceBaseData& desc_face : kFaceArray) {
        LtMesh::Vertex_index v1_index = get_or_add_vertex(LtKernel::Point_3(
            desc_face.p_1.x,
            desc_face.p_1.y,
            desc_face.p_1.z
        ));
        LtMesh::Vertex_index v2_index = get_or_add_vertex(LtKernel::Point_3(
            desc_face.p_2.x,
            desc_face.p_2.y,
            desc_face.p_2.z
        ));
        LtMesh::Vertex_index v3_index = get_or_add_vertex(LtKernel::Point_3(
            desc_face.p_3.x,
            desc_face.p_3.y,
            desc_face.p_3.z
        ));
        LtMesh::Vertex_index v4_index = get_or_add_vertex(LtKernel::Point_3(
            desc_face.p_4.x,
            desc_face.p_4.y,
            desc_face.p_4.z
        ));

        if (desc_face.triangle) {
            desc_mesh.add_face(v1_index, v2_index, v3_index);
        }
        else {
            desc_mesh.add_face(v1_index, v2_index, v3_index, v4_index);
        }
    }

    return;
}

void ChunkFaceBuilder::ApplyAngleOffset(Integer3dCoord& target_coord, const BoxesTiles::AngleOffsetData& kAngle)
{
    if (kAngle.x_enable) {
        target_coord.x += kAngle.x_offset;
    }
    if (kAngle.y_enable) {
        target_coord.y += kAngle.y_offset;
    }
    if (kAngle.z_enable) {
        target_coord.z += kAngle.z_offset;
    }
    return;
}

void ChunkFaceBuilder::SplitFace(const LtFaceData& kFaceData, FaceBaseData& triangle_1, FaceBaseData& triangle_2)
{
    if (kFaceData.flipped) {
        triangle_1.triangle = true;
        triangle_1.p_1 = kFaceData.p_2; // 0
        triangle_1.p_2 = kFaceData.p_3; // 1
        triangle_1.p_3 = kFaceData.p_4; // 3
        triangle_2.triangle = true;
        triangle_2.p_1 = kFaceData.p_2; // 1
        triangle_2.p_2 = kFaceData.p_4; // 2
        triangle_2.p_3 = kFaceData.p_1; // 3
    }
    else {
        triangle_1.triangle = true;
        triangle_1.p_1 = kFaceData.p_1; // 0
        triangle_1.p_2 = kFaceData.p_2; // 1
        triangle_1.p_3 = kFaceData.p_3; // 2
        triangle_2.triangle = true;
        triangle_2.p_1 = kFaceData.p_1; // 0
        triangle_2.p_2 = kFaceData.p_3; // 2
        triangle_2.p_3 = kFaceData.p_4; // 3
    }
}

void ChunkFaceBuilder::GetFaceArray(std::vector<ChunkFaceBuilder::LtFaceData>& desc_face_array, BoxesTiles::const_iterator& tile_it)
{
    Integer3dCoord p_1, p_2, p_3, p_4, p_5, p_6, p_7, p_8;
    p_1 = { tile_it->x_1,  tile_it->y_1,  tile_it->z_2 };  // WDS V1
    p_2 = { tile_it->x_1,  tile_it->y_1,  tile_it->z_1 };  // WDN V2
    p_3 = { tile_it->x_2,  tile_it->y_1,  tile_it->z_1 };  // EDN V3
    p_4 = { tile_it->x_2,  tile_it->y_1,  tile_it->z_2 };  // EDS V4
    p_5 = { tile_it->x_1,  tile_it->y_2,  tile_it->z_1 };  // WUN V5
    p_6 = { tile_it->x_1,  tile_it->y_2,  tile_it->z_2 };  // WUS V6
    p_7 = { tile_it->x_2,  tile_it->y_2,  tile_it->z_2 };  // EUS V7
    p_8 = { tile_it->x_2,  tile_it->y_2,  tile_it->z_1 };  // EUN V8

    const BoxesTiles::AngleOffsetData& p_1_offset = tile_it->offset_data[BoxesTiles::TileBoxData::WDS];
    const BoxesTiles::AngleOffsetData& p_2_offset = tile_it->offset_data[BoxesTiles::TileBoxData::WDN];
    const BoxesTiles::AngleOffsetData& p_3_offset = tile_it->offset_data[BoxesTiles::TileBoxData::EDN];
    const BoxesTiles::AngleOffsetData& p_4_offset = tile_it->offset_data[BoxesTiles::TileBoxData::EDS];
    const BoxesTiles::AngleOffsetData& p_5_offset = tile_it->offset_data[BoxesTiles::TileBoxData::WUN];
    const BoxesTiles::AngleOffsetData& p_6_offset = tile_it->offset_data[BoxesTiles::TileBoxData::WUS];
    const BoxesTiles::AngleOffsetData& p_7_offset = tile_it->offset_data[BoxesTiles::TileBoxData::EUS];
    const BoxesTiles::AngleOffsetData& p_8_offset = tile_it->offset_data[BoxesTiles::TileBoxData::EUN];

    desc_face_array.push_back({ p_1,p_2,p_3,p_4, false, p_1_offset, p_2_offset, p_3_offset, p_4_offset , tile_it->flipped_data.down });
    desc_face_array.push_back({ p_5,p_6,p_7,p_8, false, p_5_offset, p_6_offset, p_7_offset, p_8_offset , tile_it->flipped_data.up });
    desc_face_array.push_back({ p_8,p_3,p_2,p_5, false, p_8_offset, p_3_offset, p_2_offset, p_5_offset , tile_it->flipped_data.north });
    desc_face_array.push_back({ p_6,p_1,p_4,p_7, false, p_6_offset, p_1_offset, p_4_offset, p_7_offset , tile_it->flipped_data.south });
    desc_face_array.push_back({ p_5,p_2,p_1,p_6, false, p_5_offset, p_2_offset, p_1_offset, p_6_offset , tile_it->flipped_data.west });
    desc_face_array.push_back({ p_7,p_4,p_3,p_8, false, p_7_offset, p_4_offset, p_3_offset, p_8_offset , tile_it->flipped_data.east });

    return;
}

void ChunkFaceBuilder::ToBlockSize(LtMesh& desc_mesh, const galib::minecraft::littletiles::GridType& kGrid)
{
    if (kGrid == 1) { return; }

    for (auto v : desc_mesh.vertices()) {

        LtKernel::Point_3 point = desc_mesh.point(v);
        float x = 1.0f / (float)kGrid;
        LtKernel::Point_3 scaled_point(
            point.x() * x,
            point.y() * x,
            point.z() * x
        );

        desc_mesh.point(v) = scaled_point;
    }
    return;
}

void galib::minecraft::littletiles::ChunkFaceBuilder::ToBlockCoord(galib::minecraft::littletiles::geometry::LtMesh& desc_mesh, const galib::minecraft::Coord::WorldBlockCoord& block_coord)
{
    for (auto v : desc_mesh.vertices()) {

        LtKernel::Point_3 point = desc_mesh.point(v);
        LtKernel::Point_3 scaled_point(
            point.x() + block_coord.x,
            point.y() + block_coord.y,
            point.z() + block_coord.z
        );

        desc_mesh.point(v) = scaled_point;
    }
    return;
}

bool galib::minecraft::littletiles::ObjFormatBuilder::AddChunkFace(const ChunkFaceBuilder& kFaceBuilderManager)
{
    if (kFaceBuilderManager.IsEmpty()) {
        return false;
    }

    if (HasChunkCoord_(kFaceBuilderManager.GetChunkCoord())) {
        return true;
    }

    for (ChunkFaceBuilder::mesh_map_constIterator mesh_map_it = kFaceBuilderManager.cbegin(); mesh_map_it != kFaceBuilderManager.cend(); ++mesh_map_it) {
        // Get block id
        std::string block_id_temp = mesh_map_it->first;

        std::pair<face_container::iterator, bool> face_map_result = stdface_map_.insert(face_pair(block_id_temp, std::vector<ObjFaceStdData>()));
        face_container::iterator face_map_it = face_map_result.first;
        std::vector<ObjFaceStdData>& face_array = face_map_it->second;

        for (std::vector<LtMesh>::const_iterator mesh_it = mesh_map_it->second.cbegin(); mesh_it != mesh_map_it->second.cend(); ++mesh_it) {
            for (const auto& face : mesh_it->faces()) {
                Float3dCoord points[4];
                size_t points_size = 0;
                for (const auto& vertex : mesh_it->vertices_around_face(mesh_it->halfedge(face))) {
                    if (points_size > 4) { break; }
                    ++points_size;
                    LtPoint point = mesh_it->point(vertex);
                    points[points_size - 1].x = point.x();
                    points[points_size - 1].y = point.y();
                    points[points_size - 1].z = point.z();
                }
                ObjFaceStdData stdface_data_temp = GetFaceDataAddVertex_(points, points_size == 3);
                face_array.push_back(stdface_data_temp);
            }
        }

        if (face_array.empty()) {
            stdface_map_.erase(face_map_it);
        }
    }

    return true;
}

const std::string galib::minecraft::littletiles::ObjFormatBuilder::BuildFormatStr(bool has_block_id) const
{
    std::string temp;
    for (const ObjVertexStdData& v : stdvertex_array_) {
        temp += (std::string("v ") + std::to_string(v.x) + " " + std::to_string(v.y) + " " + std::to_string(v.z) + "\n");
    }
    for (const std::pair<std::string, std::vector<ObjFaceStdData>>& face_map : stdface_map_) {
        if (has_block_id) {
            temp += (std::string("usemtl ") + face_map.first + "\n");
        }
        for (const ObjFaceStdData& f : face_map.second) {
            temp += (std::string("f ") + std::to_string(f.p_1_index) + " " + std::to_string(f.p_2_index) + " " + std::to_string(f.p_3_index)) + " ";
            if (!f.triangle) {
                temp += std::to_string(f.p_4_index);
            }
            temp += "\n";
        }
    }

    return temp;
}

bool galib::minecraft::littletiles::ObjFormatBuilder::BuildFormatToFile(const char* const kPFolderPath, const char* const kPFileName, bool has_block_id) const
{
    if (IsEmpty()) {
        return false;
    }
    std::ofstream out_file(std::string(kPFolderPath) + "/" + kPFileName + ".obj");
    std::ofstream out_mtlfile(std::string(kPFolderPath) + "/" + kPFileName + ".mtl");

    if (!out_file) {
        return false;
    }
    if (!out_mtlfile) {
        return false;
    }
    out_file << "mtllib " << kPFileName << ".mtl\n";
    for (const ObjVertexStdData& v : stdvertex_array_) {
        out_file << (std::string("v ") + std::to_string(v.x) + " " + std::to_string(v.y) + " " + std::to_string(v.z) + "\n");
    }
    for (const std::pair<std::string, std::vector<ObjFaceStdData>>& face_map : stdface_map_) {
        if (has_block_id) {
            std::string block_id;
            std::string block_tx_path;
            for (char ch : face_map.first) {
                if (ch == ':') {
                    block_id += '_';
                    block_tx_path += '/';
                }
                else {
                    block_id += ch;
                    block_tx_path += ch;
                }
            }
            block_tx_path += ".png";
            out_file << (std::string("usemtl ") + block_id + "\n");
            out_mtlfile << "newmtl " << block_id << "\nKa 1.0 1.0 1.0\nKd 1.0 1.0 1.0\nKs 0.0 0.0 0.0\n" << \
                "map_Kd textures/" + block_tx_path + "\nNs 100.0\nd 1.0\n";
        }
        for (const ObjFaceStdData& f : face_map.second) {
            out_file << (std::string("f ") + std::to_string(f.p_1_index) + " " + std::to_string(f.p_2_index) + " " + std::to_string(f.p_3_index)) + " ";
            if (!f.triangle) {
                out_file << std::to_string(f.p_4_index);
            }
            out_file << "\n";
        }
    }

    out_file.close();
    out_mtlfile.close();
    return true;
}

void galib::minecraft::littletiles::ObjFormatBuilder::Clear()
{
    stdface_map_.clear();
    stdvertex_array_.clear();
    saved_chunk_coord_.clear();
}

bool galib::minecraft::littletiles::ObjFormatBuilder::IsEmpty() const
{
    return stdface_map_.empty();
}

ObjFormatBuilder::vertex_diff_type ObjFormatBuilder::FindVertexData_(const ObjVertexStdData& kVertexData) const
{
    for (size_t index = 0; index < stdvertex_array_.size(); ++index) {
        if (stdvertex_array_[index] == kVertexData) {
            return index;
        }
    }
    return -2;
}

bool ObjFormatBuilder::HasChunkCoord_(const ChunkCoord& kChunkCoord)
{
    for (const ChunkCoord& current_coord : saved_chunk_coord_) {
        if (current_coord == kChunkCoord) {
            return true;
        }
    }
    return false;
}

ObjFormatBuilder::ObjVertexStdData ObjFormatBuilder::FloatCoordToVertexData_(const Float3dCoord& kFloatCoord)
{
    ObjVertexStdData temp;
    temp.x = kFloatCoord.x;
    temp.y = kFloatCoord.y;
    temp.z = kFloatCoord.z;
    return temp;
}

ObjFormatBuilder::ObjFaceStdData ObjFormatBuilder::GetFaceDataAddVertex_(const Float3dCoord* p_coord_array, bool t)
{

    
    // Get Build Data
    ObjVertexStdData p_1 = FloatCoordToVertexData_(p_coord_array[0]);
    ObjVertexStdData p_2 = FloatCoordToVertexData_(p_coord_array[1]);
    ObjVertexStdData p_3 = FloatCoordToVertexData_(p_coord_array[2]);
    ObjVertexStdData p_4;
    if (!t) {
        p_4 = FloatCoordToVertexData_(p_coord_array[3]);
    }

    // not found, return -2
    vertex_diff_type p_1_index{ FindVertexData_(p_1) + 1 };
    if (p_1_index < 0) {
        stdvertex_array_.push_back(p_1);
        p_1_index = stdvertex_array_.size();
    }
    vertex_diff_type p_2_index{ FindVertexData_(p_2) + 1 };
    if (p_2_index < 0) {
        stdvertex_array_.push_back(p_2);
        p_2_index = stdvertex_array_.size();
    }
    vertex_diff_type p_3_index{ FindVertexData_(p_3) + 1 };
    if (p_3_index < 0) {
        stdvertex_array_.push_back(p_3);
        p_3_index = stdvertex_array_.size();
    }
    vertex_diff_type p_4_index{ FindVertexData_(p_4) + 1 };
    if (!t) {
        if (p_4_index < 0) {
            stdvertex_array_.push_back(p_4);
            p_4_index = stdvertex_array_.size();
        }
    }

    // Obj Face Data
    ObjFaceStdData temp;
    temp.p_1_index = p_1_index;
    temp.p_2_index = p_2_index;
    temp.p_3_index = p_3_index;
    temp.p_4_index = p_4_index;

    if (t) {
        temp.triangle = true;
    }

    return temp;
}
