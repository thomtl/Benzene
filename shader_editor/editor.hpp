#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <cassert>
#include "libs/imnodes.h"

namespace ImNodes = imnodes;

struct IdGen {
    IdGen(): curr{0} {}

    int next(){
        return curr++;
    }
    private:
    int curr;
};

struct Edge {
    Edge() = default;
    Edge(int from, int to): from{from}, to{to} {}

    int from, to;

    int opposite(int id) {
        return (id == from) ? to : from;
    }
};

struct Node;

struct Attribute {
    enum class Direction {
        In,
        Out
    } direction;
    std::string name;
    int parent_id;
};

struct Node {
    std::string name;
    std::unordered_map<int, Attribute> in;
    std::unordered_map<int, Attribute> out;

    bool has_attr(int attr_id) const {
        return in.count(attr_id) || out.count(attr_id);
    }
};


class Graph {
    public:
    Graph() = default;
    Graph(IdGen& id_gen): id_gen{&id_gen} {}
    using AdjacencyArray = std::vector<int>;

    Node& node(int id){
        assert(nodes.count(id) == 1);
        return nodes[id];
    }

    Edge& edge(int id){
        assert(edges.count(id) == 1);
        return edges[id];
    }

    AdjacencyArray& get_edges_from_node(int id){
        return edges_from_node[id];
    }

    AdjacencyArray& get_edges_to_node(int id){
        return edges_to_node[id];
    }

    void add_node(int id, const Node& node){
        nodes.emplace(std::make_pair(id, node));
        //edges_from_node.emplace(std::make_pair(id, AdjacencyArray{}));
        //edges_to_node.emplace(std::make_pair(id, AdjacencyArray{}));
    }

    void erase_node(const int id){
        std::vector<int> edges_to_erase{};
        for(auto& [attr_id, attr] : nodes[id].in)
            for(auto edge : edges_to_node[attr_id])
                edges_to_erase.push_back(edge);

        for(auto& [attr_id, attr] : nodes[id].out)
            for(auto edge : edges_from_node[attr_id])
                edges_to_erase.push_back(edge);

        for(auto edge : edges_to_erase)
            erase_edge(edge);

        nodes.erase(id);
        edges_from_node.erase(id);
        edges_to_node.erase(id);
    }

    int add_edge(int from, int to){
        int id = id_gen->next();

        edges[id] = Edge{from, to};
        edges_from_node[from].push_back(id);
        edges_to_node[to].push_back(id);

        return id;
    }

    void erase_edge(const int id){
        auto edge = edges.find(id);
        assert(edge != edges.end());

        {
            auto& edges_from = edges_from_node[edge->second.from];
            edges_from.erase(std::find(edges_from.begin(), edges_from.end(), id));
        }

        {
            auto& edges_to = edges_to_node[edge->second.to];
            edges_to.erase(std::find(edges_to.begin(), edges_to.end(), id));
        }

        edges.erase(edge);
    }

    std::unordered_map<int, Node> nodes;
    std::unordered_map<int, Edge> edges;
    std::unordered_map<int, AdjacencyArray> edges_from_node;
    std::unordered_map<int, AdjacencyArray> edges_to_node;

    private:
    IdGen* id_gen;
};

class Editor {
    public:
    Editor() = default;
    Editor(const std::string& name): name{name}, wants_to_exit{false} {
        context = ImNodes::EditorContextCreate();
        graph = Graph{id_gen};
    }

    bool exit(){
        return wants_to_exit;
    }

    void clean(){
        ImNodes::EditorContextFree(context);
    }
    void show();


    private:
    void compile();

    std::string name;
    bool wants_to_exit;

    Graph graph;

    ImNodes::EditorContext* context;

    static IdGen id_gen;
};