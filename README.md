# Kokkos Mesh Processing Pipeline

1. Parse `.meshb` files using `libMeshb`
2. Extract edges and build left/right neighbor lookup tables (or faster data structure)
3. Compute triangle areas using `Kokkos`
4. Compute median dual cell areas using `Kokkos`
5. Assert mesh-wide area conservation (total primal area == total dual area)
6. Visual inspection using `ViZiR`
