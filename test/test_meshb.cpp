#include <libmeshb8.h>
#include <print>
#include <string>

int main(int argc, char** argv) {
    std::string filename = (argc > 1) ? argv[1] : "data/unit_square_132.meshb";

    std::println("=== Test libMeshb ===");
    std::println("Fichier cible         : {}", filename);

    int ver = 0, dim = 0;
    int64_t msh = GmfOpenMesh(filename.c_str(), GmfRead, &ver, &dim);
    if (!msh) {
        std::println(stderr, "[FAIL] Impossible d'ouvrir le fichier maillage : {}", filename);
        return 1;
    }

    int64_t nb_vertices = GmfStatKwd(msh, GmfVertices);
    int64_t nb_triangles = GmfStatKwd(msh, GmfTriangles);

    std::println("Version               : {}", ver);
    std::println("Dimension             : {}", dim);
    std::println("Nombre de sommets     : {}", nb_vertices);
    std::println("Nombre de triangles   : {}", nb_triangles);

    GmfCloseMesh(msh);

    if (nb_vertices <= 0 || nb_triangles <= 0) {
        std::println(stderr, "[FAIL] Le maillage ne contient aucun sommet ou triangle valide");
        return 1;
    }

    std::println("[OK] Lecture libMeshb validée");
    return 0;
}
