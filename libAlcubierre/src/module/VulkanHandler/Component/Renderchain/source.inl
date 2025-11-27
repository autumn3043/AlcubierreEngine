//Space left for shorthand defines
#define NULL_BIT 0x0

VulkanRenderchainComponent::VulkanRenderchainComponent(VulkanHandler* _parent, Registry* _registry_ptr) : parent(_parent), registry_ptr(_registry_ptr) {
    CreateGraphicsPipeline();
    CreateCommandPool();
    CreateCommandBuffers();

    framebufferResizedFlag = dynamic_cast<IWindowManager*>(registry_ptr->FetchService(WINDOW_MANAGER))->getFramebufferResizedFlag();
};

VulkanRenderchainComponent::~VulkanRenderchainComponent() {
    if(parent->device->Device != VK_NULL_HANDLE, CommandPool != VK_NULL_HANDLE) {
        vkDestroyCommandPool(parent->device->Device, CommandPool, nullptr);
        DM().Log("Successfully destroyed command pool");
    }

    if((parent->device->Device != VK_NULL_HANDLE) && (Pipeline != VK_NULL_HANDLE)) {
        vkDestroyPipeline(parent->device->Device, Pipeline, nullptr);
        vkDestroyPipelineLayout(parent->device->Device, pipelineLayout, nullptr);
        DM().Log("Successfully destroyed graphics pipeline");
    }
}

#include <fstream>

int VulkanRenderchainComponent::CreateGraphicsPipeline() {
    std::ifstream file("basic_shader.spv", std::ios::ate | std::ios::binary);
    if(!file.is_open()) throw VulkanException("Failed to open shader file");
    size_t fileSize = static_cast<size_t>(file.tellg());
    file.seekg(0, std::ios::beg);
    std::vector<uint32_t> shader(fileSize / sizeof(uint32_t));
    if (!file.read(reinterpret_cast<char*>(shader.data()), static_cast<std::streamsize>(fileSize))) {
        throw VulkanException("Failed to read shader");
    }
    file.close();

    VkShaderModuleCreateInfo shaderCreateInfo {};
    shaderCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;

    shaderCreateInfo.codeSize = shader.size() * sizeof(uint32_t);
    shaderCreateInfo.pCode = shader.data();

    VkShaderModule basicShader;
    vkCreateShaderModule(parent->device->Device, &shaderCreateInfo, nullptr, &basicShader);

    VkGraphicsPipelineCreateInfo pipeCreateInfo {};
    pipeCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;

    VkPipelineRenderingCreateInfo pipelineRenderingCreateInfo {};
    pipelineRenderingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
    pipelineRenderingCreateInfo.colorAttachmentCount = 1;
    pipelineRenderingCreateInfo.pColorAttachmentFormats = &parent->swapchain->ChainFormat;
    pipeCreateInfo.pNext = &pipelineRenderingCreateInfo;

    std::vector<AlcPipelineShaderStageCreateInfo> shaderStageCreateInfos_arr; 
    FetchShaderStageCreateInfos(shaderStageCreateInfos_arr, basicShader);
    std::vector<VkPipelineShaderStageCreateInfo> shaderStageCreateInfos;
    shaderStageCreateInfos.reserve(shaderStageCreateInfos_arr.size());
    for(AlcPipelineShaderStageCreateInfo& shaderStage : shaderStageCreateInfos_arr) {
        shaderStageCreateInfos.push_back(*shaderStage.Get());
    }

    pipeCreateInfo.stageCount = static_cast<uint32_t>(shaderStageCreateInfos.size());
    pipeCreateInfo.pStages = shaderStageCreateInfos.data();

    VkPipelineVertexInputStateCreateInfo vertexInputInfo {};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    pipeCreateInfo.pVertexInputState = &vertexInputInfo;

    VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo {};
    inputAssemblyInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    pipeCreateInfo.pInputAssemblyState = &inputAssemblyInfo;

    VkPipelineViewportStateCreateInfo viewportInfo {};
    viewportInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportInfo.viewportCount = 1;
    viewportInfo.scissorCount = 1;
    pipeCreateInfo.pViewportState = &viewportInfo;

    VkPipelineRasterizationStateCreateInfo rasterizationInfo {};
    rasterizationInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizationInfo.depthClampEnable = VK_FALSE;
    rasterizationInfo.rasterizerDiscardEnable = VK_FALSE;
    rasterizationInfo.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizationInfo.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizationInfo.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasterizationInfo.depthBiasEnable = VK_FALSE;
    rasterizationInfo.depthBiasSlopeFactor = 1.0f;
    rasterizationInfo.lineWidth = 1.0f;
    pipeCreateInfo.pRasterizationState = &rasterizationInfo;

    VkPipelineMultisampleStateCreateInfo multisampleInfo {};
    multisampleInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampleInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisampleInfo.sampleShadingEnable = VK_FALSE;
    pipeCreateInfo.pMultisampleState = &multisampleInfo;

    VkPipelineColorBlendAttachmentState colorAttachment {};
    colorAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorAttachment.blendEnable = VK_FALSE;

    VkPipelineColorBlendStateCreateInfo colorBlendInfo {};
    colorBlendInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlendInfo.attachmentCount = 1;
    colorBlendInfo.pAttachments = &colorAttachment;
    pipeCreateInfo.pColorBlendState = &colorBlendInfo;

    VkPipelineDynamicStateCreateInfo dynamicStateInfo {};
    dynamicStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    std::vector<VkDynamicState> dynamicStates = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };
    dynamicStateInfo.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
    dynamicStateInfo.pDynamicStates = dynamicStates.data();
    pipeCreateInfo.pDynamicState = &dynamicStateInfo;

    VkPipelineLayoutCreateInfo layoutInfo {};
    layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layoutInfo.setLayoutCount = 0;
    layoutInfo.pushConstantRangeCount = 0;

    vkCreatePipelineLayout(parent->device->Device, &layoutInfo, nullptr, &pipelineLayout);
    pipeCreateInfo.layout = pipelineLayout;

    pipeCreateInfo.renderPass = VK_NULL_HANDLE;

    VkResult hold = vkCreateGraphicsPipelines(parent->device->Device, nullptr, 1, &pipeCreateInfo, nullptr, &Pipeline);

    vkDestroyShaderModule(parent->device->Device, basicShader, nullptr);

    if(hold == VK_SUCCESS) {
        DM().Log("Successfully created graphics pipeline");
        return 0;
    } else {
        DM().Log("Failed to create Vulkan graphics pipeline due to error: " + std::to_string(hold), 2);
        return 1;
    }
}

