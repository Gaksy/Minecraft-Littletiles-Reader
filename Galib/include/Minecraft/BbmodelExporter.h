// Copyright (c) 2026 Gaksy (Fuhongren)
// SPDX-License-Identifier: LGPL-3.0-only
#ifndef GALIB_MINECRAFT_BBMODELEXPORTER_H
#define GALIB_MINECRAFT_BBMODELEXPORTER_H

#include <array>
#include <boost/json.hpp>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "Minecraft/LtStructure.h"

namespace galib::minecraft::littletiles {

struct BbmodelReport {
  std::size_t source_boxes{0};
  std::size_t cubes{0};
  std::size_t textures{0};
  // Encoded, tinted first-frame PNGs in model texture order, for linked export.
  std::vector<std::vector<unsigned char>> texture_pngs;
};

// Geometry-only preflight; uses exactly the same restrictions as both exporters.
void ValidateJavaBbmodelGeometry(const LtStructure& structure);

struct BbmodelBlockProject {
  // Normalized grid coordinate, preserving game axes; each component is 0..2.
  std::array<std::int64_t, 3> block;
  std::array<std::int64_t, 3> source_block;
  boost::json::object model;
  BbmodelReport report;
};

// Clips standard cuboids into occupied source-grid cells. Each independent
// project uses [0,16] local coordinates and embedded textures, without scaling
// or recentering. Empty cells are omitted. Both BB modes require the entire
// source geometry to fit within at most 3x3x3 source-grid cells (27 projects).
std::vector<BbmodelBlockProject> BuildJavaBbmodelBlocks(
    const LtStructure& structure, const std::string& assets_root);

// Builds a Java Block/Item project with embedded PNGs. Rejects the entire export
// on offsets, invalid boxes, missing assets or Java model size violations.
// Geometry remains at 16 model units per block; large boxes are split at block
// boundaries to preserve texture repetition. Does not mutate the structure.
boost::json::object BuildJavaBbmodel(const LtStructure& structure,
                                     const std::string& assets_root,
                                     bool center,
                                     BbmodelReport* report = nullptr);

}  // namespace galib::minecraft::littletiles
#endif
