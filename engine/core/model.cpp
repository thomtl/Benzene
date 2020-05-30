#include <benzene/benzene.hpp>
#include "../libs/imgui/imgui.h"
using namespace benzene;

#include "format.hpp"

void help_marker(const char* description){
    ImGui::TextDisabled("(?)");
    if(ImGui::IsItemHovered()){
        ImGui::BeginTooltip();
        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35);

        ImGui::TextUnformatted(description);

        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }
}

void Batch::show_inspector(const std::string& window_name, bool* opened, size_t i){
    ImGui::Begin(window_name.c_str(), opened);

    if(ImGui::CollapsingHeader("Transform")){
        float step = 0.1, step_fast = 1.0;
        ImGui::Indent();
        ImGui::Text("Position: ");
        ImGui::SameLine(0, 0);
        ImGui::InputScalarN("##Position", ImGuiDataType_Float, &transforms[i].pos.x, 3, &step, &step_fast);
    
        ImGui::Text("Rotation: ");
        ImGui::SameLine(0, 0);
        ImGui::InputScalarN("##Rotation", ImGuiDataType_Float, &transforms[i].rotation.x, 3, &step, &step_fast);

        ImGui::Text("Scale:    ");
        ImGui::SameLine(0, 0);
        ImGui::InputScalarN("##Scale", ImGuiDataType_Float, &transforms[i].scale.x, 3, &step, &step_fast);
        ImGui::Unindent();
    }

    if(ImGui::CollapsingHeader("Meshes")){
        ImGui::Indent();
        for(size_t i = 0; i < meshes.size(); i++){
            if(ImGui::CollapsingHeader(format_to_str("Mesh {:d}", i).c_str())){
                ImGui::Indent();
                if(ImGui::CollapsingHeader("Material")){
                    ImGui::Indent();

                    ImGui::Text("Shininess: ");
                    ImGui::SameLine(0, 0);
                    ImGui::InputScalar("##Shininess", ImGuiDataType_Float, &meshes[i].material.shininess);

                    ImGui::Unindent();
                }

                if(ImGui::CollapsingHeader("Vertices")){
                    ImGui::Indent();
                    
                    for(size_t j = 0; j < meshes[i].vertices.size(); j++){
                        auto& vertex = meshes[i].vertices[j];
                        if(ImGui::TreeNode(format_to_str("{:d}", j).c_str())){
                            ImGui::Text("Position:  ");
                            ImGui::SameLine(0, 0);
                            ImGui::InputScalarN("##Position", ImGuiDataType_Float, &vertex.pos.x, 3);

                            ImGui::Text("Normal:    ");
                            ImGui::SameLine(0, 0);
                            ImGui::InputScalarN("##Normal", ImGuiDataType_Float, &vertex.normal.x, 3);

                            ImGui::Text("Tangent:   ");
                            ImGui::SameLine(0, 0);
                            ImGui::InputScalarN("##Tangent", ImGuiDataType_Float, &vertex.tangent.x, 3);

                            auto bitangent = glm::cross(vertex.normal, vertex.tangent);
                            ImGui::TextDisabled("Bitangent: %f %f %f", bitangent.x, bitangent.y, bitangent.z);

                            ImGui::Text("UV:        ");
                            ImGui::SameLine(0, 0);
                            ImGui::InputScalarN("##UV", ImGuiDataType_Float, &vertex.uv.s, 2);

                            ImGui::TreePop();
                        }    
                    }

                    ImGui::Unindent();
                }

                if(ImGui::CollapsingHeader("Textures")){
                    ImGui::Indent();

                    for(size_t j = 0; j < meshes[i].textures.size(); j++){
                        auto& texture = meshes[i].textures[j];

                        if(ImGui::TreeNode(format_to_str("Texture {:d}", j).c_str())){
                            const auto [width, height] = texture.dimensions();

                            ImGui::Text("Name: %s", texture.get_shader_name().c_str());
                            ImGui::Text("Dimensions %dx%d", width, height);
                            ImGui::Text("Channels: %d", texture.get_channels());
                            ImGui::Text("Gamut: %s", Texture::gamut_to_str(texture.get_gamut()));
                            
                            ImGui::TreePop();
                        }
                    }

                    ImGui::Unindent();
                }

                ImGui::Unindent();
            }
        }

        if(ImGui::Button("Update Mesh"))
            this->update();
        ImGui::SameLine();
        help_marker("The backend needs a heads up that some vertex, index or texture data changed, this gives it that");

            
        ImGui::Unindent();
    }

    ImGui::End();
}