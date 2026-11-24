// Copyright (c) 2026 Gaksy (Fuhongren)
// SPDX-License-Identifier: LGPL-3.0-only
#include "Minecraft/BbmodelExporter.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <functional>
#include <limits>
#include <map>
#include <random>
#include <stdexcept>
#include <vector>

#include "Minecraft/TextureSupport/AssetsPackage.h"
#include "Minecraft/TextureSupport/MaterialManager.h"
#include "Minecraft/TextureSupport/PngImage.h"

namespace galib::minecraft::littletiles {
namespace {
namespace json = boost::json;
using namespace texture_support;
using Vec = std::array<double, 3>;

std::string Base64(const std::vector<unsigned char>& bytes) {
  constexpr char alphabet[] =
      "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
  std::string result;
  result.reserve((bytes.size() + 2) / 3 * 4);
  for (std::size_t i = 0; i < bytes.size(); i += 3) {
    const unsigned value =
        (unsigned(bytes[i]) << 16) |
        (i + 1 < bytes.size() ? unsigned(bytes[i + 1]) << 8 : 0) |
        (i + 2 < bytes.size() ? unsigned(bytes[i + 2]) : 0);
    result += alphabet[(value >> 18) & 63];
    result += alphabet[(value >> 12) & 63];
    result += i + 1 < bytes.size() ? alphabet[(value >> 6) & 63] : '=';
    result += i + 2 < bytes.size() ? alphabet[value & 63] : '=';
  }
  return result;
}

struct Box {
  Vec from, to;
  const LtStructure::Group* group;
  std::size_t index;
};

using Cell = std::array<std::int64_t, 3>;
struct BlockLayout {
  Cell first{}, last{};
};

std::vector<Box> SourceBoxes(const LtStructure& structure) {
  if (structure.grid() <= 0) throw std::runtime_error("invalid root grid");
  std::vector<Box> boxes;
  structure.VisitGroups([&](const LtStructure::Group& group) {
    if (group.grid <= 0) throw std::runtime_error("invalid structure grid");
    for (std::size_t i = 0; i < group.boxes.size(); ++i) {
      const auto& tile = group.boxes[i];
      if (tile.has_any_offset_enable())
        throw std::runtime_error(
            "Java Block export does not support corner-offset or slanted "
            "tiles: " +
            group.block_id);
      const auto& a = tile.pos_1();
      const auto& b = tile.pos_2();
      Box box{{a.x * 16.0 / group.grid, a.y * 16.0 / group.grid,
               a.z * 16.0 / group.grid},
              {b.x * 16.0 / group.grid, b.y * 16.0 / group.grid,
               b.z * 16.0 / group.grid},
              &group,
              i};
      for (int axis = 0; axis < 3; ++axis)
        if (!(box.to[axis] > box.from[axis]))
          throw std::runtime_error("non-positive box dimensions: " +
                                   group.block_id);
      boxes.push_back(box);
    }
  });
  if (boxes.empty())
    throw std::runtime_error("structure has no boxes to export");
  return boxes;
}

BlockLayout LayoutOf(const std::vector<Box>& boxes) {
  BlockLayout layout;
  layout.first.fill(std::numeric_limits<std::int64_t>::max());
  layout.last.fill(std::numeric_limits<std::int64_t>::min());
  for (const auto& box : boxes)
    for (int axis = 0; axis < 3; ++axis) {
      layout.first[axis] =
          std::min(layout.first[axis],
                   static_cast<std::int64_t>(std::floor(box.from[axis] / 16)));
      layout.last[axis] =
          std::max(layout.last[axis],
                   static_cast<std::int64_t>(std::ceil(box.to[axis] / 16)) - 1);
    }
  for (int axis = 0; axis < 3; ++axis)
    if (layout.last[axis] - layout.first[axis] + 1 > 3)
      throw std::runtime_error(
          "BB export supports at most 3x3x3 source-grid blocks; selected span "
          "is " +
          std::to_string(layout.last[0] - layout.first[0] + 1) + "x" +
          std::to_string(layout.last[1] - layout.first[1] + 1) + "x" +
          std::to_string(layout.last[2] - layout.first[2] + 1) +
          "; no scaling applied");
  return layout;
}

std::string Location(const LtStructure::Group& group, std::size_t index) {
  return group.block_id + " box " + std::to_string(index + 1);
}

json::array Array(const Vec& value) { return {value[0], value[1], value[2]}; }

// Java cube UV corner ordering, sampled from the same per-face basis as OBJ.
// A split box lies in one texture cell, including its upper boundary at 16.
json::array FaceUv(int face, const Vec& lo, const Vec& hi) {
  switch (static_cast<FaceDirection>(face)) {
    case FaceDirection::kDown:
      return {16 - lo[0], hi[2], 16 - hi[0], lo[2]};
    case FaceDirection::kUp:
      return {lo[0], lo[2], hi[0], hi[2]};
    case FaceDirection::kNorth:
      return {16 - hi[0], 16 - hi[1], 16 - lo[0], 16 - lo[1]};
    case FaceDirection::kSouth:
      return {lo[0], 16 - hi[1], hi[0], 16 - lo[1]};
    case FaceDirection::kWest:
      return {lo[2], 16 - hi[1], hi[2], 16 - lo[1]};
    case FaceDirection::kEast:
      return {16 - hi[2], 16 - hi[1], 16 - lo[2], 16 - lo[1]};
  }
  throw std::runtime_error("invalid face direction");
}
}  // namespace

namespace {
json::object PreparedFace(int face, const Vec& lo, const Vec& hi,
                          const std::array<double, 6>& matrix) {
  const auto rect = FaceUv(face, lo, hi);
  const double u0 = json::value_to<double>(rect[0]),
               v0 = json::value_to<double>(rect[1]);
  const double u1 = json::value_to<double>(rect[2]),
               v1 = json::value_to<double>(rect[3]);
  const std::array<std::array<double, 2>, 4> corners{
      {{{0, 0}}, {{1, 0}}, {{1, 1}}, {{0, 1}}}};
  std::array<std::array<double, 2>, 4> target{};
  double min_u = 1e30, min_v = 1e30, max_u = -1e30, max_v = -1e30;
  for (int i = 0; i < 4; ++i) {
    const double u = (u0 + corners[i][0] * (u1 - u0)) / 16;
    const double v = (v0 + corners[i][1] * (v1 - v0)) / 16;
    target[i] = {{16 * (matrix[0] * u + matrix[1] * v + matrix[2]),
                  16 * (matrix[3] * u + matrix[4] * v + matrix[5])}};
    min_u = std::min(min_u, target[i][0]);
    max_u = std::max(max_u, target[i][0]);
    min_v = std::min(min_v, target[i][1]);
    max_v = std::max(max_v, target[i][1]);
  }
  for (int rotation = 0; rotation < 4; ++rotation)
    for (int flip_u = 0; flip_u < 2; ++flip_u)
      for (int flip_v = 0; flip_v < 2; ++flip_v) {
        const double a = flip_u ? max_u : min_u, b = flip_v ? max_v : min_v;
        const double c = flip_u ? min_u : max_u, d = flip_v ? min_v : max_v;
        bool match = true;
        for (int i = 0; i < 4; ++i) {
          double s = corners[i][0], t = corners[i][1];
          for (int r = 0; r < rotation; ++r) {
            const double old = s;
            s = t;
            t = 1 - old;
          }
          if (std::abs(a + s * (c - a) - target[i][0]) > 1e-7 ||
              std::abs(b + t * (d - b) - target[i][1]) > 1e-7)
            match = false;
        }
        if (match) {
          json::object result{{"uv", json::array{a, b, c, d}}};
          if (rotation) result["rotation"] = rotation * 90;
          return result;
        }
      }
  throw std::runtime_error(
      "prepared UV transform cannot be represented by a Java cube face");
}
}  // namespace

namespace {
boost::json::object BuildProject(const LtStructure& structure,
                                 const std::string& assets_root,
                                 const std::vector<Box>& boxes,
                                 const Vec& shift, BbmodelReport* report) {
  std::string error;
  auto package = AssetsPackage::Open(assets_root, &error);
  if (!package)
    throw std::runtime_error("BB export requires an assets package: " + error);
  MaterialManager materials(*package);
  std::mt19937 rng(std::random_device{}());
  auto uuid = [&]() {
    char text[37];
    const unsigned a = rng(), b = rng(), c = rng(), d = rng();
    std::snprintf(text, sizeof(text), "%08x-%04x-%04x-%04x-%04x%08x", a,
                  b >> 16, (b & 0x0fff) | 0x4000, ((c >> 16) & 0x3fff) | 0x8000,
                  c & 0xffff, d);
    return std::string(text);
  };
  json::object model{
      {"meta", json::object{{"format_version", "5.0"},
                            {"model_format", "java_block"},
                            {"box_uv", false}}},
      {"name", structure.name()},
      {"resolution", json::object{{"width", 16}, {"height", 16}}},
      {"elements", json::array{}},
      {"groups", json::array{}},
      {"outliner", json::array{}},
      {"textures", json::array{}}};
  auto& elements = model["elements"].as_array();
  auto& groups = model["groups"].as_array();
  auto& textures = model["textures"].as_array();
  std::map<const LtStructure::Group*, json::array> group_cubes;
  for (const Box& box : boxes) {
    BlockFaceTextures face_textures;
    if (!package->Lookup(box.group->block_id, &face_textures))
      throw std::runtime_error("missing material mapping: " +
                               box.group->block_id);
    std::array<std::size_t, 6> texture_indices{};
    for (int face = 0; face < 6; ++face) {
      const auto direction = static_cast<FaceDirection>(face);
      const auto& path = face_textures.Path(direction);
      if (path.empty() || !package->HasTexture(path))
        throw std::runtime_error(
            "missing face texture: " + box.group->block_id + " / " +
            FaceDirectionName(direction));
      std::uint32_t tint = 0xffffffff;
      package->TintOverride(box.group->block_id, face_textures.Tint(direction),
                            &tint);
      texture_indices[face] = materials.Claim(
          path, tint & 0xffffff,
          box.group->has_color ? static_cast<std::uint32_t>(box.group->color)
                               : 0xffffffff);
    }
    // Split at unshifted block boundaries: centring must never move the texture.
    std::array<std::vector<double>, 3> cuts;
    for (int axis = 0; axis < 3; ++axis) {
      cuts[axis].push_back(box.from[axis]);
      for (double p = (std::floor(box.from[axis] / 16) + 1) * 16;
           p < box.to[axis] - 1e-8; p += 16)
        cuts[axis].push_back(p);
      cuts[axis].push_back(box.to[axis]);
    }
    for (std::size_t x = 1; x < cuts[0].size(); ++x)
      for (std::size_t y = 1; y < cuts[1].size(); ++y)
        for (std::size_t z = 1; z < cuts[2].size(); ++z) {
          const Vec from{cuts[0][x - 1], cuts[1][y - 1], cuts[2][z - 1]};
          const Vec to{cuts[0][x], cuts[1][y], cuts[2][z]};
          Vec lo{}, hi{}, positioned_from{}, positioned_to{};
          for (int axis = 0; axis < 3; ++axis) {
            const double cell = std::floor(from[axis] / 16) * 16;
            lo[axis] = from[axis] - cell;
            hi[axis] = to[axis] - cell;
            positioned_from[axis] =
                std::clamp(from[axis] + shift[axis], -16.0, 32.0);
            positioned_to[axis] =
                std::clamp(to[axis] + shift[axis], -16.0, 32.0);
          }
          const std::string id = uuid();
          json::object faces;
          for (int face = 0; face < 6; ++face) {
            auto prepared =
                PreparedFace(face, lo, hi, face_textures.uv_transforms[face]);
            prepared["texture"] = texture_indices[face];
            faces[FaceDirectionName(static_cast<FaceDirection>(face))] =
                std::move(prepared);
          }
          elements.push_back(
              json::object{{"type", "cube"},
                           {"uuid", id},
                           {"name", Location(*box.group, box.index)},
                           {"from", Array(positioned_from)},
                           {"to", Array(positioned_to)},
                           {"origin", json::array{8, 8, 8}},
                           {"box_uv", false},
                           {"autouv", 0},
                           {"faces", std::move(faces)}});
          group_cubes[box.group].emplace_back(id);
        }
  }
  std::function<json::object(const LtStructure::Node&, const std::string&)>
      hierarchy;
  hierarchy = [&](const LtStructure::Node& node, const std::string& name) {
    const std::string id = uuid();
    json::array children;
    for (const auto& group : node.groups)
      for (const auto& child : group_cubes[&group]) children.push_back(child);
    for (std::size_t i = 0; i < node.children.size(); ++i)
      children.push_back(
          hierarchy(node.children[i], "structure " + std::to_string(i + 1)));
    groups.push_back(json::object{
        {"uuid", id}, {"name", name}, {"origin", json::array{0, 0, 0}}});
    return json::object{
        {"uuid", id}, {"isOpen", true}, {"children", std::move(children)}};
  };
  model["outliner"].as_array().push_back(
      hierarchy(structure.root(),
                structure.name().empty() ? "LittleTiles" : structure.name()));
  // One decode per source; differently tinted variants remain separate textures.
  std::map<std::string, PngImage> decoded;
  std::vector<std::vector<unsigned char>> texture_pngs;
  for (std::size_t i = 0; i < materials.size(); ++i) {
    const auto& key = materials.Key(i);
    auto found = decoded.find(key.texture);
    if (found == decoded.end()) {
      PngImage image;
      if (!image.LoadTextureFrame(package->ResolveTexture(key.texture), &error))
        throw std::runtime_error("cannot decode texture " + key.texture + ": " +
                                 error);
      if (image.width() != image.height())
        throw std::runtime_error("BB v1 requires square static textures: " +
                                 key.texture);
      found = decoded.emplace(key.texture, std::move(image)).first;
    }
    PngImage baked = found->second;
    ApplyTintAndTileColor(&baked, key.tint_rgb, key.tile_color);
    std::vector<unsigned char> png;
    if (!baked.Encode(&png, &error)) throw std::runtime_error(error);
    textures.push_back(
        json::object{{"uuid", uuid()},
                     {"id", std::to_string(i)},
                     {"name", materials.Name(i) + ".png"},
                     {"internal", true},
                     {"width", baked.width()},
                     {"height", baked.height()},
                     {"uv_width", 16},
                     {"uv_height", 16},
                     {"source", "data:image/png;base64," + Base64(png)}});
    texture_pngs.push_back(std::move(png));
  }
  if (report) *report = {boxes.size(), elements.size(), textures.size(), std::move(texture_pngs)};
  return model;
}
}  // namespace

void ValidateJavaBbmodelGeometry(const LtStructure& structure) {
  LayoutOf(SourceBoxes(structure));
}

boost::json::object BuildJavaBbmodel(const LtStructure& structure,
                                     const std::string& assets_root,
                                     bool center, BbmodelReport* report) {
  if (report) *report = {};
  const auto boxes = SourceBoxes(structure);
  const auto layout = LayoutOf(boxes);
  Vec shift{};
  for (int axis = 0; axis < 3; ++axis) {
    // Translate by whole grid cells first. When centering, center the occupied
    // cell grid, preserving offsets of partial cubes within those cells.
    const auto span = layout.last[axis] - layout.first[axis] + 1;
    shift[axis] = -layout.first[axis] * 16.0;
    if (center)
      shift[axis] += 8 - span * 8.0;
    else if (span == 3)
      shift[axis] -= 16;
  }
  auto model = BuildProject(structure, assets_root, boxes, shift, report);
  model["reader_block_grid"] = boost::json::object{
      {"source_origin",
       boost::json::array{layout.first[0], layout.first[1], layout.first[2]}},
      {"size", boost::json::array{layout.last[0] - layout.first[0] + 1,
                                  layout.last[1] - layout.first[1] + 1,
                                  layout.last[2] - layout.first[2] + 1}},
      {"translation_model_units", Array(shift)}};
  return model;
}

std::vector<BbmodelBlockProject> BuildJavaBbmodelBlocks(
    const LtStructure& structure, const std::string& assets_root) {
  const auto source_boxes = SourceBoxes(structure);
  const auto layout = LayoutOf(source_boxes);
  std::map<Cell, std::vector<Box>> cells;
  std::size_t fragments = 0;
  for (const auto& box : source_boxes) {
    Cell first{}, last{};
    for (int axis = 0; axis < 3; ++axis) {
      first[axis] = static_cast<std::int64_t>(std::floor(box.from[axis] / 16));
      last[axis] = static_cast<std::int64_t>(std::ceil(box.to[axis] / 16)) - 1;
    }
    for (auto x = first[0]; x <= last[0]; ++x)
      for (auto y = first[1]; y <= last[1]; ++y)
        for (auto z = first[2]; z <= last[2]; ++z) {
          const Cell cell{x, y, z};
          Box clipped = box;
          for (int axis = 0; axis < 3; ++axis) {
            clipped.from[axis] = std::max(box.from[axis], cell[axis] * 16.0);
            clipped.to[axis] = std::min(box.to[axis], (cell[axis] + 1) * 16.0);
          }
          cells[cell].push_back(clipped);
          if (++fragments > 1000000)
            throw std::runtime_error(
                "BB batch export exceeds 1000000 box fragments");
        }
  }
  std::vector<BbmodelBlockProject> projects;
  projects.reserve(cells.size());
  for (const auto& [cell, boxes] : cells) {
    const Cell local{cell[0] - layout.first[0], cell[1] - layout.first[1],
                     cell[2] - layout.first[2]};
    const Vec shift{-cell[0] * 16.0, -cell[1] * 16.0, -cell[2] * 16.0};
    BbmodelReport report;
    auto model = BuildProject(structure, assets_root, boxes, shift, &report);
    model["reader_block_grid"] = boost::json::object{
        {"block", boost::json::array{local[0], local[1], local[2]}},
        {"size", boost::json::array{layout.last[0] - layout.first[0] + 1,
                                    layout.last[1] - layout.first[1] + 1,
                                    layout.last[2] - layout.first[2] + 1}}};
    projects.push_back({local, cell, std::move(model), report});
  }
  return projects;
}
}  // namespace galib::minecraft::littletiles
