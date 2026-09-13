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
#include <set>
#include <tuple>
#include <vector>

#include "GalibNamespaceDef.h"
#include "Log/GalibLog.h"
#include "Minecraft/CgalSupport/CgalTypeDef.h"
#include "Minecraft/LittleTiles.h"

using galib::minecraft::littletiles::GridType;
using galib::minecraft::littletiles::LittleTilesCoord;

using galib::minecraft::littletiles::AngleID;
using galib::minecraft::littletiles::Flipped;
using galib::minecraft::littletiles::TileEntity;
using galib::minecraft::littletiles::TileFace;
using galib::minecraft::littletiles::TileFaceID;

using galib::minecraft::cgal_support::LtSurfaceMesh;

using CGAL::SM_Vertex_index;

// ---------------------------------------------------------------------------
// Face building helpers: a planar quad is emitted as an n-gon, and only a non-planar
// one falls back to two triangles
// ---------------------------------------------------------------------------
namespace {
using LtVertexIndex =
    galib::minecraft::cgal_support::SurfaceMeshType::Vertex_index;
using LtPoint = galib::minecraft::cgal_support::LtPoint3;
using SurfaceMeshType = galib::minecraft::cgal_support::SurfaceMeshType;

// Whether four points are coplanar (coordinates here are in grid units with a scale
// of a few hundred at most, so an absolute tolerance of 1e-9 is enough)
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
  }  // degenerate face; let CGAL reject it

  const double adx = kD.x() - kA.x(), ady = kD.y() - kA.y(),
               adz = kD.z() - kA.z();
  const double distance = std::fabs(nx * adx + ny * ady + nz * adz) / length;
  return distance < 1e-9;
}

bool isSamePoint(const LtPoint& kA, const LtPoint& kB) {
  return std::fabs(kA.x() - kB.x()) < 1e-9 &&
         std::fabs(kA.y() - kB.y()) < 1e-9 && std::fabs(kA.z() - kB.z()) < 1e-9;
}

// kIndices/kPoints must be given in the same winding order (the order determines the
// face normal direction)
void addQuadFace(galib::minecraft::cgal_support::SurfaceMeshType& mesh,
                 const std::array<LtVertexIndex, 4>& kIndices,
                 const std::array<LtPoint, 4>& kPoints, const bool kFlipped) {
  // Corner offsets can make two corners land on the same position (a degenerate quad).
  // The coincident points must be removed first, otherwise CGAL rejects the whole face
  // as an "invalid polygon".
  std::array<LtVertexIndex, 4> indices{};
  std::array<LtPoint, 4> points{};
  std::size_t count = 0;
  for (std::size_t i = 0; i < 4; ++i) {
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
  }  // degenerated to a point or a line; no face to add

  if (count == 3) {
    mesh.add_face(indices[0], indices[1], indices[2]);
    return;
  }

  if (isPlanarQuad(points[0], points[1], points[2], points[3])) {
    // Planar quad: keep it as one n-gon (no superfluous diagonals appear in wireframe view)
    mesh.add_face(indices[0], indices[1], indices[2], indices[3]);
    return;
  }

  // Non-planar quad: keep the original two-triangle split, with Flipped deciding which
  // diagonal is used
  if (!kFlipped) {
    mesh.add_face(indices[0], indices[1], indices[3]);
    mesh.add_face(indices[1], indices[2], indices[3]);
  } else {
    mesh.add_face(indices[0], indices[1], indices[2]);
    mesh.add_face(indices[0], indices[2], indices[3]);
  }
}
}  // namespace

