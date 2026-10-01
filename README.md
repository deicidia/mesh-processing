# Kokkos Mesh Processing Pipeline

## Problem & Motivation

<p align="center">
  <img src="img/median_dual_light.png#gh-light-mode-only" alt="Median Dual Cells" width="750">
  <img src="img/median_dual_dark.png#gh-dark-mode-only" alt="Median Dual Cells" width="750">
</p>

Each triangle in the primal mesh contributes one third of its area to the median dual cell of each of its 3 vertices (sub-regions 1, 2, and 3 above). 

A naive implementation results in a scatter memory pattern with severe atomic contention on vertex accumulators. The goal of this project is to explore and optimize memory layouts and traversal orders to maximize GPU memory throughput and verify mesh-wide area conservation.

### Dual Traversal & Memory Ordering

<p align="center">
  <img src="img/dual_traversal_light.png#gh-light-mode-only" alt="Dual Mesh Traversal" width="700">
  <img src="img/dual_traversal_dark.png#gh-dark-mode-only" alt="Dual Mesh Traversal" width="700">
</p>

My first idea was to order triangles by circulating around vertices. But drawing it by hand, I quickly realized that picking the next triangle and a starting point easily gets stuck in loops or dead ends.

To avoid loops, I thought about building a Minimum Spanning Tree (MST) on the dual graph using Kruskal's algorithm. However, Kruskal relies on Union-Find, which is mostly sequential and hard to run efficiently on GPU.

Another idea is to grow space-filling patterns from multiple seed points at the same time. Each seed would cover a small local patch of triangles that can be assigned directly to a GPU thread block, keeping data in local cache.

## ROADMAP

1. Parse `.meshb` files using `libMeshb`
2. Extract edges and build left/right neighbor lookup tables (or faster data structure)
3. Compute triangle areas using `Kokkos`
4. Compute median dual cell areas using `Kokkos`
5. Assert mesh-wide area conservation (total primal area == total dual area)
6. Visual inspection using `ViZiR`

## WIP

- [x] Replace node-based hash maps with a flat **Half-Edge** structure to eliminate cache misses, avoid millions of heap allocations on large meshes, and ensure native GPU/Kokkos compatibility.
- [ ] Use cache-oblivious reordering to optimize triangle memory locality before building the half-edge structure.