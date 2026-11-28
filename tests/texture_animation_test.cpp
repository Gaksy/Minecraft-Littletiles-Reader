#include <boost/json.hpp>
#include <chrono>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <stdexcept>

#include "File/Utf8Path.h"
#include "Minecraft/BbmodelExporter.h"
#include "Minecraft/CgalSupport/CgalLittletilesBuilder.h"
#include "Minecraft/SnbtParser.h"
#include "Minecraft/TextureSupport/AssetsPackage.h"
#include "Minecraft/TextureSupport/MaterialManager.h"
#include "Minecraft/TextureSupport/PngImage.h"

namespace fs = std::filesystem;
using namespace galib::minecraft::texture_support;
using namespace galib::minecraft::littletiles;

void Check(bool condition, const char* message) {
  if (!condition) throw std::runtime_error(message);
}

int main() {
  const auto parent = fs::temp_directory_path();
  const auto root =
      parent /
      ("lt-animation-" +
       std::to_string(
           std::chrono::steady_clock::now().time_since_epoch().count()));
  fs::create_directories(root / "textures");
  int result = 0;
  try {
    Check(LtStructure::FromSnbt(galib::minecraft::snbt::Parse(
        R"({tiles:1,tile0:{block:"minecraft:stone",bBoxminX:0b,bBoxminY:0b,bBoxminZ:0b,bBoxmaxX:16b,bBoxmaxY:16b,bBoxmaxZ:16b}})")).BoxCount() == 1,
        "early indexed recipe and prefixed box bytes");
    Check(LtStructure::FromSnbt(galib::minecraft::snbt::Parse(
        R"({tiles:1,names:"minecraft:stone", "0":"0.0.0.16.16.16.0.0.-2134097824"})")).BoxCount() == 1,
        "early compact export retains geometry");
    // Three pixels per 2-bit indexed row exercises padding and tRNS alpha.
    const unsigned char packed_png[] = {137,80,78,71,13,10,26,10,0,0,0,13,73,72,68,82,0,0,0,3,0,0,0,2,2,3,0,0,0,224,26,142,137,0,0,0,12,80,76,84,69,10,20,30,40,50,60,100,110,120,200,210,220,209,175,144,241,0,0,0,4,116,82,78,83,0,64,128,255,213,136,156,156,0,0,0,12,73,68,65,84,120,156,99,144,97,216,2,0,1,12,0,209,194,101,191,148,0,0,0,0,73,69,78,68,174,66,96,130};
    const auto packed_path = root / "packed.png";
    { std::ofstream out(packed_path, std::ios::binary); out.write(reinterpret_cast<const char*>(packed_png), sizeof(packed_png)); }
    PngImage packed_image;
    Check(packed_image.Load(galib::Utf8String(packed_path)) && packed_image.width() == 3 && packed_image.height() == 2 &&
              packed_image.Pixel(0,0)[3] == 0 && packed_image.Pixel(2,0)[0] == 200 &&
              packed_image.Pixel(0,1)[0] == 100 && packed_image.Pixel(0,1)[3] == 128 && packed_image.Pixel(2,1)[3] == 64,
          "packed palette row padding, colors and transparency");
    const auto path = galib::Utf8String(root / "textures/sprite.png");
    PngImage source(2, 6);
    for (int y = 0; y < 6; ++y)
      for (int x = 0; x < 2; ++x) {
        auto* p = source.Pixel(x, y);
        p[0] = static_cast<unsigned char>(30 + (y / 2) * 60);
        p[1] = static_cast<unsigned char>(40 + x);
        p[2] = 120;
        p[3] = 255;
      }
    Check(source.Save(path), "fixture PNG");
    PngImage image;
    Check(image.LoadTextureFrame(path) && image.width() == 2 &&
              image.height() == 6,
          "unannotated rectangular PNG must not be guessed as animation");
    auto metadata = [&](const char* text) {
      std::ofstream out(galib::Utf8Path(path + ".mcmeta"));
      out << text;
    };
    metadata(R"({"texture":{"blur":false}})");
    Check(image.LoadTextureFrame(path) && image.height() == 6,
          "non-animation metadata");
    metadata(R"({"animation":{"frametime":5}})");
    Check(image.LoadTextureFrame(path) && image.width() == 2 &&
              image.height() == 2,
          "default vertical animation frame size");
    Check(image.Pixel(0, 0)[0] == 30 && image.Pixel(1, 1)[1] == 41,
          "first frame pixels are copied, not rescaled");
    PngImage raw;
    Check(raw.Load(path) && raw.height() == 6,
          "raw PNG decoder remains unchanged");
    metadata(R"({"animation":{"frames":[2,0,1]}})");
    Check(image.LoadTextureFrame(path) && image.Pixel(0, 0)[0] == 150,
          "first scheduled frame can have a nonzero sprite index");
    metadata(R"({"animation":{"frames":[{"index":1,"time":3},0]}})");
    Check(image.LoadTextureFrame(path) && image.Pixel(0, 0)[0] == 90,
          "object-form scheduled frame");
    metadata(R"({"animation":{"width":2,"height":2,"frames":[]}})");
    Check(image.LoadTextureFrame(path) && image.Pixel(0, 0)[0] == 30,
          "empty schedule uses first grid frame");
    // Verify both writer paths: the uncoloured OBJ fast-copy path must not
    // bypass frame extraction; BB must embed the same static frame.
    std::ofstream(root / "block_textures.tsv")
        << "minecraft:stone\tsprite\tsprite\tsprite\tsprite\tsprite\tsprite\n";
    std::string error;
    auto package = AssetsPackage::Open(galib::Utf8String(root), &error);
    Check(package.has_value(), "fixture package");
    MaterialManager materials(*package);
    materials.Claim("sprite", 0xffffff, 0xffffffff);
    materials.Claim("sprite", 0xffffff, 0xff80ffff);
    Check(materials.WriteTextures(galib::Utf8String(root / "output"), &error) ==
              2,
          "both tinted and untinted variants written");
    for (std::size_t i = 0; i < 2; ++i) {
      Check(image.Load(galib::Utf8String(root / "output" /
                                         (materials.Name(i) + ".png"))) &&
                image.width() == 2 && image.height() == 2,
            "material output is one frame");
      Check(image.Pixel(0, 0)[0] == (i ? 15 : 30),
            "frame extraction precedes tile tint");
    }
    const auto structure = LtStructure::FromSnbt(galib::minecraft::snbt::Parse(
        R"({tiles:[{boxes:[[I;0,0,0,16,16,16]],tile:{block:"minecraft:stone"}}]})"));
    const auto model =
        BuildJavaBbmodel(structure, galib::Utf8String(root), false);
    Check(model.at("textures").as_array()[0].at("width") == 2 &&
              model.at("textures").as_array()[0].at("height") == 2,
          "BB embeds one frame");
    metadata(R"({"animation":{"width":3}})");
    Check(!image.LoadTextureFrame(path, &error) && !image.is_valid(),
          "invalid dimensions fail");
    metadata(R"({"animation":{"frames":[3]}})");
    Check(!image.LoadTextureFrame(path, &error),
          "out-of-range first frame fails");
    metadata(R"({"animation":42})");
    Check(!image.LoadTextureFrame(path, &error), "malformed animation fails");
    // A horizontal, row-major frame grid uses explicit dimensions.
    PngImage horizontal(4, 2);
    for (int y = 0; y < 2; ++y)
      for (int x = 0; x < 4; ++x) {
        auto* p = horizontal.Pixel(x, y);
        p[0] = x < 2 ? 11 : 77;
        p[1] = 0;
        p[2] = 0;
        p[3] = 255;
      }
    Check(horizontal.Save(path), "horizontal fixture");
    metadata(R"({"animation":{"width":2,"height":2,"frames":[1]}})");
    Check(image.LoadTextureFrame(path) && image.width() == 2 &&
              image.height() == 2 && image.Pixel(0, 0)[0] == 77,
          "horizontal frame grid selection");
    // Browser recipes must remain usable without source images and must never
    // call the PNG baker, even if a source file would fail to decode.
    std::ofstream(root / "manifest.json")
        << R"({"format_version":1,"minecraft_version":"1.12.2"})";
    fs::remove(galib::Utf8Path(path));
    using namespace galib::minecraft::cgal_support;
    ObjExportOptions options;
    options.assets_root = galib::Utf8String(root);
    options.material_recipe = true;
    options.quiet = true;
    ObjMeshBuilder builder(options);
    AddStructureToObjBuilder(structure, &builder);
    Check(builder.WriteToFile(galib::Utf8String(root / "recipe.obj")),
          "mapping-only OBJ recipe export without PNG");
    Check(builder.stats().textures_written == 0 &&
              !fs::exists(root / "recipe.mtl") &&
              !fs::exists(root / "recipe_textures"),
          "recipe does not produce MTL or texture products");
    std::ifstream recipe_file(root / "recipe.render.json");
    const std::string recipe_text((std::istreambuf_iterator<char>(recipe_file)), {});
    const auto recipe = boost::json::parse(recipe_text);
    Check(recipe.at("schema") == "lt-render-recipe/1" &&
              recipe.at("gameVersion") == "1.12.2" &&
              recipe.at("materials").as_object().size() == 1,
          "recipe version and material identity");
    ObjMeshBuilder missing(options);
    AddStructureToObjBuilder(LtStructure::FromSnbt(galib::minecraft::snbt::Parse(
        R"({tiles:[{boxes:[[I;0,0,0,16,16,16]],tile:{block:"minecraft:missing"}}]})")), &missing);
    Check(!missing.WriteToFile(galib::Utf8String(root / "missing.obj")),
          "recipe fails rather than silently exporting missing materials");
    ObjMeshBuilder invalid_meta(options);
    AddStructureToObjBuilder(LtStructure::FromSnbt(galib::minecraft::snbt::Parse(
        R"({tiles:[{boxes:[[I;0,0,0,16,16,16]],tile:{block:"minecraft:stone:999"}}]})")), &invalid_meta);
    Check(!invalid_meta.WriteToFile(galib::Utf8String(root / "invalid-meta.obj")),
          "recipe never strips an unknown legacy metadata value to a base block");
    for (const char* version : {"1.7.10","1.10.2","1.12","1.12.1","1.12.2","1.19.4","1.20","1.20.1","1.20.2","1.21.1"}) {
      { std::ofstream manifest(root / "manifest.json"); manifest << "{\"format_version\":1,\"minecraft_version\":\"" << version << "\"}"; }
      Check(AssetsPackage::Open(galib::Utf8String(root), &error, true).has_value(),
            "every explicit material recipe version can open mappings without PNGs");
    }
    std::ofstream(root / "manifest.json")
        << R"({"format_version":1,"minecraft_version":"1.21.2"})";
    Check(!AssetsPackage::Open(galib::Utf8String(root), &error, true),
          "mapping-only mode rejects unsupported exact game versions");
  } catch (const std::exception& error) {
    std::fprintf(stderr, "FAIL: %s\n", error.what());
    result = 1;
  }
  if (root.parent_path() == parent &&
      root.filename().string().rfind("lt-animation-", 0) == 0)
    fs::remove_all(root);
  return result;
}
