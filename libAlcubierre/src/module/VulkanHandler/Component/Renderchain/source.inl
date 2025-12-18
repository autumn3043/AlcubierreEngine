VulkanRenderchainComponent::VulkanRenderchainComponent(VulkanHandler* _parent, Registry* _registry_ptr) : parent(_parent), registry_ptr(_registry_ptr) {
    framebufferResizedFlag = dynamic_cast<IWindowManager*>(registry_ptr->FetchService(WINDOW_MANAGER))->getFramebufferResizedFlag();
    maxFramesInFlight = dynamic_cast<IConfigManager*>(registry_ptr->FetchService(CONFIGURATION_MANAGER))->Get<int>({"graphics", "max_frames_in_flight"}, 2);

    CreateGraphicsPipeline();
    CreateTransferCommandPool();
    CreateGraphicalCommandPool();
    CreateVertexBuffers();
};

VulkanRenderchainComponent::~VulkanRenderchainComponent() {
    for(int i = 0; i < vertexBuffers.size(); i++) {
        if(vertexBuffers[i]) delete vertexBuffers[i];
    }
    if(transferCommandPool) delete transferCommandPool;
    if(graphicalCommandPool) delete graphicalCommandPool;
    if(pipeline) delete pipeline;
}

int VulkanRenderchainComponent::CreateGraphicsPipeline() {
    IConfigManager* CM = dynamic_cast<IConfigManager*>(registry_ptr->FetchService(CONFIGURATION_MANAGER));

    std::vector<ShaderModule> shaders;
    int shaderCount = CM->Get<int>({"graphics", "shaders", "count"}, 1);
    shaders.reserve(shaderCount);
    for(int i = 0; i < shaderCount; i++) {
        shaders.emplace_back(parent->device->Device);
    }
    DM().Log("Constructed " + std::to_string(shaderCount) + " shaders");

    std::vector<ShaderModuleStage> shaderStages;
    int shaderStageCount = CM->Get<int>({"graphics", "shaders", "stages", CFGARRAY_SIZE_T}, 0);
    shaderStages.reserve(shaderStageCount);
    for(int i = 0; i < shaderStageCount; i++) {
        std::string type = CM->Get<std::string>({"graphics", "shaders", "stages", std::to_string(i), "type"}, "all");
        std::string name = CM->Get<std::string>({"graphics", "shaders", "stages", std::to_string(i), "name"});
        int shaderIndex = CM->Get<int>({"graphics", "shaders", "stages", std::to_string(i), "shader"}, 0);
        if(shaderIndex > shaders.size() - 1) {
            DM().Log("Shader stage " + std::to_string(i) + " requested shader " + std::to_string(shaderIndex) + " but only " + std::to_string(shaders.size()) + " existed so it used the default shader 0", 1);
            shaderIndex = 0;
        }
        shaderStages.emplace_back(i, name, type, shaders[shaderIndex]);
    }
    DM().Log("Prepared " + std::to_string(shaderStageCount) + " shader stages");

    if(pipeline) delete pipeline;
    pipeline = new GraphicsPipeline(parent->device->Device, parent->swapchain->ChainFormat, shaderStages);

    return 0;
}

#include <fstream>
VulkanRenderchainComponent::ShaderModule::ShaderModule(VkDevice& _device) : device(_device) {
    //TEMP
        std::ifstream file("basic_shader.spv", std::ios::ate | std::ios::binary);
        if(!file.is_open()) throw VulkanException("Failed to open shader file");
        size_t fileSize = static_cast<size_t>(file.tellg());
        file.seekg(0, std::ios::beg);
        std::vector<uint32_t> shaderData(fileSize / sizeof(uint32_t));
        if (!file.read(reinterpret_cast<char*>(shaderData.data()), static_cast<std::streamsize>(fileSize))) {
            throw VulkanException("Failed to read shader");
        }
        file.close();

    VkShaderModuleCreateInfo shaderCreateInfo {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = shaderData.size() * sizeof(uint32_t),
        .pCode = shaderData.data()
    };
    vkCreateShaderModule(device, &shaderCreateInfo, nullptr, &module);
}

VulkanRenderchainComponent::ShaderModule::~ShaderModule() {
    if(device != VK_NULL_HANDLE) {
        if(module) vkDestroyShaderModule(device, module, nullptr);
    } else {
        DM().Log("Attempted to discard shader module, but VkDevice handle was null, which would have caused a segfault. This memory will leak");
    }
}

