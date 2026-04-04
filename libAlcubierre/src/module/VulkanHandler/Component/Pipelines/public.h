#ifndef ALCENGINE_MODULE_VULKANHANDLER_COMPONENT_PIPELINES_PUBLIC_H
#define ALCENGINE_MODULE_VULKANHANDLER_COMPONENT_PIPELINES_PUBLIC_H

#include "spirv_reflect.h"

class VulkanPipelineComponent {
    private:
        VulkanHandler* parent = nullptr;
        Registry*& registry_ptr;

    public:
        VulkanPipelineComponent(VulkanHandler* _parent, Registry*& _registry_ptr);
        ~VulkanPipelineComponent();

    private:
        createDescriptorSetLayouts();

    public:
        VkDescriptorSetLayout frameDescriptorSetLayout = VK_NULL_HANDLE;
        VkDescriptorSetLayout materialDescriptorSetLayout = VK_NULL_HANDLE;
        VkDescriptorSetLayout objectDescriptorSetLayout = VK_NULL_HANDLE;

    private:
        class PipelineLayout {
            public:
                PipelineLayout(VkDevice& _device);
                ~PipelineLayout();

            private:
                VkDevice& device;

            public:
                VkPipelineLayout instance = VK_NULL_HANDLE;
        };

        class Shader {
            public:
                Shader(VkDevice& _device, IShaders::AlcShader* shaderData);
                ~Shader();

            private:
                VkDevice& device;

            public:
                std::vector<VkDescriptorSetLayout> layouts;

                VkShaderModule instance = VK_NULL_HANDLE;

                VkPipelineShaderStageCreateInfo stageCreateInfo;

                std::string name;
                VkShaderStageFlagBits stage;
        };
        Shader* basicVertexShader;
        Shader* basicFragmentShader;

        class Pipeline {
            public:
                Pipeline(VkDevice& _device, VkPipelineLayout& _layout, VkFormat& imageFormat, std::vector<Shader*> _shaders);
                ~Pipeline();

            private:
                VkDevice& device;

            public:            
                VkPipeline instance = VK_NULL_HANDLE;
                VkPipelineLayout layout = VK_NULL_HANDLE;

                std::vector<Shader*> shaders;
        };

    public:
        PipelineLayout* pipelineLayout = nullptr;
        Pipeline* opaqueGeometryPipe = nullptr;
};

#endif