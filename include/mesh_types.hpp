#pragma once

#include <vector>
#include <array>

struct Point {
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;
    int ref = 0;
};

struct HalfEdge {
    int to = -1;
    int twin = -1;
};

struct Mesh {
    int dim = 0;
    int ver = 0;
    std::vector<Point> vertices;
    std::vector<std::array<int, 4>> triangles;
    std::vector<HalfEdge> half_edges;
};

constexpr int he_face(int h) { return h / 3; }
constexpr int he_next(int h) { return (h % 3 == 2) ? h - 2 : h + 1; }
constexpr int he_prev(int h) { return (h % 3 == 0) ? h + 2 : h - 1; }
