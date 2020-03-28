#include <iostream>
#include <benzene/benzene.hpp>

int main([[maybe_unused]] int argc, [[maybe_unused]] char const *argv[])
{
    benzene::Instance engine{"Benzene-test", 800, 600};

    std::vector<benzene::Mesh::Vertex> raw_vertices = {
        {{-0.5f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}},
        {{0.5f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}},
        {{0.5f, 0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}},
        {{-0.5f, 0.5f, 0.0f}, {1.0f, 1.0f, 1.0f}}
    };
    
    std::vector<uint16_t> raw_indices = {
        0, 1, 2, 2, 3, 0
    };

    benzene::Mesh mesh{};
    mesh.indices = raw_indices;
    mesh.vertices = raw_vertices;

    benzene::Model model{};
    model.mesh = mesh;
    model.pos = {0, 0, 0};

    engine.add_model(&model);

    benzene::Model model2{};
    model2.mesh = mesh;
    model2.pos = {0, 0, 0};

    engine.add_model(&model2);

    engine.run([&model, &model2](benzene::FrameData& data){
        ImGui::Begin("Test");

        ImGui::TextUnformatted("This is a debug window made in the main loop\n");
        ImGui::SliderFloat3("Model 1: ", &model.pos.x, -5, 5);
        ImGui::SliderFloat3("Model 2: ", &model2.pos.x, -5, 5);

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
