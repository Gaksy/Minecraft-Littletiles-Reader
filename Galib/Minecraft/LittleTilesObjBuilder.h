#ifndef _GALIB_MINECRAFT_LITTLE_TILES_OBJ_BUILDER_H_
#define _GALIB_MINECRAFT_LITTLE_TILES_OBJ_BUILDER_H_

#include <vector>
#include <map>

#include "LittleTiles.h"

#include "CGAL/Exact_predicates_inexact_constructions_kernel.h"
#include "CGAL/Point_set_3.h"
#include "CGAL/Surface_mesh/Surface_mesh.h"

namespace galib {
    namespace minecraft {
        namespace littletiles {
            namespace geometry {
                typedef CGAL::Simple_cartesian<float>                                               LtKernel;
                typedef CGAL::Point_3<CGAL::Simple_cartesian<float>>                                LtPoint;
                typedef CGAL::Surface_mesh<CGAL::Point_3<CGAL::Simple_cartesian<float>>>            LtMesh;
            }

            class ChunkFaceBuilder {
            private:
                typedef std::map<std::string, std::vector<geometry::LtMesh>>                    MeshMap;
                typedef std::pair<std::string, std::vector<geometry::LtMesh>>                   MeshMapPair;
                typedef std::vector<geometry::LtMesh>                                           MeshArray;

                struct LtFaceData;

                struct FaceBaseData {
                    galib::minecraft::Coord::Integer3dCoord p_1, p_2, p_3, p_4;
                    bool triangle{ false };

                    inline FaceBaseData& operator=(const LtFaceData& RHS) {
                        p_1 = RHS.p_1;
                        p_2 = RHS.p_2;
                        p_3 = RHS.p_3;
                        p_4 = RHS.p_4;
                        triangle = RHS.triangle;
                        return *this;
                    }

                    inline bool IsLine()const {
                        if (!triangle) {
                            return ((p_1 == p_2 && p_3 == p_4) || (p_1 == p_4 && p_2 == p_3));
                        }
                        return (p_1 == p_2) || (p_2 == p_3) || (p_1 == p_3);
                    }

                    inline bool IsPoint()const {
                        if (!triangle) {
                            return (p_1 == p_2) && (p_2 == p_3) && (p_3 == p_4);
                        }
                        return (p_1 == p_2) && (p_2 == p_3);
                    }

                    inline bool IsFace()const {
                        return !(IsLine() || IsPoint());
                    }

                };

                struct LtFaceData {
                    galib::minecraft::Coord::Integer3dCoord p_1, p_2, p_3, p_4;
                    bool triangle{ false };
                    const galib::minecraft::littletiles::BoxesTiles::AngleOffsetData p_1_state;
                    const galib::minecraft::littletiles::BoxesTiles::AngleOffsetData p_2_state;
                    const galib::minecraft::littletiles::BoxesTiles::AngleOffsetData p_3_state;
                    const galib::minecraft::littletiles::BoxesTiles::AngleOffsetData p_4_state;
                    bool flipped{ false };

                    inline bool HasAnyOffset()const {
                        return p_1_state.HasAnyEnable() || p_2_state.HasAnyEnable() || p_3_state.HasAnyEnable() || p_4_state.HasAnyEnable();
                    }

                    inline bool IsShouldTriangle()const {
                        if ((p_1.y == p_4.y) && (p_1.z == p_4.z) && (p_2.y == p_3.y) && (p_2.z == p_3.z) && (p_1.x == p_2.x) && (p_4.x == p_3.x) ||
                            (p_1.y == p_2.y) && (p_1.x == p_2.x) && (p_3.y == p_4.y) && (p_3.x == p_4.x) && (p_2.z == p_3.z) && (p_1.z == p_4.z)) {
                            return false;
                        }
                        return true;
                    }

                    inline bool IsLine()const {
                        if (!triangle) {
                            return ((p_1 == p_2 && p_3 == p_4) || (p_1 == p_4 && p_2 == p_3));
                        }
                        return (p_1 == p_2) || (p_2 == p_3) || (p_1 == p_3);
                    }

                    inline bool IsPoint()const {
                        if (!triangle) {
                            return (p_1 == p_2) && (p_2 == p_3) && (p_3 == p_4);
                        }
                        return (p_1 == p_2) && (p_2 == p_3);
                    }

                    inline bool IsFace()const {
                        return !(IsLine() || IsPoint());
                    }

                    const FaceBaseData to_base()const {
                        FaceBaseData temp;
                        temp = *this;
                        return temp;
                    }
                };

            public:
                typedef std::vector<geometry::LtMesh>                                           mesh_array_container;
                typedef std::map<std::string, std::vector<geometry::LtMesh>>                    mesh_map_container;
                typedef std::map<std::string, std::vector<geometry::LtMesh>>::const_iterator    mesh_map_constIterator;
                
