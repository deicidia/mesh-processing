module; 

#include <libmeshb8.h>
#include <Kokkos_Core.hpp>
#include <print>
#include <cstdio>
#include <string> 

export module mesh_reader; 

export void read_mesh(const std::string& filename) {
    int ver = 0, dim = 0;
    int64_t msh = GmfOpenMesh(filename.c_str(), GmfRead, &ver, &dim);
    if (!msh) {
        std::println(stderr, "Impossible d'ouvrir le fichier : {}", filename);
        return;
    }

    int64_t nb_vertices = GmfStatKwd(msh, GmfVertices);
    int64_t nb_triangles = GmfStatKwd(msh, GmfTriangles);

    std::println("Fichier: {}", filename);
    std::println("Version: {}, Dimension: {}", ver, dim);
    std::println("Sommets: {}, Triangles: {}", nb_vertices, nb_triangles);

    GmfCloseMesh(msh);
}
