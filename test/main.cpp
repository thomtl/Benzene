#include <iostream>
#include <benzene/benzene.hpp>
#define PAR_OCTASPHERE_IMPLEMENTATION
#include "octasphere.h"

int main([[maybe_unused]] int argc, [[maybe_unused]] char const *argv[])
{
    benzene::Instance engine{"Benzene-test", 800, 600};
    //engine.get_backend().set_fps_cap(true, 60);

    /*auto mesh = benzene::Mesh::Primitives::cube();
    mesh.textures.push_back(benzene::Texture::load_from_file("../engine/resources/obama.jpg", "diffuse", benzene::Texture::Gamut::Srgb));
    mesh.textures.push_back(benzene::Texture::load_from_colour(glm::vec3{1}, "specular"));
    mesh.textures.push_back(benzene::Texture::load_from_colour(glm::vec3{0, 0, 0.5}, "normal"));
    mesh.material.shininess = 64.0f;

    std::array<glm::vec3, 10> positions = {
        glm::vec3( 0.0f,  0.0f,  0.0f),
        glm::vec3( 2.0f,  5.0f, -15.0f),
        glm::vec3(-1.5f, -2.2f, -2.5f),
        glm::vec3(-3.8f, -2.0f, -12.3f),
        glm::vec3( 2.4f, -0.4f, -3.5f),
        glm::vec3(-1.7f,  3.0f, -7.5f),
        glm::vec3( 1.3f, -2.0f, -2.5f),
        glm::vec3( 1.5f,  2.0f, -2.5f),
        glm::vec3( 1.5f,  0.2f, -1.5f),
        glm::vec3(-1.3f,  1.0f, -1.5f)
    };

    benzene::Batch batch{};
    batch.meshes.push_back(mesh);

    for(size_t i = 0; i < positions.size(); i++)
        batch.transforms.push_back({.pos = positions[i], .rotation = {}, .scale = {1, 1, 1}});

    engine.add_batch(&batch);*/

    /*benzene::Model model2{};
    model2.load_mesh_data_from_file("../engine/resources/rungholt/", "rungholt.obj");
    model2.pos = {0, 0, 0};
    model2.scale = {1, 1, 1};
    for(auto& mesh : model2.meshes){
        //mesh.textures.push_back(benzene::Texture::load_from_file("../engine/resources/rungholt/rungholt-RGBA.png", "diffuse", benzene::Texture::Gamut::Linear));
        mesh.textures.push_back(benzene::Texture::load_from_colour(glm::vec3{1}, "diffuse"));
        mesh.textures.push_back(benzene::Texture::load_from_colour(glm::vec3{1}, "specular"));
        mesh.textures.push_back(benzene::Texture::load_from_colour(glm::vec3{0, 0, 1}, "normal"));
    }*/

    //engine.add_batch(&model2);
    engine.set_property(benzene::BackendProperties::ClearColour, {0, 0, 0, 1});


    /* Specify a 100x100x20 rounded box. */
    const par_octasphere_config cfg = {
        .corner_radius = 5,
        .width = 0,
        .height = 0,
        .depth = 0,
        .num_subdivisions = 1,
        .uv_mode = PAR_OCTASPHERE_UV_LATLONG,
        .normals_mode = PAR_OCTASPHERE_NORMALS_SMOOTH
    };

    // Allocate memory for the mesh and opt-out of normals.
    uint32_t num_indices;
    uint32_t num_vertices;
    par_octasphere_get_counts(&cfg, &num_indices, &num_vertices);
    
    std::vector<float> vertices, normals, texcoords;
    vertices.resize(num_vertices * 3);
    normals.resize(num_vertices * 3);
    texcoords.resize(num_vertices * 2);

    std::vector<uint16_t> indices;
    indices.resize(num_indices);
    

    par_octasphere_mesh mesh = {
        .positions = vertices.data(),
        .normals = normals.data(),
        .texcoords = texcoords.data(),
        .indices = indices.data(),
        .num_indices = 0,
        .num_vertices = 0
    };

    // Generate vertex coordinates, UV's, and triangle indices.
    par_octasphere_populate(&cfg, &mesh);

    benzene::Mesh api_mesh{};

    for(size_t i = 0; i < num_vertices * 3; i += 3){
        auto pos = glm::vec3{vertices[i], vertices[i + 1], vertices[i + 2]};
        auto normal = glm::vec3{normals[i], normals[i + 1], normals[i + 2]};

        benzene::Mesh::Vertex v{v.pos = pos, v.normal = normal, v.tangent = glm::vec3{}, v.uv = glm::vec2{}};
        api_mesh.vertices.push_back(v);
    }

    for(size_t i = 0; i < num_vertices * 2; i += 2){
        api_mesh.vertices[i / 2].uv = glm::vec2{texcoords[i], 1 - texcoords[i + 1]};
    }

    for(size_t i = 0; i < num_vertices; i += 3){
        auto calculate_surface_tangent = [](benzene::Mesh::Vertex v0, benzene::Mesh::Vertex v1, benzene::Mesh::Vertex v2) -> glm::vec3 {
            auto dv1 = v1.pos - v0.pos;
            auto dv2 = v2.pos - v0.pos;

            auto duv1 = v1.uv - v0.uv;
            auto duv2 = v2.uv - v0.uv;

            auto f = 1.0f / (duv1.x * duv2.y - duv1.y * duv2.x);

            return glm::normalize((dv1 * duv2.y - dv2 * duv1.y) * f);
        };

        auto& v0 = api_mesh.vertices[i];
        auto& v1 = api_mesh.vertices[i + 1];
        auto& v2 = api_mesh.vertices[i + 2];
        v0.tangent = v1.tangent = v2.tangent = calculate_surface_tangent(v0, v1, v2);
    }

    for(size_t i = 0; i < num_indices; i++)
        api_mesh.indices.push_back(indices[i]);

    api_mesh.textures.push_back(benzene::Texture::load_from_colour(glm::vec3{1}, "specular"));
    api_mesh.textures.push_back(benzene::Texture::load_from_colour(glm::vec3{0, 0, 0.5}, "normal"));
    api_mesh.material.shininess = 64.0f;

    api_mesh.textures.push_back(benzene::Texture::load_from_file("../engine/resources/obama.jpg", "diffuse", benzene::Texture::Gamut::Srgb));

    benzene::Batch obama_model{};
    obama_model.meshes.push_back(std::move(api_mesh));
    obama_model.transforms.push_back({.pos = {0, 0, 0}, .rotation = {0, 0, 0}, .scale = {3, 3, 3}});

    engine.add_batch(&obama_model);



    // Ring
    auto asteroid_mesh = benzene::Mesh::Primitives::cube();
    asteroid_mesh.textures.push_back(benzene::Texture::load_from_file("../engine/resources/obama.jpg", "diffuse", benzene::Texture::Gamut::Srgb));
    asteroid_mesh.textures.push_back(benzene::Texture::load_from_colour(glm::vec3{1}, "specular"));
    asteroid_mesh.textures.push_back(benzene::Texture::load_from_colour(glm::vec3{0, 0, 0.5}, "normal"));
    asteroid_mesh.material.shininess = 64.0f;

    benzene::Batch ring{};
    ring.meshes.push_back(std::move(asteroid_mesh));

    constexpr size_t n_asteroids = 5000;
    srand(time(NULL));
    auto radius = 50.0f;
    auto offset = 12.5f;
    for(size_t i = 0; i < n_asteroids; i++){
        auto angle = (float)i / (float)n_asteroids * 360.0f;
        auto displacement = [offset]() -> float {
            return (rand() % (int)(2 * offset * 100)) / 100.0f - offset;
        };

        auto x = sin(angle) * radius + displacement();
        auto y = displacement() * 0.4f;
        auto z = cos(angle) * radius + displacement();
        auto position = glm::vec3{x, y, z};

        auto scale_factor = (rand() % 20) / 100.0f + 0.05f;
        auto scale = glm::vec3{scale_factor, scale_factor, scale_factor};

        float rot_x = rand() % 360;
        float rot_y = rand() % 360;
        float rot_z = rand() % 360;
        auto rotation = glm::vec3{rot_x, rot_y, rot_z};
        ring.transforms.push_back(benzene::Batch::Transform{.pos = position, .rotation = rotation, .scale = scale});
    }

    engine.add_batch(&ring);

    engine.run([&](benzene::FrameData& data){
        obama_model.show_inspector("Obama land");
        if(ImGui::BeginMainMenuBar()){
            if(ImGui::BeginMenu("File")){
                ImGui::Checkbox("Debug", &data.display_debug_window);
                if(ImGui::MenuItem("Exit"))
                    data.should_exit = true;
                ImGui::EndMenu();
            }
            ImGui::EndMainMenuBar();
        }
    });
    return 0;
}