void VulkanRenderchainComponent::FetchShaderStageCreateInfos(std::vector<AlcPipelineShaderStageCreateInfo>& ReturnBundlesArray, VkShaderModule& shaderModule) {
    IConfigManager* CM = dynamic_cast<IConfigManager*>(registry_ptr->FetchService(CONFIGURATION_MANAGER));

    int requiredShaderStages = CM->Get<int>({"graphics", "shaders", "stages", CFGARRAY_SIZE_T}, 0);

    if(requiredShaderStages == 0) {
        DM().Log("Applying default shader stage settings", 1);
        CM->Set<int>({"graphics", "shaders", "stages", "0", "bit"}, std::to_string(VK_SHADER_STAGE_VERTEX_BIT));
        CM->Set<std::string>({"graphics", "shaders", "stages", "0", "name"}, "\"vertMain\"");
        CM->Set<int>({"graphics", "shaders", "stages", "1", "bit"}, std::to_string(VK_SHADER_STAGE_FRAGMENT_BIT));
        CM->Set<std::string>({"graphics", "shaders", "stages", "1", "name"}, "\"fragMain\"");
        requiredShaderStages = 2;
    }

    ReturnBundlesArray.reserve(requiredShaderStages);

    for(int i = 0; i < requiredShaderStages; i++) {
        AlcPipelineShaderStageCreateInfo& shaderStage = ReturnBundlesArray.emplace_back(AlcPipelineShaderStageCreateInfo());
        shaderStage._flags = NULL_BIT;
        shaderStage._stage = static_cast<VkShaderStageFlagBits>(CM->Get<int>({"graphics", "shaders", "stages", std::to_string(i), "bit"}));
        shaderStage._module = shaderModule;
        shaderStage._pName = CM->Get<std::string>({"graphics", "shaders", "stages", std::to_string(i), "name"});
    }
}

int VulkanRenderchainComponent::CreateCommandPool() {
    AlcCommandPoolCreateInfo CreateInfo {};
    GetCommandPoolCreateInfo(CreateInfo);

    VkResult hold = vkCreateCommandPool(parent->device->Device, CreateInfo.Get(), nullptr, &CommandPool);

    if(hold == VK_SUCCESS) {
        DM().Log("Successfully created command pool");
    } else throw VulkanException("Failed to create command pool");

    return 0;
}

