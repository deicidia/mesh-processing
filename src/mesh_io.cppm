module; 

#include <libmeshb8.h>
#include <print>
#include <cstdio>
#include <string>
#include <filesystem>
#include <vector>
#include <array>
#include <algorithm>
#include <cstdint>
#include <format>

export module mesh_reader; 

/*
    * GmfGetBlock(MeshIdx, KeyWord, BeginLine, EndLine, MapType, RenumberingMap, Procedure, ...)
    * - MeshIdx        : Descripteur retourné par GmfOpenMesh.
    * - KeyWord        : Mot-clé des entités à lire (ex: GmfTriangles, GmfVertices).
    * - BeginLine      : Ligne de départ (1-based, permet la lecture parallèle par morceaux).
    * - EndLine        : Dernière ligne à lire (ex: nb_triangles).
    * - MapType        : Type d'entier pour la table de renumérotation optionnelle (0 si inutilisé).
    * - RenumberingMap : Pointeur vers table de renumérotation (nullptr si inutilisé).
    * - Procedure      : Callback exécuté après lecture de chaque bloc (nullptr si inutilisé).
    * - arguments...   : Description des données de sortie :
    *                    - Mode scalaire : Type, &premier, &dernier
    *                    - Mode vecteur  : TypeVec, taille_vecteur, &premier[0], &dernier[0]
    *                      (Ici: GmfIntVec, 4 entiers par triangle [v1, v2, v3, ref])
    */

export struct Point {
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;
    int ref = 0;
};

export struct HalfEdge {
    int to = -1;
    int twin = -1;
};

export struct Mesh {
    int dim = 0;
    int ver = 0;
    std::vector<Point> vertices;
    std::vector<std::array<int, 4>> triangles;
    std::vector<HalfEdge> half_edges;
};

export constexpr int he_face(int h) { return h / 3; }
export constexpr int he_next(int h) { return (h % 3 == 2) ? h - 2 : h + 1; }
export constexpr int he_prev(int h) { return (h % 3 == 0) ? h + 2 : h - 1; }

struct EdgeRef {
    int u;
    int v;
    int h;
};


auto read_vertices_block(int64_t msh, int64_t nb_vertices, int dim) -> std::vector<Point> {
    if (nb_vertices <= 0) return {};
    std::vector<Point> vertices(nb_vertices);

    if (dim == 2) {
        GmfGetBlock(msh, GmfVertices, 1, nb_vertices, 0, nullptr, nullptr,
                    GmfDouble, &vertices.front().x, &vertices.back().x,
                    GmfDouble, &vertices.front().y, &vertices.back().y,
                    GmfInt,    &vertices.front().ref, &vertices.back().ref);
    } else if (dim == 3) {
        GmfGetBlock(msh, GmfVertices, 1, nb_vertices, 0, nullptr, nullptr,
                    GmfDouble, &vertices.front().x, &vertices.back().x,
                    GmfDouble, &vertices.front().y, &vertices.back().y,
                    GmfDouble, &vertices.front().z, &vertices.back().z,
                    GmfInt,    &vertices.front().ref, &vertices.back().ref);
    }

    return vertices;
}

auto read_triangles_block(int64_t msh, int64_t nb_triangles) -> std::vector<std::array<int, 4>> {
    if (nb_triangles <= 0) return {};
    std::vector<std::array<int, 4>> tri_buffer(nb_triangles);

    GmfGetBlock(msh, GmfTriangles, 1, nb_triangles, 0, nullptr, nullptr,
                GmfIntVec, 4, 
                &tri_buffer.front()[0], 
                &tri_buffer.back()[0]);

    return tri_buffer;
}

