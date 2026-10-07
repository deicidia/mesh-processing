#include "mesh_io.hpp"
#include <chrono>
#include <print>
#include <vector>
#include <algorithm>
#include <string>
#include <filesystem>

struct TimingStats {
    double min_ms = 0.0;
    double mean_ms = 0.0;
    double median_ms = 0.0;
};

auto measure_benchmark(int warmup_runs, int measured_runs, auto&& func) -> TimingStats {
    for (int i = 0; i < warmup_runs; ++i) {
        func();
    }

    std::vector<double> times;
    times.reserve(measured_runs);

    for (int i = 0; i < measured_runs; ++i) {
        auto start = std::chrono::high_resolution_clock::now();
        func();
        auto end = std::chrono::high_resolution_clock::now();
        double elapsed_ms = std::chrono::duration<double, std::milli>(end - start).count();
        times.push_back(elapsed_ms);
    }

    std::ranges::sort(times);
    double sum = std::ranges::fold_left(times, 0.0, std::plus<>{});
    double mean = sum / times.size();
    double median = times[times.size() / 2];

    return TimingStats{
        .min_ms = times.front(),
        .mean_ms = mean,
        .median_ms = median
    };
}

int main(int argc, char** argv) {
    std::string file = (argc > 1) ? argv[1] : "data/unit_square_1m.meshb";
    if (!std::filesystem::exists(file) && std::filesystem::exists("../" + file)) {
        file = "../" + file;
    }

    std::println("################################################################################");
    std::println("# BENCHMARK : build_hash_map vs build_half_edges                               #");
    std::println("################################################################################");

    Mesh mesh = read_mesh(file);
    if (mesh.triangles.empty()) {
        std::println(stderr, "Error: unable to load mesh file: {}", file);
        return 1;
    }

    const auto& triangles = mesh.triangles;
    size_t nb_tri = triangles.size();
    size_t nb_half_edges = 3 * nb_tri;

    std::println("File: {} ({} triangles, {} half-edges)\n", file, nb_tri, nb_half_edges);

    // Equivalence verification
    auto edge_map = build_hash_map(triangles);
    auto half_edges = build_half_edges(triangles);

    size_t matches = 0;
    size_t mismatches = 0;
    for (int h = 0; h < static_cast<int>(half_edges.size()); ++h) {
        int u = half_edges[he_prev(h)].to;
        int v = half_edges[h].to;
        int f = he_face(h);

        uint64_t key = encode_edge(u, v);
        auto it = edge_map.find(key);
        if (it == edge_map.end()) {
            mismatches++;
            continue;
        }

        const auto& faces = it->second;
        int twin = half_edges[h].twin;

        if (twin == -1) {
            if (faces.size() == 1 && faces[0] == f) {
                matches++;
            } else {
                mismatches++;
            }
        } else {
            int f_twin = he_face(twin);
            if (faces.size() == 2 &&
                (faces[0] == f || faces[1] == f) &&
                (faces[0] == f_twin || faces[1] == f_twin)) {
                matches++;
            } else {
                mismatches++;
            }
        }
    }

    std::println("Verification: {}/{} matches (mismatches: {})", matches, nb_half_edges, mismatches);

    // Warmup & timing
    int warmup = 1;
    int runs = 3;

    auto stats_map = measure_benchmark(warmup, runs, [&]() {
        auto res = build_hash_map(triangles);
        (void)res;
    });

    auto stats_he = measure_benchmark(warmup, runs, [&]() {
        auto res = build_half_edges(triangles);
        (void)res;
    });

    std::println("\n{:<38} | {:>10} | {:>10} | {:>10} | {:>12}",
                 "Structure (mesh_io)", "Min (ms)", "Avg (ms)", "Med (ms)", "Throughput (MTri/s)");
    std::println("{:-<38}-+-{:-<10}-+-{:-<10}-+-{:-<10}-+-{:-<12}", "", "", "", "", "");

    auto print_row = [&](const std::string& name, const TimingStats& s) {
        double throughput = (static_cast<double>(nb_tri) / 1e6) / (s.median_ms / 1000.0);
        std::println("{:<38} | {:>10.3f} | {:>10.3f} | {:>10.3f} | {:>12.2f}",
                     name, s.min_ms, s.mean_ms, s.median_ms, throughput);
    };

    print_row("1. build_hash_map   (unordered_map)", stats_map);
    print_row("2. build_half_edges (Half-Edges sort)", stats_he);

    double speedup = stats_map.median_ms / stats_he.median_ms;
    std::println("\n--> build_half_edges is {:.2f}x faster than build_hash_map\n", speedup);

    return 0;
}
