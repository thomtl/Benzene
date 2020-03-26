#include "renderer.hpp"

using namespace benzene::vulkan;

#pragma region render_pass

RenderPass::RenderPass(): instance{nullptr},renderpass{nullptr}, swapchain{nullptr} {}
RenderPass::RenderPass(Instance* instance, SwapChain* swapchain): instance{instance}, swapchain{swapchain} {
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

    vk::SubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
    dependency.srcAccessMask = {};
    dependency.dstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
    dependency.dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;

            
    vk::RenderPassCreateInfo render_pass_create_info{};
    render_pass_create_info.attachmentCount = 1;
    render_pass_create_info.pAttachments = &colour_attachment;
    render_pass_create_info.subpassCount = 1;
    render_pass_create_info.pSubpasses = &subpass;
    render_pass_create_info.dependencyCount = 1;
    render_pass_create_info.pDependencies = &dependency;

    this->renderpass = instance->device.createRenderPass(render_pass_create_info);
}

void RenderPass::clean(){
    this->instance->device.destroyRenderPass(this->renderpass);
}

vk::RenderPass& RenderPass::handle(){
    return renderpass;
}

#pragma endregion

#pragma region pipeline

RenderPipeline::RenderPipeline(): instance{nullptr}, swapchain{nullptr}, renderpass{}, layout{nullptr}, pipeline{nullptr} {}
RenderPipeline::RenderPipeline(Instance* instance, SwapChain* swapchain): instance{instance}, swapchain{swapchain} {
    auto vert_code = benzene::read_binary_file("../engine/shaders/vertex.spv");
    Shader vertex{instance, vert_code};

    auto frag_code = benzene::read_binary_file("../engine/shaders/fragment.spv");
    Shader fragment{instance, frag_code};

    vk::PipelineShaderStageCreateInfo vert_info{};
    vert_info.stage = vk::ShaderStageFlagBits::eVertex;
    vert_info.module = vertex.handle();
    vert_info.pName = "main";

    vk::PipelineShaderStageCreateInfo frag_info{};
    frag_info.stage = vk::ShaderStageFlagBits::eFragment;
    frag_info.module = fragment.handle();
    frag_info.pName = "main";

    vk::PipelineShaderStageCreateInfo shader_stages[] = {vert_info, frag_info};

    auto binding_description = Vertex::get_binding_description();
    auto attribute_descriptions = Vertex::get_attribute_descriptions();

    vk::PipelineVertexInputStateCreateInfo vertex_input_create_info{};
    vertex_input_create_info.vertexBindingDescriptionCount = 1;
    vertex_input_create_info.pVertexBindingDescriptions = &binding_description;
    vertex_input_create_info.vertexAttributeDescriptionCount = attribute_descriptions.size();
    vertex_input_create_info.pVertexAttributeDescriptions = attribute_descriptions.data();

    vk::PipelineInputAssemblyStateCreateInfo input_assembly_create_info{};
    input_assembly_create_info.topology = vk::PrimitiveTopology::eTriangleList;
    input_assembly_create_info.primitiveRestartEnable = false;

            

    vk::PipelineViewportStateCreateInfo viewport_create_info{};
    viewport_create_info.viewportCount = 1;
    viewport_create_info.pViewports = nullptr; // Dynamic and thus ignored
    viewport_create_info.scissorCount = 1;
    viewport_create_info.pScissors = nullptr; // Dynamic and thus ignored

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
    colour_blending.logicOp = vk::LogicOp::eCopy;
    colour_blending.attachmentCount = 1;
    colour_blending.pAttachments = &colour_blend_attachment;
    colour_blending.blendConstants[0] = 0.0f;
    colour_blending.blendConstants[1] = 0.0f;
    colour_blending.blendConstants[2] = 0.0f;
    colour_blending.blendConstants[3] = 0.0f;

    vk::DynamicState dynamic_states[] = {
        vk::DynamicState::eViewport,
        vk::DynamicState::eScissor
    };

    vk::PipelineDynamicStateCreateInfo dynamic_state{};
    dynamic_state.dynamicStateCount = 2;
    dynamic_state.pDynamicStates = dynamic_states;

    vk::PipelineLayoutCreateInfo layout_create_info{};
    layout_create_info.setLayoutCount = 0;
    layout_create_info.pSetLayouts = nullptr;
    layout_create_info.pushConstantRangeCount = 0;
    layout_create_info.pPushConstantRanges = nullptr;

    this->layout = instance->device.createPipelineLayout(layout_create_info);
            
    this->renderpass = RenderPass{instance, swapchain};

    vk::GraphicsPipelineCreateInfo pipeline_create_info{};
    pipeline_create_info.stageCount = 2;
    pipeline_create_info.pStages = shader_stages;
    pipeline_create_info.pVertexInputState = &vertex_input_create_info;
    pipeline_create_info.pInputAssemblyState = &input_assembly_create_info;
    pipeline_create_info.pViewportState = &viewport_create_info;           
    pipeline_create_info.pRasterizationState = &rasterizer;
    pipeline_create_info.pMultisampleState = &multisampling;
    pipeline_create_info.pColorBlendState = &colour_blending;
    pipeline_create_info.pDynamicState = &dynamic_state;
    pipeline_create_info.layout = layout;
    pipeline_create_info.renderPass = renderpass.handle();
    pipeline_create_info.subpass = 0;
    pipeline_create_info.basePipelineHandle = {nullptr};
    pipeline_create_info.basePipelineIndex = -1;

    this->pipeline = instance->device.createGraphicsPipeline({nullptr}, pipeline_create_info);
            

    vertex.clean();
    fragment.clean();
}

void RenderPipeline::clean(){
    instance->device.destroyPipeline(this->pipeline);
    instance->device.destroyPipelineLayout(this->layout);
    renderpass.clean();
}

#pragma endregion