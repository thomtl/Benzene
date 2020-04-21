#pragma once

#include "base.hpp"
#include "renderer.hpp"

#include "../../libs/imgui/imgui.h"

namespace benzene::vulkan
{
    class Imgui {
        public:
        struct PushConstBlock {
            glm::vec2 scale;
            glm::vec2 translate;
        } push_const_block;
        

        Imgui(Instance* instance, SwapChain* swapchain, RenderPass* renderpass, size_t n_buffers): instance{instance}, swapchain{swapchain}, renderpass{renderpass} {
            vertices.resize(n_buffers);
            indices.resize(n_buffers);

            IMGUI_CHECKVERSION();
            ImGui::CreateContext();
            ImGui::StyleColorsDark();

            auto& io = ImGui::GetIO();
            io.DisplaySize = ImVec2{(float)swapchain->get_extent().width, (float)swapchain->get_extent().height};
            io.DisplayFramebufferScale = ImVec2{1.0f, 1.0f};

            uint8_t* font_data;
            int tex_width, tex_height;
            io.Fonts->GetTexDataAsRGBA32(&font_data, &tex_width, &tex_height);

            font = Texture{instance, (size_t)tex_width, (size_t)tex_height, font_data};

            std::array<vk::DescriptorPoolSize, 1> pool_size {
                vk::DescriptorPoolSize{vk::DescriptorType::eCombinedImageSampler, 1}
            };

            vk::DescriptorPoolCreateInfo pool_info{};
            pool_info.poolSizeCount = pool_size.size();
            pool_info.pPoolSizes = pool_size.data();
            pool_info.maxSets = 2;
            this->descriptor_pool = instance->device.createDescriptorPool(pool_info);

            std::array<vk::DescriptorSetLayoutBinding, 1> set_layout_bindings = {
                vk::DescriptorSetLayoutBinding{0, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment}
            };
            
            vk::DescriptorSetLayoutCreateInfo set_layout_info{};
            set_layout_info.bindingCount = set_layout_bindings.size();
            set_layout_info.pBindings = set_layout_bindings.data();
            this->descriptor_set_layout = instance->device.createDescriptorSetLayout(set_layout_info);

            vk::DescriptorSetAllocateInfo set_alloc_info{};
            set_alloc_info.descriptorPool = this->descriptor_pool;
            set_alloc_info.pSetLayouts = &this->descriptor_set_layout;
            set_alloc_info.descriptorSetCount = 1;
            auto descriptor_sets = this->instance->device.allocateDescriptorSets(set_alloc_info);
            this->descriptor_set = descriptor_sets[0];

            vk::DescriptorImageInfo font_descriptor{};
            font_descriptor.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
            font_descriptor.imageView = this->font.get_view();
            font_descriptor.sampler = this->font.get_sampler();

            std::array<vk::WriteDescriptorSet, 1> write_descriptor_sets = {
                vk::WriteDescriptorSet{descriptor_set, 0, 0, 1, vk::DescriptorType::eCombinedImageSampler, &font_descriptor}
            };
            instance->device.updateDescriptorSets({write_descriptor_sets}, {});

            vk::PipelineCacheCreateInfo pipeline_cache_info{};
            this->pipeline_cache = instance->device.createPipelineCache(pipeline_cache_info);

            vk::PushConstantRange push_constant_range{};
            push_constant_range.stageFlags = vk::ShaderStageFlagBits::eVertex;
            push_constant_range.size = sizeof(PushConstBlock);
            push_constant_range.offset = 0;

            vk::PipelineLayoutCreateInfo layout_create_info{};
            layout_create_info.setLayoutCount = 1;
            layout_create_info.pSetLayouts = &descriptor_set_layout;
            layout_create_info.pushConstantRangeCount = 1;
            layout_create_info.pPushConstantRanges = &push_constant_range;

            this->pipeline_layout = instance->device.createPipelineLayout(layout_create_info);

            vk::PipelineInputAssemblyStateCreateInfo input_assembly_create_info{};
            input_assembly_create_info.topology = vk::PrimitiveTopology::eTriangleList;
            input_assembly_create_info.primitiveRestartEnable = false;

            vk::PipelineRasterizationStateCreateInfo rasterizer{};
            rasterizer.depthClampEnable = false;
            rasterizer.polygonMode = vk::PolygonMode::eFill;
            rasterizer.lineWidth = 1.0f;
            rasterizer.cullMode = vk::CullModeFlagBits::eNone;
            rasterizer.frontFace = vk::FrontFace::eCounterClockwise;

            vk::PipelineColorBlendAttachmentState colour_blend_attachment{};
            colour_blend_attachment.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
            colour_blend_attachment.blendEnable = true;
            colour_blend_attachment.srcColorBlendFactor = vk::BlendFactor::eSrcAlpha;
            colour_blend_attachment.dstColorBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha;
            colour_blend_attachment.colorBlendOp = vk::BlendOp::eAdd;
            colour_blend_attachment.srcAlphaBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha;
            colour_blend_attachment.dstAlphaBlendFactor = vk::BlendFactor::eZero;
            colour_blend_attachment.alphaBlendOp = vk::BlendOp::eAdd;

            vk::PipelineColorBlendStateCreateInfo colour_blending{};
            colour_blending.logicOpEnable = false;
            colour_blending.attachmentCount = 1;
            colour_blending.pAttachments = &colour_blend_attachment;
            colour_blending.blendConstants[0] = 0.0f;
            colour_blending.blendConstants[1] = 0.0f;
            colour_blending.blendConstants[2] = 0.0f;
            colour_blending.blendConstants[3] = 0.0f;

            vk::PipelineDepthStencilStateCreateInfo depth_stencil{};
            depth_stencil.depthTestEnable = false;
            depth_stencil.depthWriteEnable = false;
            depth_stencil.depthCompareOp = vk::CompareOp::eLessOrEqual;
            depth_stencil.depthCompareOp = vk::CompareOp::eAlways;

            vk::PipelineViewportStateCreateInfo viewport_create_info{};
            viewport_create_info.viewportCount = 1;
            viewport_create_info.pViewports = nullptr; // Dynamic and thus ignored
            viewport_create_info.scissorCount = 1;
            viewport_create_info.pScissors = nullptr; // Dynamic and thus ignored

            vk::PipelineMultisampleStateCreateInfo multisampling{};
            multisampling.rasterizationSamples = instance->find_max_msaa_samples();

            std::array<vk::DynamicState, 2> dynamic_states = {
                vk::DynamicState::eViewport,
                vk::DynamicState::eScissor
            };

            vk::PipelineDynamicStateCreateInfo dynamic_state{};
            dynamic_state.dynamicStateCount = dynamic_states.size();
            dynamic_state.pDynamicStates = dynamic_states.data();

            std::array<vk::VertexInputBindingDescription, 1> vertex_input_bindings = {
                vk::VertexInputBindingDescription{0, sizeof(ImDrawVert), vk::VertexInputRate::eVertex}
            };

            std::array<vk::VertexInputAttributeDescription, 3> vertex_input_attributes = {
                vk::VertexInputAttributeDescription{0, 0, vk::Format::eR32G32Sfloat, offsetof(ImDrawVert, pos)},
                vk::VertexInputAttributeDescription{1, 0, vk::Format::eR32G32Sfloat, offsetof(ImDrawVert, uv)},
                vk::VertexInputAttributeDescription{2, 0, vk::Format::eR8G8B8A8Unorm, offsetof(ImDrawVert, col)}
            };

            vk::PipelineVertexInputStateCreateInfo vertex_input{};
            vertex_input.vertexBindingDescriptionCount = vertex_input_bindings.size();
            vertex_input.pVertexBindingDescriptions = vertex_input_bindings.data();
            vertex_input.vertexAttributeDescriptionCount = vertex_input_attributes.size();
            vertex_input.pVertexAttributeDescriptions = vertex_input_attributes.data();

            auto vert_code = benzene::read_binary_file("../engine/backends/vulkan/shaders/imgui_vertex.spv");
            Shader vertex{instance, vert_code, "main", vk::ShaderStageFlagBits::eVertex};

            auto frag_code = benzene::read_binary_file("../engine/backends/vulkan/shaders/imgui_fragment.spv");
            Shader fragment{instance, frag_code, "main", vk::ShaderStageFlagBits::eFragment};

            vk::PipelineShaderStageCreateInfo vert_info{};
            vert_info.stage = vk::ShaderStageFlagBits::eVertex;
            vert_info.module = vertex.handle();
            vert_info.pName = "main";

            vk::PipelineShaderStageCreateInfo frag_info{};
            frag_info.stage = vk::ShaderStageFlagBits::eFragment;
            frag_info.module = fragment.handle();
            frag_info.pName = "main";

            std::array<vk::PipelineShaderStageCreateInfo, 2> shader_stages = {vert_info, frag_info};

            vk::GraphicsPipelineCreateInfo pipeline_info{};
            pipeline_info.layout = this->pipeline_layout;
            pipeline_info.renderPass = renderpass->handle();
            pipeline_info.basePipelineIndex = -1;
            pipeline_info.basePipelineHandle = {nullptr};
            pipeline_info.pInputAssemblyState = &input_assembly_create_info;
            pipeline_info.pRasterizationState = &rasterizer;
            pipeline_info.pColorBlendState = &colour_blending;
            pipeline_info.pMultisampleState = &multisampling;
            pipeline_info.pViewportState = &viewport_create_info;
            pipeline_info.pDepthStencilState = &depth_stencil;
            pipeline_info.pDynamicState = &dynamic_state;
            pipeline_info.pVertexInputState = &vertex_input;
            pipeline_info.stageCount = shader_stages.size();
            pipeline_info.pStages = shader_stages.data();

            this->pipeline = instance->device.createGraphicsPipeline(this->pipeline_cache, pipeline_info);

            vertex.clean();
            fragment.clean();
        }