VulkanRenderchainComponent::ShaderModuleStage::ShaderModuleStage(int _index, std::string _name, std::string _type, ShaderModule& shader) : index(_index), name(_name) {
    if(_type == "vertex") type = VK_SHADER_STAGE_VERTEX_BIT;
    else if(_type == "tessellation_control") type = VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
    else if(_type == "tessellation_evaluation") type = VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
    else if(_type == "geometry") type = VK_SHADER_STAGE_GEOMETRY_BIT;
    else if(_type == "fragment") type = VK_SHADER_STAGE_FRAGMENT_BIT;
    else if(_type == "compute") type = VK_SHADER_STAGE_COMPUTE_BIT;
    else if(_type == "all_graphics") type = VK_SHADER_STAGE_ALL_GRAPHICS;
    else if(_type == "all") type = VK_SHADER_STAGE_ALL;
    else throw VulkanException("Invalid shader stage '" + _type + "' passed to shader stage " + std::to_string(index));

    createInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .pNext = nullptr,
        .flags = NULL_BIT,
        .stage = type,
        .module = shader.module,
        .pName = name.c_str(),
    };
}

VulkanRenderchainComponent::GraphicsPipeline::GraphicsPipeline(VkDevice& _device, VkFormat& format, std::vector<ShaderModuleStage>& shaderStages) : device(_device) {
    VkGraphicsPipelineCreateInfo pipeCreateInfo {};
    pipeCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;

    VkPipelineRenderingCreateInfo pipelineRenderingCreateInfo {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
        .colorAttachmentCount = 1,
        .pColorAttachmentFormats = &format
    };
    pipeCreateInfo.pNext = &pipelineRenderingCreateInfo;

    std::vector<VkPipelineShaderStageCreateInfo> shaderStageCreateInfos;
    for(ShaderModuleStage shaderStage : shaderStages) {
        shaderStageCreateInfos.push_back(shaderStage.createInfo);
        //Shallow copy is okay, parent vector exists for the lifetime of this function
    }
    pipeCreateInfo.stageCount = static_cast<uint32_t>(shaderStageCreateInfos.size());
    pipeCreateInfo.pStages = shaderStageCreateInfos.data();

    std::vector<VkVertexInputBindingDescription> bindingDescriptions = {{0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX}};
    std::vector<VkVertexInputAttributeDescription> attributeDescriptions = {{0, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, Vertex::position)}, {1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, Vertex::colour)}};

    VkPipelineVertexInputStateCreateInfo vertexInputInfo {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .vertexBindingDescriptionCount = static_cast<uint32_t>(bindingDescriptions.size()),
        .pVertexBindingDescriptions = bindingDescriptions.data(),
        .vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size()),
        .pVertexAttributeDescriptions = attributeDescriptions.data()
    };
    pipeCreateInfo.pVertexInputState = &vertexInputInfo;

    VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST
        // .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP
    };
    pipeCreateInfo.pInputAssemblyState = &inputAssemblyInfo;

    VkPipelineViewportStateCreateInfo viewportInfo {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .viewportCount = 1,
        .scissorCount = 1
    };
    pipeCreateInfo.pViewportState = &viewportInfo;

    VkPipelineRasterizationStateCreateInfo rasterizationInfo {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .depthClampEnable = VK_FALSE,
        .rasterizerDiscardEnable = VK_FALSE,
        .polygonMode = VK_POLYGON_MODE_FILL,
        // .cullMode = VK_CULL_MODE_BACK_BIT,
        .cullMode = VK_CULL_MODE_NONE,
        .frontFace = VK_FRONT_FACE_CLOCKWISE,
        .depthBiasEnable = VK_FALSE,
        .depthBiasSlopeFactor = 1.0f,
        .lineWidth = 1.0f
    };
    pipeCreateInfo.pRasterizationState = &rasterizationInfo;

    VkPipelineMultisampleStateCreateInfo multisampleInfo {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
        .sampleShadingEnable = VK_FALSE
    };
    pipeCreateInfo.pMultisampleState = &multisampleInfo;

    VkPipelineColorBlendAttachmentState colorAttachment {
        .blendEnable = VK_FALSE,
        .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT
    };

    VkPipelineColorBlendStateCreateInfo colorBlendInfo {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .attachmentCount = 1,
        .pAttachments = &colorAttachment,
    };
    pipeCreateInfo.pColorBlendState = &colorBlendInfo;

    std::vector<VkDynamicState> dynamicStates = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
    VkPipelineDynamicStateCreateInfo dynamicStateInfo {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .dynamicStateCount = static_cast<uint32_t>(dynamicStates.size()),
        .pDynamicStates = dynamicStates.data()
    };
    pipeCreateInfo.pDynamicState = &dynamicStateInfo;

    VkPipelineLayoutCreateInfo layoutInfo {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = 0,
        .pushConstantRangeCount = 0
    };

    vkCreatePipelineLayout(device, &layoutInfo, nullptr, &layout);
    pipeCreateInfo.layout = layout;

    pipeCreateInfo.renderPass = VK_NULL_HANDLE;

    VkResult hold = vkCreateGraphicsPipelines(device, nullptr, 1, &pipeCreateInfo, nullptr, &pipeline);

    if(hold == VK_SUCCESS) {
        DM().Log("Successfully created graphics pipeline");
    } else {
        DM().Log("Failed to create Vulkan graphics pipeline due to error: " + std::to_string(hold), 2);
    }
}

