#include <Kokkos_Core.hpp>
#include <print>
#include <string_view>
#include <iostream>

int main(int argc, char** argv) {
    Kokkos::initialize(argc, argv);
    int exit_code = 0;
    {
        std::println("=== Test Kokkos ===");
        std::println("Kokkos Version        : {}.{}.{}",
                     KOKKOS_VERSION_MAJOR, KOKKOS_VERSION_MINOR, KOKKOS_VERSION_PATCH);
        std::println("Execution Space       : {}", Kokkos::DefaultExecutionSpace::name());
        std::println("Memory Space          : {}", Kokkos::DefaultExecutionSpace::memory_space::name());

        // Kernel de validation : réduction parallèle sur l'espace d'exécution
        constexpr int N = 1000;
        int sum = 0;
        Kokkos::parallel_reduce("test_kernel_reduce", N,
            KOKKOS_LAMBDA(const int i, int& lsum) {
                lsum += 1;
            }, sum
        );

        if (sum == N) {
            std::println("[OK] Kernel de calcul validé (somme = {})", sum);
        } else {
            std::println(stderr, "[FAIL] Erreur d'exécution kernel : attendu {}, obtenu {}", N, sum);
            exit_code = 1;
        }

        for (int i = 1; i < argc; ++i) {
            if (std::string_view(argv[i]) == "--verbose" || std::string_view(argv[i]) == "-v") {
                std::println("\n--- Configuration complète Kokkos ---");
                Kokkos::print_configuration(std::cout);
                break;
            }
        }
    }
    Kokkos::finalize();
    return exit_code;
}
