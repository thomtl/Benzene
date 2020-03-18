#include <benzene/benzene.hpp>
#include <iostream>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>

#include "../backends/vulkan/core.hpp"
#include "format.hpp"

benzene::instance::instance(size_t width, size_t height): width{width}, height{height} {
    print("benzene: Starting\n");

    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE); // TODO

    this->window = glfwCreateWindow(this->width, this->height, "Benzene", nullptr, nullptr);

    
    this->backend = std::make_unique<vulkan::backend>();
}

void benzene::instance::run(std::function<void(void)> functor){
    while(!glfwWindowShouldClose(window)){
        glfwPollEvents();
        functor();
    }
}

benzene::instance::~instance(){
    this->backend.~unique_ptr();

    glfwDestroyWindow(window);

    glfwTerminate();
}