VulkanRenderchainComponent::GraphicsPipeline::~GraphicsPipeline() {
    if(device != VK_NULL_HANDLE) {
        if(pipeline) vkDestroyPipeline(device, pipeline, nullptr);
        if(layout) vkDestroyPipelineLayout(device, layout, nullptr);
        DM().Log("Successfully destroyed graphics pipeline");
    } else {
        DM().Log("Tried to destroy graphics pipeline, but VkDevice handle was null, which would have caused a segfault. This memory will leak");
    }
}

int VulkanRenderchainComponent::CreateTransferCommandPool() {
    IConfigManager* CM = dynamic_cast<IConfigManager*>(registry_ptr->FetchService(CONFIGURATION_MANAGER));

    int commandTimeoutSeconds = CM->Get<int>({"graphics", "command_timeout_seconds"}, 2);

    transferCommandPool = new CommandPool(parent->device->Device, parent->device->getQueue(VK_QUEUE_TRANSFER_BIT).familyIndex, 1, commandTimeoutSeconds);
    return 0;
}

int VulkanRenderchainComponent::CreateGraphicalCommandPool() {
    IConfigManager* CM = dynamic_cast<IConfigManager*>(registry_ptr->FetchService(CONFIGURATION_MANAGER));
    
    int maxFramesInFlight = CM->Get<int>({"graphics", "max_frames_in_flight"}, 1);
    int commandTimeoutSeconds = CM->Get<int>({"graphics", "command_timeout_seconds"}, 2);

    graphicalCommandPool = new CommandPool(parent->device->Device, parent->device->getQueue(VK_QUEUE_GRAPHICS_BIT, true).familyIndex, maxFramesInFlight, commandTimeoutSeconds);
    return 0;
}

VulkanRenderchainComponent::CommandPool::CommandPool(VkDevice& _device, int queueIndex, int _bufferCount, int commandTimeoutSeconds) : device(_device), bufferCount(_bufferCount), commandTimeout(commandTimeoutSeconds * 1000000000) {
    VkCommandPoolCreateInfo createInfo {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = static_cast<uint32_t>(queueIndex)
    };

    VkResult hold = vkCreateCommandPool(device, &createInfo, nullptr, &commandPool);

    if(hold == VK_SUCCESS) {
        DM().Log("Successfully created command pool");
    } else throw VulkanException("Failed to create command pool");

    buffers.reserve(bufferCount);
    for(int i = 0; i < bufferCount; i++) {
        buffers.emplace_back(device, commandPool, commandTimeout);
    }
}

VulkanRenderchainComponent::CommandPool::~CommandPool() {
    if(device != VK_NULL_HANDLE) {
        if(commandPool) vkDestroyCommandPool(device, commandPool, nullptr);
        commandPool = VK_NULL_HANDLE;
        DM().Log("Successfully destroyed command pool");
    } else {
        DM().Log("Tried to destroy command pool, but VkDevice handle was null, which would have caused a segfault. This memory will leak", 2);
    }
}

