# Kokkos Mesh Processing Pipeline

![C++23](https://img.shields.io/badge/C%2B%2B-23%20Modules-blue.svg)
![Kokkos](https://img.shields.io/badge/Kokkos-5.0-orange.svg)
![Clang](https://img.shields.io/badge/Clang-18-purple.svg)
![CMake](https://img.shields.io/badge/CMake-%3E%3D%203.28-064F8C.svg?logo=cmake)
![Ninja](https://img.shields.io/badge/Build-Ninja-black.svg?logo=ninja)

An experimental mesh processing project using Kokkos and C++23 modules to explore GPU memory ordering and parallel area computations.

## Problem & Motivation

<p align="center">
  <img src="img/median_dual_light.png#gh-light-mode-only" alt="Median Dual Cells" width="700">
  <img src="img/median_dual_dark.png#gh-dark-mode-only" alt="Median Dual Cells" width="700">
</p>

Each triangle in the mesh contributes one third of its area to the median dual cell of each of its 3 vertices (sub-regions 1, 2, and 3 above). 

When computing this in parallel on GPU, multiple threads try to add their area contributions to the same vertex at the same time using `atomic_add`. When triangles are stored in an arbitrary order, these atomic collisions slow down memory throughput. The goal is to explore memory layouts and traversal orders that reduce collisions, while verifying mesh-wide area conservation.

## Quickstart

### Prerequisites
- **CMake** $\ge$ 3.28 (C++23 module support)
- **Ninja** build system
- **Clang / Clang++** $\ge$ 17

### Build & Run

```bash
# Configure & compile (dependencies fetched automatically)
cmake --preset default
cmake --build --preset default

# Run on a mesh
./build/app data/unit_square_132.meshb
```

> [!NOTE]
> Large `.meshb` datasets are excluded from Git. Provide your own or place them in `data/`.

## Exploration: Ordering Triangles for the GPU

<p align="center">
  <img src="img/dual_traversal_light.png#gh-light-mode-only" alt="Dual Mesh Traversal" width="700">
  <img src="img/dual_traversal_dark.png#gh-dark-mode-only" alt="Dual Mesh Traversal" width="700">
</p>

To reduce atomic collisions on shared vertices, neighboring triangles should be stored and processed close together in memory. Finding a good order was a progression of intuitions:

1. **Circulating around vertices:** My first idea was to follow triangles in a spiral around vertices. But drawing it by hand, I quickly realized that picking the next triangle easily gets stuck in loops, borders, or dead ends.
2. **Dual Spanning Tree (Kruskal):** A tree on the dual graph would avoid loops, but building an MST with Kruskal requires Union-Find, which is mostly sequential and hard to run efficiently on GPU.
3. **Growing patches from seeds:** Growing local clusters from multiple seed points at once so each GPU block handles a small patch. Promising, but coordinating cluster boundaries in parallel is tricky.
4. **Spatial sorting (SFC):** Instead of following graph connectivity, sort triangles by their 3D coordinates along a Space-Filling Curve (SFC). Mapping 3D space to a 1D curve keeps geometrically close triangles together in memory, and sorting keys in parallel on GPU is very fast.
5. **Gray code on hypercube:** Can we build such a curve by treating quantized 3D coordinates as a hypercube? A Gray code visits the hypercube by flipping only one bit at a time, moving continuously between neighboring cells without spatial jumps.

## Status & Roadmap

- [x] Read `.meshb` files via `libMeshb`
- [x] Flat **Half-Edge** structure (contiguous `Kokkos::View`, zero dynamic heap allocations)
- [x] Naive parallel area computation and reduction (`parallel_for` + `parallel_reduce`)
- [x] Machine-precision area conservation test
- [ ] Spatial sorting of triangle primitives with SFC or Gray-code
- [ ] Memory throughput benchmark: unordered vs. space-filling order under atomic contention
- [ ] Visual inspection using `ViZiR`