cmake_minimum_required(VERSION 3.28)

project(MyProject LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(mesh_core)
target_sources(mesh_core
  PUBLIC
  FILE_SET CXX_MODULES FILES
  src/mesh_io.cppm
)

include(FetchContent)

find_package(Kokkos QUIET)
if(NOT Kokkos_FOUND)
  FetchContent_Declare(
    Kokkos
    URL https://github.com/kokkos/kokkos/archive/refs/tags/5.0.0.zip
  )
  FetchContent_MakeAvailable(Kokkos)
endif()

find_package(libMeshb 8 QUIET)
if(NOT libMeshb_FOUND)
  FetchContent_Declare(
    libmeshb
    URL https://github.com/LoicMarechal/libMeshb/archive/refs/heads/master.zip
  )
  FetchContent_MakeAvailable(libmeshb)
endif()

add_executable(kokkos_info test/kokkos_info.cpp)
target_link_libraries(kokkos_info Kokkos::kokkos)

add_executable(check_meshb test/check_meshb.cpp)
target_link_libraries(check_meshb Meshb.8 Kokkos::kokkos)

target_link_libraries(mesh_core PUBLIC Meshb.8 Kokkos::kokkos)
add_executable(app src/main.cpp)
target_link_libraries(app PRIVATE mesh_core)

