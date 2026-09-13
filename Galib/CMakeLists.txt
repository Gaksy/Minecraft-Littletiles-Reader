# Galib core library: parsing + geometry building, with no CLI / file output
# entry point. It is currently a static library; switch to SHARED here if a web
# service later needs to consume it as a dynamic library.
add_library(galib STATIC
  Minecraft/Anvil.cpp
  Minecraft/BlockIdTable.cpp
  Minecraft/ChunkBlocks.cpp
  Minecraft/MinecraftCoord.cpp
  Minecraft/ChunkTileEntities.cpp
  Minecraft/BlockTileEntities.cpp
  Minecraft/LittleTilesEntity.cpp
  Minecraft/LittleTiles.cpp
  Minecraft/SaveFolder.cpp
  Minecraft/SnbtParser.cpp
  Minecraft/LtStructure.cpp
  Minecraft/CgalSupport/CgalTypeDef.cpp
  Minecraft/CgalSupport/CgalLtSupport.cpp
  Minecraft/CgalSupport/CgalLittletilesBuilder.cpp
  Minecraft/CgalSupport/CgalWorldBlocks.cpp
  Minecraft/TextureSupport/AssetsPackage.cpp
  Minecraft/TextureSupport/BlockTextureTable.cpp
  Minecraft/TextureSupport/MaterialManager.cpp
  Minecraft/TextureSupport/PngImage.cpp
  File/FileState.cpp
    File/PathFormat.cpp
    File/Utf8Path.cpp
    Log/GalibLog.cpp
    Log/GalibText.cpp
    Exception/MinecraftException.cpp
    Exception/LittleTilesException.cpp
    Version.cpp
)

target_include_directories(galib PUBLIC include)

# Let a build override the release channel without editing Version.h
# (see the root CMakeLists for the cache variable).
if(GALIB_VERSION_CHANNEL)
  target_compile_definitions(galib PUBLIC GALIB_VERSION_CHANNEL="${GALIB_VERSION_CHANNEL}")
endif()

# Dependencies are exposed as PUBLIC, so consumers (the CLI / a future web
# service) only need to link against galib
target_link_libraries(galib PUBLIC Boost::iostreams Boost::json ZLIB::ZLIB nbt++)

if(TARGET CGAL::CGAL)
    target_link_libraries(galib PUBLIC CGAL::CGAL)
else()
    target_link_libraries(galib PUBLIC ${CGAL_LIBRARIES})
endif()