void VulkanRenderchainComponent::GetCommandPoolCreateInfo(AlcCommandPoolCreateInfo& ReturnBundle) {
    ReturnBundle._flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    ReturnBundle._queueFamilyIndex = parent->device->GraphicsQueue.index;
}

int VulkanRenderchainComponent::CreateCommandBuffers() {
    AlcCommandBufferCreateInfo CreateInfo{};
    GetCommandBufferCreateInfo(CreateInfo);

    int s = CreateInfo.Get()->commandBufferCount;
    renderFrames.clear();
    std::vector<VkCommandBuffer> hold(s, VK_NULL_HANDLE);
    VkResult commandBufferAllocateResult = vkAllocateCommandBuffers(parent->device->Device, CreateInfo.Get(), hold.data());
    renderFrames.reserve(s);
    for(int i = 0; i < s; i++) {
        RenderFrame& frame = renderFrames.emplace_back(parent->device->Device);
        frame.commandBuffer = hold[i];
    }
    if(commandBufferAllocateResult == VK_SUCCESS) {
        DM().Log("Successfully created " + std::to_string(s) + " command buffers");
        return 0;
    } else throw VulkanException("Failed to create command buffers");
}

void VulkanRenderchainComponent::GetCommandBufferCreateInfo(AlcCommandBufferCreateInfo& ReturnBundle) {
    ReturnBundle._commandPool = CommandPool;
    ReturnBundle._level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    ReturnBundle._commandBufferCount = max_frames_in_flight;
}

VulkanRenderchainComponent::RenderFrame::RenderFrame(VkDevice& _device) : device(_device) {
    VkFenceCreateInfo FenceCreateInfo { 
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .flags = VK_FENCE_CREATE_SIGNALED_BIT
    };

    vkCreateFence(device, &FenceCreateInfo, nullptr, &fence);
    CreateSemaphores();
}

VulkanRenderchainComponent::RenderFrame::~RenderFrame() {
    if(fence) vkDestroyFence(device, fence, nullptr);
    fence = VK_NULL_HANDLE;
    if(semaphore) vkDestroySemaphore(device, semaphore, nullptr);
    semaphore = VK_NULL_HANDLE;
}

void VulkanRenderchainComponent::RenderFrame::CreateSemaphores() {
    if(semaphore) vkDestroySemaphore(device, semaphore, nullptr);

    VkSemaphoreCreateInfo SemaphoreCreateInfo { 
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO 
    };

    vkCreateSemaphore(device, &SemaphoreCreateInfo, nullptr, &semaphore);
}

int VulkanRenderchainComponent::DrawFrame() {
    uint64_t timeout = TIMEOUT_SET * t_second;

    RenderFrame& frame = renderFrames[currentFrame]; //Get (or wait for) next frame free out of max frames in flight
    vkWaitForFences(parent->device->Device, 1, &frame.fence, VK_TRUE, timeout);

    uint32_t imageIndex;
    VkResult result_acquireNextImage = vkAcquireNextImageKHR(parent->device->Device, parent->swapchain->Swapchain, timeout, frame.semaphore, VK_NULL_HANDLE, &imageIndex);

    if(result_acquireNextImage == VK_ERROR_OUT_OF_DATE_KHR || *framebufferResizedFlag) {
        DM().Log("Attempt to acquire next swapchain image indicated swapchain was out of date");
        RecreateSwapchain();
        return 1;
    }

    VulkanSwapchainComponent::SwapchainImageWrapper& image = parent->swapchain->Images[imageIndex]; //Get the next image from the swapchain
    
    vkResetFences(parent->device->Device, 1, &frame.fence);
    vkResetCommandBuffer(frame.commandBuffer, NULL_BIT);

    RecordCommandBuffer(frame.commandBuffer, &image); //Record our render pass onto this frame's command buffer

    VkPipelineStageFlags pipelineStageFlags = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    VkSubmitInfo SubmitInfo = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .pNext = nullptr,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &frame.semaphore,
        .pWaitDstStageMask = &pipelineStageFlags,
        .commandBufferCount = 1,
        .pCommandBuffers = &frame.commandBuffer,
        .signalSemaphoreCount = 1,
        .pSignalSemaphores = &image.semaphore
    };

    vkQueueSubmit(parent->device->GraphicsQueue.queue, 1, &SubmitInfo, frame.fence);

    VkPresentInfoKHR presentInfo {
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .pNext = nullptr,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &image.semaphore,
        .swapchainCount = 1,
        .pSwapchains = &parent->swapchain->Swapchain,
        .pImageIndices = &imageIndex,
        .pResults = nullptr
    };

    VkResult result_queuePresent = vkQueuePresentKHR(parent->device->SurfacePresentQueue.queue, &presentInfo);

    if(result_queuePresent == VK_ERROR_OUT_OF_DATE_KHR || result_queuePresent == VK_SUBOPTIMAL_KHR || *framebufferResizedFlag) {
        DM().Log("Attempt to present render pass to surface queue indicated swapchain was out of date or suboptimal");
        RecreateSwapchain();
    }

    currentFrame = (currentFrame + 1) % max_frames_in_flight;

    return 0;
}

