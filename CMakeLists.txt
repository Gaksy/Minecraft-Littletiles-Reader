cmake_minimum_required(VERSION 3.28)

# Check if the VCPKG environment is set and retrieve the VCPKG_ROOT path
if(DEFINED ENV{VCPKG_ROOT})
    message("VCPKG_ROOT is found: $ENV{VCPKG_ROOT}")
else ()
    message(FATAL_ERROR "VCPKG_ROOT is not found")
endif ()

# Set the CMake toolchain file for vcpkg
set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE PATH "Toolchain file for vcpkg")
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

# Disable CGAL warnings about CMake build type
set(CGAL_DO_NOT_WARN_ABOUT_CMAKE_BUILD_TYPE TRUE)

# Define the executable target
add_executable(LittleTilesReader main.cpp
        Galib/Coord/CoordTraits.h
        Galib/GalibNamespaceDef.h
        Galib/Coord/Coord2D.h
        Galib/Coord/Coord3D.h
        Galib/Coord/Coord.h
)


# From project root path
include_directories(${CMAKE_SOURCE_DIR})

# Link required libraries to the executable
target_link_libraries(${PROJECT_NAME} LINK_PUBLIC ${Boost_LIBRARIES})
target_link_libraries(${PROJECT_NAME} ZLIB::ZLIB)
target_link_libraries(${PROJECT_NAME} CGAL)