#pragma once

#include <vulkan/vulkan.hpp>

#include "shader.hpp"
#include "swap_chain.hpp"
#include "../../core/format.hpp"
#include "../../core/utils.hpp"

namespace benzene::vulkan
{
    class render_pass {
        public:
        render_pass(): renderpass{nullptr}, device{nullptr}, swapchain{nullptr} {}
        render_pass(vk::Device device, swap_chain* swapchain): device{device}, swapchain{swapchain} {
            vk::AttachmentDescription colour_attachment{};
            colour_attachment.format = swapchain->get_format();
            colour_attachment.samples = vk::SampleCountFlagBits::e1;
            colour_attachment.loadOp = vk::AttachmentLoadOp::eClear;
            colour_attachment.storeOp = vk::AttachmentStoreOp::eStore;
            colour_attachment.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
            colour_attachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
            colour_attachment.initialLayout = vk::ImageLayout::eUndefined;
            colour_attachment.finalLayout = vk::ImageLayout::ePresentSrcKHR;

            vk::AttachmentReference colour_attachment_ref{};
            colour_attachment_ref.layout = vk::ImageLayout::eColorAttachmentOptimal;
            colour_attachment_ref.attachment = 0;

            vk::SubpassDescription subpass{};
            subpass.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
            subpass.colorAttachmentCount = 1;
            subpass.pColorAttachments = &colour_attachment_ref;
            
            vk::RenderPassCreateInfo render_pass_create_info{};
            render_pass_create_info.attachmentCount = 1;
            render_pass_create_info.pAttachments = &colour_attachment;
            render_pass_create_info.subpassCount = 1;
            render_pass_create_info.pSubpasses = &subpass;

            this->renderpass = device.createRenderPass(render_pass_create_info);
        }

        void clean(){
            this->device.destroyRenderPass(this->renderpass);
        }

        vk::RenderPass& handle(){
            return renderpass;
        }

        private:
        vk::RenderPass renderpass;
        vk::Device device;
        swap_chain* swapchain;
    };