int VulkanRenderchainComponent::RecreateSwapchain() {
    *framebufferResizedFlag = false;
    parent->recreateSwapchain();
    for(RenderFrame& frame : renderFrames) {
        frame.CreateSemaphores();
    }
    return 0;
}

int VulkanRenderchainComponent::RecordCommandBuffer(VkCommandBuffer& CommandBuffer, VulkanSwapchainComponent::SwapchainImageWrapper* image) {
    VkCommandBufferBeginInfo BeginInfo { 
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .pNext = nullptr,
        .flags = NULL_BIT,
        .pInheritanceInfo = nullptr
    };
    vkBeginCommandBuffer(CommandBuffer, &BeginInfo); //We have to init the command buffer before we start recording commands to it

    image->TransitionImageLayout(CommandBuffer, parent->swapchain->imageLayoutPresets[VulkanSwapchainComponent::LAYOUT_DETAILS_PRESET_COLOURATTACHMENT]);
    
    VkClearValue _clearValue = { .color = VkClearColorValue({0.0f, 0.0f, 0.0f, 1.0f}) };
    
    VkRenderingAttachmentInfo colorRenderingAttachment {
        .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
        .pNext = nullptr,
        .imageView = image->imageView,
        .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .clearValue = _clearValue
    };

    VkRenderingInfo RenderingInfo {
        .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
        .pNext = nullptr,
        .flags = NULL_BIT,
        .renderArea = { .offset = {0,0}, .extent = image->extent },
        .layerCount = 1,
        .viewMask = 0,
        .colorAttachmentCount = 1,
        .pColorAttachments = &colorRenderingAttachment,
        .pDepthAttachment = nullptr,
        .pStencilAttachment = nullptr
    };

    vkCmdBeginRendering(CommandBuffer, &RenderingInfo);

    vkCmdBindPipeline(CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, Pipeline);

    VkViewport viewport = VkViewport(0.0f, 0.0f, static_cast<float>(image->extent.width), static_cast<float>(image->extent.height), 0.0f, 1.0f);
    vkCmdSetViewport(CommandBuffer, 0, 1, &viewport);

    VkRect2D scissor = VkRect2D(VkOffset2D(0.0f, 0.0f), image->extent);
    vkCmdSetScissor(CommandBuffer, 0, 1, &scissor);

    vkCmdDraw(CommandBuffer, 3, 1, 0, 0);

    vkCmdEndRendering(CommandBuffer);

    image->TransitionImageLayout(CommandBuffer, parent->swapchain->imageLayoutPresets[VulkanSwapchainComponent::LAYOUT_DETAILS_PRESET_PRESENT]);

    vkEndCommandBuffer(CommandBuffer);

    return 0;
}

int VulkanSwapchainComponent::SwapchainImageWrapper::TransitionImageLayout(VkCommandBuffer& CommandBuffer, AlcImageLayoutDetails& target) {
    VkImageMemoryBarrier2 imageMemoryBarrier {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
        .pNext = nullptr,

        .srcStageMask = layoutDetails.stageMask,
        .srcAccessMask = layoutDetails.accessMask,
        .dstStageMask = target.stageMask,
        .dstAccessMask = target.accessMask,
        .oldLayout = layoutDetails.layout,
        .newLayout = target.layout,
        .srcQueueFamilyIndex = layoutDetails.queueIndex,
        .dstQueueFamilyIndex = target.queueIndex,

        .image = imageHandle,

        .subresourceRange = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1
        }
    };

    VkDependencyInfo dependencyInfo {
        .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
        .pNext = nullptr,
        .dependencyFlags = {},
        .imageMemoryBarrierCount = 1,
        .pImageMemoryBarriers = &imageMemoryBarrier
    };

    vkCmdPipelineBarrier2(CommandBuffer, &dependencyInfo);

    layoutDetails = target;

    return 0;
}

//Undefine shorthands!!
#undef NULL_BIT