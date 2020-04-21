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
    this->backend.~unique_ptr();

    glfwDestroyWindow(window);

    glfwTerminate();
}

benzene::ModelId benzene::Instance::add_model(benzene::Model* model){
    auto id = id_gen.next();
    this->render_models[id] = model;
    return id;
}