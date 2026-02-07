VulkanPipelineComponent::VulkanPipelineComponent(VulkanHandler* _parent, Registry*& _registry_ptr) : parent(_parent), registry_ptr(_registry_ptr) {
    IShaders* SL = dynamic_cast<IShaders*>(registry_ptr->FetchService(SHADER_LOADER));
    basicVertexShader = new Shader(parent->device->Device, SL->fetchShader(0));
    basicFragmentShader = new Shader(parent->device->Device, SL->fetchShader(1));

    geometryLayout = new PipelineLayout(parent->device->Device, {basicVertexShader, basicFragmentShader});

    opaqueGeometryPipe = new Pipeline(parent->device->Device, geometryLayout);
}

~VulkanPipelineComponent::VulkanPipelineComponent() {
    if(opaqueGeometryPipe) delete opaqueGeometryPipe;
    if(geometryLayout) delete geometryLayout;
    if(basicFragmentShader) delete basicFragmentShader;
    if(basicVertexShader) delete basicVertexShader;
}

VulkanPipelineComponent::Shader(VkDevice& _device, IShaders::AlcShader* shaderData) : device(_device) {
    SpvReflectShaderModule reflect;
    spvReflectCreateShaderModule(shaderData->size(), shaderData->data(), &reflect);
    
    name = reflect.entry_point_name;
    stage = static_cast<VkShaderStageFlagBits>(reflect.shader_stage);

    int descriptorSetCount = reflect.descriptor_set_count;
    layouts.reserve(descriptorSetCount);

    for(int i = 0; i < descriptorSetCount; i++) {
        SpvReflectDescriptorSet& set = reflect.descriptor_sets[i];

        std::vector<VkDescriptorSetLayoutBinding> bindings(set.binding_count);
        for(int j = 0; j < set.binding_count; j++) {
            SpvReflectDescriptorBinding*& setBinding = set.bindings[j];

            bindings[j] = {
                .binding = setBinding->binding,
                .descriptorType = static_cast<VkDescriptorType>(setBinding->descriptor_type),
                .descriptorCount = setBinding->count,
                .stageFlags = static_cast<VkShaderStageFlags>(reflect.shader_stage),
                .pImmutableSamplers = nullptr
            };
        }

        VkDescriptorSetLayoutCreateInfo createInfo = {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
            .pNext = nullptr,
            .flags = NULL_BIT,
            .bindingCount = static_cast<uint32_t>(bindings.size()),
            .pBindings = bindings.data()
        };
        vkCreateDescriptorSetLayout(device, &createInfo, nullptr, &layouts[i]);
    }

    spvReflectDestroyShaderModule(&reflect);

    sets.resize(descriptorSetCount);

    VkDescriptorPoolSize poolSize = {
        .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .descriptorCount = descriptorSetCount
    };

    VkDescriptorPoolCreateInfo poolCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .pNext = nullptr,
        .flags = NULL_BIT,
        .maxSets = descriptorSetCount,
        .poolSizeCount = 1,
        .pPoolSizes = &poolSize
    };

    vkCreateDescriptorPool(device, &poolCreateInfo, nullptr, &pool);

    VkDescriptorSetAllocateInfo allocateInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .pNext = nullptr,
        .descriptorPool = pool,
        .descriptorSetCount = layouts.size(),
        .pSetLayouts = layouts.data()
    };
    vkAllocateDescriptorSets(device, &allocateInfo, sets.data());

    VkShaderModuleCreateInfo shaderCreateInfo {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = shaderData->size(),
        .pCode = shaderData->data()
    };
    vkCreateShaderModule(device, &shaderCreateInfo, nullptr, &module);
}

VulkanPipelineComponent::~Shader() {
    vkDestroyDescriptorPool(device, pool, nullptr);
    vkDestroyShaderModule(device, module, nullptr);
}