            public:
                ChunkFaceBuilder();
                ~ChunkFaceBuilder()=default;

            
            public:
                bool SetChunkTiles(const galib::minecraft::littletiles::ChunkTiles& kDescChunkTiles, const galib::minecraft::Coord::WorldBlockCoord& offset);
                const galib::minecraft::Coord::ChunkCoord& GetChunkCoord()const;

                mesh_map_constIterator cbegin()const;
                mesh_map_constIterator cend()const;

                void Clear();
                bool IsEmpty()const;

            private:
                void IsEmptyAndTherow(const char* const kPMessage)const;

                static void GetMeshFromTileBoxDataWithoutOffset(
                    galib::minecraft::littletiles::geometry::LtMesh& desc_mesh,
                    const galib::minecraft::littletiles::BoxesTiles::TileBoxData& kTileBoxData
                );
                static void GetMeshFromFaceBaseArray(
                    galib::minecraft::littletiles::geometry::LtMesh& desc_mesh,
                    const std::vector<ChunkFaceBuilder::FaceBaseData>& kFaceArray
                );

                static void ApplyAngleOffset(
                    galib::minecraft::Coord::Integer3dCoord& target_coord,
                    const galib::minecraft::littletiles::BoxesTiles::AngleOffsetData& kAngle
                );

                static void SplitFace(
                    const LtFaceData& kFaceData,
                    FaceBaseData& triangle_1,
                    FaceBaseData& triangle_2
                );

                static void GetFaceArray(
                    std::vector<ChunkFaceBuilder::LtFaceData>& desc_face_array,
                    galib::minecraft::littletiles::BoxesTiles::const_iterator& tile_it
                );

                static void ToBlockSize(
                    galib::minecraft::littletiles::geometry::LtMesh& desc_mesh,
                    const galib::minecraft::littletiles::GridType& kGrid
                );

                static void ToBlockCoord(
                    galib::minecraft::littletiles::geometry::LtMesh& desc_mesh,
                    const galib::minecraft::Coord::WorldBlockCoord& block_coord
                );

            private:
                std::map<std::string, std::vector<geometry::LtMesh>> mesh_map_;
                galib::minecraft::Coord::ChunkCoord chunk_coord_;
            };

            class ObjFormatBuilder {
            private:
                struct ObjFaceStdData {
                    size_t p_1_index;
                    size_t p_2_index;
                    size_t p_3_index;
                    size_t p_4_index;

                    bool triangle{ false };
                };

                struct ObjVertexStdData {
                    float x{ 0.0f };
                    float y{ 0.0f };
                    float z{ 0.0f };

                    inline bool operator==(const ObjVertexStdData& RHS)const {
                        return x == RHS.x && y == RHS.y && z == RHS.z;
                    }
                };

            public:
                typedef std::map<std::string, std::vector<ObjFaceStdData>>                  face_container;
                typedef std::map<std::string, std::vector<ObjFaceStdData>>::const_iterator  face_const_iterator;
                typedef std::pair<std::string, std::vector<ObjFaceStdData>>                 face_pair;
                typedef std::vector<ObjVertexStdData>                                       vertex_container;
                typedef std::vector<ObjVertexStdData>::const_iterator                       vertex_const_iterator;

            private:
                typedef std::vector<ObjVertexStdData>::difference_type                      vertex_diff_type;
                typedef std::map<std::string, std::vector<ObjFaceStdData>>::iterator        face_iterator;

            public:
                ObjFormatBuilder() = default;
                ~ObjFormatBuilder() = default;

            public:
                // Add block face from ObjFaceBuilderLtiles
                bool AddChunkFace(const ChunkFaceBuilder& kFaceBuilderManager);
                // Construct Obj string data
                const std::string BuildFormatStr(bool has_block_id = true)const;
                bool BuildFormatToFile(const char* const kPFilePath, const char* const kPFileName, bool has_block_id = true)const;

                void Clear();
                bool IsEmpty()const;

            private:
                vertex_diff_type FindVertexData_(const ObjVertexStdData& kVertexData)const;
                bool HasChunkCoord_(const galib::minecraft::Coord::ChunkCoord& kChunkCoord);
                static ObjVertexStdData FloatCoordToVertexData_(const galib::minecraft::Coord::Float3dCoord& kFloatCoord);
                ObjFaceStdData GetFaceDataAddVertex_(
                    const galib::minecraft::Coord::Float3dCoord* p_coord_array,
                    bool t
                );

            private:
                std::map<std::string, std::vector<ObjFaceStdData>> stdface_map_;
                std::vector<ObjVertexStdData> stdvertex_array_;
                std::vector<galib::minecraft::Coord::ChunkCoord> saved_chunk_coord_;
            };
        }
    }
}

#endif // !_GALIB_MINECRAFT_LITTLE_TILES_OBJ_BUILDER_H_