export auto build_half_edges(const std::vector<std::array<int, 4>>& tri_buffer) -> std::vector<HalfEdge> {
    int64_t nb_triangles = static_cast<int64_t>(tri_buffer.size());
    std::vector<HalfEdge> half_edges(3 * nb_triangles);
    std::vector<EdgeRef> refs(3 * nb_triangles);

    for (int64_t f = 0; f < nb_triangles; ++f)
    {
        int v0 = tri_buffer[f][0];
        int v1 = tri_buffer[f][1];
        int v2 = tri_buffer[f][2];

        int h0 = static_cast<int>(3 * f + 0);
        int h1 = static_cast<int>(3 * f + 1);
        int h2 = static_cast<int>(3 * f + 2);

        half_edges[h0].to = v1;
        half_edges[h1].to = v2;
        half_edges[h2].to = v0;

        refs[h0] = { std::min(v0, v1), std::max(v0, v1), h0 };
        refs[h1] = { std::min(v1, v2), std::max(v1, v2), h1 };
        refs[h2] = { std::min(v2, v0), std::max(v2, v0), h2 };
    }

    std::sort(refs.begin(), refs.end(), [](const EdgeRef& a, const EdgeRef& b) {
        if (a.u != b.u) return a.u < b.u;
        return a.v < b.v;
    });

    size_t i = 0;
    while (i < refs.size())
    {
        if (i + 1 < refs.size() && refs[i].u == refs[i + 1].u && refs[i].v == refs[i + 1].v)
        {
            int h1 = refs[i].h;
            int h2 = refs[i + 1].h;
            half_edges[h1].twin = h2;
            half_edges[h2].twin = h1;
            i += 2;
        }
        else
        {
            i += 1;
        }
    }

    return half_edges;
}

export void print_mesh_info(const Mesh& mesh, size_t limit = 10) {
    std::println("Fichier chargé : Version {}, Dimension {}", mesh.ver, mesh.dim);
    std::println("Sommets: {}, Triangles: {}", mesh.vertices.size(), mesh.triangles.size());

    int boundary_count = 0;
    for (const auto& he : mesh.half_edges)
    {
        if (he.twin == -1) {
            boundary_count++;
        }
    }
    std::println("Demi-aretes: {} (Internes: {}, Bord: {})\n", 
                 mesh.half_edges.size(), mesh.half_edges.size() - boundary_count, boundary_count);

    int nb_triangles = static_cast<int>(mesh.triangles.size());
    for (int f = 0; f < std::min<int>(limit , nb_triangles); ++f)
    {
        int v0 = mesh.triangles[f][0];
        int v1 = mesh.triangles[f][1];
        int v2 = mesh.triangles[f][2];
        int ref = mesh.triangles[f][3];

        int h0 = 3 * f + 0;
        int h1 = 3 * f + 1;
        int h2 = 3 * f + 2;

        auto fmt_nb = [&](int h) -> std::string {
            int twin = mesh.half_edges[h].twin;
            if (twin == -1) return "[BORD]";
            return std::format("Triangle {:>3}", he_face(twin) + 1);
        };

        std::println("Triangle {:>3} : [ {:>3} {:>3} {:>3} ] Ref {:>2} -> Voisins: [ {}, {}, {} ]",
                     f + 1, v0, v1, v2, ref,
                     fmt_nb(h0), fmt_nb(h1), fmt_nb(h2));
    }

    std::println("");
    for (int h = 0; h < std::min<int>(limit, static_cast<int>(mesh.half_edges.size())); ++h)
    {
        int twin = mesh.half_edges[h].twin;
        std::println("HalfEdge {:>3} (Triangle {:>3}, To: {:>3}) -> Twin: {:>3} ({})",
                     h, he_face(h) + 1, mesh.half_edges[h].to, 
                     twin, 
                     (twin != -1 ? std::format("Triangle {}", he_face(twin) + 1) : "[BORD]"));
    }
}

export auto read_mesh(const std::string& filename) -> Mesh {
    std::string path = filename;
    if (!std::filesystem::exists(path) && std::filesystem::exists("../" + path)) {
        path = "../" + path;
    }

    int ver = 0, dim = 0;
    int64_t msh = GmfOpenMesh(path.c_str(), GmfRead, &ver, &dim);
    if (!msh) {
        std::println(stderr, "Impossible d'ouvrir le fichier : {}", path);
        return {};
    }

    int64_t nb_vertices = GmfStatKwd(msh, GmfVertices);
    int64_t nb_triangles = GmfStatKwd(msh, GmfTriangles);

    Mesh mesh;
    mesh.dim = dim;
    mesh.ver = ver;
    mesh.vertices = read_vertices_block(msh, nb_vertices, dim);
    mesh.triangles = read_triangles_block(msh, nb_triangles);
    mesh.half_edges = build_half_edges(mesh.triangles);

    GmfCloseMesh(msh);
    return mesh;
}
