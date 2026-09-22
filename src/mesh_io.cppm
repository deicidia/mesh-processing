module; 

#include <libmeshb8.h>
#include <print>
#include <cstdio>
#include <string>
#include <filesystem>

export module mesh_reader; 

export void read_mesh(const std::string& filename) {
    std::string path = filename;
    if (!std::filesystem::exists(path) && std::filesystem::exists("../" + path)) {
        path = "../" + path;
    }

    int ver = 0, dim = 0;
    int64_t msh = GmfOpenMesh(path.c_str(), GmfRead, &ver, &dim);
    if (!msh) {
        std::println(stderr, "Impossible d'ouvrir le fichier : {}", path);
        return;
    }

    int64_t nb_vertices = GmfStatKwd(msh, GmfVertices);
    int64_t nb_triangles = GmfStatKwd(msh, GmfTriangles);

    std::println("Fichier: {}", filename);
    std::println("Version: {}, Dimension: {}", ver, dim);
    std::println("Sommets: {}, Triangles: {}", nb_vertices, nb_triangles);

    GmfCloseMesh(msh);
}
