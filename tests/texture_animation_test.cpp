#include <boost/json.hpp>
#include <chrono>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <stdexcept>

#include "File/Utf8Path.h"
#include "Minecraft/BbmodelExporter.h"
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
  } catch (const std::exception& error) {
    std::fprintf(stderr, "FAIL: %s\n", error.what());
    result = 1;
  }
  if (root.parent_path() == parent &&
      root.filename().string().rfind("lt-animation-", 0) == 0)
    fs::remove_all(root);
  return result;
}
