#ifndef ALCENGINE_MODULE_SHADERPROCESSOR_H
#define ALCENGINE_MODULE_SHADERPROCESSOR_H

#include "core/Registry/public.h"

//Services
//Depends
//Provides
#include "core/Registry/interface/IShaders.h"

#include "core/DebugManager/public.h"

class shaderProcessorException : public AlcEngineException {
    public:
        shaderProcessorException(std::string message) : AlcEngineException(DebugReport(message)) {}
};

class shaderProcessor : public WrapperBaseClass {
    public:
        shaderProcessor(void* registry);

        class IShadersImpl : public IShaders {
            private:
                shaderProcessor* parent;

            public:
                IShadersImpl(shaderProcessor* _parent) : parent(_parent) {}

                AlcShader* fetchShader(int index) override { return parent->fetchShader(index); }
        };
        IShadersImpl IShaders_shaderProcessor;

        struct AlcShaderImpl : public IShaders::AlcShader {
            uint32_t* dataArray;
            uint32_t dataArraySize;

            uint32_t* const& data() const;
            const uint32_t size() const;
        };

        void init();

        AlcShaderImpl vertexShader;
        AlcShaderImpl fragmentShader;

        AlcShaderImpl* fetchShader(int index);
    
    private:
        static ModuleRegistryBundle bundle;
        Registry* registry_ptr = nullptr;
};

#endif