        void clean(){
            ImGui::DestroyContext();

            for(auto& vertex : vertices)
                vertex.clean();
            
            for(auto& index : indices)
                index.clean();
            
            font.clean();

            instance->device.destroyPipelineCache(pipeline_cache);
            instance->device.destroyPipeline(pipeline);
            instance->device.destroyPipelineLayout(pipeline_layout);

            instance->device.destroyDescriptorPool(descriptor_pool);
            instance->device.destroyDescriptorSetLayout(descriptor_set_layout);
        }
        
        void update_buffers(size_t i){
            auto* draw_data = ImGui::GetDrawData();

            size_t vertex_size = draw_data->TotalVtxCount * sizeof(ImDrawVert);
            size_t index_size = draw_data->TotalIdxCount * sizeof(ImDrawIdx);

            std::vector<std::byte> vertex_data{};
            vertex_data.resize(vertex_size);

            std::vector<std::byte> index_data{};
            index_data.resize(index_size);

            auto* vertex_dst = (ImDrawVert*)vertex_data.data();
            auto* index_dst = (ImDrawIdx*)index_data.data();

            for(int i = 0; i < draw_data->CmdListsCount; i++){
                const auto& cmd_list = *draw_data->CmdLists[i];
                memcpy(vertex_dst, cmd_list.VtxBuffer.Data, cmd_list.VtxBuffer.Size * sizeof(ImDrawVert));
                memcpy(index_dst, cmd_list.IdxBuffer.Data, cmd_list.IdxBuffer.Size * sizeof(ImDrawIdx));
                vertex_dst += cmd_list.VtxBuffer.Size;
                index_dst += cmd_list.IdxBuffer.Size;
            }

            this->vertices[i].clean();
            this->vertices[i] = BouncedBuffer<std::byte>{instance, vertex_data, vk::BufferUsageFlagBits::eVertexBuffer};
            
            this->indices[i].clean();
            this->indices[i] = BouncedBuffer<std::byte>{instance, index_data, vk::BufferUsageFlagBits::eIndexBuffer};
        }

