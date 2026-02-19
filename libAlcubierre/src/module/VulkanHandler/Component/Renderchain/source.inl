VulkanRenderchainComponent::VulkanRenderchainComponent(VulkanHandler* _parent, Registry*& _registry_ptr) : parent(_parent), registry_ptr(_registry_ptr) {
    IConfigManager* CM = dynamic_cast<IConfigManager*>(registry_ptr->FetchService(CONFIGURATION_MANAGER));
    maxFramesInFlight = CM->Get<int>({"graphics", "max_frames_in_flight"}, 1);
    commandTimeout = CM->Get<int>({"graphics", "command_timeout_seconds"}, 2) * 1000000000;

    graphicalQueue = &parent->graphicsRenderingQueue;
    presentationQueue = parent->device->fetchQueueHandle(VulkanDeviceComponent::queueType::PRESENT);

    FRAMEDELTACACHESIZE = CM->Get<int>({"graphics", "frame_delta_cache_size"}, 10);
    frameDeltas.reserve(FRAMEDELTACACHESIZE);

    createFrames();
}

VulkanRenderchainComponent::~VulkanRenderchainComponent() {
    float total;
    for(int i = 0; i < frameDeltas.size(); i++) {
        total += frameDeltas[i];
    }
    logIdentity("Average frame rendering latency across " + std::to_string(framesEver) + " (only " + std::to_string(FRAMEDELTACACHESIZE) + " most recent frame deltas cached) was " + std::to_string(total / frameDeltas.size()) + "ms");
    vkDestroyQueryPool(parent->device->Device, queryPool, nullptr);

    frames.clear();
    if(descriptorSetPool) vkDestroyDescriptorPool(parent->device->Device, descriptorSetPool, nullptr);
    if(renderingCommandPool) vkDestroyCommandPool(parent->device->Device, renderingCommandPool, nullptr);
    if(deviceMemory) vkFreeMemory(parent->device->Device, deviceMemory, nullptr);
}

