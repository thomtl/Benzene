#include <benzene/benzene.hpp>
#include <glm/gtx/string_cast.hpp>

benzene::Mesh benzene::Mesh::Primitives::cube(){
    std::vector<benzene::Mesh::Vertex> vertices = {
        // Back face
        {{-0.5f, -0.5f, -0.5f}, {0.0f,  0.0f, -1.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}}, // Bottom-left
        {{0.5f,  0.5f, -0.5f}, {0.0f,  0.0f, -1.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 1.0f}}, // top-right
        {{0.5f, -0.5f, -0.5f}, {0.0f,  0.0f, -1.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}}, // bottom-right         
        {{0.5f,  0.5f, -0.5f}, {0.0f,  0.0f, -1.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 1.0f}}, // top-right
        {{-0.5f, -0.5f, -0.5f}, {0.0f,  0.0f, -1.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}}, // bottom-left
        {{-0.5f,  0.5f, -0.5f}, {0.0f,  0.0f, -1.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f}}, // top-left
        // Front face
        {{-0.5f, -0.5f,  0.5f}, {0.0f,  0.0f, 1.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}}, // bottom-left
        {{0.5f, -0.5f,  0.5f}, {0.0f,  0.0f, 1.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}}, // bottom-right
        {{0.5f,  0.5f,  0.5f}, {0.0f,  0.0f, 1.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 1.0f}}, // top-right
        {{0.5f,  0.5f,  0.5f}, {0.0f,  0.0f, 1.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 1.0f}}, // top-right
        {{-0.5f,  0.5f,  0.5f}, {0.0f,  0.0f, 1.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f}}, // top-left
        {{-0.5f, -0.5f,  0.5f}, {0.0f,  0.0f, 1.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}}, // bottom-left
        // Left face
        {{-0.5f,  0.5f,  0.5f}, {-1.0f,  0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}}, // top-right
        {{-0.5f,  0.5f, -0.5f}, {-1.0f,  0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f}}, // top-left
        {{-0.5f, -0.5f, -0.5f}, {-1.0f,  0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f}}, // bottom-left
        {{-0.5f, -0.5f, -0.5f}, {-1.0f,  0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f}}, // bottom-left
        {{-0.5f, -0.5f,  0.5f}, {-1.0f,  0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}}, // bottom-right
        {{-0.5f,  0.5f,  0.5f}, {-1.0f,  0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}}, // top-right
        // Right face
        {{0.5f,  0.5f,  0.5f}, {1.0f,  0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}}, // top-left
        {{0.5f, -0.5f, -0.5f}, {1.0f,  0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f}}, // bottom-right
        {{0.5f,  0.5f, -0.5f}, {1.0f,  0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f}}, // top-right         
        {{0.5f, -0.5f, -0.5f}, {1.0f,  0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f}}, // bottom-right
        {{0.5f,  0.5f,  0.5f}, {1.0f,  0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}}, // top-left
        {{0.5f, -0.5f,  0.5f}, {1.0f,  0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}}, // bottom-left     
        // Bottom face
        {{-0.5f, -0.5f, -0.5f}, {0.0f,  -1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f}}, // top-right
        {{0.5f, -0.5f, -0.5f}, {0.0f,  -1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 1.0f}}, // top-left
        {{0.5f, -0.5f,  0.5f}, {0.0f,  -1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}}, // bottom-left
        {{0.5f, -0.5f,  0.5f}, {0.0f,  -1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}}, // bottom-left
        {{-0.5f, -0.5f,  0.5f}, {0.0f,  -1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}}, // bottom-right
        {{-0.5f, -0.5f, -0.5f}, {0.0f,  -1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f}}, // top-right
        // Top face
        {{-0.5f,  0.5f, -0.5f}, {0.0f,  1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f}}, // top-left
        {{0.5f,  0.5f,  0.5f}, {0.0f,  1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}}, // bottom-right
        {{0.5f,  0.5f, -0.5f}, {0.0f,  1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 1.0f}}, // top-right     
        {{0.5f,  0.5f,  0.5f}, {0.0f,  1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}}, // bottom-right
        {{-0.5f,  0.5f, -0.5f}, {0.0f,  1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f}}, // top-left
        {{-0.5f,  0.5f,  0.5f}, {0.0f,  1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}}  // bottom-left
    };
    assert((vertices.size() % 3) == 0);

    std::vector<uint32_t> indices{};
    for(size_t i = 0; i < vertices.size(); i++)
        indices.push_back(i);

    benzene::Mesh mesh{};
    mesh.indices = indices;
    mesh.vertices = vertices;

    return mesh;
}

benzene::Mesh benzene::Mesh::Primitives::quad(){
    std::vector<benzene::Mesh::Vertex> vertices = {
        {{-0.5f, 0.0f, -0.5f}, {0.0f, 0.0f, 1.0f}, {-1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
        {{0.5f, 0.0f, -0.5f}, {0.0f, 0.0f, 1.0f}, {-1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
        {{0.5f, 0.0f, 0.5f}, {0.0f, 0.0f, 1.0f}, {-1.0f, 0.0f, 0.0f}, {0.0f, 1.0f}},
        {{-0.5f, 0.0f, 0.5f}, {0.0f, 0.0f, 1.0f}, {-1.0f, 0.0f, 0.0f}, {1.0f, 1.0f}}
    };

    std::vector<uint32_t> indices = {
        2, 1, 0, 0, 3, 2
    };

    benzene::Mesh mesh{};
    mesh.indices = indices;
    mesh.vertices = vertices;

    return mesh;
}