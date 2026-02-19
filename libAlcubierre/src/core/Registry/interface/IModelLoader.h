#ifndef ALCENGINE_CORE_REGISTRY_INTERFACE_IMODELLOADER_H
#define ALCENGINE_CORE_REGISTRY_INTERFACE_IMODELLOADER_H

#include "core/Registry/base/InterfaceBaseClass.h"

class IModelLoader : public InterfaceBaseClass {
    public:
        std::string token() override { return "IModelLoader"; }

        struct rawModelData {
            uint32_t hash;

            void* data;
            uint64_t size;

            rawModelData(uint32_t _hash, void* _data, uint64_t _size) : hash(_hash), data(_data), size(_size) {}
        };

        virtual int loadModel(rawModelData model) = 0;
};

#endif