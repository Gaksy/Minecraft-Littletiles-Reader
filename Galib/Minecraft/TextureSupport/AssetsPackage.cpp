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
 * Date Created: 09/13/2026
 */

#include "Minecraft/TextureSupport/AssetsPackage.h"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <unordered_set>

#include <boost/json.hpp>

namespace galib::minecraft::texture_support {

namespace {

// Maximum number of missing textures listed in the lint report, to avoid flooding the output
constexpr std::size_t kMaxMissingTextureExamples = 5;

// Default grass/foliage colours of the 1.12 plains biome.
//
// This is the fallback used when the package has no tint.tsv, not the only answer:
// LittleTiles saves do not record the biome of each tile, so a fixed default colour
// is the only option (the reference Java mod also uses a fixed coordinate to look up
// the default biome colour). To change the palette, ship a tint.tsv in the package.
constexpr std::uint32_t kDefaultGrassColor = 0xFF91BD59u;
constexpr std::uint32_t kDefaultFoliageColor = 0xFF79C05Au;

// Wildcard key: matches any block
constexpr const char* kWildcardKey = "*";

std::string BlockNameOf(const std::string& kBlockId) {
  const std::size_t colon = kBlockId.find(':');
  const std::string without_namespace =
      colon == std::string::npos ? kBlockId : kBlockId.substr(colon + 1);
  const std::size_t second = without_namespace.find(':');
  return second == std::string::npos ? without_namespace
                                     : without_namespace.substr(0, second);
}

// Block name -> which default colour to use. Returning false means this tintindex
// is not tinted.
bool DefaultTintForBlock(const std::string& kBlockId, std::uint32_t* p_desc_argb) {
  const std::string name = BlockNameOf(kBlockId);
  if (name == "leaves" || name == "leaves2" || name == "vine" ||
      name == "vine_1") {
    if (p_desc_argb) {
      *p_desc_argb = kDefaultFoliageColor;
    }
    return true;
  }
  // Everything else (grass, crop stems, redstone, ...) keeps the old behaviour:
  // treated as grass colour. This is a known approximation; for exact
  // differentiation, ship a tint.tsv in the package (see docs/assets-package.md
  // section 6, pending).
  if (p_desc_argb) {
    *p_desc_argb = kDefaultGrassColor;
  }
  return true;
}

bool ReadWholeFile(const std::string& kPath, std::string* p_desc_text) {
  std::ifstream input(kPath, std::ios::binary);
  if (!input) {
    return false;
  }
  std::ostringstream buffer;
  buffer << input.rdbuf();
  if (p_desc_text) {
    *p_desc_text = buffer.str();
  }
  return true;
}

bool ParseArgb(const std::string& kToken, std::uint32_t* p_desc_argb) {
  if (kToken.empty()) {
    return false;
  }
  char* end = nullptr;
  const unsigned long value = std::strtoul(kToken.c_str(), &end, 16);
  if (end == kToken.c_str()) {
    return false;
  }
  if (p_desc_argb) {
    *p_desc_argb = static_cast<std::uint32_t>(value);
  }
  return true;
}

}  // namespace

std::optional<AssetsPackage> AssetsPackage::Open(const std::string& kDir,
                                                 std::string* const p_desc_error) {
  const auto fail = [p_desc_error](const std::string& kReason) {
    if (p_desc_error) {
      *p_desc_error = kReason;
    }
    return std::nullopt;
  };

  if (kDir.empty()) {
    return fail("assets directory is empty");
  }

  const std::filesystem::path dir(kDir);
  std::error_code error;
  if (!std::filesystem::is_directory(dir, error)) {
    return fail("assets directory not found: " + kDir);
  }

  AssetsPackage package;
  package.dir_ = kDir;
  package.info_.dir = kDir;

  // manifest.json is optional; only format_version is validated
  const std::filesystem::path manifest_path = dir / "manifest.json";
  if (std::filesystem::is_regular_file(manifest_path, error)) {
    std::string text;
    if (!ReadWholeFile(manifest_path.string(), &text)) {
      return fail("cannot read manifest: " + manifest_path.string());
    }
    package.info_.has_manifest = true;
    // A broken manifest is reported instead of being silently treated as version 1:
    // the whole point of the field is to fail loudly on a package we do not understand.
    boost::system::error_code parse_error;
    const boost::json::value document = boost::json::parse(text, parse_error);
    if (parse_error) {
      return fail("manifest is not valid JSON (" + parse_error.message() + "): " +
                  manifest_path.string());
    }
    const boost::json::object* const object = document.if_object();
    if (object == nullptr) {
      return fail("manifest must contain a JSON object: " +
                  manifest_path.string());
    }
    int version = 1;
    if (const auto found = object->find("format_version");
        found != object->end()) {
      if (found->value().is_int64()) {
        version = static_cast<int>(found->value().as_int64());
      } else if (found->value().is_uint64()) {
        version = static_cast<int>(found->value().as_uint64());
      } else {
        return fail("manifest: format_version must be an integer: " +
                    manifest_path.string());
      }
    }
    package.info_.format_version = version;
    if (version > kSupportedFormatVersion) {
      return fail("unsupported manifest format_version " +
                  std::to_string(version) + " (this build supports <= " +
                  std::to_string(kSupportedFormatVersion) + "): " +
                  manifest_path.string());
    }
  }

  // The mapping table is required
  const std::filesystem::path table_path = dir / "block_textures.tsv";
  if (!std::filesystem::is_regular_file(table_path, error)) {
    return fail("missing block_textures.tsv in " + kDir);
  }
  if (!package.table_.LoadFromTsv(table_path.string())) {
    return fail("cannot read block_textures.tsv (empty or unreadable): " +
                table_path.string());
  }

  // The tint table is optional
  const std::filesystem::path tint_path = dir / "tint.tsv";
  if (std::filesystem::is_regular_file(tint_path, error)) {
    package.LoadTintTable(tint_path.string());
  }

  // lint: count the unique referenced textures and check whether they exist on disk
  std::unordered_set<std::string> seen;
  for (const auto& entry : package.table_.entries()) {
    for (const std::string& path : entry.second.paths) {
      if (path.empty() || path == "-") {
        continue;
      }
      if (!seen.insert(path).second) {
        continue;
      }
      if (!package.HasTexture(path)) {
        ++package.info_.missing_texture_count;
        if (package.info_.missing_texture_examples.size() <
            kMaxMissingTextureExamples) {
          package.info_.missing_texture_examples.push_back(path);
        }
      }
    }
  }
  package.info_.texture_ref_count = seen.size();
  package.info_.block_count = package.table_.size();
  package.info_.tint_rule_count = package.tint_rules_.size();
  package.valid_ = true;
  return package;
}

bool AssetsPackage::LoadTintTable(const std::string& kPath) {
  std::ifstream input(kPath);
  if (!input) {
    return false;
  }
  std::string line;
  while (std::getline(input, line)) {
    if (line.empty() || line[0] == '#') {
      continue;
    }
    // Separated by tabs or whitespace: <key> <tintindex> <argb>
    std::istringstream stream(line);
    std::string key;
    std::string tint_token;
    std::string argb_token;
    if (!(stream >> key >> tint_token >> argb_token)) {
      continue;
    }
    const int tint_index = std::atoi(tint_token.c_str());
    std::uint32_t argb = 0;
    if (!ParseArgb(argb_token, &argb)) {
      continue;
    }
    tint_rules_[{key, tint_index}] = argb;
  }
  return true;
}

bool AssetsPackage::Lookup(const std::string& kBlockId,
                           BlockFaceTextures* const p_desc_out) const {
  return table_.Lookup(kBlockId, p_desc_out);
}

std::string AssetsPackage::ResolveTexture(const std::string& kRel) const {
  return (std::filesystem::path(dir_) / "textures" / (kRel + ".png")).string();
}

bool AssetsPackage::HasTexture(const std::string& kRel) const {
  std::error_code error;
  return std::filesystem::is_regular_file(ResolveTexture(kRel), error);
}

bool AssetsPackage::TintOverride(const std::string& kBlockId,
                                 const int kTintIndex,
                                 std::uint32_t* const p_desc_argb) const {
  if (kTintIndex < 0) {
    return false;
  }
  // First the exact block, then with the meta stripped, and finally the wildcard
  const std::string without_meta = StripBlockMeta(kBlockId);
  for (const std::string& key : {kBlockId, without_meta, std::string(kWildcardKey)}) {
    const auto found = tint_rules_.find({key, kTintIndex});
    if (found != tint_rules_.end()) {
      if (p_desc_argb) {
        *p_desc_argb = found->second;
      }
      return true;
    }
  }
  return DefaultTintForBlock(kBlockId, p_desc_argb);
}

}  // namespace galib::minecraft::texture_support
