cmake_minimum_required(VERSION 3.2)

set(CMAKE_OSX_DEPLOYMENT_TARGET "10.11" CACHE STRING "Minimum macOS deployment version")

project(external)

set(ASSIMP_BUILD_TESTS OFF CACHE BOOL "")
set(ASSIMP_BUILD_SAMPLES OFF CACHE BOOL "")
set(ASSIMP_BUILD_ASSIMP_TOOLS OFF CACHE BOOL "")
if(LUMIN)
    set(ASSIMP_BUILD_ZLIB OFF CACHE BOOL "")
else()
    set(ASSIMP_BUILD_ZLIB ON CACHE BOOL "")
endif()
set(SYSTEM_IRRXML OFF CACHE BOOL "")
set(ASSIMP_NO_EXPORT ON CACHE BOOL "")
set(ASSIMP_BUILD_ALL_IMPORTERS_BY_DEFAULT FALSE CACHE BOOL "")
set(ASSIMP_BUILD_OBJ_IMPORTER TRUE CACHE BOOL "")
set(ASSIMP_BUILD_FBX_IMPORTER TRUE CACHE BOOL "")
set(ASSIMP_BUILD_GLTF_IMPORTER TRUE CACHE BOOL "")
set(ASSIMP_INSTALL_PDB OFF CACHE BOOL "")

set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "")
set(GLFW_BUILD_TESTS OFF CACHE BOOL "")
set(GLFW_BUILD_DOCS OFF CACHE BOOL "")

set(GFLAGS_BUILD_SHARED_LIBS ON CACHE BOOL "")
set(GFLAGS_BUILD_STATIC_LIBS ON CACHE BOOL "")
set(GFLAGS_BUILD_gflags_nothreads_LIB ON CACHE BOOL "")
set(GFLAGS_BUILD_TESTING OFF CACHE BOOL "")
set(GFLAGS_BUILD_PACKAGING OFF CACHE BOOL "")
set(GFLAGS_INSTALL_HEADERS ON CACHE BOOL "")
set(GFLAGS_INSTALL_SHARED_LIBS ON CACHE BOOL "")
set(GFLAGS_INSTALL_STATIC_LIBS ON CACHE BOOL "")
set(GFLAGS_INCLUDE_DIR "gflags" CACHE STRING "")
set(GFLAGS_LIBRARY_INSTALL_DIR "lib" CACHE STRING "")

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/assimp-v.5.0.0.rc1 ${CMAKE_BINARY_DIR}/assimp-v.5.0.0.rc1)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/gflags-2.2.2 ${CMAKE_BINARY_DIR}/gflags-2.2.2)
if(NOT LUMIN)
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/glfw-3.2.1 ${CMAKE_BINARY_DIR}/glfw-3.2.1)
endif()
