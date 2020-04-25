#include <benzene/benzene.hpp>
#include <iostream>

#ifdef BENZENE_VULKAN
#include "../backends/vulkan/core.hpp"
#endif

#ifdef BENZENE_OPENGL
#include "../backends/opengl/core.hpp"
#endif

#include "format.hpp"

benzene::Texture benzene::Texture::load_from_file(const std::string& filename){
    int width, height, channels;
    stbi_set_flip_vertically_on_load(true);

    auto* data = stbi_load(filename.c_str(), &width, &height, &channels, STBI_rgb_alpha);
    if(!data) {
        print("vulkan/texture: Failed to load texture data, error: {:s}\n", stbi_failure_reason());
        throw std::runtime_error("vulkan/texture: Failed to load image data from file");
    }

    Texture tex{};
    tex.width = width;
    tex.height = height;
    tex.channels = channels;

    size_t size = width * height * 4;
    tex.data.resize(size);
    memcpy(tex.data.data(), data, size);

    stbi_image_free(data);

    return tex;
}

benzene::Mesh benzene::Mesh::load_from_file(const std::string& folder, const std::string& file){
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
    
    bool manually_calculate_surface_normals = false;
    if(attrib.normals.size() == 0)
        manually_calculate_surface_normals = true;//throw std::runtime_error("benzene/Model: Tried to load model with no normal vectors\n");
    
    if(attrib.texcoords.size() == 0)
        throw std::runtime_error("benzene/Model: Tried to load model with no uv's\n");

    Mesh mesh{};

    std::unordered_map<Vertex, uint32_t> unique_vertices{};
    for(const auto& shape : shapes){
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

            if(attrib.colors.size() == attrib.vertices.size())
                vertex.colour = glm::vec3{attrib.colors[3 * index.vertex_index], attrib.colors[3 * index.vertex_index + 1], attrib.colors[3 * index.vertex_index + 2]};
            else
                vertex.colour = glm::vec3{1.0f, 1.0f, 1.0f};

            if(!manually_calculate_surface_normals){
                vertex.normal = {
                    attrib.normals[3 * index.normal_index],
                    attrib.normals[3 * index.normal_index + 1],
                    attrib.normals[3 * index.normal_index + 2]
                };

                if(unique_vertices.count(vertex) == 0){
                    unique_vertices[vertex] = mesh.vertices.size();
                    mesh.vertices.push_back(vertex);
                }
                mesh.indices.push_back(unique_vertices[vertex]);
            } else { // If we need to manually calculate normals we can't use share vertices like this, since the normals might differ
                mesh.vertices.push_back(vertex);
                mesh.indices.push_back(mesh.indices.size());
            }    
        }
    }

    if(manually_calculate_surface_normals){
        assert((mesh.indices.size() % 3) == 0);

        for(size_t i = 0; i < mesh.indices.size(); i += 3){
            auto& v1 = mesh.vertices[i];
            auto& v2 = mesh.vertices[i + 1];
            auto& v3 = mesh.vertices[i + 2];

            // https://www.khronos.org/opengl/wiki/Calculating_a_Surface_Normal#Algorithm
            auto U = v2.pos - v1.pos;
            auto V = v3.pos - v1.pos;

            auto normal_x = (U.y * V.z) - (U.z * V.y);
            auto normal_y = (U.z * V.x) - (U.x * V.z);
            auto normal_z = (U.x * V.y) - (U.y * V.x);

            

            auto normal = glm::normalize(glm::vec3{normal_x, normal_y, normal_z});

            v1.normal = normal;
            v2.normal = normal;
            v3.normal = normal;
        }
    }

    return mesh;
}
//#include <GLFW/glfw3.h>

benzene::Instance::Instance(const char* name, size_t width, size_t height): width{width}, height{height} {
    print("benzene: Starting\n");

    glfwInit();

    #ifdef BENZENE_VULKAN
    vulkan::Backend::glfw_window_hints();
    #endif

    #ifdef BENZENE_OPENGL
    opengl::Backend::glfw_window_hints();
    #endif

    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    this->window = glfwCreateWindow(this->width, this->height, name, nullptr, nullptr);
    if(!this->window){
        const char* err;
        glfwGetError(&err);
        print("benzene: Couldn't create GLFW window, error {:s}\n", err);
        return;
    }
    
    #ifdef BENZENE_OPENGL
    glfwMakeContextCurrent(this->window);
    #endif

    glfwSetFramebufferSizeCallback(window, [](GLFWwindow* window, int width, int height){
        auto& backend = *(IBackend*)glfwGetWindowUserPointer(window);
        backend.framebuffer_resize_callback(width, height);
    });
    glfwSetMouseButtonCallback(window, [](GLFWwindow* window, int button, int action, [[maybe_unused]] int mods){
        auto& backend = *(IBackend*)glfwGetWindowUserPointer(window);
        backend.mouse_button_callback(button, action);
    });
    glfwSetCursorPosCallback(window, [](GLFWwindow* window, double x, double y){
        auto& backend = *(IBackend*)glfwGetWindowUserPointer(window);
        backend.mouse_pos_callback(x, y);
    });
    glfwSetScrollCallback(window, [](GLFWwindow* window, double xoffset, double yoffset){
        auto& backend = *(IBackend*)glfwGetWindowUserPointer(window);
        backend.mouse_scroll_callback(xoffset, yoffset);
    });
    
    #ifdef BENZENE_VULKAN
    this->backend = std::make_unique<vulkan::Backend>(name, this->window);
    #endif

    #ifdef BENZENE_OPENGL
    this->backend = std::make_unique<opengl::Backend>(name, this->window);
    #endif

    glfwSetWindowUserPointer(window, (void*)&(*this->backend));
}

void benzene::Instance::run(std::function<void(benzene::FrameData&)> functor){
    FrameData frame_data{};
    while(!glfwWindowShouldClose(window) && !frame_data.should_exit){
        glfwPollEvents();
        this->backend->imgui_update();
        ImGui::NewFrame();
        functor(frame_data);

        if(frame_data.display_debug_window)
            this->backend->draw_debug_window();

        ImGui::Render();
        this->backend->frame_update(render_models);

        #ifdef BENZENE_OPENGL
        glfwSwapBuffers(window);
        #endif
    }
    this->backend->end_run();
}

benzene::Instance::~Instance(){
    delete this->backend.release();

    glfwDestroyWindow(window);

    glfwTerminate();
}

benzene::ModelId benzene::Instance::add_model(benzene::Model* model){
    auto id = id_gen.next();
    this->render_models[id] = model;
    return id;
}