VulkanRenderchainComponent::CommandBuffer::CommandBuffer(VkDevice& _device, VkCommandPool& commandPool, int _timeout) : device(_device), timeout(_timeout) {
    VkFenceCreateInfo FenceCreateInfo { 
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .flags = VK_FENCE_CREATE_SIGNALED_BIT
    };
    vkCreateFence(device, &FenceCreateInfo, nullptr, &fence);

    VkSemaphoreCreateInfo SemaphoreCreateInfo { 
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO 
    };
    if(semaphore) vkDestroySemaphore(device, semaphore, nullptr);
    vkCreateSemaphore(device, &SemaphoreCreateInfo, nullptr, &semaphore);

    VkCommandBufferAllocateInfo createInfo{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = commandPool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1
    };
    vkAllocateCommandBuffers(device, &createInfo, &commandBuffer);
}

VulkanRenderchainComponent::CommandBuffer::~CommandBuffer() {
    if(device != VK_NULL_HANDLE) {
        vkWaitForFences(device, 1, &fence, VK_TRUE, timeout);
        if(fence) vkDestroyFence(device, fence, nullptr);
        fence = VK_NULL_HANDLE;
        if(semaphore) vkDestroySemaphore(device, semaphore, nullptr);
        semaphore = VK_NULL_HANDLE;
    } else {
        DM().Log("Tried to destroy command buffer utilites, but VkDevice handle was null, which would have caused a segfault. This memory will leak");
    }
}

int VulkanRenderchainComponent::CreateVertexBuffers() {
    //TEMP
        IConfigManager* CM = dynamic_cast<IConfigManager*>(registry_ptr->FetchService(CONFIGURATION_MANAGER));
        int count = CM->Get<int>({"vertices_temp", CFGARRAY_SIZE_T}, 0);
        vertices_temp.resize(count);
        for(int i = 0; i < count; i++) {
            float scale = static_cast<float>(CM->Get<double>({"scale"}));
            float x = static_cast<float>(CM->Get<double>({"vertices_temp", std::to_string(i), "position", "x"})) * scale;
            float y = static_cast<float>(CM->Get<double>({"vertices_temp", std::to_string(i), "position", "y"})) * scale;
            float r = static_cast<float>(CM->Get<double>({"vertices_temp", std::to_string(i), "colour", "r"}));
            float g = static_cast<float>(CM->Get<double>({"vertices_temp", std::to_string(i), "colour", "g"}));
            float b = static_cast<float>(CM->Get<double>({"vertices_temp", std::to_string(i), "colour", "b"}));
            Vertex hold {.position = {x, y}, .colour = {r, g, b}};
            vertices_temp[i] = hold;
        }

    vertexBuffers.resize(1);
    vertexBuffers[0] = new VertexBuffer(parent->allocator, parent->device->Device, sizeof(vertices_temp[0]) * vertices_temp.size(), parent->device->getQueue(VK_QUEUE_TRANSFER_BIT).familyIndex);
    vertexBuffers[0]->fillBufferMemory(vertices_temp);
    return 0;
}

VulkanRenderchainComponent::VertexBuffer::VertexBuffer(VulkanMemoryAllocatorComponent* _allocator, VkDevice& _device, uint32_t _bufferSize, uint32_t transferQueueIndex) : allocator(_allocator), device(_device), bufferSize(_bufferSize) {
    VkBufferCreateInfo createInfo {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .pNext = nullptr,
        .size = bufferSize,
        .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 1,
        .pQueueFamilyIndices = &transferQueueIndex
    };

    VkResult hold = vkCreateBuffer(device, &createInfo, nullptr, &bufferInstance);
    if(hold != VK_SUCCESS) {
        throw VulkanException("Failed to create vertex buffer. Vulkan error code: " + std::to_string(hold));
    }

    allocation = &allocator->bindBufferMemory(bufferInstance, bufferSize);

    DM().Log("Successfully created and bound empty memory buffer sized " + std::to_string(bufferSize) + " bytes");
}

VulkanRenderchainComponent::VertexBuffer::~VertexBuffer() {
    if(device != VK_NULL_HANDLE) {
        allocator->freeBufferMemory(*allocation);
        vkDestroyBuffer(device, bufferInstance, nullptr);
        bufferInstance = VK_NULL_HANDLE;
        DM().Log("Successfully freed vertex buffer");
    } else {
        DM().Log("Tried to free vertex buffer, but VkDevice handle was null, which would have caused a segfault. This memory will leak");
    }
}

