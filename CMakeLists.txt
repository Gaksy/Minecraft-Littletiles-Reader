cmake_minimum_required(VERSION 3.28)

if(POLICY CMP0167)
    cmake_policy(SET CMP0167 NEW)
endif()

# Dependency source selection, highest priority first:
#   1. CMAKE_TOOLCHAIN_FILE passed explicitly from the command line / IDE
#   2. When the VCPKG_ROOT environment variable is set, use vcpkg on all platforms
#   3. macOS falls back to Homebrew
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
    # Get the Homebrew installation prefix
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

project(LittleTilesReader VERSION 0.2.0 LANGUAGES CXX)

# Release channel for the version string; the default lives in Galib/include/Version.h.
# A CI job can build a test build without touching the header:
#   cmake -DGALIB_VERSION_CHANNEL=beta ..
set(GALIB_VERSION_CHANNEL "" CACHE STRING "Release channel: stable or beta (empty = use the header default)")
# The code actually uses C++17 features (if-init, nested namespace definitions,
# std::filesystem), and CGAL 6.x also requires C++17; the previous declaration of
# 14 did not match the code.
set(CMAKE_CXX_STANDARD 17)

# Locate the required libraries
add_subdirectory(3rdparty)
find_package(Boost REQUIRED COMPONENTS iostreams json)
find_package(ZLIB REQUIRED)
find_package(CGAL REQUIRED)

# Check whether zlib was found
if (${ZLIB_FOUND})
    message(STATUS "Found zlib: ${ZLIB_LIBRARIES}, Version: ${ZLIB_VERSION}")
else()
    message(FATAL_ERROR "zlib is not found")
endif()

# Check whether CGAL was found
if (${CGAL_FOUND})
    message(STATUS "Found CGAL: ${CGAL_LIBRARIES}, Version: ${CGAL_VERSION}")
else()
    message(FATAL_ERROR "CGAL is not found")
endif()

# Disable CGAL's warning about the CMake build type
set(CGAL_DO_NOT_WARN_ABOUT_CMAKE_BUILD_TYPE TRUE)

# Define the core library target
add_subdirectory(Galib)

# Define the executable target: the CLI is just one consumer of the core library
add_executable(${CMAKE_PROJECT_NAME} main.cpp)
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE galib)

if(WIN32)
    add_custom_command(TARGET ${CMAKE_PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:nbt++>
            $<TARGET_FILE_DIR:${CMAKE_PROJECT_NAME}>
        COMMENT "Copying libnbt++ runtime DLL next to LittleTilesReader")
endif()
