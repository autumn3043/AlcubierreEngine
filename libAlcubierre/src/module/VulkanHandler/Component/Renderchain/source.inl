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
    logIdentity("Average frame rendering latency across " + std::to_string(framesEver) + " frames (only " + std::to_string(FRAMEDELTACACHESIZE) + " most recent frame deltas cached) was " + std::to_string(total / frameDeltas.size()) + "ms");
    vkDestroyQueryPool(parent->device->Device, queryPool, nullptr);

    frames.clear();
    if(descriptorSetPool) vkDestroyDescriptorPool(parent->device->Device, descriptorSetPool, nullptr);
    if(renderingCommandPool) vkDestroyCommandPool(parent->device->Device, renderingCommandPool, nullptr);

    if(frameDataHostBuffer) vkDestroyBuffer(parent->device->Device, frameDataHostBuffer, nullptr);
    if(frameDataHostMemory) vkFreeMemory(parent->device->Device, frameDataHostMemory, nullptr);
    if(frameDataDeviceBuffer) vkDestroyBuffer(parent->device->Device, frameDataDeviceBuffer, nullptr);
    if(frameDataDeviceMemory) vkFreeMemory(parent->device->Device, frameDataDeviceMemory, nullptr);
}

int VulkanRenderchainComponent::createFrames() {
    //Debug and timing utilities
    VkQueryPoolCreateInfo queryPoolInfo {
        .sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO,
        .queryType = VK_QUERY_TYPE_TIMESTAMP,
        .queryCount = static_cast<uint32_t>(2 * maxFramesInFlight)
    };
    vkCreateQueryPool(parent->device->Device, &queryPoolInfo, nullptr, &queryPool);

    //Rendering command buffers
    VkCommandPoolCreateInfo commandPoolCreateInfo {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = graphicalQueue->deviceQueueFamilyIndex
    };
    vkCreateCommandPool(parent->device->Device, &commandPoolCreateInfo, nullptr, &renderingCommandPool);

    commandBuffers.resize(maxFramesInFlight);
    VkCommandBufferAllocateInfo commandBufferCreateInfo {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = renderingCommandPool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = static_cast<uint32_t>(maxFramesInFlight)
    };
    vkAllocateCommandBuffers(parent->device->Device, &commandBufferCreateInfo, commandBuffers.data());

    //Frame data memory buffers and descriptor sets
    VkDeviceSize frameDataBufferSize = sizeof(Frame::FrameData) * maxFramesInFlight;

    //Host
    {
        VkBufferCreateInfo createInfo {
            .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
            .pNext = nullptr,
            .size = frameDataBufferSize,
            .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
            .queueFamilyIndexCount = 1,
            .pQueueFamilyIndices = &graphicalQueue->deviceQueueFamilyIndex
        };
        vkCreateBuffer(parent->device->Device, &createInfo, nullptr, &frameDataHostBuffer);

        VkMemoryRequirements requirements;
        vkGetBufferMemoryRequirements(parent->device->Device, frameDataHostBuffer, &requirements);
        VkDeviceSize requiredSize = ((frameDataBufferSize + requirements.alignment - 1) / requirements.alignment) * requirements.alignment;

        VkMemoryAllocateInfo allocateInfo {
            .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
            .pNext = nullptr,
            .allocationSize = requiredSize,
            .memoryTypeIndex = static_cast<uint32_t>(parent->device->fetchDeviceProperties().memory.hostVisibleIndex)
        };
        vkAllocateMemory(parent->device->Device, &allocateInfo, nullptr, &frameDataHostMemory);

        vkBindBufferMemory(parent->device->Device, frameDataHostBuffer, frameDataHostMemory, 0);

        void* accessPointer;
        vkMapMemory(parent->device->Device, frameDataHostMemory, 0, VK_WHOLE_SIZE, NULL_BIT, &accessPointer);
        frameDataHostBufferAccessIndex = static_cast<char*>(accessPointer);
    }       

    //Device
    {
        VkBufferCreateInfo createInfo {
            .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
            .pNext = nullptr,
            .size = frameDataBufferSize,
            .usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
            .queueFamilyIndexCount = 1,
            .pQueueFamilyIndices = &graphicalQueue->deviceQueueFamilyIndex
        };
        vkCreateBuffer(parent->device->Device, &createInfo, nullptr, &frameDataDeviceBuffer);

        VkMemoryRequirements requirements;
        vkGetBufferMemoryRequirements(parent->device->Device, frameDataDeviceBuffer, &requirements);
        VkDeviceSize requiredSize = ((frameDataBufferSize + requirements.alignment - 1) / requirements.alignment) * requirements.alignment;

        VkMemoryAllocateInfo allocateInfo {
            .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
            .pNext = nullptr,
            .allocationSize = requiredSize,
            .memoryTypeIndex = static_cast<uint32_t>(parent->device->fetchDeviceProperties().memory.deviceLocalIndex)
        };
        vkAllocateMemory(parent->device->Device, &allocateInfo, nullptr, &frameDataDeviceMemory);

        vkBindBufferMemory(parent->device->Device, frameDataDeviceBuffer, frameDataDeviceMemory, 0);
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

    descriptorSets.resize(maxFramesInFlight);
    std::vector<VkDescriptorSetLayout> descriptorSetLayouts(maxFramesInFlight, parent->pipelines->frameDescriptorSetLayout);
    VkDescriptorSetAllocateInfo descriptorAllocateInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .pNext = nullptr,
        .descriptorPool = descriptorSetPool,
        .descriptorSetCount = static_cast<uint32_t>(descriptorSetLayouts.size()),
        .pSetLayouts = descriptorSetLayouts.data()
    };
    vkAllocateDescriptorSets(parent->device->Device, &descriptorAllocateInfo, descriptorSets.data());

    std::vector<VkDescriptorBufferInfo> bufferInfos(maxFramesInFlight * 2);
    std::vector<VkWriteDescriptorSet> writeDescriptorSets(maxFramesInFlight * 2);
    for(int i = 0; i < maxFramesInFlight; i++) {
        bufferInfos[i] = {
            .buffer = frameDataDeviceBuffer,
            .offset = i * sizeof(Frame::FrameData),
            .range = sizeof(glm::mat4)
        };
        writeDescriptorSets[i] = {
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .pNext = nullptr,
            .dstSet = descriptorSets[i],
            .dstBinding = 0,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .pBufferInfo = &bufferInfos[i]
        };

        bufferInfos[i + 1] = {
            .buffer = frameDataDeviceBuffer,
            .offset = i * sizeof(Frame::FrameData) + sizeof(glm::mat4),
            .range = sizeof(glm::mat4)
        };
        writeDescriptorSets[i + 1] = {
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .pNext = nullptr,
            .dstSet = descriptorSets[i],
            .dstBinding = 1,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .pBufferInfo = &bufferInfos[i + 1]
        };
    }

    vkUpdateDescriptorSets(parent->device->Device, writeDescriptorSets.size(), writeDescriptorSets.data(), 0, nullptr);

    frames.reserve(maxFramesInFlight);
    for(int i = 0; i < maxFramesInFlight; i++) {
        frames.emplace_back(parent->device->Device, i);
    }

    return 0;
}

