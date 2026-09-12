# Galib 核心库：解析 + 几何构建，不包含任何 CLI / 文件输出入口。
# 目前使用静态库，后续若要给 Web 服务以动态库形式调用，可在此切换为 SHARED。
add_library(galib STATIC
  Minecraft/Anvil.cpp
  Minecraft/BlockIdTable.cpp
  Minecraft/ChunkBlocks.cpp
  Minecraft/MinecraftCoord.cpp
    Minecraft/ChunkTileEntities.cpp
    Minecraft/BlockTileEntities.cpp
    Minecraft/LittleTilesEntity.cpp
    Minecraft/LittleTiles.cpp
  Minecraft/CgalSupport/CgalTypeDef.cpp
  Minecraft/CgalSupport/CgalLtSupport.cpp
  Minecraft/CgalSupport/CgalLittletilesBuilder.cpp
  Minecraft/CgalSupport/CgalWorldBlocks.cpp
  Minecraft/TextureSupport/BlockTextureTable.cpp
  Minecraft/TextureSupport/PngImage.cpp
  Minecraft/TextureSupport/TextureBaker.cpp
  File/FileState.cpp
    File/PathFormat.cpp
    Log/GalibLog.cpp
    Log/GalibText.cpp
    Exception/MinecraftException.cpp
    Exception/LittleTilesException.cpp
)

target_include_directories(galib PUBLIC include)

# 依赖以 PUBLIC 暴露，调用方（CLI / 将来的 Web 服务）只需链接 galib
target_link_libraries(galib PUBLIC Boost::iostreams ZLIB::ZLIB nbt++)

if(TARGET CGAL::CGAL)
    target_link_libraries(galib PUBLIC CGAL::CGAL)
else()
    target_link_libraries(galib PUBLIC ${CGAL_LIBRARIES})
endif()