    class render_pipeline {
        public:
        render_pipeline(): device{nullptr}, swapchain{nullptr}, renderpass{}, layout{nullptr}, pipeline{nullptr} {}
        render_pipeline(vk::Device device, swap_chain* swapchain): device{device}, swapchain{swapchain} {
            auto vert_code = benzene::read_binary_file("../engine/shaders/vertex.spv");
            shader vertex{device, vert_code};

            auto frag_code = benzene::read_binary_file("../engine/shaders/fragment.spv");
            shader fragment{device, frag_code};

            vk::PipelineShaderStageCreateInfo vert_info{};
            vert_info.stage = vk::ShaderStageFlagBits::eVertex;
            vert_info.module = vertex.handle();
            vert_info.pName = "main";

            vk::PipelineShaderStageCreateInfo frag_info{};
            frag_info.stage = vk::ShaderStageFlagBits::eFragment;
            frag_info.module = fragment.handle();
            frag_info.pName = "main";

            vk::PipelineShaderStageCreateInfo shader_stages[] = {vert_info, frag_info};

            vk::PipelineVertexInputStateCreateInfo vertex_input_create_info{};
            vertex_input_create_info.vertexBindingDescriptionCount = 0;
            vertex_input_create_info.pVertexBindingDescriptions = nullptr;
            vertex_input_create_info.vertexAttributeDescriptionCount = 0;
            vertex_input_create_info.pVertexAttributeDescriptions = nullptr;

            vk::PipelineInputAssemblyStateCreateInfo input_assembly_create_info{};
            input_assembly_create_info.topology = vk::PrimitiveTopology::eTriangleList;
            input_assembly_create_info.primitiveRestartEnable = false;

            vk::Viewport viewport{};
            viewport.x = 0;
            viewport.y = 0;
            viewport.width = (float)swapchain->get_extent().width;
            viewport.height = (float)swapchain->get_extent().height;
            viewport.minDepth = 0.0f;
            viewport.maxDepth = 1.0f;

            vk::Rect2D scissor{};
            scissor.offset = vk::Offset2D{0, 0};
            scissor.extent = swapchain->get_extent();

            vk::PipelineViewportStateCreateInfo viewport_create_info{};
            viewport_create_info.viewportCount = 1;
            viewport_create_info.pViewports = &viewport;
            viewport_create_info.scissorCount = 1;
            viewport_create_info.pScissors = &scissor;

            vk::PipelineRasterizationStateCreateInfo rasterizer{};
            rasterizer.depthClampEnable = false;
            rasterizer.rasterizerDiscardEnable = false;
            rasterizer.polygonMode = vk::PolygonMode::eFill;
            rasterizer.lineWidth = 1.0f;
            rasterizer.cullMode = vk::CullModeFlagBits::eBack;
            rasterizer.frontFace = vk::FrontFace::eClockwise;
            rasterizer.depthBiasEnable = false;

            vk::PipelineMultisampleStateCreateInfo multisampling{};
            multisampling.sampleShadingEnable = false;
            multisampling.rasterizationSamples = vk::SampleCountFlagBits::e1;
            multisampling.minSampleShading = 1.0f;
            multisampling.alphaToCoverageEnable = false;
            multisampling.alphaToOneEnable = false;

            vk::PipelineColorBlendAttachmentState colour_blend_attachment{};
            colour_blend_attachment.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
            colour_blend_attachment.blendEnable = false;
            
            vk::PipelineColorBlendStateCreateInfo colour_blending{};
            colour_blending.logicOpEnable = false;
            colour_blending.attachmentCount = 1;
            colour_blending.pAttachments = &colour_blend_attachment;

            /*vk::DynamicState dynamic_states[] = {
                (vk::DynamicState)VK_DYNAMIC_STATE_VIEWPORT,
                (vk::DynamicState)VK_DYNAMIC_STATE_LINE_WIDTH
            };

            vk::PipelineDynamicStateCreateInfo dynamic_state{};
            dynamic_state.dynamicStateCount = 2;
            dynamic_state.pDynamicStates = &dynamic_states;*/

            vk::PipelineLayoutCreateInfo layout_create_info{};
            layout_create_info.setLayoutCount = 0;
            layout_create_info.pSetLayouts = nullptr;
            layout_create_info.pushConstantRangeCount = 0;
            layout_create_info.pPushConstantRanges = nullptr;

            this->layout = device.createPipelineLayout(layout_create_info);
            
            this->renderpass = render_pass{device, swapchain};

            vk::GraphicsPipelineCreateInfo pipeline_create_info{};
            pipeline_create_info.stageCount = 2;
            pipeline_create_info.pStages = shader_stages;
            pipeline_create_info.pVertexInputState = &vertex_input_create_info;
            pipeline_create_info.pInputAssemblyState = &input_assembly_create_info;
            pipeline_create_info.pViewportState = &viewport_create_info;
            pipeline_create_info.pRasterizationState = &rasterizer;
            pipeline_create_info.pMultisampleState = &multisampling;
            pipeline_create_info.pColorBlendState = &colour_blending;
            pipeline_create_info.layout = layout;
            pipeline_create_info.renderPass = renderpass.handle();
            pipeline_create_info.subpass = 0;
            pipeline_create_info.basePipelineHandle = {nullptr};
            pipeline_create_info.basePipelineIndex = -1;

            this->pipeline = device.createGraphicsPipeline({nullptr}, pipeline_create_info);
            

            vertex.clean();
            fragment.clean();
        }

        void clean(){
            device.destroyPipeline(this->pipeline);
            device.destroyPipelineLayout(this->layout);
            renderpass.clean();
        }

        private:
        vk::Device device;
        swap_chain* swapchain;

        render_pass renderpass;
        
        vk::PipelineLayout layout;
        vk::Pipeline pipeline;
    };
} // namespace benzene::vulkan
