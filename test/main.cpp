#include <iostream>
#include <benzene/benzene.hpp>

int main([[maybe_unused]] int argc, [[maybe_unused]] char const *argv[])
{
    benzene::Instance engine{"Benzene-test", 800, 600};
    //engine.get_backend().set_fps_cap(true, 60);

    std::vector<benzene::Mesh::Vertex> raw_vertices = {
        {{-0.5f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
        {{0.5f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
        {{0.5f, 0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
        {{-0.5f, 0.5f, 0.0f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}}
    };
    
    std::vector<uint16_t> raw_indices = {
        0, 1, 2, 2, 3, 0
    };

    benzene::Mesh mesh{};
    mesh.indices = raw_indices;
    mesh.vertices = raw_vertices;

    benzene::Model model{};
    model.mesh = mesh;
    model.pos = {0, 0, 0.512};
    model.scale = {1, 1, 0};
    model.texture = benzene::Texture::load_from_file("../engine/resources/sample_texture.jpg");

    benzene::Model model2{};
    model2.mesh = mesh;
    model2.pos = {0, 0, 0};
    model2.scale = {1, 1, 0};
    model2.texture = benzene::Texture::load_from_file("../engine/resources/wall.jpg");

    engine.add_model(&model);
    engine.add_model(&model2);

    engine.run([&](benzene::FrameData& data){
        ImGui::Begin("Test");

        ImGui::TextUnformatted("Model 1\n");
        ImGui::SliderFloat3("Position 1: ", &model.pos.x, -5, 5);
        ImGui::SliderFloat3("Rotation 1: ", &model.rotation.x, 0, 360);
        ImGui::SliderFloat3("Scale 1: ", &model.scale.x, 1, 5);

        ImGui::TextUnformatted("Model 2\n");
        ImGui::SliderFloat3("Position 2: ", &model2.pos.x, -5, 5);
        ImGui::SliderFloat3("Rotation 2: ", &model2.rotation.x, 0, 360);
        ImGui::SliderFloat3("Scale 2: ", &model2.scale.x, 1, 5);

        ImGui::End();

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
