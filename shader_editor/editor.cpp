#include "editor.hpp"
#include "compiler.hpp"
#include <iostream>
#include <benzene/benzene.hpp>

IdGen Editor::id_gen;

void Editor::show(){
    ImNodes::EditorContextSet(context);
    
    ImGui::Begin(name.c_str(), NULL, ImGuiWindowFlags_MenuBar);
    if(ImGui::BeginMenuBar()){
        if(ImGui::MenuItem("Exit")){
           this->wants_to_exit = true;
        }
        if(ImGui::MenuItem("Compile")){
            this->compile();
        }
        ImGui::EndMenuBar();
    }
    ImNodes::BeginNodeEditor();

    for(auto& [id, node] : graph.nodes){
        ImNodes::BeginNode(id);

        ImNodes::BeginNodeTitleBar();
        ImGui::Text("%s", node.name.c_str());
        ImNodes::EndNodeTitleBar();

        for(auto& [id, in] : node.in){
            ImNodes::BeginInputAttribute(id, ImNodes::PinShape_QuadFilled);

            ImGui::Text("%s", in.name.c_str());

            ImNodes::EndAttribute();
        }

        for(auto& [id, out] : node.out){
            ImNodes::BeginOutputAttribute(id, ImNodes::PinShape_CircleFilled);

            ImGui::Text("%s", out.name.c_str());

            ImNodes::EndAttribute();
        }

        ImNodes::EndNode();
    }

    for(auto& [id, link] : graph.edges)
        ImNodes::Link(id, link.from, link.to);

    const bool open_popup = (!ImGui::IsAnyItemHovered() && ImNodes::IsEditorHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right));
    ImNodes::EndNodeEditor();

    if(open_popup)
        ImGui::OpenPopup("Context Menu");

    if(ImGui::BeginPopup("Context Menu")){
        int new_id = -1;
        auto pos = ImGui::GetMousePosOnOpeningCurrentPopup();

        ImGui::Text("Create new node");

        if(ImGui::MenuItem("In")){
            new_id = id_gen.next();
            Node node{};
            node.name = "Input";

            Attribute result{};
            result.name = "out";
            result.direction = Attribute::Direction::Out;

            node.out[id_gen.next()] = result;

            graph.add_node(new_id, node);
        }

        if(ImGui::MenuItem("Out")){
            new_id = id_gen.next();
            Node node{};
            node.name = "Output";

            Attribute result{};
            result.name = "in";
            result.direction = Attribute::Direction::In;
            result.parent_id = new_id;

            node.in[id_gen.next()] = result;

            graph.add_node(new_id, node);
        }

        if(ImGui::MenuItem("Add")){
            new_id = id_gen.next();
            Node node{};
            node.name = "Add";

            Attribute a{};
            Attribute b{};
            a.name = "a";
            b.name = "b";
            a.direction = Attribute::Direction::In;
            b.direction = Attribute::Direction::In;
            a.parent_id = new_id;
            b.parent_id = new_id;

            Attribute result{};
            result.name = "out";
            result.parent_id = new_id;

            node.in[id_gen.next()] = a;
            node.in[id_gen.next()] = b;
            node.out[id_gen.next()] = result;

            graph.add_node(new_id, node);
        }

        if(ImGui::MenuItem("Subtract")){
            new_id = id_gen.next();
            Node node{};
            node.name = "Subtract";

            Attribute a{};
            Attribute b{};
            a.name = "a";
            b.name = "b";
            a.parent_id = new_id;
            b.parent_id = new_id;            

            Attribute result{};
            result.name = "out";
            result.parent_id = new_id;

            node.in[id_gen.next()] = a;
            node.in[id_gen.next()] = b;
            node.out[id_gen.next()] = result;

            graph.add_node(new_id, node);
        }

        ImGui::EndPopup();

        if(new_id != -1)
            ImNodes::SetNodeScreenSpacePos(new_id, pos);
    }

    if(int n_selected = ImNodes::NumSelectedLinks(); n_selected > 0 && ImGui::IsKeyReleased(ImGui::GetKeyIndex(ImGuiKey_Delete))){
        std::vector<int> selected_links{};
        selected_links.resize(n_selected, -1);
        ImNodes::GetSelectedLinks(selected_links.data());

        for(const auto link : selected_links)
            graph.erase_edge(link);
    }

    if(int n_selected = ImNodes::NumSelectedNodes(); n_selected > 0 && ImGui::IsKeyReleased(ImGui::GetKeyIndex(ImGuiKey_Delete))){
        std::vector<int> selected_nodes{};
        selected_nodes.resize(n_selected, -1);
        ImNodes::GetSelectedNodes(selected_nodes.data());

        for(const auto node : selected_nodes)
            graph.erase_node(node);
    }

    if(int to, from; ImNodes::IsLinkCreated(&from, &to)){
        graph.add_edge(from, to);
    }

    ImGui::End();
}

void Editor::compile(){
    auto& [id, root] = *std::find_if(graph.nodes.begin(), graph.nodes.end(), [](const auto& node){
        return node.second.name == "Output";
    });
    Compiler::compile(graph, id, root);
}