void galib::minecraft::cgal_support::CreateMeshFromTileEntity(
    LtSurfaceMesh& mesh_data, const TileEntity& kTileEntity,
    const bool kApplyOffset) {
  SurfaceMeshType& mesh = mesh_data.surface_mesh();
  using VertexIndex = SurfaceMeshType::Vertex_index;

  const LtPoint p_eun =
      ConvertToCgalPoint(kTileEntity.GetVertices(AngleID::EUN, kApplyOffset));
  const LtPoint p_eus =
      ConvertToCgalPoint(kTileEntity.GetVertices(AngleID::EUS, kApplyOffset));
  const LtPoint p_edn =
      ConvertToCgalPoint(kTileEntity.GetVertices(AngleID::EDN, kApplyOffset));
  const LtPoint p_eds =
      ConvertToCgalPoint(kTileEntity.GetVertices(AngleID::EDS, kApplyOffset));
  const LtPoint p_wun =
      ConvertToCgalPoint(kTileEntity.GetVertices(AngleID::WUN, kApplyOffset));
  const LtPoint p_wus =
      ConvertToCgalPoint(kTileEntity.GetVertices(AngleID::WUS, kApplyOffset));
  const LtPoint p_wdn =
      ConvertToCgalPoint(kTileEntity.GetVertices(AngleID::WDN, kApplyOffset));
  const LtPoint p_wds =
      ConvertToCgalPoint(kTileEntity.GetVertices(AngleID::WDS, kApplyOffset));

  const VertexIndex eun = mesh.add_vertex(p_eun);
  const VertexIndex eus = mesh.add_vertex(p_eus);
  const VertexIndex edn = mesh.add_vertex(p_edn);
  const VertexIndex eds = mesh.add_vertex(p_eds);
  const VertexIndex wun = mesh.add_vertex(p_wun);
  const VertexIndex wus = mesh.add_vertex(p_wus);
  const VertexIndex wdn = mesh.add_vertex(p_wdn);
  const VertexIndex wds = mesh.add_vertex(p_wds);

  const Flipped& flipped_data = kTileEntity.flipped_data();
  const bool east_flipped = flipped_data.east;
  const bool west_flipped = flipped_data.west;
  const bool south_flipped = flipped_data.south;
  const bool north_flipped = flipped_data.north;
  const bool up_flipped = flipped_data.up;
  const bool down_flipped = flipped_data.down;

  // The winding order matches the original two triangles (the normal direction is
  // unchanged)
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
// Half-space clipping: replaces corefine_and_compute_intersection
// ---------------------------------------------------------------------------
namespace {
struct ClipVec3 {
  double x{0.0};
  double y{0.0};
  double z{0.0};
};

using ClipPolygon = std::vector<ClipVec3>;
using ClipPolyhedron = std::vector<ClipPolygon>;

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
  return std::fabs(kA.x - kB.x) < kClipEpsilon &&
         std::fabs(kA.y - kB.y) < kClipEpsilon &&
         std::fabs(kA.z - kB.z) < kClipEpsilon;
}

ClipVec3 clipCross(const ClipVec3& kA, const ClipVec3& kB) {
  return {kA.y * kB.z - kA.z * kB.y, kA.z * kB.x - kA.x * kB.z,
          kA.x * kB.y - kA.y * kB.x};
}

double clipSquaredLength(const ClipVec3& kA) { return clipDot(kA, kA); }

bool clipIsCollinearMiddlePoint(const ClipVec3& kPrev, const ClipVec3& kPoint,
                                const ClipVec3& kNext) {
  const ClipVec3 before = clipSub(kPoint, kPrev);
  const ClipVec3 after = clipSub(kNext, kPoint);
  const double before_length = std::sqrt(clipSquaredLength(before));
  const double after_length = std::sqrt(clipSquaredLength(after));
  if (before_length < kClipEpsilon || after_length < kClipEpsilon) {
    return true;
  }
  const ClipVec3 cross = clipCross(before, after);
  const double cross_length = std::sqrt(clipSquaredLength(cross));
  return cross_length <=
         kClipEpsilon * std::max(1.0, before_length * after_length);
}

ClipPolygon clipSimplifyPolygon(const ClipPolygon& kPolygon) {
  ClipPolygon polygon;
  for (const ClipVec3& point : kPolygon) {
    if (polygon.empty() || !clipNearlyEqual(polygon.back(), point)) {
      polygon.push_back(point);
    }
  }
  while (polygon.size() > 1 &&
         clipNearlyEqual(polygon.front(), polygon.back())) {
    polygon.pop_back();
  }

  bool removed = true;
  while (removed && polygon.size() >= 3) {
    removed = false;
    ClipPolygon simplified;
    simplified.reserve(polygon.size());
    for (std::size_t i = 0; i < polygon.size(); ++i) {
      const ClipVec3& prev = polygon[(i + polygon.size() - 1) % polygon.size()];
      const ClipVec3& point = polygon[i];
      const ClipVec3& next = polygon[(i + 1) % polygon.size()];
      if (clipIsCollinearMiddlePoint(prev, point, next)) {
        removed = true;
        continue;
      }
      simplified.push_back(point);
    }
    polygon.swap(simplified);
  }
  return polygon;
}

// Clip a convex polyhedron with the half-space n.p >= d; kept faces retain their
// winding order and a new n-gon (cap) is added for the cut plane.
ClipPolyhedron clipPolyhedronByPlane(const ClipPolyhedron& kPolyhedron,
                                     const ClipVec3& kN, const double kD) {
  ClipPolyhedron result;
  ClipPolygon cut_points;

  for (const ClipPolygon& face : kPolyhedron) {
    const std::size_t count = face.size();
    if (count < 3) {
      continue;
    }

    ClipPolygon clipped;
    for (std::size_t i = 0; i < count; ++i) {
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
        if (std::fabs(denominator) < kClipEpsilon) {
          continue;
        }
        const ClipVec3 cross_point =
            clipLerp(current, next, current_distance / denominator);
        clipped.push_back(cross_point);
        cut_points.push_back(cross_point);
      }
    }
    // Vertices lying exactly on the clipping plane produce duplicate points, and the cap
    // face may also carry collinear cut points lying on its edges. Both must be removed,
    // otherwise CGAL rejects the whole face.
    const ClipPolygon simplified = clipSimplifyPolygon(clipped);
    if (simplified.size() >= 3) {
      result.push_back(simplified);
    }
  }

  // Cap face: all newly produced points lie on the clipping plane, so after
  // deduplication, sorting by in-plane polar angle yields the correct winding order
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

    // Build an in-plane basis (u, v) with u x v = -n:
    // the kept side is n.p >= d and the solid is in the +n direction, so the cap face's
    // outward normal points along -n.
    ClipVec3 u =
        (std::fabs(kN.x) <= std::fabs(kN.y) &&
         std::fabs(kN.x) <= std::fabs(kN.z))
            ? ClipVec3{1.0, 0.0, 0.0}
            : ((std::fabs(kN.y) <= std::fabs(kN.z)) ? ClipVec3{0.0, 1.0, 0.0}
                                                    : ClipVec3{0.0, 0.0, 1.0});
    const double u_along_n = clipDot(u, kN);
    u = clipSub(u, {kN.x * u_along_n, kN.y * u_along_n, kN.z * u_along_n});
    const double u_length = std::sqrt(clipDot(u, u));
    if (u_length < kClipEpsilon) {
      return result;
    }
    u = {u.x / u_length, u.y / u_length, u.z / u_length};
    const ClipVec3 v{u.y * kN.z - u.z * kN.y, u.z * kN.x - u.x * kN.z,
                     u.x * kN.y - u.y * kN.x};

    std::sort(unique_points.begin(), unique_points.end(),
              [&center, &u, &v](const ClipVec3& kLhs, const ClipVec3& kRhs) {
                const ClipVec3 lhs = clipSub(kLhs, center);
                const ClipVec3 rhs = clipSub(kRhs, center);
                return std::atan2(clipDot(lhs, v), clipDot(lhs, u)) <
                       std::atan2(clipDot(rhs, v), clipDot(rhs, u));
              });
    const ClipPolygon cap = clipSimplifyPolygon(unique_points);
    if (cap.size() >= 3) {
      result.push_back(cap);
    }
  }

  return result;
}

ClipVec3 clipToVec3(
    const galib::minecraft::littletiles::LittleTilesCoord& kCoord) {
  return {kCoord.x, kCoord.y, kCoord.z};
}