int VulkanRenderchainComponent::createFrames() {
    VkQueryPoolCreateInfo queryPoolInfo {
        .sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO,
        .queryType = VK_QUERY_TYPE_TIMESTAMP,
        .queryCount = static_cast<uint32_t>(2 * maxFramesInFlight)
    };
    vkCreateQueryPool(parent->device->Device, &queryPoolInfo, nullptr, &queryPool);

    VkCommandPoolCreateInfo commandPoolCreateInfo {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = graphicalQueue->deviceQueueFamilyIndex
    };
    vkCreateCommandPool(parent->device->Device, &commandPoolCreateInfo, nullptr, &renderingCommandPool);

    std::vector<VkCommandBuffer> commandBuffers(maxFramesInFlight);
    VkCommandBufferAllocateInfo commandBufferCreateInfo {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = renderingCommandPool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = static_cast<uint32_t>(maxFramesInFlight)
    };
    vkAllocateCommandBuffers(parent->device->Device, &commandBufferCreateInfo, commandBuffers.data());

    std::vector<VkBuffer> uniformBuffers(maxFramesInFlight);
    std::vector<char*> uniformBufferMappings(maxFramesInFlight);
    uint32_t uniformBufferSize = sizeof(UniformBuffer);
    for(int i = 0; i < maxFramesInFlight; i++) {
        VkBufferCreateInfo uniformBufferCreateInfo {
            .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
            .pNext = nullptr,
            .size = uniformBufferSize,
            .usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
            .queueFamilyIndexCount = 1,
            .pQueueFamilyIndices = &graphicalQueue->deviceQueueFamilyIndex
        };
        vkCreateBuffer(parent->device->Device, &uniformBufferCreateInfo, nullptr, &uniformBuffers[i]);
    }

    VkMemoryRequirements requirements;
    vkGetBufferMemoryRequirements(parent->device->Device, uniformBuffers[0], &requirements);
    uniformBufferSize = ((uniformBufferSize + requirements.alignment - 1) / requirements.alignment) * requirements.alignment;

    VkMemoryAllocateInfo bufferAllocateInfo {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .pNext = nullptr,
        .allocationSize = uniformBufferSize * static_cast<uint32_t>(maxFramesInFlight),
        .memoryTypeIndex = static_cast<uint32_t>(parent->device->fetchDeviceProperties().memory.hostVisibleIndex)
    };
    vkAllocateMemory(parent->device->Device, &bufferAllocateInfo, nullptr, &deviceMemory);

    std::vector<VkDescriptorBufferInfo> bufferInfos(maxFramesInFlight);
    void* sourceUniformBufferMapping;
    vkMapMemory(parent->device->Device, deviceMemory, 0, VK_WHOLE_SIZE, NULL_BIT, &sourceUniformBufferMapping);
    for(int i = 0; i < maxFramesInFlight; i++) {
        uint32_t offset = uniformBufferSize * i;

        vkBindBufferMemory(parent->device->Device, uniformBuffers[i], deviceMemory, offset);
        uniformBufferMappings[i] = static_cast<char*>(sourceUniformBufferMapping) + offset;

        bufferInfos[i].buffer = uniformBuffers[i];
        bufferInfos[i].offset = 0;
        bufferInfos[i].range = uniformBufferSize;
    }

    VkDescriptorPoolSize descriptorPoolSize = {
        .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .descriptorCount = static_cast<uint32_t>(maxFramesInFlight) * parent->pipelines->pipelineLayout->bindingsCount
    };
    VkDescriptorPoolCreateInfo descriptorPoolCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .pNext = nullptr,
        .flags = NULL_BIT,
        .maxSets = static_cast<uint32_t>(maxFramesInFlight),
        .poolSizeCount = 1,
        .pPoolSizes = &descriptorPoolSize
    };
    vkCreateDescriptorPool(parent->device->Device, &descriptorPoolCreateInfo, nullptr, &descriptorSetPool);

    std::vector<VkDescriptorSet> descriptorSets(maxFramesInFlight);
    std::vector<VkDescriptorSetLayout> descriptorSetLayouts(maxFramesInFlight, parent->pipelines->pipelineLayout->descriptorSetLayout);
    VkDescriptorSetAllocateInfo descriptorAllocateInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .pNext = nullptr,
        .descriptorPool = descriptorSetPool,
        .descriptorSetCount = static_cast<uint32_t>(maxFramesInFlight),
        .pSetLayouts = descriptorSetLayouts.data()
    };
    vkAllocateDescriptorSets(parent->device->Device, &descriptorAllocateInfo, descriptorSets.data());

    frames.reserve(maxFramesInFlight);
    for(int i = 0; i < maxFramesInFlight; i++) {
        frames.emplace_back(parent->device->Device, commandTimeout, 2 * i, commandBuffers[i], uniformBuffers[i], uniformBufferMappings[i], bufferInfos[i], descriptorSets[i]);
    }

    return 0;
}

int VulkanRenderchainComponent::addToFrame(SceneObject object) {
    VulkanMemoryAllocatorComponent::meshHandle* handle = parent->allocator->fetchMesh(object.meshHash);

    if(!sceneTree.contains(handle->bufferSetId)) sceneTree.emplace(handle->bufferSetId, meshSet(handle->vertices.buffer, handle->indices.buffer));
    sceneTree.at(handle->bufferSetId).objects.emplace_back(handle);

    return 0;
}

int VulkanRenderchainComponent::clearFrame() {
    sceneTree.clear();
    return 0;
}