        void draw_frame(size_t i, vk::CommandBuffer buf){
            auto& io = ImGui::GetIO();

            buf.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipeline_layout, 0, {descriptor_set}, {});
            buf.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline);

            vk::Viewport viewport{};
            viewport.width = io.DisplaySize.x;
            viewport.height = io.DisplaySize.y;
            viewport.minDepth = 0.0f;
            viewport.maxDepth = 1.0f;

            buf.setViewport(0, {viewport});

            const auto& draw_data = *ImGui::GetDrawData();

            push_const_block.scale = glm::vec2{2.0f / io.DisplaySize.x, 2.0f / io.DisplaySize.y};
            push_const_block.translate = glm::vec2{-1.0f - draw_data.DisplayPos.x * push_const_block.scale.x, -1.0f - draw_data.DisplayPos.y * push_const_block.scale.y};

            buf.pushConstants<PushConstBlock>(this->pipeline_layout, vk::ShaderStageFlagBits::eVertex, 0, {push_const_block});

            int vertex_offset = 0, index_offset = 0;

            if(draw_data.CmdListsCount == 0)
                return;

            buf.bindVertexBuffers(0, {vertices[i].handle()}, {0});

            static_assert(sizeof(ImDrawIdx) == 2 || sizeof(ImDrawIdx) == 4, "//TODO: Support the 8bit indices extension??");
            buf.bindIndexBuffer(indices[i].handle(), 0, (sizeof(ImDrawIdx) == 2) ? vk::IndexType::eUint16 : vk::IndexType::eUint32);

