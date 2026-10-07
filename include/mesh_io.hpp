#pragma once

#include "mesh_types.hpp"
#include <string>
#include <unordered_map>
#include <cstdint>
#include <algorithm>

constexpr uint64_t encode_edge(int u, int v) {
    return (static_cast<uint64_t>(std::min(u, v)) << 32) | std::max(u, v);
}

auto build_hash_map(const std::vector<std::array<int, 4>>& tri_buffer) -> std::unordered_map<uint64_t, std::vector<int>>;
auto build_half_edges(const std::vector<std::array<int, 4>>& tri_buffer) -> std::vector<HalfEdge>;
void print_mesh_info(const Mesh& mesh, size_t limit = 10);
auto read_mesh(const std::string& filename) -> Mesh;
