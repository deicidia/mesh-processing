#include <Kokkos_Core.hpp>
#include <iostream>
#include <print>
#include <string_view>

int main(int argc, char **argv) {
  Kokkos::initialize(argc, argv);
  int exit_code = 0;
  {
    std::println("=== Test Kokkos ===");
    std::println("Kokkos Version        : {}.{}.{}", KOKKOS_VERSION_MAJOR,
                 KOKKOS_VERSION_MINOR, KOKKOS_VERSION_PATCH);
    std::println("Execution Space       : {}",
                 Kokkos::DefaultExecutionSpace::name());
    std::println("Memory Space          : {}",
                 Kokkos::DefaultExecutionSpace::memory_space::name());

    // Validation kernel: parallel reduction on the execution space
    constexpr int N = 1000;
    int sum = 0;
    Kokkos::parallel_reduce(
        "test_kernel_reduce", N,
        KOKKOS_LAMBDA(const int i, int &lsum) { lsum += 1; }, sum);

    if (sum == N) {
      std::println("[OK] Compute kernel validated (sum = {})", sum);
    } else {
      std::println(stderr, "[FAIL] Kernel execution error: expected {}, got {}",
                   N, sum);
      exit_code = 1;
    }

    for (int i = 1; i < argc; ++i) {
      if (std::string_view(argv[i]) == "--verbose" ||
          std::string_view(argv[i]) == "-v") {
        std::println("\n--- Full Kokkos Configuration ---");
        Kokkos::print_configuration(std::cout);
        break;
      }
    }
  }
  Kokkos::finalize();
  return exit_code;
}
