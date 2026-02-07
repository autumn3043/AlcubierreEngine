#ifndef ALCENGINE_MODULE_VULKANHANDLER_COMPONENT_PIPELINES_PUBLIC_H
#define ALCENGINE_MODULE_VULKANHANDLER_COMPONENT_PIPELINES_PUBLIC_H

class VulkanPipelineComponent {
    private:
        VulkanHandler* parent = nullptr;
        Registry*& registry_ptr;

    public:
        VulkanPipelineComponent(VulkanHandler* _parent, Registry*& _registry_ptr);
        ~VulkanPipelineComponent();

    private:
        class Shader {
            public:
                Shader(VkDevice& _device, IShaders::AlcShader* shaderData);
                ~Shader();

            private:
                VkDevice& device;

            public:
                std::vector<VkDescriptorSetLayout> layouts;
                //OPT: can be discarded after set has been created AND all associated pipeline layouts have been created
                VkDescriptorPool pool = VK_NULL_HANDLE;
                std::vector<VkDescriptorSet> sets;

                VkShaderModule instance = VK_NULL_HANDLE;

                std::string name;
                VkShaderStageFlagBits stage;
        };
        Shader* basicVertexShader;
        Shader* basicFragmentShader;

        class PipelineLayout {
            public:
                PipelineLayout(VkDevice& _device, std::vector<Shader*> _shaders);
                ~PipelineLayout();            

            private:
                VkDevice& device;

            public:
                VkPipelineLayout instance = VK_NULL_HANDLE;

                std::vector<VkDescriptorSet> getDescriptorSetHandlesArray();

            private:
                std::vector<Shader*> shaders;
        };
        PipelineLayout* geometryLayout = nullptr;

        struct PipelineCreateInfo {
            PipelineLayout* layout;
            VkFormat& imageFormat;
        };

        class Pipeline {
            public:
                Pipeline(VkDevice& _device, PipelineCreateInfo alcCreateInfo);
                ~Pipeline();

            private:
                VkDevice& device;

            public:
                int bind(VkCommandBuffer commandBuffer);
            
            private:
                VkPipeline instance = VK_NULL_HANDLE;

                PipelineLayout* layout;
        };

    public:
        Pipeline* opaqueGeometryPipe = nullptr;
};

#endif