cmake_minimum_required(VERSION 3.28)

# 依赖来源选择，优先级从高到低：
#   1. 由命令行 / IDE 显式传入的 CMAKE_TOOLCHAIN_FILE
#   2. 设置了 VCPKG_ROOT 环境变量时，所有平台统一使用 vcpkg
#   3. macOS 回退到 Homebrew
if(CMAKE_TOOLCHAIN_FILE)
    message(STATUS "Using user-specified toolchain: ${CMAKE_TOOLCHAIN_FILE}")
elseif(DEFINED ENV{VCPKG_ROOT})
    message(STATUS "VCPKG_ROOT is found: $ENV{VCPKG_ROOT}")
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE PATH "Toolchain file for vcpkg")
    if(NOT EXISTS "${CMAKE_TOOLCHAIN_FILE}")
        message(FATAL_ERROR "vcpkg toolchain is not found at ${CMAKE_TOOLCHAIN_FILE}")
    endif()
    message(STATUS "CMAKE_TOOLCHAIN_FILE is ${CMAKE_TOOLCHAIN_FILE}")
elseif(APPLE)
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
    message(FATAL_ERROR "VCPKG_ROOT is not found, and there is no Homebrew fallback on Windows")
endif()

project(LittleTilesReader)
# 代码实际使用 C++17 特性（if-init、嵌套命名空间定义、std::filesystem），
# 且 CGAL 6.x 也要求 C++17；此前声明为 14 与代码不符。
set(CMAKE_CXX_STANDARD 17)

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

# 定义核心库目标
add_subdirectory(Galib)

# 定义可执行目标：CLI 只是 core library 的一个消费者
add_executable(${CMAKE_PROJECT_NAME} main.cpp)
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE galib)

# 添加依赖项，仅在 Windows 平台
#if(WIN32)
#    add_dependencies(LittleTilesReader CopyNBTDLL)
#endif()