int VulkanRenderchainComponent::drawFrame() {
    Frame* frame = &frames[currentFrameIndex]; //Get (or wait for) next frame free out of max frames in flight
    vkWaitForFences(parent->device->Device, 1, &frame->fence_frameInFlight, VK_TRUE, frame->timeout);

    if(framesEver > maxFramesInFlight) {
        std::vector<uint32_t> lastFrameTimer(2);
        VkResult lastFrame = vkGetQueryPoolResults(parent->device->Device, queryPool, frame->queryPoolBaseIndex, 2, sizeof(uint32_t) * 2, lastFrameTimer.data(), sizeof(uint32_t), NULL_BIT);
        if(lastFrame != VK_NOT_READY) {
            if(frameDeltas.size() == FRAMEDELTACACHESIZE) frameDeltas.pop_back(); //POP ELEMENT IN 9 index not last exisitng
            frameDeltas.insert(frameDeltas.begin(), (lastFrameTimer[1] - lastFrameTimer[0]) * parent->device->fetchDeviceProperties().general.rawStruct.limits.timestampPeriod / 1000000);
        }
    }

    uint32_t imageIndex;
    vkAcquireNextImageKHR(parent->device->Device, parent->swapchain->Swapchain, frame->timeout, frame->semaphore_awaitImageGrab, VK_NULL_HANDLE, &imageIndex);
    VulkanSwapchainComponent::SwapchainImageWrapper& image = parent->swapchain->Images[imageIndex]; //Get the next image from the swapchain
    
    vkResetFences(parent->device->Device, 1, &frame->fence_frameInFlight);
    vkResetCommandBuffer(frame->commandBuffer, NULL_BIT);

    updateUniformBuffer(frame->uniformBufferAccessPointer, &image);
    VkWriteDescriptorSet writeDescriptorSet = {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .pNext = nullptr,
        .dstSet = frame->descriptorSet,
        .dstBinding = 0,
        .dstArrayElement = 0,
        .descriptorCount = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .pBufferInfo = &frame->bufferInfo
    };
    vkUpdateDescriptorSets(parent->device->Device, 1, &writeDescriptorSet, 0, nullptr);

    recordCommandBuffer(frame, &image); //Record our render pass onto this frame's command buffer

    VkPipelineStageFlags pipelineStageFlags = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

    VkSubmitInfo SubmitInfo = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .pNext = nullptr,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &frame->semaphore_awaitImageGrab,
        .pWaitDstStageMask = &pipelineStageFlags,
        .commandBufferCount = 1,
        .pCommandBuffers = &frame->commandBuffer,
        .signalSemaphoreCount = 1,
        .pSignalSemaphores = &image.semaphore
    };

    vkQueueSubmit(graphicalQueue->queue, 1, &SubmitInfo, frame->fence_frameInFlight);

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

    if(framesEver == 0) {
        vkWaitForFences(parent->device->Device, 1, &frame->fence_frameInFlight, VK_TRUE, frame->timeout);
        logIdentity("Time to first live measured at " + std::to_string(parent->timeToFirstLive.delta()) + " milliseconds", 1);
    }

    currentFrameIndex = (currentFrameIndex + 1) % frames.size();
    framesEver++;

    return 0;
}

int VulkanRenderchainComponent::updateUniformBuffer(char* buffer, VulkanSwapchainComponent::SwapchainImageWrapper* image) {
    static auto startTime = std::chrono::high_resolution_clock::now();

    auto currentTime = std::chrono::high_resolution_clock::now();
    float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

    UniformBuffer ubo {};
    glm::mat4 correction = glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
    glm::mat4 scale = glm::scale(glm::mat4(1.0f), glm::vec3(10.0f));
    ubo.model = correction * rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f)) * scale;
    ubo.view = lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    ubo.proj = glm::perspective(glm::radians(45.0f), static_cast<float>(image->extent.width) / static_cast<float>(image->extent.height), 0.1f, 10.0f);
    ubo.proj[1][1] *= -1;

    memcpy(buffer, &ubo, sizeof(UniformBuffer));

    return 0;
}

