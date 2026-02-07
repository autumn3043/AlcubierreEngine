VulkanRenderchainComponent::VulkanRenderchainComponent(VulkanHandler* _parent, Registry*& _registry_ptr) : parent(_parent), registry_ptr(_registry_ptr) {
    IConfigManager* CM = dynamic_cast<IConfigManager*>(registry_ptr->FetchService(CONFIGURATION_MANAGER));
    maxFramesInFlight = CM->Get<int>({"graphics", "max_frames_in_flight"}, 1);
    commandTimeout = CM->Get<int>({"graphics", "command_timeout_seconds"}, 2) * 1000000000;

    VkCommandPoolCreateInfo createInfo {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = static_cast<uint32_t>(parent->device->getQueue(VK_QUEUE_GRAPHICS_BIT, true).familyIndex)
    };

    VkResult hold = vkCreateCommandPool(parent->device->Device, &createInfo, nullptr, &commandPool);

    commandBuffers.reserve(maxFramesInFlight);
    for(int i = 0; i < commandBuffers.size(); i++) {
        commandBuffers[i] = new CommandBuffer(device, commandPool, commandTimeout);
    }
}

VulkanRenderchainComponent::~VulkanRenderchainComponent() {
    commandBuffers.clear();
    if(commandPool) vkDestroyCommandPool(device, commandPool, nullptr);
}

int VulkanRenderchainComponent::addToFrame(uint32_t bufferSetId, SceneObject* object) {
    if(!sceneTree.contains(bufferSetId)) {
        sceneTree.emplace(bufferSetId, {parent->allocator->fetchBufferSet(bufferSetId), {}});
    }
    sceneObjects[bufferSetId].objects.emplace_back(object);

    return 0;
}

int VulkanRenderchainComponent::clearFrame() {
    sceneObjects.clear();
    return 0;
}

int VulkanRenderchainComponent::drawFrame() {
    CommandBuffer& graphicsBuffer = commandBuffers[currentFrame]; //Get (or wait for) next frame free out of max frames in flight
    vkWaitForFences(parent->device->Device, 1, &graphicsBuffer.fence, VK_TRUE, graphicsBuffer.timeout);
    
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

    recordCommandBuffer(graphicsBuffer.commandBuffer, &image); //Record our render pass onto this frame's command buffer

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

int VulkanRenderchainComponent::recordCommandBuffer(VkCommandBuffer& commandBuffer, VulkanSwapchainComponent::SwapchainImageWrapper* image) {
    image->TransitionImageLayout(commandBuffer, VulkanSwapchainComponent::LAYOUT_DETAILS_PRESET_COLOURATTACHMENT);
    
    VkClearValue _clearValue = { .color = VkClearColorValue({0.0f, 0.0f, 0.0f, 1.0f}) };

    VkCommandBufferBeginInfo BeginInfo { 
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .pNext = nullptr,
        .flags = NULL_BIT,
        .pInheritanceInfo = nullptr
    };

    vkBeginCommandBuffer(commandBuffer, &BeginInfo);

    VkViewport viewport = VkViewport(0.0f, 0.0f, static_cast<float>(image->extent.width), static_cast<float>(image->extent.height), 0.0f, 1.0f);
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

    VkRect2D scissor = VkRect2D(VkOffset2D(0.0f, 0.0f), image->extent);
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

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

    parent->pipelines->opaqueGeometryPipe->bind(commandBuffer);
    for(iterator i = sceneTree.begin(); i != sceneTree.end(); i++) {
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, i->set.vertexBuffer, nullptr);
        vkCmdBindIndexBuffer(commandBuffer, i->set.indiceBuffer, 0, VK_INDEX_TYPE_UINT32);
        for(int j = 0; j < i->objects.size(); j++) {
            vkCmdDrawIndexed(commandBuffer, i->objects[j]->mesh->vertexCount, 1, i->objects[j]->mesh->indexOffset, i->objects[j]->mesh->vertexOffset, 0);
        }
    }

    vkCmdEndRendering();

    vkEndCommandBuffer();

    image->TransitionImageLayout(commandBuffer, VulkanSwapchainComponent::LAYOUT_DETAILS_PRESET_PRESENT);

    return 0;
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