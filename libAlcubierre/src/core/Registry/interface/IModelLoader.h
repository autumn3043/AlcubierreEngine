#ifndef ALCENGINE_CORE_REGISTRY_INTERFACE_IMODELLOADER_H
#define ALCENGINE_CORE_REGISTRY_INTERFACE_IMODELLOADER_H

#include "core/Registry/base/InterfaceBaseClass.h"

class IModelLoader : public InterfaceBaseClass {
    public:
        std::string token() override { return "IModelLoader"; }

        virtual bool isLoaded(uint32_t& modelHash) = 0;
        virtual uint32_t load(std::vector<Vector>& modelData, std::vector<uint32_t>& modelIndices, bool explicitCreation = false) = 0;
};

#endif