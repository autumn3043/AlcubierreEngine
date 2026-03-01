#ifndef ALCENGINE_CORE_REGISTRY_INTERFACE_IMODELLOADER_H
#define ALCENGINE_CORE_REGISTRY_INTERFACE_IMODELLOADER_H

#include "core/Registry/base/InterfaceBaseClass.h"

class IModelLoader : public InterfaceBaseClass {
    public:
        std::string token() override { return "IModelLoader"; }

        virtual Hash_T loadModel(char* model, uint64_t size) = 0;
};

#endif