VulkanPipelineComponent::PipelineLayout::PipelineLayout(VkDevice& _device, std::vector<Shader*> _shaders) : device(_device), shaders(_shaders) {
    std::vector<VkDescriptorSetLayout> setLayouts;
    for(int i = 0; i < shaders.size(); i++) {
        setLayouts.insert(setLayouts.end(), shaderData->layouts.begin(), shaderData->layouts.end());
    }

    VkPipelineLayoutCreateInfo layoutInfo {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .pNext = nullptr,
        .flags = NULL_BIT,
        .setLayoutCount = static_cast<uint32_t>(setLayouts.size()),
        .pSetLayouts = setLayouts.data(),
        .pushConstantRangeCount = 0,
        .pPushConstantRanges = nullptr
    };

    vkCreatePipelineLayout(device, &layoutInfo, nullptr, &instance);
}

VulkanPipelineComponent::PipelineLayout::~PipelineLayout() {
    vkDestroyPipelineLayout(device, instance, nullptr);
}

VulkanPipelineComponent::Pipeline::Pipeline(VkDevice& _device, PipelineCreateInfo alcCreateInfo) : device(_device), layout(alcCreateInfo.layout) {
    VkGraphicsPipelineCreateInfo createInfo {};
    createInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;

    VkPipelineRenderingCreateInfo pipelineRenderingCreateInfo {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
        .colorAttachmentCount = 1,
        .pColorAttachmentFormats = &alcCreateInfo.imageFormat;
    };
    createInfo.pNext = &pipelineRenderingCreateInfo;

    std::vector<VkVertexInputBindingDescription> bindingDescriptions = {{0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX}};
    std::vector<VkVertexInputAttributeDescription> attributeDescriptions = {{0, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, position)}, {1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, colour)}};

    VkPipelineVertexInputStateCreateInfo vertexInputInfo {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .vertexBindingDescriptionCount = static_cast<uint32_t>(bindingDescriptions.size()),
        .pVertexBindingDescriptions = bindingDescriptions.data(),
        .vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size()),
        .pVertexAttributeDescriptions = attributeDescriptions.data()
    };
    createInfo.pVertexInputState = &vertexInputInfo;

    VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST
    };
    createInfo.pInputAssemblyState = &inputAssemblyInfo;

    VkPipelineViewportStateCreateInfo viewportInfo {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .viewportCount = 1,
        .scissorCount = 1
    };
    createInfo.pViewportState = &viewportInfo;

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
    createInfo.pRasterizationState = &rasterizationInfo;

    VkPipelineMultisampleStateCreateInfo multisampleInfo {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
        .sampleShadingEnable = VK_FALSE
    };
    createInfo.pMultisampleState = &multisampleInfo;

    VkPipelineColorBlendAttachmentState colorAttachment {
        .blendEnable = VK_FALSE,
        .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT
    };

    VkPipelineColorBlendStateCreateInfo colorBlendInfo {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .attachmentCount = 1,
        .pAttachments = &colorAttachment,
    };
    createInfo.pColorBlendState = &colorBlendInfo;

    std::vector<VkDynamicState> dynamicStates = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
    VkPipelineDynamicStateCreateInfo dynamicStateInfo {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .dynamicStateCount = static_cast<uint32_t>(dynamicStates.size()),
        .pDynamicStates = dynamicStates.data()
    };
    createInfo.pDynamicState = &dynamicStateInfo;

    createInfo.layout = layout.instance;

    createInfo.renderPass = VK_NULL_HANDLE;

    vkCreateGraphicsPipelines(device, nullptr, 1, &createInfo, nullptr, &instance);
}

VulkanPipelineComponent::Pipeline::~Pipeline() {
    vkDestroyPipeline(device, instance, nullptr);
}

int VulkanPipelineComponent::Pipeline::bind(VkCommandBuffer& commandBuffer) {
    std::vector<VkDescriptorSet> descriptorSets = layout->getDescriptorSetHandlesArray();
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, layout->instance, 0, descriptorSets.count, descriptorSets.data(), 0, nullptr);
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineInstance);
}