#include <Kokkos_Core.hpp>
#include <string>
#include <print>
#include <cmath>
#include "kernels/mesh_compute.hpp"
#include "mesh_io.hpp"

int main(int argc, char** argv) {
    Kokkos::initialize(argc, argv);
    {
        Kokkos::Timer timer_total;

        std::string file = (argc > 1) ? argv[1] : "data/unit_square_132.meshb";
        Kokkos::Timer timer_io;
        Mesh mesh = read_mesh(file);
        double time_io = timer_io.seconds();

        if (mesh.vertices.empty() || mesh.triangles.empty()) {
            Kokkos::finalize();
            return 1;
        }

        print_mesh_info(mesh, 5);

        Kokkos::fence();
        Kokkos::Timer timer_upload;
        auto dev_mesh = upload_to_device(mesh);
        Kokkos::fence();
        double time_upload = timer_upload.seconds();

        Kokkos::fence();
        Kokkos::Timer timer_tri;
        compute_triangle_areas(dev_mesh);
        Kokkos::fence();
        double time_tri = timer_tri.seconds();

        Kokkos::fence();
        Kokkos::Timer timer_dual;
        compute_median_dual_areas_naive(dev_mesh);
        Kokkos::fence();
        double time_dual = timer_dual.seconds();

        Kokkos::fence();
        Kokkos::Timer timer_reduce;
        double total_primal = compute_total_primal_area(dev_mesh);
        double total_dual = compute_total_dual_area(dev_mesh);
        Kokkos::fence();
        double time_reduce = timer_reduce.seconds();

        double diff = std::abs(total_primal - total_dual);
        double time_compute = time_tri + time_dual + time_reduce;
        double m_tri_sec_1 = (time_tri > 0.0) ? (mesh.triangles.size() / 1e6) / time_tri : 0.0;
        double m_tri_sec_2 = (time_dual > 0.0) ? (mesh.triangles.size() / 1e6) / time_dual : 0.0;

        std::println("\n--- Parallel Kokkos Computations ---");
        std::println("Kokkos Execution Space    : {}", Kokkos::DefaultExecutionSpace::name());
        std::println("Kokkos Memory Space       : {}", Kokkos::DefaultExecutionSpace::memory_space::name());
        std::println("----------------------------------");
        std::println("Total primal area (triangles) : {:.10f}", total_primal);
        std::println("Total dual area   (cells)     : {:.10f}", total_dual);
        std::println("Absolute difference           : {:.2e}", diff);

        if (diff < 1e-9) {
            std::println("Area conservation verified.");
        } else {
            std::println(stderr, "Critical discrepancy in area conservation: {:.2e}", diff);
            Kokkos::finalize();
            return 1;
        }

        std::println("\n--- Performance & Profiling (Kokkos Timer) ---");
        std::println("I/O & Topology Reading   : {:>8.3f} ms", time_io * 1000.0);
        std::println("Host -> Device Transfer  : {:>8.3f} ms", time_upload * 1000.0);
        std::println("Kernel 1 (Primal areas)  : {:>8.3f} ms  ({:7.1f} M tri/s)", time_tri * 1000.0, m_tri_sec_1);
        std::println("Kernel 2 (Dual areas)    : {:>8.3f} ms  ({:7.1f} M tri/s)", time_dual * 1000.0, m_tri_sec_2);
        std::println("Parallel reductions      : {:>8.3f} ms", time_reduce * 1000.0);
        std::println("-------------------------------------------------");
        std::println("Total Compute (Kernels)  : {:>8.3f} ms", time_compute * 1000.0);
        std::println("Total Pipeline Time      : {:>8.3f} ms", timer_total.seconds() * 1000.0);
        std::println("-------------------------------------------------");
    }
    Kokkos::finalize();
    return 0;
}
