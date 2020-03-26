#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vulkan/vulkan.hpp>

#include <utility>
#include <stdexcept>
#include <vector>
#include <array>
#include <algorithm>
#include <mutex>
#include <chrono>

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

        struct Queue {
            Queue(): family{0xFFFFFFFF}, handle{nullptr} {}
            Queue(uint32_t family, vk::Queue&& queue): family{family}, handle{std::move(queue)} {}

            uint32_t family;
            vk::Queue& operator()(){
                return handle;
            }
            private:
            vk::Queue handle;
        };
        Queue graphics, present;
    };

    struct CommandBuffer {
        CommandBuffer(Instance* instance, Instance::Queue* submit_queue): instance{instance}, submit_queue{submit_queue}, cmd{nullptr} {}

        void lock(){
            vk::CommandBufferAllocateInfo alloc_info{};
            alloc_info.level = vk::CommandBufferLevel::ePrimary;
            alloc_info.commandPool = instance->command_pool;
            alloc_info.commandBufferCount = 1;

            auto buffers = instance->device.allocateCommandBuffers(alloc_info);
            this->cmd = buffers[0];

            vk::CommandBufferBeginInfo begin_info{};
            begin_info.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;

            cmd.begin(begin_info);
        }

        void unlock(){
            cmd.end();

            vk::SubmitInfo submit_info{};
            submit_info.commandBufferCount = 1;
            submit_info.pCommandBuffers = &cmd;
            (*submit_queue)().submit({submit_info}, {nullptr});
            (*submit_queue)().waitIdle();

            instance->device.freeCommandBuffers(instance->command_pool, {cmd});
        }

        vk::CommandBuffer& handle(){
            return cmd;
        }

        private:
        Instance* instance;
        Instance::Queue* submit_queue;
        vk::CommandBuffer cmd;
    };
} // namespace benzene::vulkan
