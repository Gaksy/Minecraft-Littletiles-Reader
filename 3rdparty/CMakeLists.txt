include(FetchContent)

# nbt++
set(NBT_BUILD_SHARED ON CACHE BOOL "Build shared libraries")
set(NBT_USE_ZLIB OFF CACHE BOOL "Build additional zlib stream functionality")
set(NBT_BUILD_TESTS OFF CACHE BOOL "Build the unit tests. Requires CxxTest.")

# 固定到明确的提交，保证不同机器、不同时间构建出同一个依赖版本。
# 该仓库上游没有 tag，因此只能钉 commit；钉 commit 时 GIT_SHALLOW 必须为 FALSE，
# 否则 git clone --branch <commit> 会失败。
set(NBT_PLUS_PLUS_GIT_TAG "687e43031df0dc641984b4256bcca50d5b3f7de3")

# 离线 / 内网环境下配置阶段的替代做法（二者任选其一）：
#   -DFETCHCONTENT_FULLY_DISCONNECTED=ON
#   -DFETCHCONTENT_SOURCE_DIR_LIBNBTPLUSPLUS=<本地已存在的源码目录>
FetchContent_Declare(
  libnbtplusplus
  GIT_REPOSITORY https://github.com/PrismLauncher/libnbtplusplus.git
  GIT_TAG        ${NBT_PLUS_PLUS_GIT_TAG}
  GIT_SHALLOW    FALSE
)

FetchContent_GetProperties(libnbtplusplus)
if(NOT libnbtplusplus_POPULATED)
    FetchContent_MakeAvailable(libnbtplusplus)
endif()

set(NBT_DLL_PATH "${CMAKE_BINARY_DIR}/_deps/libnbtplusplus-build/nbt++.dll")

# 3rdparty/CMakeLists.txt
#add_custom_target(CopyNBTDLL ALL
#        COMMAND ${CMAKE_COMMAND} -E copy ${NBT_DLL_PATH} ${CMAKE_BINARY_DIR}
#        COMMENT "Copying nbt++.dll to build directory"
#)
