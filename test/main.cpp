#include <iostream>
#include <benzene/benzene.hpp>

int main([[maybe_unused]] int argc, [[maybe_unused]] char const *argv[])
{
    benzene::Instance engine{"Benzene-test", 800, 600};
    //engine.get_backend().set_fps_cap(true, 60);

    auto mesh = benzene::Mesh::Primitives::cube();
    mesh.textures.push_back(benzene::Texture::load_from_file("../engine/resources/brickwall.jpg", "diffuse", benzene::Texture::Gamut::Srgb));
    mesh.textures.push_back(benzene::Texture::load_from_colour(glm::vec3{1}, "specular"));
    mesh.textures.push_back(benzene::Texture::load_from_file("../engine/resources/brickwall_normal.jpg", "normal", benzene::Texture::Gamut::Linear));
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

    std::vector<benzene::Model> models{};
    models.reserve(positions.size()); // Reserve enough space so the pointers passed into benzene stay consitent

    for(auto pos : positions){
        benzene::Model model{};
        model.meshes.push_back(mesh);
        model.pos = pos;
        model.scale = glm::vec3{1, 1, 1};

        auto& m = models.emplace_back(std::move(model));
        engine.add_model(&m);
    }

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

    //engine.add_model(&model2);
    engine.set_property(benzene::BackendProperties::ClearColour, {0, 0, 0, 1});

    engine.run([&](benzene::FrameData& data){
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
