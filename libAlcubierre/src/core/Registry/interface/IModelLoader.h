#ifndef ALCENGINE_CORE_REGISTRY_INTERFACE_IMODELLOADER_H
#define ALCENGINE_CORE_REGISTRY_INTERFACE_IMODELLOADER_H

#include "core/Registry/base/InterfaceBaseClass.h"

class IModelLoader : public InterfaceBaseClass {
    public:
        std::string token() override { return "IModelLoader"; }

        struct rawModelData {
            void* data;
            uint32_t size;
        };

        virtual uint32_t loadModel(rawModelData& model) = 0;
        virtual Mesh3D* fetchModel(uint32_t& modelHash) = 0;
};

#endif