int VulkanRenderchainComponent::VertexBuffer::fillBufferMemory(std::vector<Vertex>& external_membuffer) {
    assert(allocation->allocatedRegion.memorySize == sizeof(external_membuffer[0]) * external_membuffer.size());
    allocator->stageBufferMemory(*allocation, external_membuffer.data());
    allocator->submitBufferMemory(*allocation, nullptr);
    return 0;
}

int VulkanRenderchainComponent::RecreateSwapchain() {
    *framebufferResizedFlag = false;
    parent->recreateSwapchain();
    if(graphicalCommandPool) delete graphicalCommandPool;
    CreateGraphicalCommandPool();
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

    vkCmdBindPipeline(CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->pipeline);
    VkDeviceSize deviceOffset = 0;
    vkCmdBindVertexBuffers(CommandBuffer, 0, 1, &vertexBuffers[0]->allocation->executiveBuffer, &deviceOffset);

    VkViewport viewport = VkViewport(0.0f, 0.0f, static_cast<float>(image->extent.width), static_cast<float>(image->extent.height), 0.0f, 1.0f);
    vkCmdSetViewport(CommandBuffer, 0, 1, &viewport);

    VkRect2D scissor = VkRect2D(VkOffset2D(0.0f, 0.0f), image->extent);
    vkCmdSetScissor(CommandBuffer, 0, 1, &scissor);

    vkCmdDraw(CommandBuffer, vertices_temp.size(), 1, 0, 0);

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

int VulkanRenderchainComponent::DrawFrame() {
    CommandBuffer& graphicsBuffer = graphicalCommandPool->buffers[currentFrame]; //Get (or wait for) next frame free out of max frames in flight
    VulkanDeviceComponent::DeviceQueue& graphicalQueue = parent->device->getQueue(VK_QUEUE_GRAPHICS_BIT);
    VulkanDeviceComponent::DeviceQueue& presentationQueue = parent->device->getQueue(VK_QUEUE_GRAPHICS_BIT, true);
    CommandBuffer& transferBuffer = transferCommandPool->buffers[0];
    VulkanDeviceComponent::DeviceQueue& transferQueue = parent->device->getQueue(VK_QUEUE_TRANSFER_BIT);

    vkWaitForFences(parent->device->Device, 1, &graphicsBuffer.fence, VK_TRUE, graphicsBuffer.timeout);

    uint32_t imageIndex;
    VkResult result_acquireNextImage = vkAcquireNextImageKHR(parent->device->Device, parent->swapchain->Swapchain, graphicsBuffer.timeout, graphicsBuffer.semaphore, VK_NULL_HANDLE, &imageIndex);

    if(result_acquireNextImage == VK_ERROR_OUT_OF_DATE_KHR || *framebufferResizedFlag) {
        DM().Log("Attempt to acquire next swapchain image indicated swapchain was out of date");
        RecreateSwapchain();
        return 1;
    }

    VulkanSwapchainComponent::SwapchainImageWrapper& image = parent->swapchain->Images[imageIndex]; //Get the next image from the swapchain
    
    vkResetFences(parent->device->Device, 1, &graphicsBuffer.fence);
    vkResetCommandBuffer(graphicsBuffer.commandBuffer, NULL_BIT);

    RecordCommandBuffer(graphicsBuffer.commandBuffer, &image); //Record our render pass onto this frame's command buffer

    VkPipelineStageFlags pipelineStageFlags = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    VkSubmitInfo SubmitInfo = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .pNext = nullptr,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &graphicsBuffer.semaphore,
        .pWaitDstStageMask = &pipelineStageFlags,
        .commandBufferCount = 1,
        .pCommandBuffers = &graphicsBuffer.commandBuffer,
        .signalSemaphoreCount = 1,
        .pSignalSemaphores = &image.semaphore
    };

    vkQueueSubmit(graphicalQueue.queue, 1, &SubmitInfo, graphicsBuffer.fence);

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

    VkResult result_queuePresent = vkQueuePresentKHR(presentationQueue.queue, &presentInfo);

    if(result_queuePresent == VK_ERROR_OUT_OF_DATE_KHR || result_queuePresent == VK_SUBOPTIMAL_KHR || *framebufferResizedFlag) {
        DM().Log("Attempt to present render pass to surface queue indicated swapchain was out of date or suboptimal");
        RecreateSwapchain();
    }

    currentFrame = (currentFrame + 1) % maxFramesInFlight;

    return 0;
}