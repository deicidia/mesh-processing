#include <libmeshb8.h>
#include <print>
#include <string>

int main(int argc, char** argv) {
    std::string filename = (argc > 1) ? argv[1] : "data/unit_square_132.meshb";

    std::println("=== Test libMeshb ===");
    std::println("Target file           : {}", filename);

    int ver = 0, dim = 0;
    int64_t msh = GmfOpenMesh(filename.c_str(), GmfRead, &ver, &dim);
    if (!msh) {
        std::println(stderr, "[FAIL] Unable to open mesh file: {}", filename);
        return 1;
    }

    int64_t nb_vertices = GmfStatKwd(msh, GmfVertices);
    int64_t nb_triangles = GmfStatKwd(msh, GmfTriangles);

    std::println("Version               : {}", ver);
    std::println("Dimension             : {}", dim);
    std::println("Vertices count        : {}", nb_vertices);
    std::println("Triangles count       : {}", nb_triangles);

    GmfCloseMesh(msh);

    if (nb_vertices <= 0 || nb_triangles <= 0) {
        std::println(stderr, "[FAIL] Mesh contains no valid vertices or triangles");
        return 1;
    }

    std::println("[OK] libMeshb read validated");
    return 0;
}
