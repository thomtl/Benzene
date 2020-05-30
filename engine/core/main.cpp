#include <benzene/benzene.hpp>
#include <iostream>

#ifdef BENZENE_VULKAN
#include "../backends/vulkan/core.hpp"
#endif

#ifdef BENZENE_OPENGL
#include "../backends/opengl/core.hpp"
#endif
#include "display.hpp"

#include "format.hpp"

benzene::Texture benzene::Texture::load_from_file(const std::string& filename, const std::string& shader_name, benzene::Texture::Gamut gamut){
    int width, height, channels;
    stbi_set_flip_vertically_on_load(true);

    auto* data = stbi_load(filename.c_str(), &width, &height, &channels, STBI_default);
    if(!data) {
        print("benzene/texture: Failed to load texture data, error: {:s}\n", stbi_failure_reason());
        throw std::runtime_error("benzene/texture: Failed to load image data from file");
    }

    Texture tex{};
    tex.shader_name = shader_name;
    tex.width = width;
    tex.height = height;
    tex.channels = channels;
    tex.gamut = gamut;

    size_t size = width * height * channels;
    tex.data.resize(size);
    memcpy(tex.data.data(), data, size);

    stbi_image_free(data);

    return tex;
}

benzene::Texture benzene::Texture::load_from_colour(glm::vec3 colour, const std::string& shader_name){
    Texture tex{};
    tex.shader_name = shader_name;
    tex.width = 1;
    tex.height = 1;
    tex.channels = 3;
    tex.gamut = Gamut::Linear;

    tex.data.push_back(colour.r * 255);
    tex.data.push_back(colour.g * 255);
    tex.data.push_back(colour.b * 255);

    return tex;
}

void benzene::Batch::load_mesh_data_from_file(const std::string& folder, const std::string& file){
    assert(folder[folder.size() - 1] == '/');
    assert(file[0] != '/');
    const std::string file_path = folder + file;

    tinyobj::attrib_t attrib{};
    std::vector<tinyobj::shape_t> shapes{};
    std::vector<tinyobj::material_t> materials{};
    std::string warning{}, error{};
    
    if(!tinyobj::LoadObj(&attrib, &shapes, &materials, &warning, &error, file_path.c_str(), folder.c_str(), true)){
        print("benzene/Mesh: Failed to load object from {:s}, warning: {:s}, error: {:s}n\n", file_path, warning, error);
        throw std::runtime_error("benzene/Mesh: Failed to load object");
    }
    
    bool have_surface_normals = true;
    if(attrib.normals.size() == 0)
        have_surface_normals = false;//throw std::runtime_error("benzene/Model: Tried to load model with no normal vectors\n");
    
    if(attrib.texcoords.size() == 0)
        throw std::runtime_error("benzene/Model: Tried to load model with no uv's\n");
    
    auto process_mesh = [have_surface_normals, &attrib](const tinyobj::shape_t& shape) -> benzene::Mesh {
        std::vector<benzene::Mesh::Vertex> vertices{};
        for(const auto& index : shape.mesh.indices){
            Mesh::Vertex vertex{};

            vertex.pos = {
                attrib.vertices[3 * index.vertex_index],
                attrib.vertices[3 * index.vertex_index + 1],
                attrib.vertices[3 * index.vertex_index + 2]
            };

            vertex.uv = {
                attrib.texcoords[2 * index.texcoord_index],
                attrib.texcoords[2 * index.texcoord_index + 1]
            };

            if(have_surface_normals){
                vertex.normal = {
                    attrib.normals[3 * index.normal_index],
                    attrib.normals[3 * index.normal_index + 1],
                    attrib.normals[3 * index.normal_index + 2]
                };                
            }

            vertices.push_back(vertex);
        }

        if(!have_surface_normals){
            assert((vertices.size() % 3) == 0);

            for(size_t i = 0; i < vertices.size(); i += 3){
                auto& v1 = vertices[i];
                auto& v2 = vertices[i + 1];
                auto& v3 = vertices[i + 2];

                auto calculate_surface_normal = [](glm::vec3 p1, glm::vec3 p2, glm::vec3 p3) -> glm::vec3 {
                    // https://www.khronos.org/opengl/wiki/Calculating_a_Surface_Normal#Algorithm
                    auto U = p2 - p1;
                    auto V = p3 - p1;

                    return glm::normalize(glm::cross(U, V));
                };

                auto calculate_surface_tangent = [](Mesh::Vertex v0, Mesh::Vertex v1, Mesh::Vertex v2) -> glm::vec3 {
                    auto dv1 = v1.pos - v0.pos;
                    auto dv2 = v2.pos - v0.pos;

                    auto duv1 = v1.uv - v0.uv;
                    auto duv2 = v2.uv - v0.uv;

                    auto f = 1.0f / (duv1.x * duv2.y - duv1.y * duv2.x);

                    return glm::normalize((dv1 * duv2.y - dv2 * duv1.y) * f);
                };

                auto normal = calculate_surface_normal(v1.pos, v2.pos, v3.pos);
                auto tangent = calculate_surface_tangent(v1, v2, v3);

                v1.normal = v2.normal = v3.normal = normal;
                v1.tangent = v2.tangent = v3.tangent = tangent;
            }
        }

        Mesh mesh{};
        std::unordered_map<benzene::Mesh::Vertex, uint32_t> unique_vertices{};
        for(auto& vertex : vertices){
            if(unique_vertices.count(vertex) == 0){
                unique_vertices[vertex] = mesh.vertices.size();
                mesh.vertices.push_back(vertex);
            }
            mesh.indices.push_back(unique_vertices[vertex]);
        }

        if constexpr (true)
            if(mesh.vertices.size() < vertices.size())
                print("benzene/Model: Optimized submesh from {:d} vertices to {:d} unique vertices\n", vertices.size(), mesh.vertices.size());

        return mesh;
    };

    this->meshes.clear();

    for(const auto& shape : shapes)
        this->meshes.push_back(process_mesh(shape));

    if constexpr (true){
        size_t tris = 0;
        for(const auto& mesh : this->meshes)
            tris += (mesh.indices.size() / 3);
        print("benzene/Model: Loaded model with {:d} submesh(es) and {:d} triangles\n", this->meshes.size(), tris);
    }
}
//#include <GLFW/glfw3.h>

