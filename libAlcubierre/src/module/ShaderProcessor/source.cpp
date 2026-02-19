#include "module/ShaderProcessor/private.h"

static void logIdentity(std::string message, int level = 0, bool Write = true) { return DM().Log(DebugReport(message, level, "shaderProcessor"), Write); }

ModuleRegistryBundle shaderProcessor::bundle(
    [](void* registry) -> WrapperBaseClass* { return new shaderProcessor(registry); },
    {SHADER_LOADER},
    {},
    "shaderProcessor"
);

shaderProcessor::shaderProcessor(void* registry) 
    :   IShaders_shaderProcessor(this),
        registry_ptr(static_cast<Registry*>(registry))
    {
        Services = {{SHADER_LOADER, &IShaders_shaderProcessor}};
        init();
    }

#define SPV_ALIGN alignas(4)
#include "module/ShaderProcessor/Shaders/output/vertex.h"
#include "module/ShaderProcessor/Shaders/output/fragment.h"

void shaderProcessor::init() {
    vertexShader.dataArray = reinterpret_cast<uint32_t*>(__output_vertex_spv);
    vertexShader.dataArraySize = __output_vertex_spv_len;
    
    fragmentShader.dataArray = reinterpret_cast<uint32_t*>(__output_fragment_spv);
    fragmentShader.dataArraySize = __output_fragment_spv_len;
}

shaderProcessor::AlcShaderImpl* shaderProcessor::fetchShader(int index) {
    if(index == 0) return &vertexShader;
    if(index == 1) return &fragmentShader;
    return &vertexShader;
}

uint32_t* const& shaderProcessor::AlcShaderImpl::data() const {
    return dataArray;
}

const uint32_t shaderProcessor::AlcShaderImpl::size() const {
    return dataArraySize;
}