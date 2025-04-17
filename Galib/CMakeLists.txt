target_include_directories(${CMAKE_PROJECT_NAME} PUBLIC include)
target_sources(${CMAKE_PROJECT_NAME} PUBLIC
    Minecraft/Anvil.cpp
    Minecraft/MinecraftCoord.cpp
    Minecraft/LittleTiles.cpp
    File/FileState.cpp
    File/PathFormat.cpp
    Exception/MinecraftException.cpp
    Exception/LittleTilesException.cpp)
