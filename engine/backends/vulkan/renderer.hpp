#pragma once

#include <vulkan/vulkan.hpp>

#include "memory.hpp"
#include "shader.hpp"
#include "swap_chain.hpp"
#include "../../core/format.hpp"
#include "../../core/utils.hpp"

#include <glm/glm.hpp>
#include <array>

namespace benzene::vulkan
{
    struct vertex {
        glm::vec2 pos;
        glm::vec3 colour;

        static vk::VertexInputBindingDescription get_binding_description(){
            vk::VertexInputBindingDescription info{};
            info.binding = 0;
            info.stride = sizeof(vertex);
            info.inputRate = vk::VertexInputRate::eVertex;
            return info;
        }

        static std::array<vk::VertexInputAttributeDescription, 2> get_attribute_descriptions(){
            std::array<vk::VertexInputAttributeDescription, 2> desc{};
            desc[0].binding = 0;
            desc[0].location = 0;
            desc[0].format = vk::Format::eR32G32Sfloat;
            desc[0].offset = offsetof(vertex, pos);

            desc[1].binding = 0;
            desc[1].location = 1;
            desc[1].format = vk::Format::eR32G32B32Sfloat;
            desc[1].offset = offsetof(vertex, colour);

            return desc;
        }
    };

    class vertex_buffer {
        public:
        vertex_buffer(): vertex_buf{} {}
        vertex_buffer(vk::Device dev, vma::Allocator allocator, vk::Queue queue, vk::CommandPool cmd, std::vector<vertex> vertices);

        void clean();

        vk::Buffer& vertex_buffer_handle(){
            return vertex_buf.handle();
        }

        private:
        buffer vertex_buf;
    };

    class index_buffer {
        public:
        index_buffer(): index_buf{} {}
        index_buffer(vk::Device dev, vma::Allocator allocator, vk::Queue queue, vk::CommandPool cmd, std::vector<uint16_t> vertices);

        void clean();

        vk::Buffer& index_buffer_handle(){
            return index_buf.handle();
        }

        private:
        buffer index_buf;
    };

    class render_pass {
        public:
        render_pass();
        render_pass(vk::Device device, swap_chain* swapchain);

        void clean();
        vk::RenderPass& handle();

        private:
        vk::RenderPass renderpass;
        vk::Device device;
        swap_chain* swapchain;
    };

    class render_pipeline {
        public:
        render_pipeline();
        render_pipeline(vk::Device device, swap_chain* swapchain);

        render_pass& get_render_pass(){
            return renderpass;
        }

        vk::Pipeline& get_pipeline(){
            return pipeline;
        }

        void clean();

        private:
        vk::Device device;
        swap_chain* swapchain;

        render_pass renderpass;
        
        vk::PipelineLayout layout;
        vk::Pipeline pipeline;
    };
} // namespace benzene::vulkan
