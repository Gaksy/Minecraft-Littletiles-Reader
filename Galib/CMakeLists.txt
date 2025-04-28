target_include_directories(${CMAKE_PROJECT_NAME} PUBLIC include)
target_sources(${CMAKE_PROJECT_NAME} PUBLIC
    Minecraft/Anvil.cpp
    Minecraft/MinecraftCoord.cpp
    Minecraft/ChunkTileEntities.cpp
    Minecraft/BlockTileEntities.cpp
    Minecraft/LittleTilesEntity.cpp
    Minecraft/CgalSupport/CgalTypeDef.cpp
    Minecraft/CgalSupport/CgalLtSupport.cpp
    Minecraft/CgalSupport/CgalLittletilesBuider.cpp
    File/FileState.cpp
    File/PathFormat.cpp
    Exception/MinecraftException.cpp
    Exception/LittleTilesException.cpp
)
