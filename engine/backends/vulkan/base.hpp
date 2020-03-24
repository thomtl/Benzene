#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vulkan/vulkan.hpp>

#include <utility>
#include <stdexcept>
#include <vector>
#include <array>
#include <algorithm>

#include <benzene/benzene.hpp>

#include "libs/vk_mem_alloc.hpp"
#include "../../core/format.hpp"
#include "../../core/utils.hpp"

namespace benzene::vulkan
{
    struct Instance {
        GLFWwindow* window;
        vk::Instance instance;
        vk::PhysicalDevice gpu;
        vk::Device device;
        vk::CommandPool command_pool;
        vk::SurfaceKHR surface;
        vma::Allocator allocator;

        struct queue {
            queue(): family{0xFFFFFFFF}, handle{nullptr} {}
            queue(uint32_t family, vk::Queue&& queue): family{family}, handle{std::move(queue)} {}

            uint32_t family;
            vk::Queue& operator()(){
                return handle;
            }
            private:
            vk::Queue handle;
        };
        queue graphics, present;
    };
} // namespace benzene::vulkan
