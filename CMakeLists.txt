cmake_minimum_required(VERSION 3.28)

# macOS: 仅使用 Homebrew，不使用 vcpkg
if(APPLE)
    # 获取 Homebrew 安装前缀
    execute_process(
        COMMAND brew --prefix
        OUTPUT_VARIABLE HOMEBREW_PREFIX
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    if(NOT HOMEBREW_PREFIX)
        message(FATAL_ERROR "Homebrew prefix not found. Please ensure Homebrew is installed and 'brew --prefix' works.")
    endif()
    message(STATUS "Homebrew prefix: ${HOMEBREW_PREFIX}")
    set(CMAKE_PREFIX_PATH "${HOMEBREW_PREFIX}" ${CMAKE_PREFIX_PATH})
    set(CMAKE_FIND_FRAMEWORK NEVER)
    set(CMAKE_FIND_APPBUNDLE NEVER)
elseif(WIN32)
    if(DEFINED ENV{VCPKG_ROOT})
        message(STATUS "VCPKG_ROOT is found: $ENV{VCPKG_ROOT}")
    else()
        message(FATAL_ERROR "VCPKG_ROOT is not found")
    endif()
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE PATH "Toolchain file for vcpkg")
    message(STATUS "CMAKE_TOOLCHAIN_FILE is ${CMAKE_TOOLCHAIN_FILE}")
endif()

project(LittleTilesReader)
set(CMAKE_CXX_STANDARD 14)

# 查找所需的库
add_subdirectory(3rdparty)
find_package(Boost REQUIRED COMPONENTS iostreams)
find_package(ZLIB REQUIRED)
find_package(CGAL REQUIRED)

# 检查 zlib 是否找到
if (${ZLIB_FOUND})
    message(STATUS "Found zlib: ${ZLIB_LIBRARIES}, Version: ${ZLIB_VERSION}")
else()
    message(FATAL_ERROR "zlib is not found")
endif()

# 检查 CGAL 是否找到
if (${CGAL_FOUND})
    message(STATUS "Found CGAL: ${CGAL_LIBRARIES}, Version: ${CGAL_VERSION}")
else()
    message(FATAL_ERROR "CGAL is not found")
endif()

# 禁用 CGAL 对 CMake 构建类型的警告
set(CGAL_DO_NOT_WARN_ABOUT_CMAKE_BUILD_TYPE TRUE)

# 定义可执行目标
add_executable(${CMAKE_PROJECT_NAME} main.cpp)
add_subdirectory(Galib)

# 将所需的库链接到可执行文件
target_link_libraries(${CMAKE_PROJECT_NAME} PUBLIC Boost::iostreams ZLIB::ZLIB)
target_link_libraries(${CMAKE_PROJECT_NAME} PUBLIC ${ZLIB_LIBRARIES})
target_link_libraries(${CMAKE_PROJECT_NAME} PUBLIC ${CGAL_LIBRARIES})

# 将 libnbtplusplus 库链接到目标
target_link_libraries(${CMAKE_PROJECT_NAME} PUBLIC nbt++)

# 添加依赖项，仅在 Windows 平台
#if(WIN32)
#    add_dependencies(LittleTilesReader CopyNBTDLL)
#endif()