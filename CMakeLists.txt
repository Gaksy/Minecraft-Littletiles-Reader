cmake_minimum_required(VERSION 3.28)

# Check if the VCPKG environment is set and retrieve the VCPKG_ROOT path
if(DEFINED ENV{VCPKG_ROOT})
    message(STATUS "VCPKG_ROOT is found: $ENV{VCPKG_ROOT}")
else ()
    message(FATAL_ERROR "VCPKG_ROOT is not found")
endif()

# Set the CMake toolchain file for vcpkg
set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE PATH "Toolchain file for vcpkg")
message(STATUS "CMAKE_TOOLCHAIN_FILE is ${CMAKE_TOOLCHAIN_FILE}")

# Project definition
# Set C++ standard to C++14
project(LittleTilesReader)
set(CMAKE_CXX_STANDARD 14)

# Find required libraries
add_subdirectory(3rdparty)
find_package(Boost REQUIRED COMPONENTS iostreams)
find_package(ZLIB REQUIRED)
find_package(CGAL REQUIRED)

if (${ZLIB_FOUND})
    message(STATUS "Found zlib: " ${ZLIB_LIBRARIES} ", Version: " ${ZLIB_VERSION})
else()
    message(FATAL_ERROR "zlib is not found")
endif()

if (${CGAL_FOUND})
    message(STATUS "Found cgal: " ${CGAL_LIBRARIES} ", Version: " ${CGAL_VERSION})
else()
    message(FATAL_ERROR "cgal is not found")
endif()

# Disable CGAL warnings about CMake build type
set(CGAL_DO_NOT_WARN_ABOUT_CMAKE_BUILD_TYPE TRUE)

# Define the executable target
add_executable(${CMAKE_PROJECT_NAME} main.cpp)
add_subdirectory(Galib)

# From project root path
include_directories(${CMAKE_SOURCE_DIR})

# Link required libraries to the executable
target_link_libraries(${CMAKE_PROJECT_NAME} PUBLIC Boost::iostreams)
target_link_libraries(${CMAKE_PROJECT_NAME} PUBLIC ${ZLIB_LIBRARIES})
target_link_libraries(${CMAKE_PROJECT_NAME} PUBLIC ${CGAL_LIBRARIES})

# Link the libnbtplusplus library to your target
target_link_libraries(${CMAKE_PROJECT_NAME} PUBLIC nbt++)
