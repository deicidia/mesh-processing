#include <print>

import mesh_reader;

int main() {
    std::println("Mesh module test");
    read_mesh("data/unit_square_132.meshb");
    std::println("Mesh loaded with success via the module");
    return 0;
}
