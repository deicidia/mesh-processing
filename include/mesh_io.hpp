#pragma once

#include "mesh_types.hpp"
#include <string>

auto build_half_edges(const std::vector<std::array<int, 4>>& tri_buffer) -> std::vector<HalfEdge>;
void print_mesh_info(const Mesh& mesh, size_t limit = 10);
auto read_mesh(const std::string& filename) -> Mesh;
