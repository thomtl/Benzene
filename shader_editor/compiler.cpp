#include "compiler.hpp"
#include <cassert>
#include <functional>
#include <stack>
#include "../engine/core/format.hpp"

/*static int parse_node(FILE* file, IdGen& id_gen, Graph& graph, Node& node){
    std::vector<int> in_vars{};
    for(auto& attr : node.in)
        for(auto& [id, link] : links)
            if(link.to == &attr)
                in_vars.push_back(parse_node(file, id_gen, links, *link.from->parent));

    int id = id_gen.next();
    if(node.name == "Input"){
        assert(node.in.size() == 0);
        assert(node.out.size() == 1);

        auto str = format_to_str("%{:d} := 0 // In\n", id);
        fwrite(str.c_str(), str.size(), 1, file);
    } else if(node.name == "Output"){
        assert(node.in.size() == 1);
        assert(node.out.size() == 0);

        auto str = format_to_str("Out %{:d}\n", in_vars[0]);
        fwrite(str.c_str(), str.size(), 1, file);
    } else if(node.name == "Add"){
        assert(node.in.size() == 2);
        assert(node.out.size() == 1);

        auto str = format_to_str("%{:d} := add %{:d} %{:d}\n", id, in_vars[0], in_vars[1]);
        fwrite(str.c_str(), str.size(), 1, file);
    } else if(node.name == "Subtract"){
        assert(node.in.size() == 2);
        assert(node.out.size() == 1);
        
        auto str = format_to_str("%{:d} := sub %{:d} %{:d}\n", id, in_vars[0], in_vars[1]);
        fwrite(str.c_str(), str.size(), 1, file);
    } else {
        print("Unknown Node {}\n", node.name);
    }

    return id;
}*/

void Compiler::compile(Graph& graph, int root_id, Node& root){
    assert(root.name == "Output");
    assert(root.in.size() == 1);
    assert(root.out.size() == 0);

    IdGen ssa_id_gen{};

    //parse_node(file, ssa_id_gen, graph, root);

    std::stack<int> preorder{}, postorder{};

    preorder.push(root_id);
    while(!preorder.empty()){
        int node_id = preorder.top();
        Node& node = graph.node(node_id);

        preorder.pop();

        postorder.push(node_id);
        for(auto& [attr_id, attr] : node.in){
            for(auto& edge : graph.edges_to_node[attr_id]){
                const int neighbour = graph.edge(edge).opposite(attr_id);
                assert(neighbour != root_id);

                auto& [result_id, result] = *std::find_if(graph.nodes.begin(), graph.nodes.end(), [neighbour](const auto& node){
                    return node.second.has_attr(neighbour);
                });

                preorder.push(result_id);
            }
        }
    }

    print("-------------------------------\n");

    std::stack<int> value_ids{};

    auto push = [&value_ids](int v){ 
        value_ids.push(v); 
    };
    
    auto pop = [&value_ids]() -> int { 
        int v = value_ids.top(); 
        value_ids.pop(); 
        return v; 
    };


    while(!postorder.empty()){
        int id = postorder.top();
        postorder.pop();

        auto& node = graph.node(id);

        if(node.name == "Input"){
            assert(node.in.size() == 0);
            assert(node.out.size() == 1);

            int id = ssa_id_gen.next();
            print("%{:d} := 0 // In\n", id);
            push(id);
        } else if(node.name == "Output"){
            assert(node.in.size() == 1);
            assert(node.out.size() == 0);

            int id = pop();
            print("Out %{:d}\n", id);
        } else if(node.name == "Add"){
            assert(node.in.size() == 2);
            assert(node.out.size() == 1);
            
            int a = pop();
            int b = pop();
            int id = ssa_id_gen.next();
            push(id);
            print("%{:d} := add %{:d} %{:d}\n", id, b, a);
        } else if(node.name == "Subtract"){
            assert(node.in.size() == 2);
            assert(node.out.size() == 1);


            int a = pop();
            int b = pop();
            int id = ssa_id_gen.next();
            push(id);
            print("%{:d} := sub %{:d} %{:d}\n", id, b, a);
        } else {
            print("Unknown Node {}\n", node.name);
        }
    }

    print("-------------------------------\n");
}