include(FetchContent)

# nbt++
set(NBT_BUILD_SHARED ON CACHE BOOL "Build shared libraries")
set(NBT_USE_ZLIB OFF CACHE BOOL "Build additional zlib stream functionality")
set(NBT_BUILD_TESTS OFF CACHE BOOL "Build the unit tests. Requires CxxTest.")

# Pin to an explicit commit so that every machine and every point in time builds
# the exact same dependency version. The upstream repository has no tags, so the
# commit is the only option; when pinning a commit GIT_SHALLOW must be FALSE,
# otherwise git clone --branch <commit> fails.
set(NBT_PLUS_PLUS_GIT_TAG "687e43031df0dc641984b4256bcca50d5b3f7de3")

# Alternatives for the configure stage in offline / intranet environments
# (pick either one):
#   -DFETCHCONTENT_FULLY_DISCONNECTED=ON
#   -DFETCHCONTENT_SOURCE_DIR_LIBNBTPLUSPLUS=<path to an existing local source tree>
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