benzene::Instance::Instance(const char* name, size_t width, size_t height): width{width}, height{height} {
    print("benzene: Starting\n");

    glfwInit();

    #if defined(BENZENE_VULKAN)
    vulkan::Backend::glfw_window_hints();
    #elif defined(BENZENE_OPENGL)
    opengl::Backend::glfw_window_hints();
    #endif
    auto& display = Display::instance();
    display.set_hint(GLFW_RESIZABLE, GLFW_TRUE);
    display.create_window({name}, width, height);
    
    #if defined(BENZENE_VULKAN)
    this->backend = std::make_unique<vulkan::Backend>(name);
    #elif defined(BENZENE_OPENGL)
    display.make_context_current();
    this->backend = std::make_unique<opengl::Backend>(name);
    #endif

    display.set_window_backend(this->backend.get());
}

void benzene::Instance::run(std::function<void(benzene::FrameData&)> functor){
    FrameData frame_data{};
    while(!glfwWindowShouldClose(Display::instance()()) && !frame_data.should_exit){
        glfwPollEvents();
        this->backend->imgui_update();
        ImGui::NewFrame();
        functor(frame_data);

        if(frame_data.display_debug_window)
            this->backend->draw_debug_window();

        ImGui::Render();
        this->backend->frame_update(render_batches, frame_data);

        #ifdef BENZENE_OPENGL
        Display::instance().swap_buffers();
        #endif
    }
    this->backend->end_run();
}

benzene::Instance::~Instance(){
    delete this->backend.release();
    Display::instance().clean();

    glfwTerminate();
}

benzene::ModelId benzene::Instance::add_batch(benzene::Batch* model){
    auto id = id_gen.next();
    this->render_batches[id] = model;
    return id;
}

void benzene::Instance::set_property(benzene::BackendProperties property, glm::vec4 v){
    this->backend->set_property(property, v);
}