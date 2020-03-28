#include <benzene/benzene.hpp>
#include <iostream>

#include "../backends/vulkan/core.hpp"
#include "format.hpp"

benzene::Instance::Instance(const char* name, size_t width, size_t height): width{width}, height{height} {
    print("benzene: Starting\n");

    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    this->window = glfwCreateWindow(this->width, this->height, name, nullptr, nullptr);
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
    
    this->backend = std::make_unique<vulkan::Backend>(name, this->window);
    glfwSetWindowUserPointer(window, (void*)&(*this->backend));
}

void benzene::Instance::run(std::function<void(benzene::FrameData&)> functor){
    FrameData frame_data{};
    frame_data.backend = this->backend.get();
    while(!glfwWindowShouldClose(window) && !frame_data.should_exit){
        glfwPollEvents();
        ImGui::NewFrame();
        functor(frame_data);

        if(frame_data.display_debug_window)
            this->backend->draw_debug_window();

        ImGui::Render();
        this->backend->frame_update(render_models);
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