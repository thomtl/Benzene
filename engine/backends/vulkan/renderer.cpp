#include "renderer.hpp"

using namespace benzene::vulkan;

#pragma region vertex_buffer

vertex_buffer::vertex_buffer(vk::Device dev, vk::PhysicalDevice physical_dev, std::vector<vertex> vertices): dev{dev}, physical_dev{physical_dev} {
    vk::BufferCreateInfo create_info{};
    create_info.size = sizeof(vertex) * vertices.size();
    create_info.usage = vk::BufferUsageFlagBits::eVertexBuffer;
    create_info.sharingMode = vk::SharingMode::eExclusive;

    this->buffer = dev.createBuffer(create_info);

    auto requirements = dev.getBufferMemoryRequirements(this->buffer);


    auto find_memory_type = [&physical_dev](uint32_t type_filter, vk::MemoryPropertyFlags properties) -> uint32_t {
        auto mem_properties = physical_dev.getMemoryProperties();

        for(size_t i = 0; i < mem_properties.memoryTypeCount; i++)
            if(type_filter & (1 << i) && (mem_properties.memoryTypes[i].propertyFlags & properties) == properties)
                return i;

        throw std::runtime_error("Couldn't find suitable memory type");
    };

    vk::MemoryAllocateInfo alloc_info{};
    alloc_info.allocationSize = requirements.size;
    alloc_info.memoryTypeIndex = find_memory_type(requirements.memoryTypeBits, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);

    this->memory = dev.allocateMemory(alloc_info);

    dev.bindBufferMemory(buffer, memory, 0);

    void* data = dev.mapMemory(memory, 0, create_info.size);
    memcpy(data, vertices.data(), (size_t)create_info.size);
    dev.unmapMemory(memory);
}

void vertex_buffer::clean(){
    this->dev.destroyBuffer(this->buffer);
    this->dev.freeMemory(this->memory);
}

#pragma endregion

#pragma region render_pass

render_pass::render_pass(): renderpass{nullptr}, device{nullptr}, swapchain{nullptr} {}
render_pass::render_pass(vk::Device device, swap_chain* swapchain): device{device}, swapchain{swapchain} {
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

    this->renderpass = device.createRenderPass(render_pass_create_info);
}

void render_pass::clean(){
    this->device.destroyRenderPass(this->renderpass);
}

vk::RenderPass& render_pass::handle(){
    return renderpass;
}

#pragma endregion

#pragma region pipeline

render_pipeline::render_pipeline(): device{nullptr}, swapchain{nullptr}, renderpass{}, layout{nullptr}, pipeline{nullptr} {}
render_pipeline::render_pipeline(vk::Device device, swap_chain* swapchain): device{device}, swapchain{swapchain} {
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

    auto binding_description = vertex::get_binding_description();
    auto attribute_descriptions = vertex::get_attribute_descriptions();

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
    pipeline_create_info.pDynamicState = &dynamic_state;
    pipeline_create_info.layout = layout;
    pipeline_create_info.renderPass = renderpass.handle();
    pipeline_create_info.subpass = 0;
    pipeline_create_info.basePipelineHandle = {nullptr};
    pipeline_create_info.basePipelineIndex = -1;

    this->pipeline = device.createGraphicsPipeline({nullptr}, pipeline_create_info);
            

    vertex.clean();
    fragment.clean();
}

void render_pipeline::clean(){
    device.destroyPipeline(this->pipeline);
    device.destroyPipelineLayout(this->layout);
    renderpass.clean();
}

#pragma endregion