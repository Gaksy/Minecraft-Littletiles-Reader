cmake_minimum_required(VERSION 3.28)

# Check if the VCPKG environment is set and retrieve the VCPKG_ROOT path
if(DEFINED ENV{VCPKG_ROOT})
    message("VCPKG_ROOT is found: $ENV{VCPKG_ROOT}")
else ()
    message(FATAL_ERROR "VCPKG_ROOT is not found")
endif()

# Set the CMake toolchain file for vcpkg
set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE PATH "Toolchain file for vcpkg")

# Allow users to set these variables
set(LIBNBT_INCLUDE "D:/Development/CmakeLib/libnbtplusplus/include" CACHE PATH "Path to libnbt++ include folder")
set(LIBNBT_LIBRARY "D:/Development/CmakeLib/libnbtplusplus/lib" CACHE PATH "Path to libnbt++ lib folder")

# Verify that the user has set the paths correctly
if(NOT EXISTS ${LIBNBT_INCLUDE})
    message(FATAL_ERROR "The specified libnbt++ include directory does not exist: ${LIBNBT_INCLUDE}")
endif()

if(NOT EXISTS ${LIBNBT_LIBRARY})
    message(FATAL_ERROR "The specified libnbt++ library directory does not exist: ${LIBNBT_LIBRARY}")
endif()

message("CMAKE_TOOLCHAIN_FILE is ${CMAKE_TOOLCHAIN_FILE}")

# Project definition
project(LittleTilesReader)

# Set C++ standard to C++14
set(CMAKE_CXX_STANDARD 14)

# Find required libraries
message("Searching for Boost library")
find_package(Boost REQUIRED)
message("Searching for Zlib library")
find_package(ZLIB REQUIRED)
message("Searching for CGAL library")
find_package(CGAL REQUIRED)

# Include the header files from the user-provided directory
include_directories(${LIBNBT_INCLUDE})

# Optionally: Find the library file if not already set by user
find_library(LIBNBT_FOUND NAMES nbt++ PATHS ${LIBNBT_LIBRARY})
if(NOT LIBNBT_FOUND)
    message(FATAL_ERROR "libnbt++ library not found in the specified directory: ${LIBNBT_LIBRARY}")
endif()

# Disable CGAL warnings about CMake build type
set(CGAL_DO_NOT_WARN_ABOUT_CMAKE_BUILD_TYPE TRUE)

# Define the executable target
add_executable(LittleTilesReader main.cpp
        Galib/Coord/CoordTraits.h
        Galib/GalibNamespaceDef.h
        Galib/Coord/Coord2D.h
        Galib/Coord/Coord3D.h
        Galib/Coord/Coord.h
        Galib/Coord/CoordString.h
        Galib/Exception/GalibExceptionBasic.h
        Galib/File/FileState.h
        Galib/File/FileState.cpp
        Galib/File/PathFormat.h
        Galib/File/PathFormat.cpp
        Galib/File/FileOperator.h
        Galib/Exception/MinecraftException.h
        Galib/Exception/MinecraftException.cpp
        Galib/Minecraft/MinecraftCoord.cpp
        Galib/Minecraft/MinecraftCoord.h
        Galib/Minecraft/Anvil.cpp
        Galib/Minecraft/Anvil.h
)

# From project root path
include_directories(${CMAKE_SOURCE_DIR})

# Link required libraries to the executable
target_link_libraries(${PROJECT_NAME} PUBLIC ${Boost_LIBRARIES})
target_link_libraries(${PROJECT_NAME} PUBLIC ZLIB::ZLIB)
target_link_libraries(${PROJECT_NAME} PUBLIC CGAL)

# Link the libnbtplusplus library to your target
target_link_libraries(${PROJECT_NAME} PUBLIC ${LIBNBT_FOUND})
