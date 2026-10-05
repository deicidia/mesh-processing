#pragma once

#include <Kokkos_Core.hpp>
#include "mesh_types.hpp"

struct MeshDeviceData {
    int nb_vertices = 0;
    int nb_triangles = 0;

    Kokkos::View<double*[3]> coords;
    Kokkos::View<int*[3]> triangles;
    
    Kokkos::View<double*> triangle_areas;
    Kokkos::View<double*> dual_areas;
};

auto upload_to_device(const Mesh& mesh) -> MeshDeviceData;
void compute_triangle_areas(MeshDeviceData& dev);
void compute_median_dual_areas_naive(MeshDeviceData& dev);
auto compute_total_primal_area(const MeshDeviceData& dev) -> double;
auto compute_total_dual_area(const MeshDeviceData& dev) -> double;
