#include "spirv_reflect.h"

VulkanRenderchainComponent::VulkanRenderchainComponent(VulkanHandler* _parent, Registry* _registry_ptr) : parent(_parent), registry_ptr(_registry_ptr) {
    maxFramesInFlight = dynamic_cast<IConfigManager*>(registry_ptr->FetchService(CONFIGURATION_MANAGER))->Get<int>({"graphics", "max_frames_in_flight"}, 2);

    CreateGraphicsPipelines();
    CreateCommandPools();
};

VulkanRenderchainComponent::~VulkanRenderchainComponent() {
    for(VertexBuffer* buffer : vertexBuffersInMemory) {
        delete buffer;
    }
    if(transferCommandPool) delete transferCommandPool;
    if(graphicalCommandPool) delete graphicalCommandPool;
}

int VulkanRenderchainComponent::CreateGraphicsPipelines() {
    pipeline_geometry.init(parent->device->Device, parent->swapchain->ChainFormat);

    return 0;
}

VulkanRenderchainComponent::graphicsPipeline::graphicsPipeline() {}

VulkanRenderchainComponent::graphicsPipeline::~graphicsPipeline() {
    if(device != VK_NULL_HANDLE) {
        if(pipeline) vkDestroyPipeline(device, pipeline, nullptr);
        if(layout) vkDestroyPipelineLayout(device, layout, nullptr);
        if(shaderModule) vkDestroyShaderModule(device, shaderModule, nullptr);
        logIdentity("Successfully destroyed graphics pipeline");
    } else {
        logIdentity("Tried to destroy graphics pipeline, but VkDevice handle was null, which would have caused a segfault. This memory will leak");
    }
}

#include <fstream>
int VulkanRenderchainComponent::opaqueGeometryPipe::init(VkDevice& _device, VkFormat& format) {
    device = _device;

    VkGraphicsPipelineCreateInfo pipeCreateInfo {};
    pipeCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;

    VkPipelineRenderingCreateInfo pipelineRenderingCreateInfo {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
        .colorAttachmentCount = 1,
        .pColorAttachmentFormats = &format
    };
    pipeCreateInfo.pNext = &pipelineRenderingCreateInfo;

    //TEMP
        std::ifstream file("vertex.spv", std::ios::ate | std::ios::binary);
        if(!file.is_open()) throw VulkanException("Failed to open shader file");
        size_t fileSize = static_cast<size_t>(file.tellg());
        file.seekg(0, std::ios::beg);
        std::vector<uint32_t> shaderData(fileSize / sizeof(uint32_t));
        if (!file.read(reinterpret_cast<char*>(shaderData.data()), static_cast<std::streamsize>(fileSize))) {
            throw VulkanException("Failed to read shader");
        }
        file.close();

    uint32_t dataSize = shaderData.size() * sizeof(uint32_t);

    VkShaderModuleCreateInfo shaderCreateInfo {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = dataSize,
        .pCode = shaderData.data()
    };
    vkCreateShaderModule(device, &shaderCreateInfo, nullptr, &shaderModule);

    SpvReflectShaderModule reflect;
    spvReflectCreateShaderModule(dataSize, shaderData.data(), &reflect);

    VkPipelineShaderStageCreateInfo stageCreateInfo {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage = static_cast<VkShaderStageFlagBits>(reflect.shader_stage),
        .module = shaderModule,
        .pName  = reflect.entry_point_name
    };

    pipeCreateInfo.stageCount = 1;
    pipeCreateInfo.pStages = &stageCreateInfo;

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
        .pushConstantRangeCount = 0,
        .pPushConstantRanges = nullptr
    };

    vkCreatePipelineLayout(device, &layoutInfo, nullptr, &layout);
    pipeCreateInfo.layout = layout;

    pipeCreateInfo.renderPass = VK_NULL_HANDLE;

    VkResult hold = vkCreateGraphicsPipelines(device, nullptr, 1, &pipeCreateInfo, nullptr, &pipeline);

    if(hold == VK_SUCCESS) {
        logIdentity("Successfully created graphics pipeline");
    } else {
        logIdentity("Failed to create Vulkan graphics pipeline due to error: " + std::to_string(hold), 2);
    }

    spvReflectDestroyShaderModule(&reflect);

    return 0;
}