int VulkanRenderchainComponent::addToFrame(SceneObject object) {
    VulkanMemoryAllocatorComponent::meshHandle* handle = parent->allocator->fetchMesh(object.meshHash);

    if(!sceneTree.contains(handle->bufferSetId)) sceneTree.emplace(handle->bufferSetId, meshSet(handle->vertices.buffer, handle->indices.buffer));
    sceneTree.at(handle->bufferSetId).objects.emplace_back(object);

    return 0;
}

int VulkanRenderchainComponent::clearFrame() {
    sceneTree.clear();
    return 0;
}

int VulkanRenderchainComponent::drawFrame() {
    Frame* frame = &frames[currentFrameIndex]; //Get (or wait for) next frame free out of max frames in flight
    uint64_t& frameIndex = frame->frameIndex;
    vkWaitForFences(parent->device->Device, 1, &frame->fence_frameInFlight, VK_TRUE, frame->timeout);

    if(framesEver > maxFramesInFlight) {
        std::vector<uint32_t> lastFrameTimer(2);
        VkResult lastFrame = vkGetQueryPoolResults(parent->device->Device, queryPool, frame->queryPoolBaseIndex, 2, sizeof(uint32_t) * 2, lastFrameTimer.data(), sizeof(uint32_t), NULL_BIT);
        if(lastFrame != VK_NOT_READY) {
            if(frameDeltas.size() == FRAMEDELTACACHESIZE) frameDeltas.pop_back(); //Pop element in index 9 not last exisitng
            frameDeltas.insert(frameDeltas.begin(), (lastFrameTimer[1] - lastFrameTimer[0]) * parent->device->fetchDeviceProperties().general.rawStruct.limits.timestampPeriod / 1000000);
        }
    }

    uint32_t imageIndex;
    vkAcquireNextImageKHR(parent->device->Device, parent->swapchain->Swapchain, commandTimeout, frame->semaphore_awaitImageGrab, VK_NULL_HANDLE, &imageIndex);
    VulkanSwapchainComponent::SwapchainImageWrapper& image = parent->swapchain->Images[imageIndex]; //Get the next image from the swapchain
    
    vkResetFences(parent->device->Device, 1, &frame->fence_frameInFlight);
    vkResetCommandBuffer(commandBuffers[frameIndex], NULL_BIT);

    recordCommandBuffer(frame, &image); //Record our render pass onto this frame's command buffer

    VkPipelineStageFlags pipelineStageFlags = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

    VkSubmitInfo SubmitInfo = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .pNext = nullptr,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &frame->semaphore_awaitImageGrab,
        .pWaitDstStageMask = &pipelineStageFlags,
        .commandBufferCount = 1,
        .pCommandBuffers = &commandBuffers[frameIndex],
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
        vkWaitForFences(parent->device->Device, 1, &frame->fence_frameInFlight, VK_TRUE, commandTimeout);
        logIdentity("Time to first live measured at " + std::to_string(parent->timeToFirstLive.delta()) + " milliseconds", 1);
    }

    currentFrameIndex = (currentFrameIndex + 1) % frames.size();
    framesEver++;

    return 0;
}

