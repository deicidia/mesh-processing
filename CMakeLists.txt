cmake_minimum_required(VERSION 3.28)

project(MyProject LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(Kokkos_ENABLE_HIP)
  find_path(ROCM_INCLUDE_DIR NAMES hip/hip_runtime.h
    PATHS /opt/rocm/core-7.14/include /opt/rocm/include $ENV{ROCM_PATH}/include
  )
  if(ROCM_INCLUDE_DIR)
    list(REMOVE_ITEM CMAKE_CXX_IMPLICIT_INCLUDE_DIRECTORIES "${ROCM_INCLUDE_DIR}")
    include_directories(SYSTEM ${ROCM_INCLUDE_DIR})
    add_compile_options(-isystem ${ROCM_INCLUDE_DIR})
  endif()
endif()

if(Kokkos_ENABLE_CUDA)
  find_path(CUDA_INCLUDE_DIR NAMES cuda_runtime.h
    PATHS /usr/local/cuda/include $ENV{CUDA_ROOT}/include $ENV{CUDA_PATH}/include
  )
  if(CUDA_INCLUDE_DIR)
    include_directories(SYSTEM ${CUDA_INCLUDE_DIR})
  endif()
endif()

add_library(mesh_core
  src/mesh_io.cpp
  src/kernels/mesh_compute.cpp
)
target_include_directories(mesh_core PUBLIC include src)

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

