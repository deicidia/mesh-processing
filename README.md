# Kokkos Mesh Processing Pipeline

![C++23](https://img.shields.io/badge/C%2B%2B-23-blue.svg)
![Kokkos](https://img.shields.io/badge/Kokkos-5.0-orange.svg)
![Clang](https://img.shields.io/badge/Clang-18-purple.svg)
![CMake](https://img.shields.io/badge/CMake-%3E%3D%203.28-064F8C.svg?logo=cmake)
![Ninja](https://img.shields.io/badge/Build-Ninja-black.svg?logo=ninja)

An experimental mesh processing project using Kokkos and C++23 to explore GPU memory ordering and parallel area computations.

## Problem & Motivation

<p align="center">
  <img src="img/median_dual_light.png#gh-light-mode-only" alt="Median Dual Cells" width="700">
  <img src="img/median_dual_dark.png#gh-dark-mode-only" alt="Median Dual Cells" width="700">
</p>

Each triangle in the mesh contributes one third of its area to the median dual cell of each of its 3 vertices (sub-regions 1, 2, and 3 above). 

When computing this in parallel on GPU, multiple threads add their area contributions to shared vertices using `atomic_add`. When triangles are processed in an arbitrary order, these uncoalesced atomic writes scatter across memory, causing cache thrashing and saturating memory bandwidth. The goal is to explore memory layouts and traversal orders that maximize cache locality, while verifying mesh-wide area conservation.

## Quickstart

### Prerequisites
- **CMake** $\ge$ 3.28 (C++23 module support)
- **Ninja** build system
- **Clang / Clang++** $\ge$ 18 with `clang-tools-18` (`clang-scan-deps`)
- **GCC / libstdc++** $\ge$ 14 (`g++-14` for C++23 `<print>`)
- *(Optional for GPU)* **ROCm / HIP** $\ge$ 6.2 (AMD GPU) or **CUDA Toolkit** $\ge$ 12.0 (NVIDIA GPU)

```bash
# On Ubuntu 24.04:
sudo apt install -y clang-18 clang-tools-18 g++-14 ninja-build
```

### Build & Run

#### 1. CPU (Default - Serial)
```bash
# Configure & compile
cmake --preset default
cmake --build --preset default

# Run on a mesh
./build/app data/unit_square_132.meshb
```

#### 2. AMD GPU (ROCm / HIP)
```bash
# Configure & compile with HIP
cmake --preset hip
cmake --build --preset hip

# Run on GPU
./build-hip/app data/unit_square_132.meshb
```

#### 3. NVIDIA GPU (CUDA)
```bash
# Configure & compile with CUDA
cmake --preset cuda
cmake --build --preset cuda

# Run on GPU
./build-cuda/app data/unit_square_132.meshb
```

### Quickstart with Makefile

#### Local Execution (Host)
```bash
make amd           # Build & run on AMD GPU (ROCm / HIP)
make nvidia        # Build & run on NVIDIA GPU (CUDA)
make cpu           # Build & run on CPU (Serial)
```

#### Docker Execution
```bash
make docker-amd    # Run on AMD GPU in Docker
make docker-nvidia # Run on NVIDIA GPU in Docker
make docker-cpu    # Run on CPU in Docker

# Custom mesh example:
make docker-amd MESH=data/unit_square_11k.meshb
```

#### Tests & Validation (CTest)
```bash
# Via Make:
make test              # Run test suite on CPU (Serial)
make test-amd          # Run test suite on AMD GPU (ROCm / HIP)
make test-docker-amd   # Run test suite inside Docker (AMD GPU)
make test-docker-cpu   # Run test suite inside Docker (CPU)

# Or directly with CTest presets:
ctest --preset default
ctest --preset hip
```

> [!NOTE]
> Large `.meshb` datasets are excluded from Git. Provide your own or place them in `data/`.

## Exploration: Ordering Triangles for the GPU

<p align="center">
  <img src="img/dual_traversal_light.png#gh-light-mode-only" alt="Dual Mesh Traversal" width="700">
  <img src="img/dual_traversal_dark.png#gh-dark-mode-only" alt="Dual Mesh Traversal" width="700">
</p>

To maximize cache reuse and avoid uncoalesced memory scatter, neighboring triangles should be stored and processed close together in memory. Finding a good order was a progression of intuitions:

1. **Circulating around vertices:** My first idea was to follow triangles in a spiral around vertices. But drawing it by hand, I quickly realized that picking the next triangle easily gets stuck in loops, borders, or dead ends.
2. **Dual Spanning Tree:** A tree on the dual graph would eliminate loops, but building a spanning tree requires graph algorithms that are inherently sequential and hard to run efficiently on GPU.
3. **Growing patches from seeds:** Growing local clusters from multiple seed points at once so each GPU block handles a small patch. Promising, but coordinating cluster boundaries in parallel is tricky.
4. **Spatial sorting (SFC):** Instead of traversing the mesh graph, sort triangles by their 3D coordinates along a Space-Filling Curve (SFC). Bypassing graph connectivity entirely keeps geometrically close triangles together in memory, and sorting keys in parallel on GPU is very fast.
5. **Gray code on hypercube:** Can we build such a curve by treating quantized 3D coordinates as a hypercube? A Gray code visits the hypercube by flipping only one bit at a time, moving continuously between neighboring cells without spatial jumps.

## Status & Roadmap

- [x] Read `.meshb` files via `libMeshb`
- [x] Flat **Half-Edge** structure on host (contiguous array, zero dynamic heap allocations, GPU-ready)
- [x] Naive parallel area computation and reduction (`parallel_for` + `parallel_reduce`)
- [x] Machine-precision area conservation test
- [ ] Spatial sorting of triangle primitives with SFC or Gray code
- [ ] Memory throughput benchmark: unordered vs. space-filling order under atomic contention