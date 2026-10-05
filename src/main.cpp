#include <Kokkos_Core.hpp>
#include <string>
#include <print>
#include <cmath>
#include "kernels/mesh_compute.hpp"
#include "mesh_io.hpp"

int main(int argc, char** argv) {
    Kokkos::initialize(argc, argv);
    {
        std::string file = (argc > 1) ? argv[1] : "data/unit_square_132.meshb";
        Mesh mesh = read_mesh(file);
        if (mesh.vertices.empty() || mesh.triangles.empty()) {
            Kokkos::finalize();
            return 1;
        }

        print_mesh_info(mesh, 5);

        auto dev_mesh = upload_to_device(mesh);

        compute_triangle_areas(dev_mesh);
        compute_median_dual_areas_naive(dev_mesh);

        double total_primal = compute_total_primal_area(dev_mesh);
        double total_dual = compute_total_dual_area(dev_mesh);
        double diff = std::abs(total_primal - total_dual);

        std::println("\n--- Calculs Parallèles Kokkos ---");

        std::println("Kokkos Execution Space    : {}", Kokkos::DefaultExecutionSpace::name());
        std::println("Kokkos Memory Space       : {}", Kokkos::DefaultExecutionSpace::memory_space::name());

        std::println("----------------------------------");

        std::println("Aire totale primale (triangles) : {:.10f}", total_primal);
        std::println("Aire totale duale   (cellules)  : {:.10f}", total_dual);
        std::println("Différence absolue              : {:.2e}", diff);

        if (diff < 1e-10) {
            std::println("Conservation de l'aire vérifiée.");
        } else {
            std::println("Écart dans la conservation de l'aire.");
        }
    }
    Kokkos::finalize();
    return 0;
}