int VulkanRenderchainComponent::opaqueGeometryPipe::recordPass(VkCommandBuffer& commandBuffer, VulkanSwapchainComponent::SwapchainImageWrapper* image, std::vector<VertexBuffer*>& buffersInMemory, std::vector<int>& buffersInFrame) {
    image->TransitionImageLayout(commandBuffer, VulkanSwapchainComponent::LAYOUT_DETAILS_PRESET_COLOURATTACHMENT);
    
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

    vkCmdBeginRendering(commandBuffer, &RenderingInfo);

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

    for(int i = 0; i < buffersInFrame.size(); i++) {
        VertexBuffer*& buffer = buffersInMemory[buffersInFrame[i]];
        VkDeviceSize zeroDeviceSize = 0;
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, &buffer->allocation->executiveBuffer, &zeroDeviceSize);
        vkCmdBindIndexBuffer(commandBuffer, buffer->allocation->executiveBuffer, buffer->bufferIndexBreakpointOffset, VK_INDEX_TYPE_UINT32);
        vkCmdDrawIndexed(commandBuffer, buffer->indexCount, 1, 0, 0, 0);
    }

    vkCmdEndRendering(commandBuffer);

    image->TransitionImageLayout(commandBuffer, VulkanSwapchainComponent::LAYOUT_DETAILS_PRESET_PRESENT);

    return 0;
}

int VulkanRenderchainComponent::CreateCommandPools() {
    IConfigManager* CM = dynamic_cast<IConfigManager*>(registry_ptr->FetchService(CONFIGURATION_MANAGER));
    
    int maxFramesInFlight = CM->Get<int>({"graphics", "max_frames_in_flight"}, 1);
    int commandTimeoutSeconds = CM->Get<int>({"graphics", "command_timeout_seconds"}, 2);

    graphicalCommandPool = new CommandPool(parent->device->Device, parent->device->getQueue(VK_QUEUE_GRAPHICS_BIT, true).familyIndex, maxFramesInFlight, commandTimeoutSeconds);
    transferCommandPool = new CommandPool(parent->device->Device, parent->device->getQueue(VK_QUEUE_TRANSFER_BIT).familyIndex, 1, commandTimeoutSeconds);

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
        logIdentity("Successfully created command pool");
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
        logIdentity("Successfully destroyed command pool");
    } else {
        logIdentity("Tried to destroy command pool, but VkDevice handle was null, which would have caused a segfault. This memory will leak", 2);
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
        logIdentity("Tried to destroy command buffer utilites, but VkDevice handle was null, which would have caused a segfault. This memory will leak");
    }
}

VulkanRenderchainComponent::VertexBuffer::VertexBuffer(VulkanMemoryAllocatorComponent* _allocator, VkDevice& _device, uint32_t _vertexCount, uint32_t _indexCount, uint32_t transferQueueIndex, int _index) 
    : allocator(_allocator), device(_device), vertexCount(_vertexCount), vertex_t_size(sizeof(Vertex)), indexCount(_indexCount), index_t_size(sizeof(uint32_t)), index(_index) {

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
}

VulkanRenderchainComponent::VertexBuffer::~VertexBuffer() {
    if(device != VK_NULL_HANDLE) {
        allocator->freeBufferMemory(*allocation);
        vkDestroyBuffer(device, bufferInstance, nullptr);
        bufferInstance = VK_NULL_HANDLE;
        logIdentity("Successfully freed vertex buffer");
    } else {
        logIdentity("Tried to free vertex buffer, but VkDevice handle was null, which would have caused a segfault. This memory will leak");
    }
}

int VulkanRenderchainComponent::VertexBuffer::fillBufferMemory(void* data, uint32_t dataSize) {
    assert(allocation->allocatedRegion.memorySize >= bufferSize);

    allocator->stageBufferMemory(*allocation, data, dataSize);
    allocator->submitBufferMemory(*allocation, nullptr);
    return 0;
}

int VulkanRenderchainComponent::RecordCommandBuffer(VkCommandBuffer& CommandBuffer, VulkanSwapchainComponent::SwapchainImageWrapper* image, std::vector<graphicsPipeline*> pipelines) {
    VkCommandBufferBeginInfo BeginInfo { 
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .pNext = nullptr,
        .flags = NULL_BIT,
        .pInheritanceInfo = nullptr
    };

    vkBeginCommandBuffer(CommandBuffer, &BeginInfo); //We have to init the command buffer before we start recording commands to it

    VkViewport viewport = VkViewport(0.0f, 0.0f, static_cast<float>(image->extent.width), static_cast<float>(image->extent.height), 0.0f, 1.0f);
    vkCmdSetViewport(CommandBuffer, 0, 1, &viewport);

    VkRect2D scissor = VkRect2D(VkOffset2D(0.0f, 0.0f), image->extent);
    vkCmdSetScissor(CommandBuffer, 0, 1, &scissor);

    //TODO: compute in parallel
    for(int i = 0; i < pipelines.size(); i++) {
        pipelines[i]->recordPass(CommandBuffer, image, vertexBuffersInMemory, vertexBuffersInFrame);
    }

    vkEndCommandBuffer(CommandBuffer);

    return 0;
}

