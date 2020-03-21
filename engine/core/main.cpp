#include <benzene/benzene.hpp>
#include <iostream>

#include "../backends/vulkan/core.hpp"
#include "format.hpp"

benzene::instance::instance(const char* name, size_t width, size_t height): width{width}, height{height} {
    print("benzene: Starting\n");

    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    this->window = glfwCreateWindow(this->width, this->height, name, nullptr, nullptr);
    glfwSetFramebufferSizeCallback(window, [](GLFWwindow* window, int width, int height){
        auto& backend = *(IBackend*)glfwGetWindowUserPointer(window);
        backend.framebuffer_resize_callback(width, height);
    });
    
    this->backend = std::make_unique<vulkan::backend>(name, this->window);
    glfwSetWindowUserPointer(window, (void*)&(*this->backend));
}

void benzene::instance::run(std::function<void(void)> functor){
    while(!glfwWindowShouldClose(window)){
        glfwPollEvents();
        functor();
        this->backend->frame_update();
    }
    this->backend->end_run();
}

benzene::instance::~instance(){
    this->backend.~unique_ptr();

    glfwDestroyWindow(window);

    glfwTerminate();
}