#include <iostream>
#include <benzene/benzene.hpp>

int main([[maybe_unused]] int argc, [[maybe_unused]] char const *argv[])
{
    benzene::Instance engine{"Benzene-test", 800, 600};
    //engine.get_backend().set_fps_cap(true, 60);

    auto mesh = benzene::Mesh::Primitives::quad();
    mesh.textures.push_back(benzene::Texture::load_from_file("../engine/resources/brickwall.jpg", "diffuse", benzene::Texture::Gamut::Srgb));
    mesh.textures.push_back(benzene::Texture::load_from_colour(glm::vec3{1}, "specular"));
    mesh.textures.push_back(benzene::Texture::load_from_file("../engine/resources/brickwall_normal.jpg", "normal", benzene::Texture::Gamut::Linear));
    mesh.material.shininess = 64.0f;

    benzene::Model model{};
    model.meshes.push_back(mesh);
    model.pos = {0, 0, 0};
    model.scale = {1, 1, 1};

    /*benzene::Model model2{};
    model2.load_mesh_data_from_file("../engine/resources/", "chalet.obj");
    model2.pos = {0, 0, 0};
    model2.scale = {1, 1, 1};
    for(auto& mesh : model2.meshes)
        mesh.textures.push_back(benzene::Texture::load_from_file("../engine/resources/chalet.jpg", "diffuse"));*/

    engine.add_model(&model);
    //engine.add_model(&model2);

    engine.run([&](benzene::FrameData& data){
        model.show_inspector();

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
