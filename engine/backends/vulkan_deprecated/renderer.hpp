#pragma once

#include "base.hpp"
#include "memory.hpp"
#include "shader.hpp"
#include "swap_chain.hpp"

namespace benzene::vulkan
{
    struct Vertex {
        glm::vec3 pos;
        glm::vec3 colour;
        glm::vec3 normal;
        glm::vec2 uv;

        static vk::VertexInputBindingDescription get_binding_description(){
            vk::VertexInputBindingDescription info{};
            info.binding = 0;
            info.stride = sizeof(Vertex);
            info.inputRate = vk::VertexInputRate::eVertex;
            return info;
        }

        static std::array<vk::VertexInputAttributeDescription, 4> get_attribute_descriptions(){
            std::array<vk::VertexInputAttributeDescription, 4> desc{};
            desc[0].binding = 0;
            desc[0].location = 0;
            desc[0].format = vk::Format::eR32G32B32Sfloat;
            desc[0].offset = offsetof(Vertex, pos);

            desc[1].binding = 0;
            desc[1].location = 1;
            desc[1].format = vk::Format::eR32G32B32Sfloat;
            desc[1].offset = offsetof(Vertex, colour);

            desc[2].binding = 0;
            desc[2].location = 2;
            desc[2].format = vk::Format::eR32G32B32Sfloat;
            desc[2].offset = offsetof(Vertex, normal);

            desc[3].binding = 0;
            desc[3].location = 3;
            desc[3].format = vk::Format::eR32G32Sfloat;
            desc[3].offset = offsetof(Vertex, uv);

            return desc;
        }
    };

    struct UniformBufferObject {
        glm::mat4 view;
        glm::mat4 proj;
        glm::vec3 camera_position;
    };

    struct PushConstants {
        glm::mat4 model;
        glm::mat3 normal;
    };

    template<typename T>
    class BouncedBuffer {
        public:
        BouncedBuffer(): instance{nullptr}, buf{} {}
        BouncedBuffer(Instance* instance, std::vector<T> items, vk::BufferUsageFlags usage): instance{instance} {
            size_t size = sizeof(T) * items.size();
            auto staging_buf = Buffer{instance, size, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent};    

            void* data = instance->allocator.mapMemory(staging_buf.allocation_handle());
            if(items.data()) // might be null when passed 0 size vector
                memcpy(data, items.data(), size);
            instance->allocator.unmapMemory(staging_buf.allocation_handle());

            this->buf = Buffer{instance, size, usage | vk::BufferUsageFlagBits::eTransferDst, vk::MemoryPropertyFlagBits::eDeviceLocal};    

            auto copy_buffer = [](Instance* instance, Buffer& src, Buffer& dst, size_t size){
                CommandBuffer cmd{instance, &instance->graphics};

                {
                    std::lock_guard guard{cmd};
                    vk::BufferCopy copy_region{};
                    copy_region.srcOffset = 0;
                    copy_region.dstOffset = 0;
                    copy_region.size = size;

                    cmd.handle().copyBuffer(src.handle(), dst.handle(), {copy_region});
                }
            };

            copy_buffer(instance, staging_buf, buf, size);

            staging_buf.clean();
        }

        size_t size(){
            return buf.size() / sizeof(T);
        }

        void clean(){
            this->buf.clean();
        }

        vk::Buffer& handle(){
            return buf.handle();
        }

        private:
        Instance* instance;
        Buffer buf;
    };

    using VertexBuffer = BouncedBuffer<Vertex>;
    using IndexBuffer = BouncedBuffer<uint32_t>;

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
        struct Options {
            std::vector<Shader> shaders;
            vk::PolygonMode polygon_mode;
        };

        RenderPipeline();
        RenderPipeline(Instance* instance, SwapChain* swapchain, Options& options);

        RenderPass& get_render_pass(){
            return renderpass;
        }

        vk::Pipeline& get_pipeline(){
            return pipeline;
        }

        vk::DescriptorSetLayout& get_descriptor_set_layout(size_t set){
            return descriptor_set_layouts[set];
        }

        vk::PipelineLayout& get_layout(){
            return layout;
        }

        void clean();

        private:
        Instance* instance;
        SwapChain* swapchain;

        RenderPass renderpass;
        
        std::array<vk::DescriptorSetLayout, 2> descriptor_set_layouts;
        vk::PipelineLayout layout;
        vk::Pipeline pipeline;
    };
} // namespace benzene::vulkan
