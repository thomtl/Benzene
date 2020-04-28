#include <iostream>
#include <benzene/benzene.hpp>
#include "editor.hpp"
#include "libs/imnodes.h"
#include "../engine/core/format.hpp"

namespace ImNodes = imnodes;

IdGen id_gen{};

std::unordered_map<int, Editor> editors{};

int main([[maybe_unused]] int argc, [[maybe_unused]] char const *argv[])
{
    benzene::Instance engine{"Benzene Shader Editor", 800, 600};
    ImNodes::Initialize();
    //engine.get_backend().set_fps_cap(true, 60);

    engine.run([&](benzene::FrameData& data){
        for(auto& [id, editor] : editors)
            editor.show();

        for(auto& [id, editor] : editors){
            if(editor.exit()){
                editor.clean();
                editors.erase(id);
            }
        }

        if(ImGui::BeginMainMenuBar()){
            if(ImGui::BeginMenu("File")){
                if(ImGui::MenuItem("New Editor")){
                    auto id = id_gen.next();
                    editors[id] = Editor{format_to_str("Editor {:d}", id)};
                }
                if(ImGui::MenuItem("Exit"))
                    data.should_exit = true;
                ImGui::EndMenu();
            }
            ImGui::EndMainMenuBar();
        }
    });

    for(auto& [id, editor] : editors)
        editor.clean();

    ImNodes::Shutdown();
    return 0;
}
