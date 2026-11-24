#include <boost/json.hpp>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>

#include "File/Utf8Path.h"
#include "Minecraft/BbmodelExporter.h"
#include "Minecraft/SnbtParser.h"
#include "Minecraft/TextureSupport/PngImage.h"

namespace json = boost::json;
using galib::minecraft::littletiles::BbmodelReport;
using galib::minecraft::littletiles::BuildJavaBbmodel;
using galib::minecraft::littletiles::LtStructure;
using galib::minecraft::texture_support::PngImage;
int failures = 0;
void Check(bool value, const char* name) {
  if (!value) {
    ++failures;
    std::fprintf(stderr, "FAIL: %s\n", name);
  }
}
bool Numbers(const json::value& actual,
             std::initializer_list<double> expected) {
  const auto& values = actual.as_array();
  if (values.size() != expected.size()) return false;
  std::size_t i = 0;
  for (double value : expected)
    if (json::value_to<double>(values[i++]) != value) return false;
  return true;
}
LtStructure Read(const std::string& text) {
  return LtStructure::FromSnbt(galib::minecraft::snbt::Parse(text));
}
template <class F>
void Reject(F f, const char* message) {
  try {
    f();
    Check(false, message);
  } catch (const std::runtime_error& e) {
    Check(std::string(e.what()).find(message) != std::string::npos, e.what());
  }
}

