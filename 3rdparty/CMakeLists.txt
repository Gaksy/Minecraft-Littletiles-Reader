include(FetchContent)

# nbt++
set(NBT_BUILD_SHARED ON CACHE BOOL "Build shared libraries")
set(NBT_USE_ZLIB OFF CACHE BOOL "Build additional zlib stream functionality")
set(NBT_BUILD_TESTS OFF CACHE BOOL "Build the unit tests. Requires CxxTest.")

FetchContent_Declare(
  libnbtplusplus
  GIT_REPOSITORY https://github.com/PrismLauncher/libnbtplusplus.git
  GIT_SHALLOW    TRUE
)

FetchContent_GetProperties(libnbtplusplus)
if(NOT libnbtplusplus_POPULATED)
    FetchContent_MakeAvailable(libnbtplusplus)
endif()

# 3rdparty/CMakeLists.txt
add_custom_target(CopyNBTDLL ALL
        COMMAND ${CMAKE_COMMAND} -E copy ${NBT_DLL_PATH} ${CMAKE_BINARY_DIR}
        COMMENT "Copying nbt++.dll to build directory"
)
