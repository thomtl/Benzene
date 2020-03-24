#pragma once

#include "base.hpp"
#include "memory.hpp"
#include "shader.hpp"
#include "swap_chain.hpp"

#include <glm/glm.hpp>

namespace benzene::vulkan
{
    struct Vertex {
        glm::vec2 pos;
        glm::vec3 colour;

        static vk::VertexInputBindingDescription get_binding_description(){
            vk::VertexInputBindingDescription info{};
            info.binding = 0;
            info.stride = sizeof(Vertex);
            info.inputRate = vk::VertexInputRate::eVertex;
            return info;
        }

        static std::array<vk::VertexInputAttributeDescription, 2> get_attribute_descriptions(){
            std::array<vk::VertexInputAttributeDescription, 2> desc{};
            desc[0].binding = 0;
            desc[0].location = 0;
            desc[0].format = vk::Format::eR32G32Sfloat;
            desc[0].offset = offsetof(Vertex, pos);

            desc[1].binding = 0;
            desc[1].location = 1;
            desc[1].format = vk::Format::eR32G32B32Sfloat;
            desc[1].offset = offsetof(Vertex, colour);

            return desc;
        }
    };

    class VertexBuffer {
        public:
        VertexBuffer(): instance{nullptr}, vertex_buf{} {}
        VertexBuffer(Instance* instance, vk::Queue queue, std::vector<Vertex> vertices);

        void clean();

        vk::Buffer& vertex_buffer_handle(){
            return vertex_buf.handle();
        }

        private:
        Instance* instance;
        Buffer vertex_buf;
    };

    class IndexBuffer {
        public:
        IndexBuffer(): instance{nullptr}, index_buf{} {}
        IndexBuffer(Instance* instance, vk::Queue queue, std::vector<uint16_t> vertices);

        void clean();

        vk::Buffer& index_buffer_handle(){
            return index_buf.handle();
        }

        private:
        Instance* instance;
        Buffer index_buf;
    };

    class RenderPass {
        public:
        RenderPass();
        RenderPass(Instance* instance, SwapChain* swapchain);

        void clean();
        vk::RenderPass& handle();

        private:
        Instance* instance;
        vk::RenderPass renderpass;
        SwapChain* swapchain;
    };

    class RenderPipeline {
        public:
        RenderPipeline();
        RenderPipeline(Instance* instance, SwapChain* swapchain);

        RenderPass& get_render_pass(){
            return renderpass;
        }

        vk::Pipeline& get_pipeline(){
            return pipeline;
        }

        void clean();

        private:
        Instance* instance;
        SwapChain* swapchain;

        RenderPass renderpass;
        
        vk::PipelineLayout layout;
        vk::Pipeline pipeline;
    };
} // namespace benzene::vulkan