bool clipIsPlanarQuad(const ClipPolygon& kQuad) {
  const ClipVec3 &a = kQuad[0], &b = kQuad[1], &c = kQuad[2], &d = kQuad[3];
  const ClipVec3 ab{b.x - a.x, b.y - a.y, b.z - a.z};
  const ClipVec3 ac{c.x - a.x, c.y - a.y, c.z - a.z};
  const ClipVec3 normal{ab.y * ac.z - ab.z * ac.y, ab.z * ac.x - ab.x * ac.z,
                        ab.x * ac.y - ab.y * ac.x};
  const double length = std::sqrt(clipDot(normal, normal));
  if (length < 1e-12) {
    return true;
  }
  const ClipVec3 ad{d.x - a.x, d.y - a.y, d.z - a.z};
  return std::fabs(clipDot(normal, ad)) / length < 1e-9;
}

SurfaceMeshType::Face_index addClipFaceWithUnweldedVertices(
    SurfaceMeshType& desc_mesh, const ClipPolygon& kFace) {
  std::vector<SurfaceMeshType::Vertex_index> face_indices;
  face_indices.reserve(kFace.size());
  for (const ClipVec3& point : kFace) {
    face_indices.push_back(
        desc_mesh.add_vertex(LtPoint(point.x, point.y, point.z)));
  }
  return desc_mesh.add_face(face_indices);
}

bool clipIsDegenerateTriangle(const ClipVec3& kA, const ClipVec3& kB,
                              const ClipVec3& kC) {
  const ClipVec3 ab = clipSub(kB, kA);
  const ClipVec3 ac = clipSub(kC, kA);
  return clipSquaredLength(clipCross(ab, ac)) < kClipEpsilon * kClipEpsilon;
}

std::size_t addClipFaceAsTriangleFanWithUnweldedVertices(
    SurfaceMeshType& desc_mesh, const ClipPolygon& kFace) {
  if (kFace.size() < 3) {
    return 0;
  }

  std::size_t added_face_count = 0;
  for (std::size_t i = 1; i + 1 < kFace.size(); ++i) {
    const ClipVec3& a = kFace[0];
    const ClipVec3& b = kFace[i];
    const ClipVec3& c = kFace[i + 1];
    if (clipIsDegenerateTriangle(a, b, c)) {
      continue;
    }

    const auto va = desc_mesh.add_vertex(LtPoint(a.x, a.y, a.z));
    const auto vb = desc_mesh.add_vertex(LtPoint(b.x, b.y, b.z));
    const auto vc = desc_mesh.add_vertex(LtPoint(c.x, c.y, c.z));
    if (desc_mesh.add_face(va, vb, vc) != SurfaceMeshType::null_face()) {
      ++added_face_count;
    }
  }
  return added_face_count;
}