int VulkanRenderchainComponent::drawFrame() {
    CommandBuffer& graphicsBuffer = graphicalCommandPool->buffers[currentFrame]; //Get (or wait for) next frame free out of max frames in flight
    vkWaitForFences(parent->device->Device, 1, &graphicsBuffer.fence, VK_TRUE, graphicsBuffer.timeout);
    CommandBuffer& transferBuffer = transferCommandPool->buffers[0];
    
    VulkanDeviceComponent::DeviceQueue& graphicalQueue = parent->device->getQueue(VK_QUEUE_GRAPHICS_BIT);
    VulkanDeviceComponent::DeviceQueue& presentationQueue = parent->device->getQueue(VK_QUEUE_GRAPHICS_BIT, true);
    VulkanDeviceComponent::DeviceQueue& transferQueue = parent->device->getQueue(VK_QUEUE_TRANSFER_BIT);

    uint32_t imageIndex;
    VkResult result_acquireNextImage = vkAcquireNextImageKHR(parent->device->Device, parent->swapchain->Swapchain, graphicsBuffer.timeout, graphicsBuffer.semaphore, VK_NULL_HANDLE, &imageIndex);

    if(result_acquireNextImage == VK_ERROR_OUT_OF_DATE_KHR) {
        logIdentity("Attempt to acquire next swapchain image indicated swapchain was out of date");
        return 1;
    }

    VulkanSwapchainComponent::SwapchainImageWrapper& image = parent->swapchain->Images[imageIndex]; //Get the next image from the swapchain
    
    vkResetFences(parent->device->Device, 1, &graphicsBuffer.fence);
    vkResetCommandBuffer(graphicsBuffer.commandBuffer, NULL_BIT);

    RecordCommandBuffer(graphicsBuffer.commandBuffer, &image, {&pipeline_geometry}); //Record our render pass onto this frame's command buffer

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

    if(result_queuePresent == VK_ERROR_OUT_OF_DATE_KHR) {
        logIdentity("Attempt to present render pass to surface queue indicated swapchain was out of date or suboptimal");
    }

    currentFrame = (currentFrame + 1) % maxFramesInFlight;

    return 0;
}

int VulkanRenderchainComponent::createObjectBuffer(int*& modelBufferIndex, IGraphicsBackend::modelData& data) {
    VertexBuffer*& buffer = vertexBuffersInMemory.emplace_back(new VertexBuffer(parent->allocator, parent->device->Device, data.vertices.size(), data.indices.size(), parent->device->getQueue(VK_QUEUE_TRANSFER_BIT).familyIndex, vertexBuffersInMemory.size()));
    modelBufferIndex = &buffer->index;

    //temp
        std::vector<Vertex> translatedVertices(data.vertices.size());
        for(int i = 0; i < data.vertices.size(); i++) {
            translatedVertices[i] = { .position = {data.vertices[i].x, data.vertices[i].y}, .colour = {0.5f, 0.5f, 0.5f} };
        }

        std::vector<std::byte> rawDataBuffer(data.vertices.size() +  data.indices.size());

        buffer->fillBufferMemory(rawDataBuffer.data(), rawDataBuffer.size());
    
    return 0;
}

int VulkanRenderchainComponent::addObjectToFrame(int& modelBufferIndex, IGraphicsBackend::placementData& data) {
    vertexBuffersInFrame.push_back(modelBufferIndex);

    return 0;
}

int VulkanRenderchainComponent::discardObjectBuffer(int& modelBufferIndex) {
    clearFrame(); //Indices to buffers in memory will be invalidated, so we discard the indices we have in frame

    std::swap(vertexBuffersInMemory[modelBufferIndex], vertexBuffersInMemory.back());
    vertexBuffersInMemory[modelBufferIndex]->index = modelBufferIndex;
    vertexBuffersInMemory.pop_back();

    return 0;
}

int VulkanRenderchainComponent::clearFrame() {
    vertexBuffersInFrame.clear();

    return 0;
}