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
        class PipelineLayout {
            public:
                PipelineLayout(VkDevice& _device);
                ~PipelineLayout();

            private:
                VkDevice& device;

            public:
                VkPipelineLayout instance = VK_NULL_HANDLE;
                VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
                uint32_t bindingsCount;
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
                int bind(VkCommandBuffer& commandBuffer, VkDescriptorSet descriptorSet);
            
            private:
                VkPipeline instance = VK_NULL_HANDLE;
                VkPipelineLayout layout = VK_NULL_HANDLE;

                std::vector<Shader*> shaders;
        };

    public:
        PipelineLayout* pipelineLayout = nullptr;
        Pipeline* opaqueGeometryPipe = nullptr;
};

#endif