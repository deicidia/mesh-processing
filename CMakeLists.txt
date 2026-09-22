cmake_minimum_required(VERSION 3.22)

if(POLICY CMP0135)
  cmake_policy(SET CMP0135 NEW)
endif()

project(MyProject LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(
  Kokkos
  URL https://github.com/kokkos/kokkos/archive/refs/tags/5.0.0.zip
)

FetchContent_MakeAvailable(Kokkos)

add_executable(HelloKokkos test/HelloKokkos.cpp)
target_link_libraries(HelloKokkos Kokkos::kokkos)