            for(int i = 0; i < draw_data.CmdListsCount; i++){
                const ImDrawList& list = *draw_data.CmdLists[i];
                for(int j = 0; j < list.CmdBuffer.Size; j++){
                    const ImDrawCmd& draw = list.CmdBuffer[j];

                    vk::Rect2D scissor{};
                    scissor.offset.x = std::max((int32_t)draw.ClipRect.x, 0);
                    scissor.offset.y = std::max((int32_t)draw.ClipRect.y, 0);
                    scissor.extent.width = (uint32_t)(draw.ClipRect.z - draw.ClipRect.x);
                    scissor.extent.height = (uint32_t)(draw.ClipRect.w - draw.ClipRect.y);
                    buf.setScissor(0, {scissor});
                    buf.drawIndexed(draw.ElemCount, 1, index_offset, vertex_offset, 0);
                    index_offset += draw.ElemCount;
                }
                vertex_offset += list.VtxBuffer.Size;
            }
        }

        Imgui(): instance{nullptr}, swapchain{nullptr}, renderpass{nullptr}, vertices{}, indices{}, font{}, pipeline_cache{nullptr}, pipeline_layout{nullptr}, pipeline{nullptr}, descriptor_pool{nullptr}, descriptor_set_layout{nullptr}, descriptor_set{nullptr} {};

        private:
        Instance* instance;
        SwapChain* swapchain;
        RenderPass* renderpass;
        
        std::vector<BouncedBuffer<std::byte>> vertices;
        std::vector<BouncedBuffer<std::byte>> indices;

        vulkan::Texture font;

        vk::PipelineCache pipeline_cache;
        vk::PipelineLayout pipeline_layout;
        vk::Pipeline pipeline;

        vk::DescriptorPool descriptor_pool;
        vk::DescriptorSetLayout descriptor_set_layout;
        vk::DescriptorSet descriptor_set;
    };
} // namespace benzene::vulkan