int main() {
  const auto root =
      std::filesystem::temp_directory_path() /
      ("lt-bb-test-" +
       std::to_string(
           std::chrono::steady_clock::now().time_since_epoch().count()));
  std::filesystem::create_directories(root / "textures");
  try {
    PngImage image(2, 2);
    for (int y = 0; y < 2; ++y)
      for (int x = 0; x < 2; ++x) {
        auto* p = image.Pixel(x, y);
        p[0] = 200;
        p[1] = 100;
        p[2] = 50;
        p[3] = 255;
      }
    Check(image.Save(galib::Utf8String(root / "textures/stone.png")),
          "fixture PNG");
    std::ofstream table(root / "block_textures.tsv");
    table << "minecraft:stone\tstone\tstone\tstone\tstone\tstone\tstone\n";
    table.close();
    const auto assets = galib::Utf8String(root);
    const std::string plain =
        "{tiles:[{boxes:[[I;0,0,0,16,16,16]],tile:{block:\"minecraft:stone\"}}]"
        ",grid:16}";
    BbmodelReport report;
    const auto model = BuildJavaBbmodel(Read(plain), assets, true, &report);
    Check(model.at("meta").at("model_format") == "java_block",
          "Java Block workspace");
    Check(report.cubes == 1 && report.source_boxes == 1 && report.textures == 1,
          "export counts and texture dedup");
    const auto& cube = model.at("elements").as_array().at(0);
    Check(Numbers(cube.at("from"), {0, 0, 0}) &&
              Numbers(cube.at("to"), {16, 16, 16}),
          "16 model units per block");
    Check(cube.at("type") == "cube" && cube.at("faces").as_object().size() == 6,
          "editable six-face cube");
    Check(Numbers(cube.at("faces").at("up").at("uv"), {0, 0, 16, 16}),
          "whole-block UV does not collapse to zero");
    Check(Numbers(cube.at("faces").at("down").at("uv"), {16, 16, 0, 0}),
          "bottom UV basis matches Reader");
    Check(model.at("textures")
              .as_array()
              .at(0)
              .at("source")
              .as_string()
              .starts_with("data:image/png;base64,"),
          "PNG embedded");
    Check(!model.at("textures").as_array().at(0).as_object().contains("path"),
          "no machine-specific texture path");
    const auto& hierarchy = model.at("outliner").as_array().at(0);
    Check(hierarchy.at("children").as_array().at(0) == cube.at("uuid"),
          "outliner points to cube");
    Check(
        hierarchy.at("uuid") == model.at("groups").as_array().at(0).at("uuid"),
        "5.0 separate groups");
    const auto partial =
        BuildJavaBbmodel(Read("{tiles:[{boxes:[[I;2,3,1,10,11,7]],tile:{block:"
                              "\"minecraft:stone\"}}],grid:16}"),
                         assets, false);
    const auto& faces = partial.at("elements").as_array()[0].at("faces");
    Check(Numbers(faces.at("up").at("uv"), {2, 1, 10, 7}), "partial up UV");
    Check(Numbers(faces.at("down").at("uv"), {14, 7, 6, 1}), "partial down UV");
    Check(Numbers(faces.at("north").at("uv"), {6, 5, 14, 13}),
          "partial north UV");
    Check(Numbers(faces.at("south").at("uv"), {2, 5, 10, 13}),
          "partial south UV");
    Check(Numbers(faces.at("west").at("uv"), {1, 5, 7, 13}), "partial west UV");
    Check(Numbers(faces.at("east").at("uv"), {9, 5, 15, 13}),
          "partial east UV");
    const auto modern = BuildJavaBbmodel(
        Read("{t:{\"minecraft:stone\":[[I;0,0,0,16,16,16]]},grid:16}"), assets,
        false);
    Check(modern.at("elements").as_array().size() == 1, "modern dialect");
    const auto split =
        BuildJavaBbmodel(Read("{tiles:[{boxes:[[I;-8,0,0,24,8,8]],tile:{block:"
                              "\"minecraft:stone\"}}],grid:16}"),
                         assets, false);
    Check(split.at("elements").as_array().size() == 3,
          "split at negative and positive block boundaries");
    Check(Numbers(
              split.at("elements").as_array()[0].at("faces").at("up").at("uv"),
              {8, 0, 16, 8}),
          "negative-cell UV");
    Check(Numbers(
              split.at("elements").as_array()[1].at("faces").at("up").at("uv"),
              {0, 0, 16, 8}),
          "cross-cell repeat UV");
    const auto child =
        BuildJavaBbmodel(Read("{tiles:[{boxes:[[I;16,0,0,24,8,8]],tile:{block:"
                              "\"minecraft:stone\"}}],grid:16,min:[I;16,0,0],"
                              "children:[{grid:32,tiles:[{boxes:[[I;32,0,0,48,"
                              "16,16]],tile:{block:\"minecraft:stone\"}}]}]}"),
                         assets, false);
    Check(child.at("elements").as_array()[0].at("from") ==
              child.at("elements").as_array()[1].at("from"),
          "child origin scales using root grid");
    Check(child.at("elements").as_array()[0].at("to") ==
              child.at("elements").as_array()[1].at("to"),
          "child own grid applied");
    Check(child.at("groups").as_array().size() == 2, "child grouping retained");
    const auto tint = BuildJavaBbmodel(
        Read("{tiles:[{boxes:[[I;0,0,0,8,8,8]],tile:{block:\"minecraft:stone\"}"
             "},{boxes:[[I;8,0,0,16,8,8]],tile:{block:\"minecraft:stone\","
             "color:-65536}}],grid:16}"),
        assets, false);
    Check(tint.at("textures").as_array().size() == 2,
          "colored and uncolored textures stay separate");
    Check(tint.at("textures").as_array()[0].at("source") !=
              tint.at("textures").as_array()[1].at("source"),
          "color baked into embedded PNG");
    Reject(
        [&] {
          BuildJavaBbmodel(Read("{tiles:[{boxes:[[I;0,0,0,64,16,16]],tile:{"
                                "block:\"minecraft:stone\"}}],grid:16}"),
                           assets, true);
        },
        "3x3x3");
    const auto translated =
        BuildJavaBbmodel(Read("{tiles:[{boxes:[[I;48,0,0,64,16,16]],tile:{"
                              "block:\"minecraft:stone\"}}],grid:16}"),
                         assets, false);
    Check(
        Numbers(translated.at("elements").as_array()[0].at("from"), {0, 0, 0}),
        "source-grid origin normalized without scaling");
    Check(BuildJavaBbmodel(Read("{tiles:[{boxes:[[I;48,0,0,64,16,16]],tile:{"
                                "block:\"minecraft:stone\"}}],grid:16}"),
                           assets, true)
                  .at("elements")
                  .as_array()
                  .size() == 1,
          "centering without scaling");
    Reject(
        [&] {
          BuildJavaBbmodel(
              LtStructure::FromSnbtFile(TEST_DATA_DIR "/legacy_sample.txt"),
              assets, true);
        },
        "corner-offset");
    Reject(
        [&] {
          BuildJavaBbmodel(
              LtStructure::FromSnbtFile(TEST_DATA_DIR "/modern_sample.txt"),
              assets, true);
        },
        "corner-offset");
    Reject(
        [&] {
          BuildJavaBbmodel(Read("{tiles:[{boxes:[[I;0,0,0,16,16,16]],tile:{"
                                "block:\"missing:block\"}}],grid:16}"),
                           assets, false);
        },
        "missing material");
    Reject(
        [&] {
          BuildJavaBbmodel(Read("{tiles:[{boxes:[[I;0,0,0,0,16,16]],tile:{"
                                "block:\"minecraft:stone\"}}],grid:16}"),
                           assets, false);
        },
        "non-positive");
    // Deliver a self-contained fixture for optional manual BB open/save checks.
    if (const char* path = std::getenv("LTR_BBMODEL_TEST_OUTPUT")) {
      std::ofstream out(galib::Utf8Path(path));
      out << json::serialize(tint);
    }
  } catch (const std::exception& e) {
    ++failures;
    std::fprintf(stderr, "unexpected: %s\n", e.what());
  }
  std::filesystem::remove_all(root);
  std::printf("BB exporter: %d failure(s)\n", failures);
  return failures ? 1 : 0;
}
