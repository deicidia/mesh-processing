#include "kernels/mesh_compute.hpp"
#include <cmath>

auto upload_to_device(const Mesh& mesh) -> MeshDeviceData {
    MeshDeviceData dev;
    dev.nb_vertices = static_cast<int>(mesh.vertices.size());
    dev.nb_triangles = static_cast<int>(mesh.triangles.size());

    dev.coords = Kokkos::View<double*[3]>("dev_coords", dev.nb_vertices);
    dev.triangles = Kokkos::View<int*[3]>("dev_triangles", dev.nb_triangles);
    dev.triangle_areas = Kokkos::View<double*>("dev_triangle_areas", dev.nb_triangles);
    dev.dual_areas = Kokkos::View<double*>("dev_dual_areas", dev.nb_vertices);

    auto h_coords = Kokkos::create_mirror_view(dev.coords);
    auto h_triangles = Kokkos::create_mirror_view(dev.triangles);

    for (int i = 0; i < dev.nb_vertices; ++i) {
        h_coords(i, 0) = mesh.vertices[i].x;
        h_coords(i, 1) = mesh.vertices[i].y;
        h_coords(i, 2) = mesh.vertices[i].z;
    }

    for (int f = 0; f < dev.nb_triangles; ++f) {
        h_triangles(f, 0) = mesh.triangles[f][0] - 1;
        h_triangles(f, 1) = mesh.triangles[f][1] - 1;
        h_triangles(f, 2) = mesh.triangles[f][2] - 1;
    }

    Kokkos::deep_copy(dev.coords, h_coords);
    Kokkos::deep_copy(dev.triangles, h_triangles);

    return dev;
}

void compute_triangle_areas(MeshDeviceData& dev) {
    auto coords = dev.coords;
    auto triangles = dev.triangles;
    auto areas = dev.triangle_areas;

    Kokkos::parallel_for("compute_triangle_areas", dev.nb_triangles,
        KOKKOS_LAMBDA(const int f) {
            int v0 = triangles(f, 0);
            int v1 = triangles(f, 1);
            int v2 = triangles(f, 2);

            // AB
            double ab_x = coords(v1, 0) - coords(v0, 0);
            double ab_y = coords(v1, 1) - coords(v0, 1);
            double ab_z = coords(v1, 2) - coords(v0, 2);

            // AC
            double ac_x = coords(v2, 0) - coords(v0, 0);
            double ac_y = coords(v2, 1) - coords(v0, 1);
            double ac_z = coords(v2, 2) - coords(v0, 2);

            double cp_x = ab_y * ac_z - ab_z * ac_y;
            double cp_y = ab_z * ac_x - ab_x * ac_z;
            double cp_z = ab_x * ac_y - ab_y * ac_x;

            double norm = std::sqrt(cp_x * cp_x + cp_y * cp_y + cp_z * cp_z);
            areas(f) = 0.5 * norm;
        }
    );
}

void compute_median_dual_areas_naive(MeshDeviceData& dev) {
    auto triangles = dev.triangles;
    auto tri_areas = dev.triangle_areas;
    auto dual_areas = dev.dual_areas;

    Kokkos::deep_copy(dual_areas, 0.0);

    Kokkos::parallel_for("compute_median_dual_areas_naive", dev.nb_triangles,
        KOKKOS_LAMBDA(const int f) {
            double contrib = tri_areas(f) / 3.0;

            int v0 = triangles(f, 0);
            int v1 = triangles(f, 1);
            int v2 = triangles(f, 2);

            Kokkos::atomic_add(&dual_areas(v0), contrib);
            Kokkos::atomic_add(&dual_areas(v1), contrib);
            Kokkos::atomic_add(&dual_areas(v2), contrib);
        }
    );
}

auto compute_total_primal_area(const MeshDeviceData& dev) -> double {
    auto areas = dev.triangle_areas;
    double total = 0.0;
    Kokkos::parallel_reduce("reduce_primal_area", dev.nb_triangles,
        KOKKOS_LAMBDA(const int f, double& lsum) {
            lsum += areas(f);
        }, total
    );
    return total;
}

auto compute_total_dual_area(const MeshDeviceData& dev) -> double {
    auto dual_areas = dev.dual_areas;
    double total = 0.0;
    Kokkos::parallel_reduce("reduce_dual_area", dev.nb_vertices,
        KOKKOS_LAMBDA(const int v, double& lsum) {
            lsum += dual_areas(v);
        }, total
    );
    return total;
}