// Add one quad face to the polyhedron to be clipped:
// coincident corners are merged first; a planar quad is kept as one face, and a
// non-planar (twisted) quad is split into two triangles following the Flipped rule -
// consistent with the rendering in CreateMeshFromTileEntity, otherwise the clipping
// result would differ from the original boolean operation.
void appendQuadFace(ClipPolyhedron& desc_polyhedron, ClipPolygon kPoints,
                    const bool kFlipped) {
  ClipPolygon polygon = clipSimplifyPolygon(kPoints);

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

// ---------------------------------------------------------------------------
// A faithful reproduction of LittleTiles 1.12
// LittleTransformableBox.requestCache():
// the box shape = the 6 axis-aligned box faces each cut by "tilted planes"
// (rather than a convex polyhedron intersection).
// - Stage 1: derive each face's tilted planes and tilted strips from the 8
//            transformed corners; the tilted strips are then clipped by the 6 axis
//            planes (keeping the back side).
// - Stage 2: each axis's box (axis-aligned) quad is clipped by the tilted planes;
//            convex faces take the intersection, non-convex faces take the union via a
//            VectorFan 2D projection (cut2d/cutAxisStrip2).
// ---------------------------------------------------------------------------
namespace {
constexpr int kDown = 0;
constexpr int kUp = 1;
constexpr int kNorth = 2;
constexpr int kSouth = 3;
constexpr int kWest = 4;
constexpr int kEast = 5;
constexpr int kAxisX = 0;
constexpr int kAxisY = 1;
constexpr int kAxisZ = 2;

// Corner ID order = AngleID order: 0 EUN 1 EUS 2 EDN 3 EDS 4 WUN 5 WUS 6 WDN 7 WDS
// The 4 corners of each face (1.12 BoxFace order; triangle selection and axis strips
// both use the same order)
constexpr int kFaceCorners[6][4] = {
    /*DOWN*/ {7, 6, 2, 3},   // WDS WDN EDN EDS
    /*UP*/   {4, 5, 1, 0},   // WUN WUS EUS EUN
    /*NORTH*/{0, 2, 6, 4},   // EUN EDN WDN WUN
    /*SOUTH*/{5, 7, 3, 1},   // WUS WDS EDS EUS
    /*WEST*/ {4, 6, 7, 5},   // WUN WDN WDS WUS
    /*EAST*/ {1, 3, 2, 0},   // EUS EDS EDN EUN
};
// Principal axis of each face
constexpr int kFaceAxis[6] = {kAxisY, kAxisY, kAxisZ, kAxisZ, kAxisX, kAxisX};
// Outward normal of each face
constexpr ClipVec3 kFaceDir[6] = {{0, -1, 0}, {0, 1, 0},   {0, 0, -1},
                                  {0, 0, 1},  {-1, 0, 0}, {1, 0, 0}};

constexpr double kVfEps = 1e-4;  // VectorFan.EPSILON

double vfComponent(const ClipVec3& p, int axis) {
  return axis == kAxisX ? p.x : (axis == kAxisY ? p.y : p.z);
}
void vfSetComponent(ClipVec3& p, int axis, double v) {
  if (axis == kAxisX) p.x = v;
  else if (axis == kAxisY) p.y = v;
  else p.z = v;
}
bool vfVeq(const ClipVec3& a, const ClipVec3& b, double eps) {
  return std::fabs(a.x - b.x) + std::fabs(a.y - b.y) +
             std::fabs(a.z - b.z) <
         eps;
}
ClipVec3 vfNorm(const ClipVec3& v) {
  double len = std::sqrt(clipSquaredLength(v));
  if (len < 1e-12) return {0.0, 0.0, 0.0};
  return {v.x / len, v.y / len, v.z / len};
}
// isFront semantics: 1 = front side (dot > eps), -1 = back side (dot < -eps),
// 0 = on the plane (|dot| <= eps)
int vfSide(const ClipVec3& p, const ClipVec3& o, const ClipVec3& n) {
  double r = clipDot(clipSub(p, o), n);
  if (std::fabs(r) < kVfEps) return 0;
  return r > 0.0 ? 1 : -1;
}

// Sutherland-Hodgman clipping with the plane (o,n): keep the strictly back side, drop
// the front side and points lying exactly on the plane
// (Python's cut uses keep = (isFront is False), and on the plane isFront returns
// None === not kept)
ClipPolygon vfCut(const ClipPolygon& poly, const ClipVec3& o,
                  const ClipVec3& n) {
  if (poly.size() < 3) return {};
  auto keep = [&](const ClipVec3& p) { return vfSide(p, o, n) < 0; };
  ClipPolygon out;
  const std::size_t count = poly.size();
  ClipVec3 prev = poly.back();
  bool pi = keep(prev);
  for (const ClipVec3& cur : poly) {
    const bool ci = keep(cur);
    if (ci != pi) {
      const ClipVec3 dv = clipSub(cur, prev);
      const double den = clipDot(n, dv);
      if (std::fabs(den) > 1e-12) {
        const double t = clipDot(clipSub(o, prev), n) / den;
        out.push_back({prev.x + t * dv.x, prev.y + t * dv.y,
                       prev.z + t * dv.z});
      }
    }
    if (ci) out.push_back(cur);
    prev = cur;
    pi = ci;
  }
  // deduplicate
  ClipPolygon res;
  for (const ClipVec3& p : out) {
    if (res.empty() || !vfVeq(res.back(), p, 1e-9)) res.push_back(p);
  }
  if (res.size() >= 3 && vfVeq(res.front(), res.back(), 1e-9)) res.pop_back();
  if (res.size() < 3) return {};
  return res;
}

// Whether the three corners of tri are flush with the box corners along the principal
// axis (i.e. not tilted)
bool vfCheckEqual(const int tri[3], int axis,
                  const std::array<ClipVec3, 8>& corners,
                  const std::array<ClipVec3, 8>& base) {
  for (int j = 0; j < 3; ++j) {
    const int c = tri[j];
    if (std::fabs(vfComponent(corners[c], axis) -
                  vfComponent(base[c], axis)) > kVfEps)
      return false;
  }
  return true;
}

ClipVec3 vfTriNormal(const int tri[3],
                     const std::array<ClipVec3, 8>& corners) {
  return clipCross(clipSub(corners[tri[1]], corners[tri[0]]),
                   clipSub(corners[tri[2]], corners[tri[0]]));
}

// Produce the triangle corners (Flipped decides which one is taken)
void vfTriangle(const int fc[4], bool inv, bool first, int tri[3]) {
  if (first) {
    tri[0] = fc[0]; tri[1] = fc[1]; tri[2] = inv ? fc[3] : fc[2];
  } else {
    tri[0] = fc[0]; tri[1] = fc[2]; tri[2] = fc[3];
    if (inv) { tri[0] = fc[1]; }
  }
}

ClipPolygon vfCreateStrip(const int* clist, int n,
                          const std::array<ClipVec3, 8>& corners) {
  ClipPolygon out;
  for (int k = 0; k < n; ++k) {
    bool dup = false;
    for (const ClipVec3& p : out) {
      if (vfVeq(corners[clist[k]], p, kVfEps)) { dup = true; break; }
    }
    if (!dup) out.push_back(corners[clist[k]]);
  }
  if (static_cast<int>(out.size()) < n) {
    return out.size() >= 3 ? out : ClipPolygon{};
  }
  return out;
}

struct VfPlane {
  ClipVec3 origin;
  ClipVec3 normal;
  bool valid = false;
};

// Axis plane of the face `facing` (origin is the box coordinate of that face's first
// corner along the principal axis)
VfPlane vfAxisPlane(int facing, const std::array<ClipVec3, 8>& base) {
  ClipVec3 origin{0.0, 0.0, 0.0};
  vfSetComponent(
      origin, kFaceAxis[facing],
      vfComponent(base[kFaceCorners[facing][0]], kFaceAxis[facing]));
  return {origin, kFaceDir[facing], true};
}

// ---- VectorFan 2D projection clipping ----
int vfGetOne(int axis) { return axis == kAxisX ? kAxisY : (axis == kAxisY ? kAxisZ : kAxisX); }
int vfGetTwo(int axis) { return axis == kAxisX ? kAxisZ : (axis == kAxisY ? kAxisX : kAxisY); }
int vfGetThird(int one, int two) {
  return (one != kAxisX && two != kAxisX) ? kAxisX
         : (one != kAxisY && two != kAxisY) ? kAxisY
                                            : kAxisZ;
}

struct VfRay2d {
  int one, two;
  double o1, o2, d1, d2;
  VfRay2d(int one_, int two_, const ClipVec3& before, const ClipVec3& vec)
      : one(one_), two(two_) {
    o1 = vfComponent(before, one);
    o2 = vfComponent(before, two);
    d1 = vfComponent(vec, one) - o1;
    d2 = vfComponent(vec, two) - o2;
  }
  double getOrigin(int a) const { return a == one ? o1 : o2; }
  double getDirection(int a) const { return a == one ? d1 : d2; }
  double getT(int a, double value) const {
    return a == one ? (value - o1) / d1 : (value - o2) / d2;
  }
  bool isCoordinateOnLine(double onev, double twov) const {
    if (d1 == 0.0) return (std::fabs(o1 - onev) < kVfEps);
    if (d2 == 0.0) return (std::fabs(o2 - twov) < kVfEps);
    const double other = o2 + d2 * (onev - o1) / d1;
    return std::fabs(other - twov) < kVfEps;
  }
  // 1 = true (right side), 0 = on the line, -1 = false (left side)
  int isCoordinateToTheRight(double onev, double twov) const {
    const double r = d1 * (twov - o2) - d2 * (onev - o1);
    if (r > -kVfEps && r < kVfEps) return 0;
    return r < 0.0 ? 1 : -1;
  }
  ClipVec3 intersect(const ClipVec3& start, const ClipVec3& end,
                     int thirdAxis, double thirdValue, bool& ok) const {
    ok = false;
    const double lo1 = vfComponent(start, one), lo2 = vfComponent(start, two);
    const double ld1 = vfComponent(end, one) - lo1;
    const double ld2 = vfComponent(end, two) - lo2;
    const double den = ld1 * d2 - d1 * ld2;
    if (den > -kVfEps && den < kVfEps) return {};
    const double t = ((lo2 - o2) * ld1 + o1 * ld2 - lo1 * ld2) / den;
    ClipVec3 p{thirdValue, thirdValue, thirdValue};
    vfSetComponent(p, one, o1 + t * d1);
    vfSetComponent(p, two, o2 + t * d2);
    ok = true;
    return p;
  }
  // Returns the intersection parameter t; raises = true means the two rays are parallel
  // and collinear (corresponding to Python raising Parallel). raises is set only when
  // collinear; parallel but non-intersecting lines give raises = false and return -1
  // (consistent with Python).
  double intersectWhen(const VfRay2d& line, bool& raises) const {
    const double den = d2 * line.d1 - d1 * line.d2;
    if (den > -kVfEps && den < kVfEps) {
      raises = isCoordinateOnLine(line.o1, line.o2);
      return -1.0;
    }
    raises = false;
    return ((line.o2 - o2) * line.d1 + o1 * line.d2 - line.o1 * line.d2) / den;
  }
};

bool vfFanEquals(const ClipPolygon& p1, const ClipPolygon& p2) {
  if (p1.size() != p2.size()) return false;
  const std::size_t n = p1.size();
  for (std::size_t start = 0; start < n; ++start) {
    const ClipVec3& f0 = p1[start];
    bool match = std::fabs(f0.x - p2[0].x) < kVfEps &&
                 std::fabs(f0.y - p2[0].y) < kVfEps &&
                 std::fabs(f0.z - p2[0].z) < kVfEps;
    if (match) {
      bool ok = true;
      for (std::size_t k = 1; k < n; ++k) {
        const ClipVec3& a = p1[(start + k) % n];
        const ClipVec3& b = p2[k];
        if (!(std::fabs(a.x - b.x) < kVfEps && std::fabs(a.y - b.y) < kVfEps &&
              std::fabs(a.z - b.z) < kVfEps)) {
          ok = false;
          break;
        }
      }
      if (ok) return true;
    }
  }
  return false;
}

bool vfIsInside2d(const ClipPolygon& fan, const ClipPolygon& other, int one,
                  int two, bool inverse) {
  const std::size_t n = fan.size();
  for (const ClipVec3& p : other) {
    const double px = vfComponent(p, one), py = vfComponent(p, two);
    bool inside = false;
    std::size_t index = 0;
    while (index + 2 < n) {
      const double f1 = vfComponent(fan[0], one), f2 = vfComponent(fan[0], two);
      const double s1 = vfComponent(fan[index + 1], one),
                   s2 = vfComponent(fan[index + 1], two);
      const double t1 = vfComponent(fan[index + 2], one),
                   t2 = vfComponent(fan[index + 2], two);
      auto rightOf = [&](double a1, double a2, double b1, double b2) -> int {
        const double r = (b1 - a1) * (py - a2) - (b2 - a2) * (px - a1);
        if (r > -kVfEps && r < kVfEps) return 0;
        return r < 0.0 ? 1 : -1;
      };
      int r = rightOf(f1, f2, s1, s2);
      if (r == 0 || (r == -1) == inverse) {
        r = rightOf(s1, s2, t1, t2);
        if (r == 0 || (r == -1) == inverse) {
          r = rightOf(t1, t2, f1, f2);
          if (r == 0 || (r == -1) == inverse) {
            inside = true;
            break;
          }
        }
      }
      ++index;
    }
    if (!inside) return false;
  }
  return true;
}

bool vfIntersect2d(const ClipPolygon& fan, const ClipPolygon& other, int one,
                   int two, bool inverse) {
  if (vfFanEquals(fan, other)) return true;
  int parallel = 0;
  const std::size_t n = fan.size(), m = other.size();
  for (std::size_t i = 1; i <= n; ++i) {
    const ClipVec3& b1 = fan[i - 1];
    const ClipVec3& v1 = fan[i % n];
    VfRay2d r1(one, two, b1, v1);
    for (std::size_t i2 = 1; i2 <= m; ++i2) {
      const ClipVec3& b2 = other[i2 - 1];
      const ClipVec3& v2 = other[i2 % m];
      VfRay2d r2(one, two, b2, v2);
      bool par1 = false, par2 = false;
      const double t = r1.intersectWhen(r2, par1);
      const double ot = r2.intersectWhen(r1, par2);
      if (par1 || par2) {
        const double st = r1.getT(one, r2.o1);
        const double et = r1.getT(one, r2.o1 + r2.d1);
        if ((st > kVfEps && st < 1 - kVfEps) ||
            (et > kVfEps && et < 1 - kVfEps)) {
          ++parallel;
          if (parallel > 1) return true;
        }
      } else if (t > kVfEps && t < 1 - kVfEps && ot > kVfEps &&
                 ot < 1 - kVfEps) {
        return true;
      }
    }
  }
  if (vfIsInside2d(fan, other, one, two, inverse) ||
      vfIsInside2d(other, fan, one, two, inverse)) {
    return true;
  }
  return false;
}

typedef std::vector<ClipPolygon> Clippolygon_done;

// 2D clipping of fan by a single edge; done collects the pieces on the discarded side
// (possibly several)
ClipPolygon vfCut2dSingle(ClipPolygon fan, const VfRay2d& ray, int one, int two,
                          bool inverse, Clippolygon_done* done) {
  const std::size_t n = fan.size();
  std::vector<int> cutted(n);
  for (std::size_t j = 0; j < n; ++j) {
    int c = ray.isCoordinateToTheRight(vfComponent(fan[j], one),
                                       vfComponent(fan[j], two));
    if (inverse && c != 0) c = -c;
    cutted[j] = c;
  }
  bool all_same = true;
  int all_value = 0;  // 0 = undetermined (equivalent to Python None), 1 = right side, -1 = left side
  bool all_value_set = (cutted[0] != 0);
  if (all_value_set) all_value = cutted[0];
  for (std::size_t j = 1; j < n; ++j) {
    if (!all_same) break;
    const int c = cutted[j];
    if (all_value_set) {
      // Python: elif allValue!=c and c is not None: allSame=False
      if (c != 0 && c != all_value) all_same = false;
    } else {
      // Python: if allValue is None: allValue=c  (take the first non-plane side as the reference)
      if (c != 0) { all_value = c; all_value_set = true; }
    }
  }
  if (all_same) {
    if (!all_value_set) return {};  // all on the clipping line, degenerate
    if (all_value == 1) return fan;  // all on the kept side
    if (done) done->push_back(fan);  // all on the discarded side
    return {};
  }
  const int third = vfGetThird(one, two);
  const double tv = vfComponent(fan[0], third);
  ClipPolygon left, right;
  int before_c = cutted[n - 1];
  ClipVec3 before_v = fan[n - 1];
  for (std::size_t j = 0; j < n; ++j) {
    const ClipVec3& v = fan[j];
    const int c = cutted[j];
    if (c == 1) {
      if (before_c == -1) {
        bool ok = false;
        const ClipVec3 iv = ray.intersect(v, before_v, third, tv, ok);
        if (ok) { left.push_back(iv); right.push_back(iv); }
      }
      right.push_back(v);
    } else if (c == -1) {
      if (before_c == 1) {
        bool ok = false;
        const ClipVec3 iv = ray.intersect(v, before_v, third, tv, ok);
        if (ok) { left.push_back(iv); right.push_back(iv); }
      }
      left.push_back(v);
    } else {
      left.push_back(v);
      right.push_back(v);
    }
    before_c = c;
    before_v = v;
  }
  if (left.size() >= 3 && done) done->push_back(left);
  if (right.size() < 3) return {};
  return right;
}

// 2D clipping of fan by the cutter polygon; returns the done piece list (includes the
// inner remainder when takeInner is set)
ClipPolygon vfCut2d(const ClipPolygon& fan, const ClipPolygon& cutter, int one,
                    int two, bool inverse, bool takeInner,
                    Clippolygon_done* done) {
  ClipPolygon to_cut = fan;
  done->clear();
  const std::size_t nc = cutter.size();
  for (std::size_t i = 1; i <= nc; ++i) {
    const bool last = (i == nc);
    const ClipVec3& vc = last ? cutter[0] : cutter[i];
    const ClipVec3& bc = cutter[i - 1];
    VfRay2d ray(one, two, bc, vc);
    to_cut = vfCut2dSingle(to_cut, ray, one, two, inverse,
                           takeInner ? nullptr : done);
    if (to_cut.empty()) return {};
  }
  if (takeInner) done->push_back(to_cut);
  return to_cut;
}

// Two-plane clipping of a non-convex face (the union of two half-space regions);
// returns several result polygons
std::vector<ClipPolygon> vfCutAxisStrip2(int facing, const ClipPolygon& strip,
                                         const VfPlane& pa, const VfPlane& pb) {
  const int axis = kFaceAxis[facing];
  const int one = vfGetOne(axis), two = vfGetTwo(axis);
  const bool inverse = (facing == kUp || facing == kSouth || facing == kEast);
  const ClipPolygon s1 =
      pa.valid ? vfCut(strip, pa.origin, pa.normal) : ClipPolygon{};
  const ClipPolygon s2 =
      pb.valid ? vfCut(strip, pb.origin, pb.normal) : ClipPolygon{};
  std::vector<ClipPolygon> result;
  if (!s1.empty() && !s2.empty() && vfIntersect2d(s1, s2, one, two, inverse)) {
    Clippolygon_done fans;
    (void)vfCut2d(s1, s2, one, two, inverse, false, &fans);
    result.push_back(s2);
    result.insert(result.end(), fans.begin(), fans.end());
    return result;
  }
  if (!s1.empty()) result.push_back(s1);
  if (!s2.empty()) result.push_back(s2);
  return result;
}

}

bool galib::minecraft::cgal_support::ClipTileEntityToBox(
    LtSurfaceMesh& desc_mesh, const TileEntity& kTileEntity,
    const bool kApplyOffset) {
  // The 8 corners: tilted = with offsets applied, base = the box (axis-aligned) corners
  std::array<ClipVec3, 8> corners{};
  std::array<ClipVec3, 8> base{};
  const AngleID angle_ids[8] = {AngleID::EUN, AngleID::EUS, AngleID::EDN,
                                AngleID::EDS, AngleID::WUN, AngleID::WUS,
                                AngleID::WDN, AngleID::WDS};
  for (int i = 0; i < 8; ++i) {
    corners[i] = clipToVec3(kTileEntity.GetVertices(angle_ids[i], kApplyOffset));
    base[i] = clipToVec3(kTileEntity.GetVertices(angle_ids[i], false));
  }

  const Flipped& flipped = kTileEntity.flipped_data();
  const bool flipped_arr[6] = {flipped.down, flipped.up, flipped.north,
                               flipped.south, flipped.west, flipped.east};

  // Stage 1: each face's tilted planes + tilted strips
  VfPlane tilted_planes[6][2];  // [face][a/b]
  ClipPolygon tilted_strips[6][2];
  bool convex[6];
  VfPlane axis_planes[6];
  for (int f = 0; f < 6; ++f) axis_planes[f] = vfAxisPlane(f, base);

  for (int f = 0; f < 6; ++f) {
    const bool inv = flipped_arr[f];
    int tri1[3], tri2[3];
    vfTriangle(kFaceCorners[f], inv, true, tri1);
    vfTriangle(kFaceCorners[f], inv, false, tri2);
    const ClipVec3 n1 = vfTriNormal(tri1, corners);
    const ClipVec3 n2 = vfTriNormal(tri2, corners);
    const bool s1 = vfCheckEqual(tri1, kFaceAxis[f], corners, base);
    const bool s2 = vfCheckEqual(tri2, kFaceAxis[f], corners, base);
    if (s1 && s2) {
      tilted_planes[f][0].valid = false;
      tilted_planes[f][1].valid = false;
      convex[f] = true;
      continue;
    }
    const ClipVec3 n1n = vfNorm(n1), n2n = vfNorm(n2);
    const bool n1zero = vfVeq(n1n, {0.0, 0.0, 0.0}, kVfEps);
    const bool n2zero = vfVeq(n2n, {0.0, 0.0, 0.0}, kVfEps);
    ClipPolygon strip1, strip2;
    const bool parallel = vfVeq(n1n, n2n, kVfEps);
    if (parallel) {
      if (!s1 && !n1zero) {
        strip1 = vfCreateStrip(kFaceCorners[f], 4, corners);
        if (!strip1.empty())
          tilted_planes[f][0] = {corners[tri1[0]], n1n, true};
      }
    } else {
      if (!s1 && !n1zero) {
        strip1 = vfCreateStrip(tri1, 3, corners);
        if (!strip1.empty())
          tilted_planes[f][0] = {corners[tri1[0]], n1n, true};
      }
      if (!s2 && !n2zero) {
        strip2 = vfCreateStrip(tri2, 3, corners);
        if (!strip2.empty())
          tilted_planes[f][1] = {corners[tri2[0]], n2n, true};
      }
    }
    // convex: whether strip2 is on the front side of plane1
    bool is_convex = true;
    if (!strip1.empty() && !strip2.empty() && tilted_planes[f][0].valid) {
      for (const ClipVec3& v : strip2) {
        if (vfSide(v, tilted_planes[f][0].origin,
                   tilted_planes[f][0].normal) == 1) {
          is_convex = false;
          break;
        }
      }
    }
    // Both strips are clipped by the 6 axis planes (keeping the back side)
    if (!strip1.empty()) {
      for (int jf = 0; jf < 6; ++jf) {
        strip1 = vfCut(strip1, axis_planes[jf].origin, axis_planes[jf].normal);
        if (strip1.empty()) break;
      }
    }
    if (!strip2.empty()) {
      for (int jf = 0; jf < 6; ++jf) {
        strip2 = vfCut(strip2, axis_planes[jf].origin, axis_planes[jf].normal);
        if (strip2.empty()) break;
      }
    }
    tilted_strips[f][0] = strip1;
    tilted_strips[f][1] = strip2;
    convex[f] = is_convex;
  }

  // Stage 2: each face's box (axis-aligned) quad is clipped by the tilted planes
  std::vector<ClipPolygon> axis_strips[6];
  for (int f = 0; f < 6; ++f) {
    ClipPolygon quad;
    for (int c = 0; c < 4; ++c) quad.push_back(base[kFaceCorners[f][c]]);
    std::size_t distinct = 0;
    for (std::size_t k = 0; k < quad.size(); ++k) {
      bool dup = false;
      for (std::size_t m = 0; m < k; ++m) {
        if (vfVeq(quad[m], quad[k], kVfEps)) { dup = true; break; }
      }
      if (!dup) ++distinct;
    }
    if (distinct < 3) {
      axis_strips[f].clear();
      continue;
    }
    std::vector<ClipPolygon> polys;
    polys.push_back(quad);
    for (int j = 0; j < 6; ++j) {
      const VfPlane& pa = tilted_planes[j][0];
      const VfPlane& pb = tilted_planes[j][1];
      if (!pa.valid && !pb.valid) continue;
      std::vector<ClipPolygon> newp;
      if (convex[j]) {
        for (ClipPolygon poly : polys) {
          if (pa.valid) {
            poly = vfCut(poly, pa.origin, pa.normal);
            if (poly.empty()) continue;
          }
          if (pb.valid) {
            poly = vfCut(poly, pb.origin, pb.normal);
            if (poly.empty()) continue;
          }
          newp.push_back(poly);
        }
      } else {
        for (const ClipPolygon& poly : polys) {
          const std::vector<ClipPolygon> r = vfCutAxisStrip2(f, poly, pa, pb);
          for (const ClipPolygon& piece : r) newp.push_back(piece);
        }
      }
      polys.swap(newp);
      if (polys.empty()) break;
    }
    axis_strips[f] = polys;
  }

  // Collect: after deduplication, all output faces are obtained
  auto round_key = [](const ClipVec3& p) {
    long long key[3];
    for (int a = 0; a < 3; ++a) {
      key[a] = static_cast<long long>(std::llround(vfComponent(p, a) * 1e6));
    }
    return std::make_tuple(key[0], key[1], key[2]);
  };
  std::vector<ClipPolygon> faces;
  {
    std::set<std::set<std::tuple<long long, long long, long long>>> seen;
    auto add_face = [&](const ClipPolygon& poly) {
      if (poly.size() < 3) return;
      std::set<std::tuple<long long, long long, long long>> pts;
      for (const ClipVec3& p : poly) pts.insert(round_key(p));
      if (pts.size() < 3) return;
      if (seen.count(pts)) return;
      seen.insert(pts);
      faces.push_back(poly);
    };
    for (int f = 0; f < 6; ++f) {
      for (const ClipPolygon& s : tilted_strips[f]) add_face(s);
    }
    for (int f = 0; f < 6; ++f) {
      for (const ClipPolygon& s : axis_strips[f]) add_face(s);
    }
  }

  SurfaceMeshType& mesh = desc_mesh.surface_mesh();
  std::map<std::array<long long, 3>, SurfaceMeshType::Vertex_index>
      welded_vertices;
  const double weld_scale = 1e6;  // in grid units a 1e-6 quantization is precise enough to distinguish real vertices
  std::size_t fallback_face_count = 0;

  for (const ClipPolygon& face : faces) {
    if (face.size() < 3) {
      continue;
    }
    if (face.size() > 4) {
      fallback_face_count +=
          addClipFaceAsTriangleFanWithUnweldedVertices(mesh, face);
      continue;
    }

    std::vector<SurfaceMeshType::Vertex_index> face_indices;
    face_indices.reserve(face.size());
    for (const ClipVec3& point : face) {
      const std::array<long long, 3> key{
          static_cast<long long>(std::llround(point.x * weld_scale)),
          static_cast<long long>(std::llround(point.y * weld_scale)),
          static_cast<long long>(std::llround(point.z * weld_scale))};
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

    // Clipping produces T-junctions: the faces close in coordinates, but the
    // Surface_mesh manifold topology cannot necessarily share those edges. When CGAL
    // rejects them, independent vertices + a triangle fan preserve the faces the OBJ
    // needs.
    if (mesh.add_face(face_indices) == SurfaceMeshType::null_face()) {
      if (face.size() == 3) {
        if (addClipFaceWithUnweldedVertices(mesh, face) !=
            SurfaceMeshType::null_face()) {
          ++fallback_face_count;
        }
      } else {
        fallback_face_count +=
            addClipFaceAsTriangleFanWithUnweldedVertices(mesh, face);
      }
    }
  }

#ifdef GALIB_DEBUG
  if (fallback_face_count > 0) {
    galib::ProgressPrintf(
        "ClipTileEntityToBox: kept %zu triangulated fallback face(s)\n",
        fallback_face_count);
  }
#endif

  return mesh.number_of_faces() > 0;
}

const LtSurfaceMesh& galib::minecraft::cgal_support::CreateIntersectionCube(
    const GridType kGrid) {
  // Static pointer, so it is created only on the first call
  static LtSurfaceMesh* p_lt_surface_mesh = nullptr;

  // If the cube has not been created yet, build it
  if (!p_lt_surface_mesh) {
    p_lt_surface_mesh = new LtSurfaceMesh();
    SurfaceMeshType& cube = p_lt_surface_mesh->surface_mesh();

    // Build the cube's vertices p1(0, 0, 0) and p2(1, 1, 1)
    using Point = SurfaceMeshType::Point;

    // Vertex coordinates
    const SM_Vertex_index eun = cube.add_vertex(Point(kGrid, kGrid, 0));
    const SM_Vertex_index eus = cube.add_vertex(Point(kGrid, kGrid, kGrid));
    const SM_Vertex_index edn = cube.add_vertex(Point(kGrid, 0, 0));
    const SM_Vertex_index eds = cube.add_vertex(Point(kGrid, 0, kGrid));
    const SM_Vertex_index wun = cube.add_vertex(Point(0, kGrid, 0));
    const SM_Vertex_index wus = cube.add_vertex(Point(0, kGrid, kGrid));
    const SM_Vertex_index wdn = cube.add_vertex(Point(0, 0, 0));
    const SM_Vertex_index wds = cube.add_vertex(Point(0, 0, kGrid));

    // Create the cube's faces (six faces, each made of two triangles)
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

  return *p_lt_surface_mesh;  // return the static pointer
}

void(galib::minecraft::cgal_support::ApplyWorldOffset)(
    LtSurfaceMesh& mesh, const BlockCoordinate& block_coordinate) {
  ApplyWorldOffset(mesh.surface_mesh(), block_coordinate);
}

void(galib::minecraft::cgal_support::ApplyWorldOffset)(
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

void(galib::minecraft::cgal_support::ApplyGrid)(LtSurfaceMesh& mesh,
                                                const GridType grid) {
  SurfaceMeshType& transformed = mesh.surface_mesh();
  using Point = SurfaceMeshType::Point;

  for (auto v : transformed.vertices()) {
    Point p = transformed.point(v);
    transformed.point(v) = Point(p.x() / static_cast<float>(grid),
                                 p.y() / static_cast<float>(grid),
                                 p.z() / static_cast<float>(grid));
  }
}

// Mesh cleanup function
void(galib::minecraft::cgal_support::CleanupMesh)(LtSurfaceMesh& mesh) {
  SurfaceMeshType& surface_mesh = mesh.surface_mesh();

  // Remove isolated vertices
  CGAL::Polygon_mesh_processing::remove_isolated_vertices(surface_mesh);

  // Remove degenerate faces.
  // Note: PMP::remove_degenerate_faces only accepts triangle meshes, whereas this
  // project's meshes may be quads/polygons (planar patches are no longer split into
  // triangles), so for polygon meshes duplicate vertices are checked manually.
  if (CGAL::is_triangle_mesh(surface_mesh)) {
    CGAL::Polygon_mesh_processing::remove_degenerate_faces(surface_mesh);
  } else {
    std::vector<SurfaceMeshType::Face_index> degenerated_faces;
    for (SurfaceMeshType::Face_index face : surface_mesh.faces()) {
      std::vector<SurfaceMeshType::Vertex_index> face_vertices;
      for (SurfaceMeshType::Vertex_index vertex :
           vertices_around_face(surface_mesh.halfedge(face), surface_mesh)) {
        face_vertices.push_back(vertex);
      }

      bool is_degenerated = face_vertices.size() < 3;
      for (std::size_t i = 0; !is_degenerated && i < face_vertices.size();
           ++i) {
        for (std::size_t j = i + 1; j < face_vertices.size(); ++j) {
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

  // Make sure the mesh is manifold
  if (!CGAL::is_valid_polygon_mesh(surface_mesh)) {
    std::cerr << "Warning: Mesh is not valid after cleanup" << std::endl;
  }
}
