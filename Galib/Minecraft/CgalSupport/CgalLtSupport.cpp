/*
 * Copyright (c) 2025 Gaksy (Fuhongren)
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

#include "Minecraft/CgalSupport/CgalLtSupport.h"

#include <CGAL/Polygon_mesh_processing/repair.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <map>
#include <vector>

#include "GalibNamespaceDef.h"
#include "Minecraft/CgalSupport/CgalTypeDef.h"
#include "Minecraft/LittleTiles.h"

using GALIB minecraft::littletiles::LittleTilesCoord;
using GALIB minecraft::littletiles::GridType;

using GALIB minecraft::littletiles::AngleID;
using GALIB minecraft::littletiles::TileFaceID;
using GALIB minecraft::littletiles::Flipped;
using GALIB minecraft::littletiles::TileEntity;
using GALIB minecraft::littletiles::TileFace;

using GALIB minecraft::cgal_support::LtSurfaceMesh;

using GALIB_CGAL SM_Vertex_index;

// ---------------------------------------------------------------------------
// 面构建辅助：平面四边形输出为 n 边形，非平面才回退到两个三角形
// ---------------------------------------------------------------------------
namespace {
using LtVertexIndex =
    galib::minecraft::cgal_support::SurfaceMeshType::Vertex_index;
using LtPoint = galib::minecraft::cgal_support::LtPoint3;

// 四个点是否共面（坐标此时是 grid 单位，尺度 <= 数百，1e-9 的绝对容差足够）
bool isPlanarQuad(const LtPoint& kA, const LtPoint& kB, const LtPoint& kC,
                  const LtPoint& kD) {
  const double abx = kB.x() - kA.x(), aby = kB.y() - kA.y(),
               abz = kB.z() - kA.z();
  const double acx = kC.x() - kA.x(), acy = kC.y() - kA.y(),
               acz = kC.z() - kA.z();
  const double nx = aby * acz - abz * acy;
  const double ny = abz * acx - abx * acz;
  const double nz = abx * acy - aby * acx;
  const double length = std::sqrt(nx * nx + ny * ny + nz * nz);
  if (length < 1e-12) {
    return true;
  }  // 退化面，交给 CGAL 自行拒绝

  const double adx = kD.x() - kA.x(), ady = kD.y() - kA.y(),
               adz = kD.z() - kA.z();
  const double distance = std::fabs(nx * adx + ny * ady + nz * adz) / length;
  return distance < 1e-9;
}

bool isSamePoint(const LtPoint& kA, const LtPoint& kB) {
  return std::fabs(kA.x() - kB.x()) < 1e-9 &&
         std::fabs(kA.y() - kB.y()) < 1e-9 && std::fabs(kA.z() - kB.z()) < 1e-9;
}

// kIndices/kPoints 必须按同一环绕顺序给出（该顺序决定面的法线方向）
void addQuadFace(galib::minecraft::cgal_support::SurfaceMeshType& mesh,
                 const std::array<LtVertexIndex, 4>& kIndices,
                 const std::array<LtPoint, 4>& kPoints, const bool kFlipped) {
  // 角点偏移可能让两个角点落在同一位置（退化四边形）。
  // 此时必须先把重合点去掉，否则 CGAL 会以"非法多边形"为由拒绝整个面。
  std::array<LtVertexIndex, 4> indices{};
  std::array<LtPoint, 4> points{};
  GALIB_STD size_t count = 0;
  for (GALIB_STD size_t i = 0; i < 4; ++i) {
    if (count > 0 && isSamePoint(points[count - 1], kPoints[i])) {
      continue;
    }
    indices[count] = kIndices[i];
    points[count] = kPoints[i];
    ++count;
  }
  while (count > 1 && isSamePoint(points[0], points[count - 1])) {
    --count;
  }
  if (count < 3) {
    return;
  }  // 退化成一个点或一条线，无面可加

  if (count == 3) {
    mesh.add_face(indices[0], indices[1], indices[2]);
    return;
  }

  if (isPlanarQuad(points[0], points[1], points[2], points[3])) {
    // 平面四边形：保留为 1 个 n 边形（线框里不会出现多余的对角线）
    mesh.add_face(indices[0], indices[1], indices[2], indices[3]);
    return;
  }

  // 非平面四边形：沿用原先的两三角形剖分，Flipped 决定用哪条对角线
  if (!kFlipped) {
    mesh.add_face(indices[0], indices[1], indices[3]);
    mesh.add_face(indices[1], indices[2], indices[3]);
  } else {
    mesh.add_face(indices[0], indices[1], indices[2]);
    mesh.add_face(indices[0], indices[2], indices[3]);
  }
}
}  // namespace

void GALIB minecraft::cgal_support::createMeshFromTileEntity(
    LtSurfaceMesh& mesh_data, const TileEntity& kTileEntity,
    const bool kApplyOffset) {
  SurfaceMeshType& mesh = mesh_data.getMesh();
  using VertexIndex = SurfaceMeshType::Vertex_index;

  const LtPoint p_eun =
      convertToCGALPoint(kTileEntity.getVertices(AngleID::EUN, kApplyOffset));
  const LtPoint p_eus =
      convertToCGALPoint(kTileEntity.getVertices(AngleID::EUS, kApplyOffset));
  const LtPoint p_edn =
      convertToCGALPoint(kTileEntity.getVertices(AngleID::EDN, kApplyOffset));
  const LtPoint p_eds =
      convertToCGALPoint(kTileEntity.getVertices(AngleID::EDS, kApplyOffset));
  const LtPoint p_wun =
      convertToCGALPoint(kTileEntity.getVertices(AngleID::WUN, kApplyOffset));
  const LtPoint p_wus =
      convertToCGALPoint(kTileEntity.getVertices(AngleID::WUS, kApplyOffset));
  const LtPoint p_wdn =
      convertToCGALPoint(kTileEntity.getVertices(AngleID::WDN, kApplyOffset));
  const LtPoint p_wds =
      convertToCGALPoint(kTileEntity.getVertices(AngleID::WDS, kApplyOffset));

  const VertexIndex eun = mesh.add_vertex(p_eun);
  const VertexIndex eus = mesh.add_vertex(p_eus);
  const VertexIndex edn = mesh.add_vertex(p_edn);
  const VertexIndex eds = mesh.add_vertex(p_eds);
  const VertexIndex wun = mesh.add_vertex(p_wun);
  const VertexIndex wus = mesh.add_vertex(p_wus);
  const VertexIndex wdn = mesh.add_vertex(p_wdn);
  const VertexIndex wds = mesh.add_vertex(p_wds);

  const Flipped& flipped_data = kTileEntity.getFlippedData();
  const bool east_flipped = flipped_data.east;
  const bool west_flipped = flipped_data.west;
  const bool south_flipped = flipped_data.south;
  const bool north_flipped = flipped_data.north;
  const bool up_flipped = flipped_data.up;
  const bool down_flipped = flipped_data.down;

  // 环绕顺序与原先两三角形的绕向一致（法线方向不变）
  addQuadFace(mesh, {eds, edn, eun, eus}, {p_eds, p_edn, p_eun, p_eus},
              east_flipped);
  addQuadFace(mesh, {wdn, wds, wus, wun}, {p_wdn, p_wds, p_wus, p_wun},
              west_flipped);
  addQuadFace(mesh, {wds, eds, eus, wus}, {p_wds, p_eds, p_eus, p_wus},
              south_flipped);
  addQuadFace(mesh, {edn, wdn, wun, eun}, {p_edn, p_wdn, p_wun, p_eun},
              north_flipped);
  addQuadFace(mesh, {wus, eus, eun, wun}, {p_wus, p_eus, p_eun, p_wun},
              up_flipped);
  addQuadFace(mesh, {wdn, edn, eds, wds}, {p_wdn, p_edn, p_eds, p_wds},
              down_flipped);
}

// ---------------------------------------------------------------------------
// 半空间裁剪：替代 corefine_and_compute_intersection
// ---------------------------------------------------------------------------
namespace {
struct ClipVec3 {
  double x{0.0};
  double y{0.0};
  double z{0.0};
};

using ClipPolygon = GALIB_STD vector<ClipVec3>;
using ClipPolyhedron = GALIB_STD vector<ClipPolygon>;

const double kClipEpsilon = 1e-9;

double clipDot(const ClipVec3& kA, const ClipVec3& kB) {
  return kA.x * kB.x + kA.y * kB.y + kA.z * kB.z;
}

ClipVec3 clipSub(const ClipVec3& kA, const ClipVec3& kB) {
  return {kA.x - kB.x, kA.y - kB.y, kA.z - kB.z};
}

ClipVec3 clipLerp(const ClipVec3& kA, const ClipVec3& kB, const double kT) {
  return {kA.x + (kB.x - kA.x) * kT, kA.y + (kB.y - kA.y) * kT,
          kA.z + (kB.z - kA.z) * kT};
}

bool clipNearlyEqual(const ClipVec3& kA, const ClipVec3& kB) {
  return GALIB_STD fabs(kA.x - kB.x) < kClipEpsilon &&
         GALIB_STD fabs(kA.y - kB.y) < kClipEpsilon &&
         GALIB_STD fabs(kA.z - kB.z) < kClipEpsilon;
}

// 用半空间 n·p >= d 裁剪一个凸多面体；保留面按原环绕顺序，切面补一个新的 n 边形（cap）。
ClipPolyhedron clipPolyhedronByPlane(const ClipPolyhedron& kPolyhedron,
                                     const ClipVec3& kN, const double kD) {
  ClipPolyhedron result;
  ClipPolygon cut_points;

  for (const ClipPolygon& face : kPolyhedron) {
    const GALIB_STD size_t count = face.size();
    if (count < 3) {
      continue;
    }

    ClipPolygon clipped;
    for (GALIB_STD size_t i = 0; i < count; ++i) {
      const ClipVec3& current = face[i];
      const ClipVec3& next = face[(i + 1) % count];
      const double current_distance = clipDot(kN, current) - kD;
      const double next_distance = clipDot(kN, next) - kD;
      const bool current_inside = current_distance >= -kClipEpsilon;
      const bool next_inside = next_distance >= -kClipEpsilon;

      if (current_inside) {
        clipped.push_back(current);
      }
      if (current_inside != next_inside) {
        const double denominator = current_distance - next_distance;
        if (GALIB_STD fabs(denominator) < kClipEpsilon) {
          continue;
        }
        const ClipVec3 cross_point =
            clipLerp(current, next, current_distance / denominator);
        clipped.push_back(cross_point);
        cut_points.push_back(cross_point);
      }
    }
    // 顶点恰好落在裁剪平面上时会产生重复点，必须去掉，
    // 否则 CGAL 会认为该多边形非法并拒绝整个面（导致网格不闭合）
    ClipPolygon simplified;
    for (const ClipVec3& point : clipped) {
      if (simplified.empty() || !clipNearlyEqual(simplified.back(), point)) {
        simplified.push_back(point);
      }
    }
    if (simplified.size() >= 2 &&
        clipNearlyEqual(simplified.front(), simplified.back())) {
      simplified.pop_back();
    }
    if (simplified.size() >= 3) {
      result.push_back(simplified);
    }
  }

  // 切面：所有新产生的点都落在裁剪平面上，去重后按平面内极角排序即可得到正确的环绕顺序
  ClipPolygon unique_points;
  for (const ClipVec3& point : cut_points) {
    bool duplicated = false;
    for (const ClipVec3& kept : unique_points) {
      if (clipNearlyEqual(point, kept)) {
        duplicated = true;
        break;
      }
    }
    if (!duplicated) {
      unique_points.push_back(point);
    }
  }

  if (unique_points.size() >= 3) {
    ClipVec3 center{0.0, 0.0, 0.0};
    for (const ClipVec3& point : unique_points) {
      center.x += point.x;
      center.y += point.y;
      center.z += point.z;
    }
    const double inverse_count =
        1.0 / static_cast<double>(unique_points.size());
    center.x *= inverse_count;
    center.y *= inverse_count;
    center.z *= inverse_count;

    // 构造平面内基底 (u, v)，使 u × v = -n：
    // 保留的是 n·p >= d 一侧，实体在 +n 方向，因此切面的外法线指向 -n。
    ClipVec3 u = (GALIB_STD fabs(kN.x) <= GALIB_STD fabs(kN.y) &&
                  GALIB_STD fabs(kN.x) <= GALIB_STD fabs(kN.z))
                     ? ClipVec3{1.0, 0.0, 0.0}
                     : ((GALIB_STD fabs(kN.y) <= GALIB_STD fabs(kN.z))
                            ? ClipVec3{0.0, 1.0, 0.0}
                            : ClipVec3{0.0, 0.0, 1.0});
    const double u_along_n = clipDot(u, kN);
    u = clipSub(u, {kN.x * u_along_n, kN.y * u_along_n, kN.z * u_along_n});
    const double u_length = GALIB_STD sqrt(clipDot(u, u));
    if (u_length < kClipEpsilon) {
      return result;
    }
    u = {u.x / u_length, u.y / u_length, u.z / u_length};
    const ClipVec3 v{u.y * kN.z - u.z * kN.y, u.z * kN.x - u.x * kN.z,
                     u.x * kN.y - u.y * kN.x};

    GALIB_STD sort(
        unique_points.begin(), unique_points.end(),
        [&center, &u, &v](const ClipVec3& kLhs, const ClipVec3& kRhs) {
          const ClipVec3 lhs = clipSub(kLhs, center);
          const ClipVec3 rhs = clipSub(kRhs, center);
          return GALIB_STD atan2(clipDot(lhs, v), clipDot(lhs, u)) <
                 GALIB_STD atan2(clipDot(rhs, v), clipDot(rhs, u));
        });
    result.push_back(unique_points);
  }

  return result;
}

ClipVec3 clipToVec3(
    const GALIB minecraft::littletiles::LittleTilesCoord& kCoord) {
  return {kCoord.x, kCoord.y, kCoord.z};
}

bool clipIsPlanarQuad(const ClipPolygon& kQuad) {
  const ClipVec3 &a = kQuad[0], &b = kQuad[1], &c = kQuad[2], &d = kQuad[3];
  const ClipVec3 ab{b.x - a.x, b.y - a.y, b.z - a.z};
  const ClipVec3 ac{c.x - a.x, c.y - a.y, c.z - a.z};
  const ClipVec3 normal{ab.y * ac.z - ab.z * ac.y, ab.z * ac.x - ab.x * ac.z,
                        ab.x * ac.y - ab.y * ac.x};
  const double length = GALIB_STD sqrt(clipDot(normal, normal));
  if (length < 1e-12) {
    return true;
  }
  const ClipVec3 ad{d.x - a.x, d.y - a.y, d.z - a.z};
  return GALIB_STD fabs(clipDot(normal, ad)) / length < 1e-9;
}

// 把一个四边形面加入待裁剪多面体：
// 角点重合时先合并退化点；平面四边形保留为 1 个面，
// 非平面（扭曲）四边形按 Flipped 规则拆成两个三角形——
// 与 createMeshFromTileEntity 的渲染一致，否则裁剪结果会与原布尔运算不同。
void appendQuadFace(ClipPolyhedron& desc_polyhedron, ClipPolygon kPoints,
                    const bool kFlipped) {
  ClipPolygon polygon;
  for (const ClipVec3& point : kPoints) {
    if (polygon.empty() || !clipNearlyEqual(polygon.back(), point)) {
      polygon.push_back(point);
    }
  }
  while (polygon.size() > 1 &&
         clipNearlyEqual(polygon.front(), polygon.back())) {
    polygon.pop_back();
  }

  if (polygon.size() < 3) {
    return;
  }
  if (polygon.size() == 3 || clipIsPlanarQuad(polygon)) {
    desc_polyhedron.push_back(polygon);
    return;
  }

  if (!kFlipped) {
    desc_polyhedron.push_back({polygon[0], polygon[1], polygon[3]});
    desc_polyhedron.push_back({polygon[1], polygon[2], polygon[3]});
  } else {
    desc_polyhedron.push_back({polygon[0], polygon[1], polygon[2]});
    desc_polyhedron.push_back({polygon[0], polygon[2], polygon[3]});
  }
}
}  // namespace

bool GALIB minecraft::cgal_support::clipTileEntityToBox(
    LtSurfaceMesh& desc_mesh, const TileEntity& kTileEntity,
    const bool kApplyOffset) {
  // 8 个角点（grid 单位）
  const ClipVec3 eun =
      clipToVec3(kTileEntity.getVertices(AngleID::EUN, kApplyOffset));
  const ClipVec3 eus =
      clipToVec3(kTileEntity.getVertices(AngleID::EUS, kApplyOffset));
  const ClipVec3 edn =
      clipToVec3(kTileEntity.getVertices(AngleID::EDN, kApplyOffset));
  const ClipVec3 eds =
      clipToVec3(kTileEntity.getVertices(AngleID::EDS, kApplyOffset));
  const ClipVec3 wun =
      clipToVec3(kTileEntity.getVertices(AngleID::WUN, kApplyOffset));
  const ClipVec3 wus =
      clipToVec3(kTileEntity.getVertices(AngleID::WUS, kApplyOffset));
  const ClipVec3 wdn =
      clipToVec3(kTileEntity.getVertices(AngleID::WDN, kApplyOffset));
  const ClipVec3 wds =
      clipToVec3(kTileEntity.getVertices(AngleID::WDS, kApplyOffset));

  // 凸六面体的 6 个面（环绕顺序与 createMeshFromTileEntity 一致）
  const Flipped& flipped = kTileEntity.getFlippedData();
  ClipPolyhedron polyhedron;
  polyhedron.reserve(12);
  appendQuadFace(polyhedron, {eds, edn, eun, eus}, flipped.east);
  appendQuadFace(polyhedron, {wdn, wds, wus, wun}, flipped.west);
  appendQuadFace(polyhedron, {wds, eds, eus, wus}, flipped.south);
  appendQuadFace(polyhedron, {edn, wdn, wun, eun}, flipped.north);
  appendQuadFace(polyhedron, {wus, eus, eun, wun}, flipped.up);
  appendQuadFace(polyhedron, {wdn, edn, eds, wds}, flipped.down);

  // 裁剪体 = 未偏移的盒子（与原先布尔求交使用的 AABB 相同）
  const ClipVec3 box_a =
      clipToVec3(kTileEntity.getVertices(AngleID::WDN, false));
  const ClipVec3 box_b =
      clipToVec3(kTileEntity.getVertices(AngleID::EUS, false));
  const ClipVec3 box_min{GALIB_STD min(box_a.x, box_b.x),
                         GALIB_STD min(box_a.y, box_b.y),
                         GALIB_STD min(box_a.z, box_b.z)};
  const ClipVec3 box_max{GALIB_STD max(box_a.x, box_b.x),
                         GALIB_STD max(box_a.y, box_b.y),
                         GALIB_STD max(box_a.z, box_b.z)};

  polyhedron = clipPolyhedronByPlane(polyhedron, {1.0, 0.0, 0.0}, box_min.x);
  polyhedron = clipPolyhedronByPlane(polyhedron, {-1.0, 0.0, 0.0}, -box_max.x);
  polyhedron = clipPolyhedronByPlane(polyhedron, {0.0, 1.0, 0.0}, box_min.y);
  polyhedron = clipPolyhedronByPlane(polyhedron, {0.0, -1.0, 0.0}, -box_max.y);
  polyhedron = clipPolyhedronByPlane(polyhedron, {0.0, 0.0, 1.0}, box_min.z);
  polyhedron = clipPolyhedronByPlane(polyhedron, {0.0, 0.0, -1.0}, -box_max.z);

  if (polyhedron.empty()) {
    return false;
  }

  SurfaceMeshType& mesh = desc_mesh.getMesh();
  GALIB_STD map<GALIB_STD array<long long, 3>, SurfaceMeshType::Vertex_index>
      welded_vertices;
  const double weld_scale = 1e6;  // grid 单位下 1e-6 的量化精度足够区分真实顶点

  for (const ClipPolygon& face : polyhedron) {
    if (face.size() < 3) {
      continue;
    }

    GALIB_STD vector<SurfaceMeshType::Vertex_index> face_indices;
    face_indices.reserve(face.size());
    for (const ClipVec3& point : face) {
      const GALIB_STD array<long long, 3> key{
          static_cast<long long>(GALIB_STD llround(point.x * weld_scale)),
          static_cast<long long>(GALIB_STD llround(point.y * weld_scale)),
          static_cast<long long>(GALIB_STD llround(point.z * weld_scale))};
      auto found = welded_vertices.find(key);
      if (found == welded_vertices.end()) {
        found =
            welded_vertices
                .emplace(key,
                         mesh.add_vertex(LtPoint(point.x, point.y, point.z)))
                .first;
      }
      face_indices.push_back(found->second);
    }

    // CGAL 会拒绝退化面与重复面，返回无效的 face_index，这里忽略即可
    mesh.add_face(face_indices);
  }

  return mesh.number_of_faces() > 0;
}

const LtSurfaceMesh& GALIB
minecraft::cgal_support::createIntersectionCube(const GridType kGrid) {
  // 静态指针，确保只在第一次调用时创建
  static LtSurfaceMesh* p_lt_surface_mesh = nullptr;

  // 如果cube尚未创建，则构建它
  if (!p_lt_surface_mesh) {
    p_lt_surface_mesh = new LtSurfaceMesh();
    SurfaceMeshType& cube = p_lt_surface_mesh->getMesh();

    // 构建正方体顶点 p1(0, 0, 0) 和 p2(1, 1, 1)
    using Point = SurfaceMeshType::Point;

    // 顶点坐标
    const SM_Vertex_index eun = cube.add_vertex(Point(kGrid, kGrid, 0));
    const SM_Vertex_index eus = cube.add_vertex(Point(kGrid, kGrid, kGrid));
    const SM_Vertex_index edn = cube.add_vertex(Point(kGrid, 0, 0));
    const SM_Vertex_index eds = cube.add_vertex(Point(kGrid, 0, kGrid));
    const SM_Vertex_index wun = cube.add_vertex(Point(0, kGrid, 0));
    const SM_Vertex_index wus = cube.add_vertex(Point(0, kGrid, kGrid));
    const SM_Vertex_index wdn = cube.add_vertex(Point(0, 0, 0));
    const SM_Vertex_index wds = cube.add_vertex(Point(0, 0, kGrid));

    // 创建正方体的面（六个面，每个面由两个三角形组成）
    cube.add_face(eds, eun, eus);
    cube.add_face(eds, edn, eun);
    cube.add_face(wdn, wus, wun);
    cube.add_face(wdn, wds, wus);
    cube.add_face(wds, eus, wus);
    cube.add_face(wds, eds, eus);
    cube.add_face(edn, wun, eun);
    cube.add_face(edn, wdn, wun);
    cube.add_face(wus, eun, wun);
    cube.add_face(wus, eus, eun);
    cube.add_face(wdn, eds, wds);
    cube.add_face(wdn, edn, eds);
  }

  return *p_lt_surface_mesh;  // 返回静态指针
}

void(GALIB minecraft::cgal_support::applyWorldOffset)(
    LtSurfaceMesh& mesh, const BlockCoordinate& block_coordinate) {
  applyWorldOffset(mesh.getMesh(), block_coordinate);
}

void(GALIB minecraft::cgal_support::applyWorldOffset)(
    SurfaceMeshType& mesh, const BlockCoordinate& block_coordinate) {
  using Point = SurfaceMeshType::Point;
  const double offset_x = block_coordinate.x;
  const double offset_y = block_coordinate.y;
  const double offset_z = block_coordinate.z;

  for (auto v : mesh.vertices()) {
    Point p = mesh.point(v);
    mesh.point(v) = Point(p.x() + offset_x, p.y() + offset_y, p.z() + offset_z);
  }
}

void(GALIB minecraft::cgal_support::applyGrid)(LtSurfaceMesh& mesh,
                                               const GridType grid) {
  SurfaceMeshType& transformed = mesh.getMesh();
  using Point = SurfaceMeshType::Point;

  for (auto v : transformed.vertices()) {
    Point p = transformed.point(v);
    transformed.point(v) = Point(p.x() / static_cast<float>(grid),
                                 p.y() / static_cast<float>(grid),
                                 p.z() / static_cast<float>(grid));
  }
}

// 网格清理函数
void(GALIB minecraft::cgal_support::cleanupMesh)(LtSurfaceMesh& mesh) {
  SurfaceMeshType& surface_mesh = mesh.getMesh();

  // 移除孤立顶点
  CGAL::Polygon_mesh_processing::remove_isolated_vertices(surface_mesh);

  // 移除退化面。
  // 注意：PMP::remove_degenerate_faces 只接受三角形网格，而本项目的网格可能是
  // 四边形/多边形（平面面片不再被拆成三角形），因此多边形网格改为手动检查重复顶点。
  if (CGAL::is_triangle_mesh(surface_mesh)) {
    CGAL::Polygon_mesh_processing::remove_degenerate_faces(surface_mesh);
  } else {
    GALIB_STD vector<SurfaceMeshType::Face_index> degenerated_faces;
    for (SurfaceMeshType::Face_index face : surface_mesh.faces()) {
      GALIB_STD vector<SurfaceMeshType::Vertex_index> face_vertices;
      for (SurfaceMeshType::Vertex_index vertex :
           vertices_around_face(surface_mesh.halfedge(face), surface_mesh)) {
        face_vertices.push_back(vertex);
      }

      bool is_degenerated = face_vertices.size() < 3;
      for (GALIB_STD size_t i = 0; !is_degenerated && i < face_vertices.size();
           ++i) {
        for (GALIB_STD size_t j = i + 1; j < face_vertices.size(); ++j) {
          if (face_vertices[i] == face_vertices[j]) {
            is_degenerated = true;
            break;
          }
        }
      }
      if (is_degenerated) {
        degenerated_faces.push_back(face);
      }
    }
    for (SurfaceMeshType::Face_index face : degenerated_faces) {
      surface_mesh.remove_face(face);
    }
    CGAL::Polygon_mesh_processing::remove_isolated_vertices(surface_mesh);
  }

  // 确保网格是流形的
  if (!CGAL::is_valid_polygon_mesh(surface_mesh)) {
    std::cerr << "Warning: Mesh is not valid after cleanup" << std::endl;
  }
}
