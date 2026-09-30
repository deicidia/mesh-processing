import mesh_reader;

#include <string>

int main(int argc, char** argv) {
    std::string file = (argc > 1) ? argv[1] : "data/unit_square_132.meshb";
    Mesh mesh = read_mesh(file);
    print_mesh_info(mesh, 12);
    return 0;
}