int VulkanRenderchainComponent::recordCommandBuffer(Frame* frame, VulkanSwapchainComponent::SwapchainImageWrapper* image) {
    VkCommandBufferBeginInfo BeginInfo { 
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .pNext = nullptr,
        .flags = NULL_BIT,
        .pInheritanceInfo = nullptr
    };

    VkCommandBuffer& commandBuffer = frame->commandBuffer;

    vkBeginCommandBuffer(commandBuffer, &BeginInfo);

    vkCmdResetQueryPool(commandBuffer, queryPool, frame->queryPoolBaseIndex, 2);
    vkCmdWriteTimestamp(commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, queryPool, frame->queryPoolBaseIndex);

    image->TransitionImageLayout(commandBuffer, VulkanSwapchainComponent::LAYOUT_DETAILS_PRESET_COLOURATTACHMENT);

    VkViewport viewport = VkViewport(0.0f, 0.0f, static_cast<float>(image->extent.width), static_cast<float>(image->extent.height), 0.0f, 1.0f);
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

    VkRect2D scissor = VkRect2D(VkOffset2D(0.0f, 0.0f), image->extent);
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

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

    parent->pipelines->opaqueGeometryPipe->bind(commandBuffer, frame->descriptorSet);
    //OPT: Move below loop to a child command buffer and resubmit that buffer per pipeline instead of re-recording per pipeline
    for(std::unordered_map<uint32_t, meshSet>::iterator it = sceneTree.begin(); it != sceneTree.end(); it++) {
        if(it->second.objects.size() == 0) continue;
        VkDeviceSize offset = 0;
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, it->second.vertexBuffer, &offset);
        vkCmdBindIndexBuffer(commandBuffer, *it->second.indexBuffer, 0, VK_INDEX_TYPE_UINT32);
        for(int j = 0; j < it->second.objects.size(); j++) {
            if(!it->second.objects[j]->memoryValid(parent->allocator)) logIdentity("Skipped drawing mesh " + std::to_string(j) + " in buffer set " + std::to_string(std::distance(sceneTree.begin(), it)) + " because its memoryValid was false");
            else {
                uint32_t firstIndex = static_cast<uint32_t>(it->second.objects[j]->indices.offset) / sizeof(uint32_t);
                int32_t vertexOffset = static_cast<int32_t>(it->second.objects[j]->vertices.offset) / sizeof(Vector3);
                vkCmdDrawIndexed(commandBuffer, it->second.objects[j]->indices.count, 1, firstIndex, vertexOffset, 0);
            } 
        }
    }
    vkCmdEndRendering(commandBuffer);
    vkCmdWriteTimestamp(commandBuffer, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, queryPool, frame->queryPoolBaseIndex + 1);

    image->TransitionImageLayout(commandBuffer, VulkanSwapchainComponent::LAYOUT_DETAILS_PRESET_PRESENT);

    vkEndCommandBuffer(commandBuffer);

    return 0;
}

VulkanRenderchainComponent::Frame::Frame(VkDevice& _device, int _timeout, int _queryPoolBaseIndex, VkCommandBuffer& _commandBuffer, VkBuffer& _uniformBuffer, char* _uniformBufferAccessPointer, VkDescriptorBufferInfo& _bufferInfo, VkDescriptorSet& _descriptorSet)
    :   device(_device),
        timeout(_timeout),
        queryPoolBaseIndex(static_cast<uint32_t>(_queryPoolBaseIndex)),
        commandBuffer(_commandBuffer), 
        uniformBuffer(_uniformBuffer),
        uniformBufferAccessPointer(_uniformBufferAccessPointer), 
        bufferInfo(_bufferInfo),
        descriptorSet(_descriptorSet) 
    {
        VkFenceCreateInfo FenceCreateInfo { 
            .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
            .flags = VK_FENCE_CREATE_SIGNALED_BIT
        };
        vkCreateFence(device, &FenceCreateInfo, nullptr, &fence_frameInFlight);

        VkSemaphoreCreateInfo SemaphoreCreateInfo { 
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO 
        };
        vkCreateSemaphore(device, &SemaphoreCreateInfo, nullptr, &semaphore_awaitImageGrab);
    }

VulkanRenderchainComponent::Frame::~Frame() {
    if(device != VK_NULL_HANDLE) {
        vkWaitForFences(device, 1, &fence_frameInFlight, VK_TRUE, timeout);
        if(fence_frameInFlight) vkDestroyFence(device, fence_frameInFlight, nullptr);
        if(semaphore_awaitImageGrab) vkDestroySemaphore(device, semaphore_awaitImageGrab, nullptr);

        vkDestroyBuffer(device, uniformBuffer, nullptr);
    } else {
        logIdentity("Tried to destroy command buffer utilites, but VkDevice handle was null, which would have caused a segfault. This memory will leak");
    }
}