int VulkanRenderchainComponent::recordCommandBuffer(Frame* frame, VulkanSwapchainComponent::SwapchainImageWrapper* image) {
    uint64_t& frameIndex = frame->frameIndex;

    VkCommandBufferBeginInfo BeginInfo { 
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .pNext = nullptr,
        .flags = NULL_BIT,
        .pInheritanceInfo = nullptr
    };

    VkCommandBuffer& commandBuffer = commandBuffers[frameIndex];

    vkBeginCommandBuffer(commandBuffer, &BeginInfo);

    vkCmdResetQueryPool(commandBuffer, queryPool, frameIndex, 2);
    vkCmdWriteTimestamp(commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, queryPool, frameIndex);

    VkDeviceSize offset = sizeof(Frame::FrameData) * frameIndex;
    VkBufferCopy region { .srcOffset = offset, .dstOffset = offset, .size = sizeof(Frame::FrameData) };
    vkCmdCopyBuffer(commandBuffer, frameDataHostBuffer, frameDataDeviceBuffer, 1, &region);

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

    VulkanPipelineComponent::Pipeline* pipeline = parent->pipelines->opaqueGeometryPipe;

    //OPT: Move below loop to a child command buffer and resubmit that buffer per pipeline instead of re-recording per pipeline
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->instance);
    for(std::unordered_map<uint32_t, meshSet>::iterator it = sceneTree.begin(); it != sceneTree.end(); it++) {
        if(it->second.objects.size() == 0) continue;
        VkDeviceSize offset = 0;
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, it->second.vertexBuffer, &offset);
        vkCmdBindIndexBuffer(commandBuffer, *it->second.indexBuffer, 0, VK_INDEX_TYPE_UINT32);
        
        for(int j = 0; j < it->second.objects.size(); j++) {
            SceneObject& object = it->second.objects[j];
            VulkanMemoryAllocatorComponent::meshHandle* mesh = parent->allocator.fetchMesh(object.mesh);
            if(!mesh->memoryValid(parent->allocator)) {
                logIdentity("Skipped drawing mesh " + std::to_string(j) + " in buffer set " + std::to_string(std::distance(sceneTree.begin(), it)) + " because its memoryValid was false");
                continue;
            }
            
            VulkanMemoryAllocatorComponent::materialHandle* material = parent->allocator.fetchMaterial(object.material); 
            if(!material->memoryValid(parent->allocator)) {
                logIdentity("Skipped drawing material " + std::to_string(j) + " in buffer set " + std::to_string(std::distance(sceneTree.begin(), it)) + " because its memoryValid was false");
                material = parent->allocator.fetchMaterial(DEFAULT_MATERIAL_HASH);
            }
            
            VulkanMemoryAllocatorComponent::worldDataHandle* worldData = parent->allocator.fetchWorldData(object.worldData);
            if(!worldData->memoryValid(parent->allocator)) {
                logIdentity("Skipped drawing world data " + std::to_string(j) + " in buffer set " + std::to_string(std::distance(sceneTree.begin(), it)) + " because its memoryValid was false");
                continue;
            }

            VkDescriptorSet* descriptors = new VkDescriptorSet[descriptorSets[frameIndex], parent->allocator->fetchDescriptorSet(material.descriptorSetIndex), worldData->descriptorSet];
            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->layout, 0, 3, descriptors, 0, nullptr);
            //OPT: descriptor set buffer, index into buffer, group meshes by set buffer and bind at top of loop
            uint32_t firstIndex = static_cast<uint32_t>(mesh->indices.block.offset) / sizeof(uint32_t);
            int32_t vertexOffset = static_cast<int32_t>(mesh->vertices.block.offset) / sizeof(Vertex);
            vkCmdDrawIndexed(commandBuffer, mesh->indices.count, 1, firstIndex, vertexOffset, 0);
        }
    }

    vkCmdEndRendering(commandBuffer);
    vkCmdWriteTimestamp(commandBuffer, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, queryPool, frameIndex + 1);

    image->TransitionImageLayout(commandBuffer, VulkanSwapchainComponent::LAYOUT_DETAILS_PRESET_PRESENT);

    vkEndCommandBuffer(commandBuffer);

    return 0;
}

VulkanRenderchainComponent::Frame::Frame(VkDevice& _device, uint64_t _frameIndex, VkCommandBuffer& _commandBuffer, VkDescriptorSet& _descriptorSet)
    :   device(_device),
        frameIndex(_frameIndex)
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
    } else {
        logIdentity("Tried to destroy frame fence and semaphore utilites, but VkDevice handle was null, which would have caused a segfault. This memory will leak");
    }
}